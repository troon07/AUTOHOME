#pragma once
#include "automation.h"

// Boot only, using mock pins; caller rebuilds the real registry afterwards.
inline bool automation_selftest() {
    safety_kernel_init();
    if(safety_kernel_register_device("bedroom","light",DEVICE_TYPE_LIGHT,-1,false)!=0 ||
       safety_kernel_register_device("bedroom","fan",DEVICE_TYPE_FAN,-1,false)!=1) return false;
    Automation a; ControlCommand c; c.id=1;
    a.tick(100,true,Occupancy::Occupied);
    if(!safety_kernel_get_device(0)->is_on || safety_kernel_get_device(1)->is_on) return false;
    c.op=ControlOp::Device; c.action=0;
    if(a.execute(c,200)!=ControlStatus::Denied) return false;
    if(a.execute(c,4000)!=ControlStatus::Applied || safety_kernel_get_device(0)->is_on) return false;
    a.tick(8000,true,Occupancy::Occupied);
    if(safety_kernel_get_device(0)->is_on) return false; // manual OFF holds
    c.op=ControlOp::Resume;
    if(a.execute(c,8000)!=ControlStatus::Applied) return false;
    a.tick(8001,false,Occupancy::Occupied);
    if(!safety_kernel_get_device(0)->is_on) return false;
    c.op=ControlOp::Scene; c.action=1;
    if(a.execute(c,12000)!=ControlStatus::Applied || !a.away) return false;
    a.tick(16000,true,Occupancy::Occupied);
    if(safety_kernel_get_device(0)->is_on) return false;
    c.action=0; if(a.execute(c,16000)!=ControlStatus::Applied || a.away) return false;
    c.op=ControlOp::Policy; c.device=1; c.action=1;
    if(a.execute(c,16000)!=ControlStatus::Applied) return false;
    a.tick(16001,true,Occupancy::Occupied);
    if(!safety_kernel_get_device(1)->is_on) return false;
    c.op=ControlOp::Timer; c.action=0; c.value=3000;
    if(a.execute(c,17000)!=ControlStatus::Applied) return false;
    a.tick(20000,false,Occupancy::Occupied);
    if(safety_kernel_get_device(1)->is_on || a.timer_remaining(1,20000)) return false;
    c.value=999; if(a.execute(c,20000)!=ControlStatus::Invalid) return false;
    c.op=ControlOp::Policy; c.device=2; c.value=30000;
    if(a.execute(c,20000)!=ControlStatus::Applied || a.grace!=30000) return false;
    c.value=29999; if(a.execute(c,20000)!=ControlStatus::Invalid) return false;
    c.op=ControlOp::Device; c.device=7; c.value=0;
    if(a.execute(c,20000)!=ControlStatus::Invalid) return false;
    CommandGate gate; uint8_t p[kControlBytes]; c.id=1; encode_control(c,p);
    if(!gate.next(1)) return false;
    gate.commit(p,1);
    if(gate.next(1) || !gate.duplicate(p) || !gate.next(2)) return false;
    p[10]^=1; if(gate.duplicate(p)) return false;
    gate.last=UINT32_MAX; if(gate.next(0)) return false;
    // A timer that crosses millis() wrap expires normally.
    Automation wrapped; c={}; c.op=ControlOp::Device; c.device=1; c.action=1;
    if(wrapped.execute(c,0xfffff000)!=ControlStatus::Applied) return false;
    c.op=ControlOp::Timer; c.action=0; c.value=5000;
    if(wrapped.execute(c,0xfffff000)!=ControlStatus::Applied) return false;
    wrapped.tick(0xfffff000u+5000u,false,Occupancy::Unknown);
    if(safety_kernel_get_device(1)->is_on) return false;
    safety_kernel_init(); return true;
}
