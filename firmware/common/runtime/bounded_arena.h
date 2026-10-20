#pragma once
#include <stddef.h>
#include <stdint.h>

/* Non-owning, fixed-capacity arena. Reset only after all users have finished.
   No malloc/free on an inference path; one owner, not thread-safe. */
class BoundedArena {
public:
    void attach(void *memory, size_t bytes) {
        base_ = static_cast<uint8_t *>(memory); capacity_ = memory ? bytes : 0;
        used_ = 0; peak_ = 0; failures_ = 0;
    }
    void *allocate(size_t bytes, size_t alignment = 16) {
        if (!base_ || !bytes || !alignment || (alignment & (alignment - 1)))
            return fail();
        uintptr_t address = reinterpret_cast<uintptr_t>(base_) + used_;
        size_t padding = (alignment - (address & (alignment - 1))) & (alignment - 1);
        size_t remaining = capacity_ - used_;
        if (padding > remaining || bytes > remaining - padding) return fail();
        void *result = base_ + used_ + padding;
        used_ += padding + bytes;
        if (used_ > peak_) peak_ = used_;
        return result;
    }
    void reset() { used_ = 0; }
    size_t used() const { return used_; }
    size_t peak() const { return peak_; }
    size_t capacity() const { return capacity_; }
    size_t failures() const { return failures_; }
private:
    void *fail() { ++failures_; return nullptr; }
    uint8_t *base_ = nullptr;
    size_t capacity_ = 0, used_ = 0, peak_ = 0, failures_ = 0;
};
