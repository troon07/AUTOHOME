#pragma once
#include "house_simulation.h"
inline bool house_selftest() {
    safety_kernel_init();
    if(safety_kernel_register_device("bedroom","light",DEVICE_TYPE_MOCK,-1,false)!=0 ||
       safety_kernel_register_device("bedroom","fan",DEVICE_TYPE_MOCK,-1,false)!=1) return false;
    HouseSimulation h; if(!h.begin()) return false;
    SimInput input; input.boot=7; input.sequence=1; input.issued_ms=10000;
    for(int i=0;i<4;++i) { input.flags[i]=56; input.temperature[i]=300; input.lux[i]=80; }
    input.flags[0]|=2;
    uint8_t bytes[kSimInputBytes]; encode_sim_input(input,bytes); SimInput decoded;
    if(!decode_sim_input(bytes,sizeof(bytes),decoded) || decoded.temperature[0]!=300 ||
       decode_sim_input(bytes,sizeof(bytes)-1,decoded)) return false;
    if(h.input(input,8,10000) || !h.input(input,7,10000) || h.input(input,7,10000)) return false;
    SimState state=h.tick(7,10000);
    if(state.outputs!=3 || state.occupied!=1 || state.known!=15 || state.accepted!=1 || state.rejected!=2) return false;
    state=h.tick(7,13000); if(state.outputs || state.known || state.occupied) return false;
    input.sequence=2; input.issued_ms=14000; input.flags[0]=32|4;
    if(!h.input(input,7,14000) || h.tick(7,16000).occupied) return false;
    ControlCommand c; c.op=ControlOp::Simulation; c.device=7; c.action=1;
    if(h.manual(c,16000)!=ControlStatus::Applied || h.tick(7,16000).outputs!=128) return false;
    c.device=8; if(h.manual(c,16000)!=ControlStatus::Invalid) return false;
    state=h.tick(7,16000); uint8_t report[kSimStateBytes]; encode_sim_state(state,report); SimState back;
    return decode_sim_state(report,sizeof(report),back) && back.outputs==state.outputs && back.sequence==2;
}
