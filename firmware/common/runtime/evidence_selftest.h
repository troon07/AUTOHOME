#pragma once
#include "evidence_monitor.h"
static constexpr unsigned kEvidenceCases=23;
inline bool evidence_case(unsigned id) {
    if(id==22) {
        EvidenceMonitor repeat; repeat.begin(1,1,true,1000);
        return !repeat.begin(1,2,true,1900) && repeat.update(2000)==EvidenceState::Fault;
    }
    if(id==16) {
        EvidenceMonitor late; late.begin(1,1,true,1000);
        ActuatorEvidence x{1,1,1,1250,true,true,true,true}; late.observe(x,1250);
        x.sequence=2; x.sampled_ms=2000; late.observe(x,2000);
        return late.state()==EvidenceState::Fault;
    }
    if(id==17) {
        EvidenceMonitor wrap; wrap.begin(1,1,true,0xffffff00u);
        ActuatorEvidence x{1,1,1,0x10,true,true,true,true}; wrap.observe(x,0x10);
        x.sequence=2; x.sampled_ms=0x20; wrap.observe(x,0x20);
        return wrap.state()==EvidenceState::VerifiedOn;
    }
    if(id==18) {
        EvidenceMonitor gap; gap.begin(1,1,true,1000);
        ActuatorEvidence x{1,1,1,1250,true,true,true,true}; gap.observe(x,1250);
        x.sequence=2; x.sampled_ms=1800; gap.observe(x,1800);
        return gap.state()!=EvidenceState::VerifiedOn;
    }
    if(id==19) {
        EvidenceMonitor reverse; reverse.begin(1,1,true,1000);
        ActuatorEvidence x{1,1,1,1250,true,true,true,true}; reverse.observe(x,1250);
        x.sequence=2; x.sampled_ms=1249;
        return !reverse.observe(x,1250);
    }
    if(id==20) {
        EvidenceMonitor equal; equal.begin(1,1,true,1000);
        ActuatorEvidence x{1,1,1,1250,true,true,true,true}; equal.observe(x,1250);
        x.sequence=2;
        return !equal.observe(x,1250) && equal.state()!=EvidenceState::VerifiedOn;
    }
    if(id==21) {
        // Independent truth-table oracle over modality availability/value,
        // shared fault domain and 32 clock origins including uint32 wrap.
        for(unsigned origin=0;origin<32;++origin) for(unsigned bits=0;bits<16;++bits) for(unsigned shared=0;shared<2;++shared) {
            EvidenceContract c; c.settle_ms=0; c.meter_domain=shared?1:2;
            EvidenceMonitor sweep(c); uint32_t t=0xfffffff0u+origin;
            if(!sweep.begin(1,1,true,t)) return false;
            ActuatorEvidence x{1,1,1,t+1,(bool)(bits&1),(bool)(bits&2),(bool)(bits&4),(bool)(bits&8)};
            sweep.observe(x,t+1); x.sequence=2; x.sampled_ms=t+2; sweep.observe(x,t+2);
            bool expected=bits==15 && !shared;
            if((sweep.state()==EvidenceState::VerifiedOn)!=expected) return false;
        }
        return true;
    }
    EvidenceMonitor m;
    if(id==0) return m.state()==EvidenceState::Unknown && m.inhibit_on();
    if(!m.begin(1,1,true,1000)) return false;
    ActuatorEvidence e; e.boot=1; e.command=1; e.sequence=1; e.sampled_ms=1250;
    e.aux_valid=e.meter_valid=e.aux_on=e.electrical_on=true;
    if(id==1) return m.update(2000)==EvidenceState::Fault && m.reason()==EvidenceFault::Timeout;
    if(id==2) { e.boot=2; return !m.observe(e,1250); }
    if(id==3) { e.command=2; return !m.observe(e,1250); }
    if(id==4) return !m.observe(e,1249);
    if(id==5) return !m.observe(e,1751);
    if(id==6) { e.sampled_ms=999; return !m.observe(e,1250); }
    if(id==7) return m.observe(e,1250) && !m.observe(e,1251) && m.state()!=EvidenceState::VerifiedOn;
    if(id==8) { e.meter_valid=false; m.observe(e,1250); ++e.sequence; m.observe(e,1260); return m.update(2000)==EvidenceState::Fault; }
    if(id==9) { EvidenceContract c; c.meter_domain=c.aux_domain; EvidenceMonitor shared(c); shared.begin(1,1,true,1000); shared.observe(e,1250); ++e.sequence; shared.observe(e,1260); return shared.update(2000)==EvidenceState::Fault; }
    if(id==10) return !m.begin(1,1,true,1500) && m.update(2000)==EvidenceState::Fault;
    if(!m.observe(e,1250)) return false;
    e.sequence=2; e.sampled_ms=1260;
    if(!m.observe(e,1260) || m.state()!=EvidenceState::VerifiedOn) return false;
    if(id==11) return true;
    if(id==12) return m.update(1761)==EvidenceState::Unknown && m.inhibit_on();
    e.sequence=3; e.sampled_ms=1270; e.aux_on=false;
    if(!m.observe(e,1270) || m.state()!=EvidenceState::Fault || m.reason()!=EvidenceFault::Disagreement) return false;
    if(id==13) return !m.begin(1,2,true,1300) && m.request_off();
    if(id==14) return !m.clear_fault(1270);
    if(id==15) {
        if(!m.begin(1,2,false,1300)) return false;
        e.command=2; e.sequence=1; e.sampled_ms=1550; e.aux_on=e.electrical_on=false;
        if(!m.observe(e,1550)) return false;
        e.sequence=2; e.sampled_ms=1560;
        return m.observe(e,1560) && m.clear_fault(1560) && m.state()==EvidenceState::VerifiedOff;
    }
    return false;
}
inline uint32_t evidence_campaign() {
    uint32_t passed=0; for(unsigned i=0;i<kEvidenceCases;++i) if(evidence_case(i)) passed|=1u<<i;
    return passed;
}
