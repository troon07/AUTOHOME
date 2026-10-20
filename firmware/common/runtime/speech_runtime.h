#pragma once
#include <stdint.h>
#include <stddef.h>
struct SpeechSnapshot {
    uint32_t job=0,samples=0,inference_us=0,max_frame_us=0,frames=0,stack_free=0,pcm_crc32=0;
    uint8_t phase=0,command_id=0; float confidence=0;
    char diagnostic_text[96]={};
};
#if defined(CLAWGRID_ESP_SR)
bool speech_begin();
uint8_t *speech_reserve_clip(size_t bytes,uint32_t &job);
void speech_finish_clip(bool complete);
SpeechSnapshot speech_snapshot();
#else
inline bool speech_begin() { return false; }
inline uint8_t *speech_reserve_clip(size_t,uint32_t&) { return nullptr; }
inline void speech_finish_clip(bool) {}
inline SpeechSnapshot speech_snapshot() { return {}; }
#endif
