#include "platform_hal.h"
#include "network_hal.h"
#include <esp_http_server.h>
#include "clawgrid_config.h"
#include "secure_link.h"
#include "control_wire.h"
#include "neural_agent.h"
#include "memory_runtime.h"
#include "rx_mailbox.h"
#include "runtime_selftest.h"
#include "dashboard.h"
#include "security_selftest.h"
#include "http_selftest.h"
#include "rtos_runtime.h"
#include "house_simulation.h"
#include "simulation_dashboard.h"
#include "speech_runtime.h"
#include "serial_bridge.h"
#include "evidence_selftest.h"

static RxMailbox s_rx;
static httpd_handle_t s_http=nullptr;
static portMUX_TYPE s_mux=portMUX_INITIALIZER_UNLOCKED;
static RoomReport s_report;
static SimState s_sim_report;
static uint32_t s_sim_received=0,s_sim_sequence=0;
static uint32_t s_received=0,s_last_seq=0,s_accepted=0,s_invalid=0,s_replayed=0;
static std::atomic<uint32_t> s_seq{0},s_tx_failed{0};
static RuntimeHealth s_health;
static DiagnosticLog s_log;
static TaskTiming s_link_timing;
static StaticTask_t s_link_tcb;
static StackType_t s_link_stack[6144];
static uint32_t s_last_control=0;
static bool s_have=false,s_radio=false,s_ok=false;
struct Transaction {
    ControlCommand command;
    ControlAck ack;
    uint32_t started=0,last_send=0,rtt_us=0,start_us=0;
    uint8_t phase=0,attempts=0; // idle, pending, acknowledged, timeout
};
static Transaction s_transaction;
static char s_json[1536],s_body[128]; // HTTP task only
static char s_serial[160]; static size_t s_serial_used=0; static bool s_serial_bad=false;
static void receive(const uint8_t *mac,const uint8_t *data,int n) { s_rx.push(mac,data,n); }
static esp_err_t json(httpd_req_t *req,int n) {
    if(n<0 || (size_t)n>=sizeof(s_json)) return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Response limit");
    httpd_resp_set_type(req,"application/json"); httpd_resp_set_hdr(req,"Cache-Control","no-store");
    return httpd_resp_send(req,s_json,n);
}
static bool authorized(httpd_req_t *req) {
    char token[64]; size_t n=httpd_req_get_hdr_value_len(req,"X-ClawGrid-Token");
    if(n!=strlen(kApiToken) || n>=sizeof(token) || httpd_req_get_hdr_value_str(req,"X-ClawGrid-Token",token,sizeof(token))!=ESP_OK) return false;
    uint8_t delta=0; for(size_t i=0;i<n;++i) delta|=token[i]^kApiToken[i]; return delta==0;
}
static esp_err_t unauthorized(httpd_req_t *req) {
    httpd_resp_set_status(req,"401 Unauthorized");
    return json(req,snprintf(s_json,sizeof(s_json),"{\"error\":\"Control token required\"}"));
}
static bool read_body(httpd_req_t *req) {
    if(!req->content_len || req->content_len>=sizeof(s_body)) return false;
    size_t n=0;
    while(n<req->content_len) { int got=httpd_req_recv(req,s_body+n,req->content_len-n); if(got<=0) return false; n+=got; }
    for(size_t i=0;i<n;++i) if((uint8_t)s_body[i]<32 || (uint8_t)s_body[i]>126) return false;
    s_body[n]=0; return true;
}
static esp_err_t bad_body(httpd_req_t *req) {
    httpd_resp_set_status(req,"400 Bad Request"); httpd_resp_set_hdr(req,"Connection","close");
    json(req,snprintf(s_json,sizeof(s_json),"{\"error\":\"Use 1 to 127 ASCII bytes\"}")); return ESP_FAIL;
}
static bool parse_fields(const char *text,ControlCommand &c) {
    // Reject signs, overflow and hidden suffixes before bounded conversion.
    size_t n=strlen(text); if(n>45) return false;
    for(size_t i=0;i<n;++i) if((text[i]<'0'||text[i]>'9') && text[i]!=' ') return false;
    unsigned long long fields[4]={}; int count=0; const char *p=text;
    while(*p) {
        while(*p==' ') ++p;
        if(!*p) break;
        if(count==4) return false;
        unsigned long long v=0;
        while(*p>='0'&&*p<='9') { v=v*10+(*p++-'0'); if(v>UINT32_MAX) return false; }
        fields[count++]=v;
    }
    if(count!=4 || fields[0]<1 || fields[0]>6 || fields[1]>(fields[0]==6?7u:2u) || fields[2]>3) return false;
    c.op=(ControlOp)fields[0]; c.device=fields[1]; c.action=fields[2]; c.value=fields[3]; return true;
}
static bool queue_command(ControlCommand c,uint32_t &id) {
    uint32_t now=millis(); bool accepted=false;
    bool healthy=s_health.healthy(kLinkReady);
    portENTER_CRITICAL(&s_mux);
    if(s_ok && healthy && s_radio && s_have && (uint32_t)(now-s_received)<5000 && s_transaction.phase!=1) {
        uint32_t last=s_report.last_command>s_last_control?s_report.last_command:s_last_control;
        if(last!=UINT32_MAX) {
            c.boot=s_report.boot_id; c.id=last+1; id=c.id;
            c.issued_ms=s_report.uptime_ms+(now-s_received); c.ttl_ms=3000;
            s_transaction={}; s_transaction.command=c; s_transaction.phase=1;
            s_transaction.started=now; s_transaction.start_us=micros(); accepted=true;
        }
    }
    portEXIT_CRITICAL(&s_mux); return accepted;
}
static esp_err_t root(httpd_req_t *req) {
    httpd_resp_set_type(req,"text/html; charset=utf-8");
    httpd_resp_set_hdr(req,"Content-Security-Policy","default-src 'self'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'");
    return httpd_resp_send(req,kDashboard,sizeof(kDashboard)-1);
}
static esp_err_t simulation_page(httpd_req_t *req) {
    httpd_resp_set_type(req,"text/html; charset=utf-8");
    httpd_resp_set_hdr(req,"Content-Security-Policy","default-src 'self'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; media-src blob:; frame-ancestors 'none'");
    return httpd_resp_send(req,kSimulationDashboard,sizeof(kSimulationDashboard)-1);
}
static esp_err_t simulation_state_api(httpd_req_t *req) {
    portENTER_CRITICAL(&s_mux); SimState s=s_sim_report; uint32_t age=millis()-s_sim_received; bool known=s_sim_received && age<3000 && s_have && s.boot==s_report.boot_id;
    portEXIT_CRITICAL(&s_mux);
    return json(req,snprintf(s_json,sizeof(s_json),"{\"input_source\":\"SIMULATED\",\"processing_source\":\"HARDWARE_MEASURED\",\"known\":%s,\"boot\":%u,\"sequence\":%u,\"age_ms\":%u,\"outputs\":%u,\"sensor_known\":%u,\"occupied\":%u,\"processing_us\":%u,\"accepted\":%u,\"rejected\":%u,\"physical_outputs_enabled\":%s}",known?"true":"false",s.boot,s.sequence,age,s.outputs,s.known,s.occupied,s.processing_us,s.accepted,s.rejected,s.physical_outputs_enabled?"true":"false"));
}
static esp_err_t simulation_input_api(httpd_req_t *req) {
    if(!authorized(req)) { httpd_resp_set_hdr(req,"Connection","close"); unauthorized(req); return ESP_FAIL; }
    if(!read_body(req)) return bad_body(req);
    unsigned values[12]={}; int count=0; const char *p=s_body;
    while(*p) {
        while(*p==' ') ++p;
        if(!*p) break;
        if(count==12 || *p<'0' || *p>'9') goto invalid_sim;
        unsigned v=0; while(*p>='0' && *p<='9') { v=v*10+(*p++-'0'); if(v>2000) goto invalid_sim; }
        if(*p && *p!=' ') goto invalid_sim;
        values[count++]=v;
    }
    if(count!=12) goto invalid_sim;
    {
        SimInput input;
        for(int i=0;i<4;++i) {
            if(values[3*i]>63 || values[3*i+1]>600) goto invalid_sim;
            input.flags[i]=values[3*i]; input.temperature[i]=values[3*i+1]; input.lux[i]=values[3*i+2];
        }
        uint32_t now=millis(); bool fresh;
        portENTER_CRITICAL(&s_mux); fresh=s_have && (uint32_t)(now-s_received)<3000;
        input.boot=s_report.boot_id; input.issued_ms=s_report.uptime_ms+(now-s_received);
        if(s_sim_report.boot==input.boot && sequence_newer(s_sim_report.sequence,s_sim_sequence)) s_sim_sequence=s_sim_report.sequence;
        input.sequence=++s_sim_sequence; if(!input.sequence) input.sequence=++s_sim_sequence;
        portEXIT_CRITICAL(&s_mux);
        if(!fresh || !s_radio || !s_health.healthy(kLinkReady)) { httpd_resp_set_status(req,"409 Conflict"); return json(req,snprintf(s_json,sizeof(s_json),"{\"error\":\"Node A unavailable\"}")); }
        uint8_t bytes[kSimInputBytes]; encode_sim_input(input,bytes);
        if(!secure_send(kNodeA,MSG_TYPE_SENSOR_EVENT,2,1,s_seq++,bytes,sizeof(bytes))) { ++s_tx_failed; httpd_resp_set_status(req,"503 Service Unavailable"); return json(req,snprintf(s_json,sizeof(s_json),"{\"error\":\"Radio busy\"}")); }
        httpd_resp_set_status(req,"202 Accepted");
        return json(req,snprintf(s_json,sizeof(s_json),"{\"queued\":true,\"sequence\":%u,\"input_source\":\"SIMULATED\"}",input.sequence));
    }
invalid_sim:
    httpd_resp_set_status(req,"400 Bad Request"); return json(req,snprintf(s_json,sizeof(s_json),"{\"error\":\"Expected flags temperature_deciC lux for four rooms\"}"));
}
static esp_err_t memory_api(httpd_req_t *req) {
    MemorySnapshot m=memory_snapshot();
    return json(req,snprintf(s_json,sizeof(s_json),"{\"source\":\"HARDWARE_MEASURED\",\"internal_free\":%u,\"internal_min\":%u,\"internal_largest\":%u,\"psram_total\":%u,\"psram_free\":%u,\"psram_largest\":%u,\"stack_task\":\"http\",\"stack_free_bytes\":%u,\"rx_dropped\":%u,\"memory_config_ok\":%s}",m.internal_free,m.internal_min,m.internal_largest,m.psram_total,m.psram_free,m.psram_largest,m.stack_free_bytes,s_rx.dropped(),memory_hardware_matches()?"true":"false"));
}
static esp_err_t rtos_api(httpd_req_t *req) {
    return json(req,snprintf(s_json,sizeof(s_json),"{\"source\":\"HARDWARE_MEASURED\",\"task\":\"LinkTask\",\"core\":0,\"priority\":3,\"period_ms\":5,\"jobs\":%u,\"max_exec_us\":%u,\"max_late_us\":%u,\"deadline_misses\":%u,\"stack_free_bytes\":%u,\"watchdog\":%s,\"task_count\":%u}",s_link_timing.jobs.load(),s_link_timing.max_exec_us.load(),s_link_timing.max_late_us.load(),s_link_timing.deadline_misses.load(),s_link_timing.handle?(unsigned)uxTaskGetStackHighWaterMark(s_link_timing.handle):0,s_link_timing.watchdog.load()?"true":"false",(unsigned)uxTaskGetNumberOfTasks()));
}
static esp_err_t state_api(httpd_req_t *req) {
    portENTER_CRITICAL(&s_mux); RoomReport r=s_report; bool have=s_have;
    uint32_t age=millis()-s_received,accepted=s_accepted,invalid=s_invalid,replayed=s_replayed;
    portEXIT_CRITICAL(&s_mux); bool known=have&&age<5000;
    if(!known) return json(req,snprintf(s_json,sizeof(s_json),"{\"room\":\"bedroom\",\"known\":false,\"occupancy\":\"UNKNOWN\",\"light\":null,\"fan\":null,\"authenticated\":true}"));
    return json(req,snprintf(s_json,sizeof(s_json),"{\"room\":\"bedroom\",\"known\":true,\"occupancy\":\"%s\",\"motion\":%s,\"light\":%s,\"fan\":%s,\"age_ms\":%u,\"reports\":%u,\"invalid\":%u,\"replayed\":%u,\"authenticated\":true,\"mode\":\"%s\",\"auto_flags\":%u,\"manual_flags\":%u,\"grace_ms\":%u,\"light_timer_ms\":%u,\"fan_timer_ms\":%u,\"light_on_ms\":%u,\"fan_on_ms\":%u,\"last_command\":%u}",occupancy_name(r.occupancy),r.motion?"true":"false",r.light?"true":"false",r.fan?"true":"false",age,accepted,invalid,replayed,r.mode?"AWAY":"HOME",r.auto_flags,r.manual_flags,r.grace_ms,r.timer_light_ms,r.timer_fan_ms,r.light_on_ms,r.fan_on_ms,r.last_command));
}
static esp_err_t transaction_api(httpd_req_t *req) {
    if(!authorized(req)) return unauthorized(req);
    portENTER_CRITICAL(&s_mux); Transaction t=s_transaction; portEXIT_CRITICAL(&s_mux);
    return json(req,snprintf(s_json,sizeof(s_json),"{\"source\":\"HARDWARE_MEASURED\",\"id\":%u,\"phase\":%u,\"status\":%u,\"light\":%s,\"fan\":%s,\"attempts\":%u,\"rtt_us\":%u,\"processing_us\":%u,\"outcome_known\":%s}",t.command.id,t.phase,(unsigned)t.ack.status,t.ack.light?"true":"false",t.ack.fan?"true":"false",t.attempts,t.rtt_us,t.ack.processing_us,t.phase==2?"true":"false"));
}
static esp_err_t control_api(httpd_req_t *req) {
    if(!authorized(req)) { httpd_resp_set_hdr(req,"Connection","close"); unauthorized(req); return ESP_FAIL; }
    if(!read_body(req)) return bad_body(req);
    ControlCommand c; uint32_t id=0;
    if(!parse_fields(s_body,c)) { httpd_resp_set_status(req,"400 Bad Request"); return json(req,snprintf(s_json,sizeof(s_json),"{\"error\":\"Expected op device action value\"}")); }
    if(!queue_command(c,id)) { httpd_resp_set_status(req,"409 Conflict"); return json(req,snprintf(s_json,sizeof(s_json),"{\"error\":\"Busy or Node A unavailable\"}")); }
    httpd_resp_set_status(req,"202 Accepted");
    return json(req,snprintf(s_json,sizeof(s_json),"{\"queued\":true,\"id\":%u,\"message\":\"Awaiting SafetyKernel acknowledgement\"}",id));
}
static esp_err_t voice_api(httpd_req_t *req) {
    if(!read_body(req)) return bad_body(req);
    neural_parse_result_t r={}; uint32_t start=micros(); neural_agent_process(s_body,nullptr,&r);
    return json(req,snprintf(s_json,sizeof(s_json),"{\"source\":\"HARDWARE_MEASURED\",\"intent\":%d,\"action\":\"%s\",\"room\":\"%s\",\"device\":\"%s\",\"requires_clarification\":%s,\"authorized\":false,\"parser_us\":%u}",(int)r.intent,r.action,r.room,r.device,r.requires_clarification?"true":"false",micros()-start));
}
static esp_err_t speech_state_api(httpd_req_t *req) {
    if(!authorized(req)) return unauthorized(req);
    SpeechSnapshot s=speech_snapshot();
#if defined(CLAWGRID_ESP_SR)
    const char *supported="true";
#else
    const char *supported="false";
#endif
    return json(req,snprintf(s_json,sizeof(s_json),"{\"processing_source\":\"HARDWARE_MEASURED\",\"supported\":%s,\"engine\":\"ESP-SR 1.7.0 MultiNet6 English\",\"phase\":%u,\"job\":%u,\"command_id\":%u,\"confidence\":%.5f,\"samples\":%u,\"inference_us\":%u,\"max_frame_us\":%u,\"frames\":%u,\"stack_free_bytes\":%u,\"pcm_crc32\":%u,\"diagnostic_text\":\"%s\",\"authorized\":false}",supported,s.phase,s.job,s.command_id,s.confidence,s.samples,s.inference_us,s.max_frame_us,s.frames,s.stack_free,s.pcm_crc32,s.diagnostic_text));
}
static esp_err_t speech_audio_api(httpd_req_t *req) {
    if(!authorized(req)) { httpd_resp_set_hdr(req,"Connection","close"); unauthorized(req); return ESP_FAIL; }
    if(req->content_len<3200 || req->content_len>128000 || (req->content_len&1)) {
        httpd_resp_set_hdr(req,"Connection","close"); httpd_resp_set_status(req,"400 Bad Request");
        json(req,snprintf(s_json,sizeof(s_json),"{\"error\":\"Use 16000 Hz mono signed PCM16LE, 3200 to 128000 bytes\"}")); return ESP_FAIL;
    }
    uint32_t job=0; uint8_t *buffer=speech_reserve_clip(req->content_len,job);
    if(!buffer) { httpd_resp_set_status(req,"409 Conflict"); httpd_resp_set_hdr(req,"Connection","close");
        json(req,snprintf(s_json,sizeof(s_json),"{\"error\":\"Speech model unavailable or busy\"}")); return ESP_FAIL;
    }
    size_t received=0; uint32_t upload_start=millis();
    while(received<req->content_len) {
        if((uint32_t)(millis()-upload_start)>5000) { speech_finish_clip(false); return ESP_FAIL; }
        int n=httpd_req_recv(req,(char*)buffer+received,std::min((size_t)4096,req->content_len-received));
        if(n<=0) { speech_finish_clip(false); return ESP_FAIL; }
        received+=n;
    }
    speech_finish_clip(true); httpd_resp_set_status(req,"202 Accepted");
    return json(req,snprintf(s_json,sizeof(s_json),"{\"queued\":true,\"job\":%u,\"authorized\":false}",job));
}
static esp_err_t evidence_api(httpd_req_t *req) {
    if(!authorized(req)) return unauthorized(req);
    // Pure shadow verification: synthetic vectors, no device requests/GPIO.
    uint32_t started=micros(),mask=evidence_campaign(),elapsed=micros()-started;
    return json(req,snprintf(s_json,sizeof(s_json),"{\"processing_source\":\"HARDWARE_MEASURED\",\"input_source\":\"SIMULATED\",\"scope\":\"shadow monitor selftest; no actuation\",\"passed_mask\":%u,\"cases\":%u,\"execution_us\":%u,\"monitor_bytes\":%u,\"physical_verification\":false}",mask,kEvidenceCases,elapsed,(unsigned)sizeof(EvidenceMonitor)));
}
static bool start_http() {
    httpd_config_t c=HTTPD_DEFAULT_CONFIG(); c.stack_size=6144; c.core_id=1; c.max_open_sockets=2;
    c.max_uri_handlers=13; c.lru_purge_enable=true; c.recv_wait_timeout=1; c.send_wait_timeout=1;
    if(httpd_start(&s_http,&c)!=ESP_OK) return false;
    const char *paths[]={"/","/api/state","/api/memory","/api/transaction","/api/control","/api/voice","/api/rtos","/simulation","/api/simulation","/api/simulation/input","/api/speech","/api/speech/audio","/api/evidence"};
    esp_err_t (*handlers[])(httpd_req_t*)={root,state_api,memory_api,transaction_api,control_api,voice_api,rtos_api,simulation_page,simulation_state_api,simulation_input_api,speech_state_api,speech_audio_api,evidence_api};
    for(int i=0;i<13;++i) { httpd_uri_t r={}; r.uri=paths[i]; r.method=((i>=4&&i<6)||i==9||i==11)?HTTP_POST:HTTP_GET; r.handler=handlers[i];
        if(httpd_register_uri_handler(s_http,&r)!=ESP_OK) { httpd_stop(s_http); s_http=nullptr; return false; }
    }
    return true;
}
static void control_tick(uint32_t now) {
    Transaction t; bool send=false,timeout=false;
    portENTER_CRITICAL(&s_mux);
    if(s_transaction.phase==1 && (!s_transaction.attempts || (uint32_t)(now-s_transaction.last_send)>=300)) {
        if(s_transaction.attempts>=4) { s_transaction.phase=3; timeout=true; }
        else { ++s_transaction.attempts; s_transaction.last_send=now; send=true; }
    }
    t=s_transaction; portEXIT_CRITICAL(&s_mux);
    if(send) { uint8_t p[kControlBytes]; encode_control(t.command,p);
        if(!secure_send(kNodeA,MSG_TYPE_DEVICE_COMMAND,2,1,s_seq++,p,sizeof(p))) ++s_tx_failed;
    }
    if(timeout) s_log.write("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"transaction\",\"id\":%u,\"phase\":3,\"attempts\":%u,\"outcome_known\":false}\n",t.command.id,t.attempts);
}
static void process_frame(const RxFrame &f) {
    clawgrid_header_t h; const uint8_t *p=nullptr;
    if(!secure_decode(f.mac,kNodeA,f.bytes,f.length,h,p) || h.src_node!=1 || h.dst_node!=2) {
        portENTER_CRITICAL(&s_mux); ++s_invalid; portEXIT_CRITICAL(&s_mux); return;
    }
    if(h.msg_type==MSG_TYPE_NODE_STATUS) {
        SimState state; if(!decode_sim_state(p,h.payload_len,state)) return;
        portENTER_CRITICAL(&s_mux);
        if(s_have && state.boot==s_report.boot_id && (!s_sim_received || state.boot!=s_sim_report.boot || sequence_newer(state.sequence,s_sim_report.sequence) || (state.sequence==s_sim_report.sequence && sequence_newer(state.at_ms,s_sim_report.at_ms)))) { s_sim_report=state; s_sim_received=millis(); }
        portEXIT_CRITICAL(&s_mux);
    } else if(h.msg_type==MSG_TYPE_ROOM_STATE) {
        RoomReport r;
        if(!decode_room_report(p,h.payload_len,r)) { portENTER_CRITICAL(&s_mux); ++s_invalid; portEXIT_CRITICAL(&s_mux); return; }
        portENTER_CRITICAL(&s_mux);
        if(s_have && (r.boot_id<s_report.boot_id || (r.boot_id==s_report.boot_id && !sequence_newer(h.seq_num,s_last_seq)))) ++s_replayed;
        else {
            if(s_have && r.boot_id!=s_report.boot_id) { s_last_control=0; if(s_transaction.phase==1) s_transaction.phase=3; }
            s_report=r; s_last_seq=h.seq_num; s_have=true; s_received=millis(); ++s_accepted;
        }
        portEXIT_CRITICAL(&s_mux);
    } else if(h.msg_type==MSG_TYPE_COMMAND_ACK) {
        ControlAck a; if(!decode_ack(p,h.payload_len,a)) return;
        bool accepted=false; Transaction t;
        portENTER_CRITICAL(&s_mux);
        uint8_t expected[kControlBytes]; encode_control(s_transaction.command,expected);
        if(s_transaction.phase==1 && a.boot==s_transaction.command.boot && a.id==s_transaction.command.id && !memcmp(a.request,expected,kControlBytes)) {
            s_transaction.ack=a; s_transaction.phase=2; s_transaction.rtt_us=micros()-s_transaction.start_us;
            if(a.status!=ControlStatus::WrongBoot && a.status!=ControlStatus::Replay) s_last_control=a.id;
            accepted=true;
        }
        t=s_transaction; portEXIT_CRITICAL(&s_mux);
        if(accepted) s_log.write("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"transaction\",\"id\":%u,\"phase\":2,\"status\":%u,\"attempts\":%u,\"rtt_us\":%u,\"processing_us\":%u,\"light\":%s,\"fan\":%s,\"outcome_known\":true}\n",a.id,(unsigned)a.status,t.attempts,t.rtt_us,a.processing_us,a.light?"true":"false",a.fan?"true":"false");
    }
}
static void diagnostic_service() {
    // LinkTask continues independently during an HTTP or memory diagnostic.
}
static void link_task(void*) {
    s_health.ready(kLinkReady,s_link_timing);
    TickType_t release=xTaskGetTickCount(); uint32_t expected=micros();
    for(;;) {
        uint32_t start=micros();
        RxFrame f; for(int i=0;i<4&&s_rx.pop(f);++i) process_frame(f);
        control_tick(millis());
        if(s_link_timing.watchdog.load()) esp_task_wdt_reset();
        s_link_timing.record(start,expected,5000); expected+=5000;
        vTaskDelayUntil(&release,pdMS_TO_TICKS(5));
    }
}
static void serial_tick() {
    for(int i=0;i<80 && Serial.available();++i) {
        char c=Serial.read();
        if(!s_serial_used && !s_serial_bad && c=='S') {
            portENTER_CRITICAL(&s_mux); RoomReport r=s_report; uint32_t age=millis()-s_received; bool known=s_have&&age<5000; portEXIT_CRITICAL(&s_mux);
            Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"state\",\"known\":%s,\"light\":%s,\"fan\":%s,\"mode\":%u,\"grace_ms\":%u,\"auto_flags\":%u,\"last_command\":%u,\"age_ms\":%u}\n",known?"true":"false",r.light?"true":"false",r.fan?"true":"false",r.mode,r.grace_ms,r.auto_flags,r.last_command,age); continue;
        }
        if(!s_serial_used && !s_serial_bad && (c=='F'||c=='R'||c=='W'||c=='V')) {
            portENTER_CRITICAL(&s_mux); ControlCommand cmd=s_transaction.command; RoomReport room=s_report; portEXIT_CRITICAL(&s_mux);
            if(c=='W') { cmd.boot=room.boot_id^0x80000000u; cmd.id=room.last_command+1; }
            if(c=='V') cmd.action^=1; // Authenticated altered payload, same ID.
            uint8_t payload[kControlBytes]; encode_control(cmd,payload);
            bool sent=false;
            if(c=='F') {
                uint8_t packet[CLAWGRID_MAX_PACKET],tag[32];
                size_t n=clawgrid_serialize(MSG_TYPE_DEVICE_COMMAND,2,1,s_seq++,0,payload,sizeof(payload),packet,sizeof(packet)-kAuthBytes);
                if(n && auth_tag(packet,n,tag)) { memcpy(packet+n,tag,kAuthBytes); packet[n]^=1; sent=esp_now_send(kNodeA,packet,n+kAuthBytes)==ESP_OK; }
            } else sent=secure_send(kNodeA,MSG_TYPE_DEVICE_COMMAND,2,1,s_seq++,payload,sizeof(payload));
            Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"injection\",\"type\":\"%c\",\"sent\":%s}\n",c,sent?"true":"false"); continue;
        }
        if(!s_serial_used && !s_serial_bad && c=='H') { http_selftest(diagnostic_service); continue; }
        if(!s_serial_used && !s_serial_bad && (c=='t'||c=='m')) {
            if(c=='m') memory_probe("B");
            else Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"selftest\",\"node\":\"B\",\"passed\":%s,\"rtos_ready\":%s}\n",(s_ok&&runtime_selftest()&&neural_agent_selftest()&&s_health.healthy(kLinkReady))?"true":"false",s_health.healthy(kLinkReady)?"true":"false");
            continue;
        }
        if(c=='\n') {
            s_serial[s_serial_used]=0; ControlCommand cmd; uint32_t id=0;
            if(!s_serial_bad && s_serial[0]=='@') {
                serial_http_bridge(s_serial); s_serial_used=0; s_serial_bad=false; continue;
            }
            bool ok=!s_serial_bad && s_serial[0]=='C' && parse_fields(s_serial+1,cmd) && queue_command(cmd,id);
            Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"queued\",\"accepted\":%s,\"id\":%u}\n",ok?"true":"false",id);
            s_serial_used=0; s_serial_bad=false;
        } else if(c!='\r') {
            if(s_serial_used+1<sizeof(s_serial)) s_serial[s_serial_used++]=c; else s_serial_bad=true;
        }
    }
}
void setup() {
    Serial.begin(115200); s_rx.begin(); s_log.begin(); neural_agent_init(); s_ok=runtime_selftest()&&neural_agent_selftest()&&security_selftest()&&s_health.begin();
    WiFi.mode(WIFI_AP_STA); WiFi.setSleep(false);
    bool ap=WiFi.softAP(kApSsid,kApPassword,CLAWGRID_WIFI_CHANNEL,false,2);
    s_radio=ap && esp_now_init()==ESP_OK && secure_peer(kNodeA);
    if(s_radio) esp_now_register_recv_cb(receive);
    if(s_ok) {
        s_link_timing.handle=xTaskCreateStaticPinnedToCore(link_task,"LinkTask",sizeof(s_link_stack),nullptr,3,s_link_stack,&s_link_tcb,0);
        if(!s_link_timing.handle) s_health.fail();
    }
    bool http=s_ok&&start_http();
    if(s_ok) speech_begin();
    Serial.printf("[B] radio=%s HTTP=%s IP=%s\n",s_radio?"ready":"failed",http?"ready":"failed",WiFi.softAPIP().toString().c_str());
}
void loop() {
    s_log.drain(); serial_tick(); uint32_t now=millis();
    static uint32_t last_memory=0;
    if((uint32_t)(now-last_memory)>=5000) {
        last_memory=now; memory_log("B",s_rx.dropped(),s_tx_failed.load());
        portENTER_CRITICAL(&s_mux); uint32_t a=s_accepted,b=s_invalid,c=s_replayed; portEXIT_CRITICAL(&s_mux);
        if(Serial) Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"link\",\"node\":\"B\",\"accepted\":%u,\"invalid\":%u,\"replayed\":%u,\"authenticated\":true,\"radio\":%s}\n",a,b,c,s_radio?"true":"false");
        s_link_timing.log("B","LinkTask",0,3,5,s_log.dropped.load());
    }
    vTaskDelay(pdMS_TO_TICKS(5));
}
