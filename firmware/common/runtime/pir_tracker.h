#pragma once
#include <stdint.h>
#include "state_wire.h"

class PirTracker {
public:
    bool sample(bool raw, uint32_t now) {
        if (raw != candidate_) { candidate_ = raw; candidate_since_ = now; }
        bool changed = false;
        if (candidate_ != stable_ && (uint32_t)(now - candidate_since_) >= 200) {
            stable_ = candidate_; changed = true;
        }
        if (stable_) { seen_ = true; last_motion_ = now; }
        return changed;
    }
    bool motion() const { return stable_; }
    Occupancy occupancy(uint32_t now, uint32_t grace_ms) const {
        if (!seen_) return Occupancy::Unknown;
        uint32_t elapsed = now - last_motion_;
        if (stable_ || elapsed < 30000) return Occupancy::Occupied;
        return elapsed < grace_ms ? Occupancy::LikelyOccupied : Occupancy::Vacant;
    }
private:
    bool candidate_ = false, stable_ = false, seen_ = false;
    uint32_t candidate_since_ = 0, last_motion_ = 0;
};
