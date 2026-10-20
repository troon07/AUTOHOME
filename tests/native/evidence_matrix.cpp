#include "evidence_monitor.h"
#include <stdio.h>
int main() {
    puts("case,input_source,execution_source,age_ms,epoch_ok,command_ok,distinct_samples,separate_domains,feedback_bits,actual_on,clock_origin,ack_only,feedback_only,fresh_bound_feedback,monitor_verified,contract_oracle");
    const int ages[]={-1,0,499,501}; unsigned id=0,violations=0;
    for(unsigned origin=0;origin<2;++origin) for(int age:ages)
    for(unsigned epoch=0;epoch<2;++epoch) for(unsigned command=0;command<2;++command)
    for(unsigned distinct=0;distinct<2;++distinct) for(unsigned separate=0;separate<2;++separate)
    for(unsigned bits=0;bits<16;++bits) for(unsigned actual=0;actual<2;++actual) {
        uint32_t base=origin?0xfffffe00u:0,now=base+1000;
        EvidenceContract c; c.settle_ms=0; c.deadline_ms=5000; c.meter_domain=separate?2:1;
        EvidenceMonitor monitor(c); if(!monitor.begin(1,1,true,base)) return 2;
        ActuatorEvidence e{epoch?1u:2u,command?1u:2u,1u,now-(uint32_t)age,
                           (bool)(bits&1),(bool)(bits&2),(bool)(bits&4),(bool)(bits&8)};
        monitor.observe(e,now);
        e.sequence=distinct?2:1; e.sampled_ms=now+10-(uint32_t)age; monitor.observe(e,now+10);
        bool full=monitor.state()==EvidenceState::VerifiedOn;
        bool feedback=bits==15, bound=feedback&&age>=0&&age<=500&&epoch&&command;
        // Independent declarative oracle for this finite two-observation model.
        bool oracle=bound&&distinct&&separate;
        violations+=full&&!oracle;
        printf("%u,SIMULATED,HOST_MEASURED,%d,%u,%u,%u,%u,%u,%u,%u,1,%u,%u,%u,%u\n",
               ++id,age,epoch,command,distinct,separate,bits,actual,origin,feedback,bound,full,oracle);
    }
    return violations?1:0;
}
