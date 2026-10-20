#pragma once
#include <stdint.h>

// Post-actuation observation only. Never writes GPIO or bypasses SafetyKernel.
// All timestamps use the actuator's monotonic clock. A valid transport tag is
// an upstream requirement, not a substitute for independent physical sensors.
enum class EvidenceState:uint8_t { Unknown,Settling,VerifiedOff,VerifiedOn,Fault };
enum class EvidenceFault:uint8_t { None,Timeout,Disagreement };
struct EvidenceContract {
    uint32_t settle_ms=250,lease_ms=500,deadline_ms=1000;
    uint8_t stable_samples=2,aux_domain=1,meter_domain=2;
};
struct ActuatorEvidence {
    uint32_t boot=0,command=0,sequence=0,sampled_ms=0;
    bool aux_valid=false,meter_valid=false,aux_on=false,electrical_on=false;
    ActuatorEvidence(uint32_t b=0,uint32_t c=0,uint32_t s=0,uint32_t at=0,
                     bool av=false,bool mv=false,bool a=false,bool e=false):
        boot(b),command(c),sequence(s),sampled_ms(at),aux_valid(av),meter_valid(mv),aux_on(a),electrical_on(e) {}
};
class EvidenceMonitor {
public:
    explicit EvidenceMonitor(EvidenceContract contract={}):contract_(contract) {}
    bool begin(uint32_t boot,uint32_t command,bool desired,uint32_t now) {
        if(!valid_contract() || !boot || !command || (boot_ && boot!=boot_) ||
           (command_ && command<=command_) || (latched_ && desired)) return false;
        if(command_ && state_==EvidenceState::Settling && desired==desired_) return false;
        boot_=boot; command_=command; desired_=desired; issued_=now;
        have_=false; stable_=0; last_sequence_=0; verified_once_=false;
        state_=latched_?EvidenceState::Fault:EvidenceState::Settling;
        return true;
    }
    bool observe(const ActuatorEvidence &e,uint32_t now) {
        int32_t age=(int32_t)(now-e.sampled_ms),since=(int32_t)(e.sampled_ms-issued_);
        if(!command_ || e.boot!=boot_ || e.command!=command_ || !e.sequence ||
           (last_sequence_ && (int32_t)(e.sequence-last_sequence_)<=0) ||
           age<0 || (uint32_t)age>contract_.lease_ms || since<0 ||
           (have_ && (int32_t)(e.sampled_ms-sample_.sampled_ms)<=0)) return false;
        if(have_ && (uint32_t)(now-sample_.sampled_ms)>contract_.lease_ms) stable_=0;
        // Sequence advances even for unavailable/contradictory observations;
        // replaying a previous agreeing sample cannot renew its evidence lease.
        last_sequence_=e.sequence; sample_=e; have_=true;
        bool eligible=e.aux_valid&&e.meter_valid&&contract_.aux_domain&&contract_.meter_domain&&
                      contract_.aux_domain!=contract_.meter_domain;
        bool agree=eligible&&e.aux_on==desired_&&e.electrical_on==desired_;
        if(agree && (uint32_t)since>=contract_.settle_ms) { if(stable_<255) ++stable_; }
        else stable_=0;
        if(eligible && !agree && verified_once_) fault(EvidenceFault::Disagreement);
        update(now); return true;
    }
    EvidenceState update(uint32_t now) {
        if(!command_) return state_=EvidenceState::Unknown;
        if(latched_) return state_=EvidenceState::Fault;
        if(!verified_once_ && (uint32_t)(now-issued_)>=contract_.deadline_ms) {
            fault(EvidenceFault::Timeout); return state_;
        }
        bool fresh=have_ && (int32_t)(now-sample_.sampled_ms)>=0 && (uint32_t)(now-sample_.sampled_ms)<=contract_.lease_ms;
        bool agree=fresh&&sample_.aux_valid&&sample_.meter_valid&&contract_.aux_domain&&
                   contract_.aux_domain!=contract_.meter_domain&&contract_.meter_domain&&
                   sample_.aux_on==desired_&&sample_.electrical_on==desired_;
        if(agree&&stable_>=contract_.stable_samples) {
            verified_once_=true; return state_=desired_?EvidenceState::VerifiedOn:EvidenceState::VerifiedOff;
        }
        if(!fresh) stable_=0;
        // A formerly verified device becomes UNKNOWN when its lease expires.
        // Neither the desired state nor the previous ACK proves current state.
        return state_=verified_once_?EvidenceState::Unknown:EvidenceState::Settling;
    }
    bool clear_fault(uint32_t now) {
        bool fresh=have_ && (int32_t)(now-sample_.sampled_ms)>=0 && (uint32_t)(now-sample_.sampled_ms)<=contract_.lease_ms;
        if(!latched_ || desired_ || !fresh || !sample_.aux_valid || !sample_.meter_valid ||
           sample_.aux_on || sample_.electrical_on || stable_<contract_.stable_samples ||
           !contract_.aux_domain || !contract_.meter_domain || contract_.aux_domain==contract_.meter_domain) return false;
        latched_=false; fault_=EvidenceFault::None; verified_once_=true; state_=EvidenceState::VerifiedOff; return true;
    }
    EvidenceState state() const { return state_; }
    EvidenceFault reason() const { return fault_; }
    bool inhibit_on() const { return latched_ || state_==EvidenceState::Unknown; }
    // A caller may propose OFF through SafetyKernel; this is not proof of OFF,
    // an emergency stop, or a remedy for welded contacts/failed power hardware.
    bool request_off() const { return latched_; }
private:
    bool valid_contract() const {
        return contract_.stable_samples && contract_.lease_ms && contract_.deadline_ms<0x80000000u &&
               contract_.lease_ms<0x80000000u && contract_.settle_ms<contract_.deadline_ms;
    }
    void fault(EvidenceFault reason) { latched_=true; state_=EvidenceState::Fault; fault_=reason; }
    EvidenceContract contract_; ActuatorEvidence sample_;
    uint32_t boot_=0,command_=0,issued_=0,last_sequence_=0;
    uint8_t stable_=0; bool desired_=false,have_=false,verified_once_=false,latched_=false;
    EvidenceState state_=EvidenceState::Unknown; EvidenceFault fault_=EvidenceFault::None;
};
