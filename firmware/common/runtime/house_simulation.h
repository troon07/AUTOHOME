#pragma once
#include "state_wire.h"
#include "safety_kernel.h"
#include "control_wire.h"
#include <string.h>

// Synthetic inputs only. They never enter the real bedroom PIR tracker.
static constexpr size_t kSimInputBytes=36,kSimStateBytes=32;
struct SimInput {
    uint32_t boot=0,sequence=0,issued_ms=0;
    uint8_t flags[4]={}; // PIR, presence, door; availability bits 3,4,5
    uint16_t temperature[4]={},lux[4]={}; // deci-Celsius; synthetic lux
};
inline void encode_sim_input(const SimInput &s,uint8_t *p) {
    memcpy(p,"SIM",3); p[3]=1; wire_put32(p+4,s.boot); wire_put32(p+8,s.sequence); wire_put32(p+12,s.issued_ms);
    for(int i=0;i<4;++i) { int o=16+i*5; p[o]=s.flags[i]; p[o+1]=s.temperature[i]>>8; p[o+2]=s.temperature[i]; p[o+3]=s.lux[i]>>8; p[o+4]=s.lux[i]; }
}
inline bool decode_sim_input(const uint8_t *p,size_t n,SimInput &s) {
    if(!p || n!=kSimInputBytes || memcmp(p,"SIM",3) || p[3]!=1) return false;
    SimInput v; v.boot=wire_get32(p+4); v.sequence=wire_get32(p+8); v.issued_ms=wire_get32(p+12);
    if(!v.sequence) return false;
    for(int i=0;i<4;++i) { int o=16+i*5; v.flags[i]=p[o]; v.temperature[i]=(uint16_t)p[o+1]<<8|p[o+2]; v.lux[i]=(uint16_t)p[o+3]<<8|p[o+4];
        if(v.flags[i]>63 || v.temperature[i]>600 || v.lux[i]>2000) return false;
    }
    s=v; return true;
}
struct SimState {
    uint32_t boot=0,sequence=0,at_ms=0,processing_us=0,accepted=0,rejected=0;
    uint8_t outputs=0,known=0,occupied=0;
    bool physical_outputs_enabled=false;
};
inline void encode_sim_state(const SimState &s,uint8_t *p) {
    memset(p,0,kSimStateBytes); memcpy(p,"SSM",3); p[3]=1;
    wire_put32(p+4,s.boot); wire_put32(p+8,s.sequence); wire_put32(p+12,s.at_ms); wire_put32(p+16,s.processing_us);
    p[20]=s.outputs; p[21]=s.known; p[22]=s.occupied; p[23]=s.physical_outputs_enabled;
    wire_put32(p+24,s.accepted); wire_put32(p+28,s.rejected);
}
inline bool decode_sim_state(const uint8_t *p,size_t n,SimState &s) {
    if(!p || n!=kSimStateBytes || memcmp(p,"SSM",3) || p[3]!=1 || p[21]>15 || p[22]>15 || p[23]>1) return false;
    s.boot=wire_get32(p+4); s.sequence=wire_get32(p+8); s.at_ms=wire_get32(p+12); s.processing_us=wire_get32(p+16);
    s.outputs=p[20]; s.known=p[21]; s.occupied=p[22]; s.physical_outputs_enabled=p[23]; s.accepted=wire_get32(p+24); s.rejected=wire_get32(p+28); return true;
}
class HouseSimulation {
public:
    bool begin() {
        for(int i=0;i<8;++i)
            if(safety_kernel_register_device(room(i/2),i%2?"fan":"light",i%2?DEVICE_TYPE_FAN:DEVICE_TYPE_LIGHT,-1,false)!=i+2) return false;
        return true;
    }
    bool input(const SimInput &v,uint32_t boot,uint32_t now) {
        int32_t age=(int32_t)(now-v.issued_ms);
        if(v.boot!=boot || !v.sequence || (state_.sequence && !sequence_newer(v.sequence,state_.sequence)) || age< -100 || age>3000) { ++state_.rejected; return false; }
        input_=v; received_=now; have_=true; state_.sequence=v.sequence; ++state_.accepted; return true;
    }
    ControlStatus manual(const ControlCommand &c,uint32_t now) {
        if(c.device>7 || c.action>2 || c.value) return ControlStatus::Invalid;
        const char *reason=nullptr;
        return safety_kernel_request_action(room(c.device/2),c.device%2?"fan":"light",(action_type_t)c.action,true,now,&reason)?ControlStatus::Applied:ControlStatus::Denied;
    }
    SimState tick(uint32_t boot,uint32_t now) {
        state_.boot=boot; state_.at_ms=now; state_.outputs=state_.known=state_.occupied=0;
        bool fresh=have_ && (uint32_t)(now-received_)<3000;
        for(int i=0;i<4;++i) {
            uint8_t f=input_.flags[i]; bool known=fresh&&(f&24);
            bool evidence=((f&8)&&(f&1)) || ((f&16)&&(f&2));
            if(known && evidence) { seen_[i]=true; last_seen_[i]=now; }
            // Five-second virtual vacancy grace makes demonstrations practical.
            // Door opening alone does not establish presence or vacancy.
            bool occupied=known&&seen_[i]&&(evidence||(uint32_t)(now-last_seen_[i])<5000);
            if(known) state_.known|=1<<i;
            if(occupied) state_.occupied|=1<<i;
            bool desired[2]={occupied && input_.lux[i]<300,occupied && input_.temperature[i]>=280};
            for(int d=0;d<2;++d) {
                const char *reason=nullptr;
                safety_kernel_request_action(room(i),d?"fan":"light",desired[d]?ACTION_ON:ACTION_OFF,false,now,&reason);
                const auto *device=safety_kernel_get_device(2+i*2+d);
                if(device && device->is_on) state_.outputs|=1<<(i*2+d);
            }
        }
        return state_;
    }
private:
    static const char *room(int i) { const char *names[]={"sim_bedroom","sim_living","sim_kitchen","sim_study"}; return names[i]; }
    SimInput input_; SimState state_; bool have_=false,seen_[4]={}; uint32_t received_=0,last_seen_[4]={};
};
