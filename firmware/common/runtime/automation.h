#pragma once
#include "safety_kernel.h"
#include "control_wire.h"
#include "policy_record.h"

struct AuditEvent { uint32_t at,id; uint8_t op,status,light,fan; };
class Automation {
public:
    bool away=false, automatic[2]={true,false};
    uint32_t grace=120000, on_ms[2]={}, transitions[2]={};
    AuditEvent events[32]={}; uint32_t event_count=0;
    PolicyConfig policy() const {
        PolicyConfig p; p.away=away; p.light_auto=automatic[0]; p.fan_auto=automatic[1]; p.grace_ms=grace; return p;
    }
    void restore_policy(const PolicyConfig &p) {
        away=p.away; automatic[0]=p.light_auto; automatic[1]=p.fan_auto; grace=p.grace_ms;
    }
    ControlStatus execute(const ControlCommand &c,uint32_t now) {
        if(c.op==ControlOp::Device) {
            if(c.device>1 || c.action>2 || c.value) return record(c,now,ControlStatus::Invalid);
            return record(c,now,act(c.device,(action_type_t)c.action,true,now)?ControlStatus::Applied:ControlStatus::Denied);
        }
        if(c.op==ControlOp::Scene) {
            // Individual outcomes are real; a denied scene may partially apply.
            if(c.device || c.action>3 || c.value) return record(c,now,ControlStatus::Invalid);
            bool a=true,b=true;
            if(c.action) {
                a=act(0,ACTION_OFF,true,now);
                b=act(1,c.action==2?ACTION_ON:ACTION_OFF,true,now);
            }
            if(a && b) {
                away=c.action==1;
                for(int i=0;i<2;++i) { timer_[i]=false; pending_[i]=false; }
                if(c.action==0) for(int i=0;i<2;++i) safety_kernel_release_manual("bedroom",name(i));
            }
            return record(c,now,a&&b?ControlStatus::Applied:ControlStatus::Denied);
        }
        if(c.op==ControlOp::Timer) {
            if(c.device>1 || c.action || (c.value && (c.value<1000 || c.value>86400000))) return record(c,now,ControlStatus::Invalid);
            timer_[c.device]=c.value!=0; timer_start_[c.device]=now; timer_duration_[c.device]=c.value;
            return record(c,now,ControlStatus::Applied);
        }
        if(c.op==ControlOp::Policy) {
            if(c.device<2 && c.action<2 && !c.value) automatic[c.device]=c.action;
            else if(c.device==2 && !c.action && c.value>=30000 && c.value<=1800000) grace=c.value;
            else return record(c,now,ControlStatus::Invalid);
            pending_[0]=pending_[1]=false;
            return record(c,now,ControlStatus::Applied);
        }
        if(c.op==ControlOp::Resume && c.device<2 && !c.action && !c.value) {
            safety_kernel_release_manual("bedroom",name(c.device));
            return record(c,now,ControlStatus::Applied);
        }
        return record(c,now,ControlStatus::Invalid);
    }
    void tick(uint32_t now,bool motion_rise,Occupancy occupancy) {
        uint32_t elapsed=now-last_tick_; last_tick_=now;
        for(int i=0;i<2;++i) {
            const managed_device_t *d=safety_kernel_get_device(i);
            if(d && previous_[i]) on_ms[i]+=elapsed;
            if(d && d->is_on!=previous_[i]) ++transitions[i];
            previous_[i]=d && d->is_on;
            if(timer_[i] && (uint32_t)(now-timer_start_[i])>=timer_duration_[i]) {
                if(act(i,ACTION_OFF,true,now)) { timer_[i]=false; pending_[i]=false; }
            }
            if(motion_rise && automatic[i] && !away) pending_[i]=true;
            if(!automatic[i] || away) continue;
            if(occupancy==Occupancy::Vacant) {
                pending_[i]=false; act(i,ACTION_OFF,false,now);
            } else if(pending_[i] && act(i,ACTION_ON,false,now)) pending_[i]=false;
        }
    }
    uint32_t timer_remaining(int i,uint32_t now) const {
        if(!timer_[i]) return 0;
        uint32_t elapsed=now-timer_start_[i];
        return elapsed>=timer_duration_[i]?1:timer_duration_[i]-elapsed;
    }
    void report(RoomReport &s,uint32_t now) const {
        s.mode=away; s.auto_flags=automatic[0]|(automatic[1]<<1); s.grace_ms=grace;
        s.timer_light_ms=timer_remaining(0,now); s.timer_fan_ms=timer_remaining(1,now);
        s.light_on_ms=on_ms[0]; s.fan_on_ms=on_ms[1];
        for(int i=0;i<2;++i) { const auto *d=safety_kernel_get_device(i); if(d && d->manual_override) s.manual_flags|=1<<i; }
    }
private:
    bool timer_[2]={},pending_[2]={},previous_[2]={};
    uint32_t timer_start_[2]={},timer_duration_[2]={},last_tick_=0;
    static const char *name(int i) { return i?"fan":"light"; }
    bool act(int i,action_type_t action,bool manual,uint32_t now) {
        const char *reason=nullptr;
        return safety_kernel_request_action("bedroom",name(i),action,manual,now,&reason);
    }
    ControlStatus record(const ControlCommand &c,uint32_t now,ControlStatus status) {
        const auto *a=safety_kernel_get_device(0),*b=safety_kernel_get_device(1);
        events[event_count++%32]={now,c.id,(uint8_t)c.op,(uint8_t)status,(uint8_t)(a&&a->is_on),(uint8_t)(b&&b->is_on)};
        return status;
    }
};
