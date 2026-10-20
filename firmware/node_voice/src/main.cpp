#include "platform_hal.h"
#include "network_hal.h"
#include <esp_wifi.h>
#include "clawgrid_config.h"
#include "secure_link.h"
#include "safety_kernel.h"
#include "safety_selftest.h"
#include "memory_runtime.h"
#include "rx_mailbox.h"
#include "pir_tracker.h"
#include "runtime_selftest.h"
#include "automation_selftest.h"
#include "security_selftest.h"
#include "rtos_runtime.h"
#include "policy_store.h"
#include "house_simulation.h"
#include "house_selftest.h"

static RxMailbox s_rx;
static StaticMailbox<ControlCommand,8> s_commands;
static StaticMailbox<ControlAck,8> s_acks;
static StaticMailbox<RoomReport,1> s_state;
static DiagnosticLog s_log;
static PolicyStore s_store;
static StaticMailbox<PolicyConfig,1> s_policy_updates;
static PolicyConfig s_saved_policy;
static HouseSimulation s_house;
static StaticMailbox<SimInput,1> s_sim_inputs;
static StaticMailbox<SimState,1> s_sim_states;
static RuntimeHealth s_health;
static TaskTiming s_control_timing,s_link_timing;
static StaticTask_t s_control_tcb,s_link_tcb;
static StackType_t s_control_stack[6144],s_link_stack[6144];
static portMUX_TYPE s_audit_mux=portMUX_INITIALIZER_UNLOCKED;
static PirTracker s_pir;
static Automation s_automation;
static CommandGate s_gate;
static ControlAck s_cached_ack;
static bool s_radio=false,s_ok=false;
static uint32_t s_boot=0;
static std::atomic<uint32_t> s_tx_failed{0},s_rejected{0},s_duplicates{0},s_last_command{0},s_command_dropped{0};
static std::atomic<bool> s_drop_ack{false},s_muted{false};
static std::atomic<bool> s_stall_control{false};
static std::atomic<uint32_t> s_mute_start{0};
static void receive(const uint8_t *mac,const uint8_t *data,int n) { s_rx.push(mac,data,n); }

// Only ControlTask accesses the safety registry after startup.
static void control(const ControlCommand &c,uint32_t now) {
    uint8_t payload[kControlBytes]; encode_control(c,payload);
    ControlAck a; a.boot=s_boot; a.id=c.id; memcpy(a.request,payload,sizeof(payload));
    bool committed=false; uint32_t start=micros();
    if(c.boot!=s_boot) a.status=ControlStatus::WrongBoot;
    else if(s_gate.duplicate(payload)) {
        ++s_duplicates; if(!s_acks.push(s_cached_ack)) ++s_tx_failed;
        s_log.write("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"duplicate_result\",\"id\":%u,\"status\":%u}\n",c.id,(unsigned)s_cached_ack.status); return;
    } else if(!s_gate.next(c.id)) a.status=ControlStatus::Replay;
    else {
        bool enabled=s_ok&&s_health.healthy(kControlReady|kLinkReady);
        portENTER_CRITICAL(&s_audit_mux);
        a.status=!control_fresh(c,now)?ControlStatus::Expired:!enabled?ControlStatus::Disabled:c.op==ControlOp::Simulation?s_house.manual(c,now):s_automation.execute(c,now);
        portEXIT_CRITICAL(&s_audit_mux);
        s_gate.commit(payload,c.id); s_last_command.store(c.id); committed=true;
    }
    a.light=safety_kernel_get_device(0)->is_on; a.fan=safety_kernel_get_device(1)->is_on;
    a.mode=s_automation.away; a.processing_us=micros()-start;
    if(committed) s_cached_ack=a;
    if(!s_acks.push(a)) ++s_tx_failed;
    s_log.write("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"actuation\",\"id\":%u,\"status\":%u,\"processing_us\":%u,\"light\":%s,\"fan\":%s}\n",a.id,(unsigned)a.status,a.processing_us,a.light?"true":"false",a.fan?"true":"false");
}
static void control_task(void*) {
    s_health.ready(kControlReady,s_control_timing);
    TickType_t release=xTaskGetTickCount(); uint32_t expected=micros();
    PolicyConfig published=s_saved_policy;
    for(;;) {
        if(s_stall_control.load()) for(;;) vTaskDelay(pdMS_TO_TICKS(1000));
        uint32_t start=micros(),now=millis();
        safety_kernel_tick_periodic(now);
        bool changed=s_pir.sample(digitalRead(NODE_A_PIR_PIN)==HIGH,now);
        if(changed) s_log.write("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"motion_event\",\"room\":\"bedroom\",\"at_ms\":%u,\"motion\":%s}\n",now,s_pir.motion()?"true":"false");
        if(s_ok&&s_health.healthy(kControlReady|kLinkReady))
            s_automation.tick(now,changed&&s_pir.motion(),s_pir.occupancy(now,s_automation.grace));
        ControlCommand c; for(int i=0;i<2&&s_commands.pop(c);++i) control(c,now);
        SimInput sim; if(s_sim_inputs.pop(sim)) s_house.input(sim,s_boot,now);
        uint32_t sim_start=micros(); SimState simulated=s_house.tick(s_boot,now);
        simulated.processing_us=micros()-sim_start;
        simulated.physical_outputs_enabled=CLAWGRID_PHYSICAL_OUTPUTS; s_sim_states.latest(simulated);
        RoomReport r; r.boot_id=s_boot; r.uptime_ms=now; r.motion=s_pir.motion();
        r.occupancy=s_pir.occupancy(now,s_automation.grace); r.last_command=s_gate.last;
        r.light=safety_kernel_get_device(0)->is_on; r.fan=safety_kernel_get_device(1)->is_on;
        s_automation.report(r,now); s_state.latest(r);
        PolicyConfig policy=s_automation.policy();
        if(policy!=published) { s_policy_updates.latest(policy); published=policy; }
        if(s_control_timing.watchdog.load()) esp_task_wdt_reset();
        s_control_timing.record(start,expected,5000); expected+=5000;
        vTaskDelayUntil(&release,pdMS_TO_TICKS(5));
    }
}
static void link_task(void*) {
    s_health.ready(kLinkReady,s_link_timing);
    TickType_t release=xTaskGetTickCount(); uint32_t expected=micros(),sequence=0,last_send=0;
    RoomReport report,last_report; bool have=false;
    for(;;) {
        uint32_t start=micros(),now=millis();
        // A newly stored start may be a millisecond newer than this task's
        // snapshot of now. Signed subtraction prevents false wrap expiry.
        if(s_muted.load() && (int32_t)(now-s_mute_start.load())>=8000) s_muted.store(false);
        RxFrame f;
        for(int i=0;i<4&&s_rx.pop(f);++i) {
            if(s_muted.load()) continue;
            clawgrid_header_t h; const uint8_t *p=nullptr; ControlCommand c;
            if(!secure_decode(f.mac,kNodeB,f.bytes,f.length,h,p) || h.src_node!=2 || h.dst_node!=1) { ++s_rejected; continue; }
            if(h.msg_type==MSG_TYPE_SENSOR_EVENT) {
                SimInput input; if(decode_sim_input(p,h.payload_len,input)) s_sim_inputs.latest(input); else ++s_rejected;
                continue;
            }
            if(h.msg_type!=MSG_TYPE_DEVICE_COMMAND || !decode_control(p,h.payload_len,c)) { ++s_rejected; continue; }
            if(!s_commands.push(c)) ++s_command_dropped;
        }
        ControlAck a;
        for(int i=0;i<2&&s_acks.pop(a);++i) {
            if(s_muted.load() || s_drop_ack.exchange(false)) continue;
            uint8_t p[kAckBytes]; encode_ack(a,p);
            if(!secure_send(kNodeB,MSG_TYPE_COMMAND_ACK,1,2,sequence++,p,sizeof(p))) ++s_tx_failed;
        }
        if(s_state.pop(report)) have=true;
        static uint32_t last_sim=0;
        if(s_radio && !s_muted.load() && (uint32_t)(now-last_sim)>=100) {
            SimState simulated;
            if(s_sim_states.pop(simulated)) { uint8_t bytes[kSimStateBytes]; encode_sim_state(simulated,bytes);
                if(!secure_send(kNodeB,MSG_TYPE_NODE_STATUS,1,2,sequence++,bytes,sizeof(bytes))) ++s_tx_failed;
                last_sim=now;
            }
        }
        if(s_radio && have && !s_muted.load() && ((uint32_t)(now-last_send)>=1000 || report.motion!=last_report.motion)) {
            uint8_t p[kRoomReportBytes]; encode_room_report(report,p);
            if(!secure_send(kNodeB,MSG_TYPE_ROOM_STATE,1,2,sequence++,p,sizeof(p))) ++s_tx_failed;
            last_send=now; last_report=report;
        }
        if(s_link_timing.watchdog.load()) esp_task_wdt_reset();
        s_link_timing.record(start,expected,5000); expected+=5000;
        vTaskDelayUntil(&release,pdMS_TO_TICKS(5));
    }
}
void setup() {
    Serial.begin(115200); s_log.begin(); s_commands.begin(); s_acks.begin(); s_state.begin(); s_rx.begin();
    s_policy_updates.begin();
    s_sim_inputs.begin(); s_sim_states.begin();
    bool safety=safety_boot_selftest(),automation=automation_selftest(),house=house_selftest();
    safety_kernel_init();
    bool registry=safety_kernel_register_device("bedroom","light",DEVICE_TYPE_LIGHT,CLAWGRID_PHYSICAL_OUTPUTS?NODE_A_RELAY_LIGHT_PIN:-1,NODE_A_RELAY_ACTIVE_LOW)==0;
    registry &= safety_kernel_register_device("bedroom","fan",DEVICE_TYPE_FAN,CLAWGRID_PHYSICAL_OUTPUTS?NODE_A_RELAY_FAN_PIN:-1,NODE_A_RELAY_ACTIVE_LOW)==1;
    registry &= s_house.begin();
    bool storage=s_store.begin(s_boot,s_saved_policy);
    if(storage) s_automation.restore_policy(s_saved_policy);
    s_ok=storage&&safety&&automation&&house&&registry&&runtime_selftest()&&security_selftest()&&s_health.begin();
    Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"persistence_boot\",\"ok\":%s,\"boot\":%u}\n",storage?"true":"false",s_boot);
    Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"boot_recovery\",\"reset_reason\":%u,\"boot\":%u,\"physical_outputs_enabled\":%s}\n",(unsigned)esp_reset_reason(),s_boot,CLAWGRID_PHYSICAL_OUTPUTS?"true":"false");
    pinMode(NODE_A_PIR_PIN,INPUT_PULLDOWN);
    WiFi.mode(WIFI_STA); WiFi.setSleep(false);
    s_radio=esp_wifi_set_channel(CLAWGRID_WIFI_CHANNEL,WIFI_SECOND_CHAN_NONE)==ESP_OK && esp_now_init()==ESP_OK && secure_peer(kNodeB);
    if(s_radio) esp_now_register_recv_cb(receive);
    if(s_ok) {
        s_control_timing.handle=xTaskCreateStaticPinnedToCore(control_task,"ControlTask",sizeof(s_control_stack),nullptr,5,s_control_stack,&s_control_tcb,1);
        s_link_timing.handle=xTaskCreateStaticPinnedToCore(link_task,"LinkTask",sizeof(s_link_stack),nullptr,3,s_link_stack,&s_link_tcb,0);
        if(!s_control_timing.handle || !s_link_timing.handle) s_health.fail();
    }
}
void loop() {
    s_log.drain();
    // Flash operations stay out of ControlTask; coalesce rapid edits. NVS is
    // wear-levelled, but a flash commit can still stall the instruction cache.
    static PolicyConfig pending=s_saved_policy;
    static uint32_t changed_at=0,last_attempt=0;
    PolicyConfig update; uint32_t persistence_now=millis();
    if(s_policy_updates.pop(update)) { pending=update; changed_at=persistence_now; }
    if(pending!=s_saved_policy && (uint32_t)(persistence_now-changed_at)>=5000 &&
       (uint32_t)(persistence_now-last_attempt)>=5000) {
        last_attempt=persistence_now; bool saved=s_store.save(pending);
        if(saved) s_saved_policy=pending;
        Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"persistence_commit\",\"ok\":%s}\n",saved?"true":"false");
    }
    if(Serial.available()) {
        char c=Serial.read();
        if(c=='X' && !CLAWGRID_PHYSICAL_OUTPUTS) {
            Serial.print("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"watchdog_injection\",\"armed\":true}\n");
            s_stall_control.store(true);
        }
        if(c=='P') Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"policy_saved\",\"boot\":%u,\"reset_reason\":%u,\"grace_ms\":%u,\"auto_flags\":%u,\"away\":%s,\"physical_outputs_enabled\":%s}\n",s_boot,(unsigned)esp_reset_reason(),s_saved_policy.grace_ms,s_saved_policy.light_auto|(s_saved_policy.fan_auto<<1),s_saved_policy.away?"true":"false",CLAWGRID_PHYSICAL_OUTPUTS?"true":"false");
        if(c=='d' || c=='O') {
            if(c=='d') s_drop_ack.store(true);
            else { s_mute_start.store(millis()); s_muted.store(true); }
            Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"fault_armed\",\"type\":\"%c\"}\n",c);
        }
        if(c=='t') Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"selftest\",\"node\":\"A\",\"passed\":%s,\"rtos_ready\":%s}\n",(s_ok&&runtime_selftest()&&s_health.healthy(kControlReady|kLinkReady))?"true":"false",s_health.healthy(kControlReady|kLinkReady)?"true":"false");
        if(c=='m') memory_probe("A");
        if(c=='a') {
            AuditEvent events[32]; uint32_t count;
            portENTER_CRITICAL(&s_audit_mux); count=s_automation.event_count; memcpy(events,s_automation.events,sizeof(events)); portEXIT_CRITICAL(&s_audit_mux);
            for(uint32_t i=count>32?count-32:0;i<count;++i) { auto &e=events[i%32];
                Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"audit\",\"at_ms\":%u,\"id\":%u,\"op\":%u,\"status\":%u,\"light\":%u,\"fan\":%u}\n",e.at,e.id,e.op,e.status,e.light,e.fan);
            }
        }
    }
    static uint32_t last=0; uint32_t now=millis();
    if((uint32_t)(now-last)>=5000) {
        last=now; memory_log("A",s_rx.dropped(),s_tx_failed.load());
        if(Serial) Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"safety\",\"node\":\"A\",\"rejected\":%u,\"duplicates\":%u,\"last_command\":%u,\"command_dropped\":%u,\"radio\":%s}\n",s_rejected.load(),s_duplicates.load(),s_last_command.load(),s_command_dropped.load(),s_radio?"true":"false");
        s_control_timing.log("A","ControlTask",1,5,5,s_log.dropped.load());
        s_link_timing.log("A","LinkTask",0,3,5,s_log.dropped.load());
    }
    vTaskDelay(pdMS_TO_TICKS(5));
}
