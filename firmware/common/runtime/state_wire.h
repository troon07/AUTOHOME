#pragma once
#include <stdint.h>
#include <stddef.h>

enum class Occupancy : uint8_t { Unknown = 0, Occupied = 1, LikelyOccupied = 2, Vacant = 3 };
struct RoomReport {
    bool motion = false, light = false, fan = false;
    Occupancy occupancy = Occupancy::Unknown;
    uint32_t boot_id = 0, uptime_ms = 0;
    uint32_t last_command=0, grace_ms=120000, timer_light_ms=0, timer_fan_ms=0;
    uint32_t light_on_ms=0, fan_on_ms=0;
    uint8_t mode=0, auto_flags=1, manual_flags=0;
};
static constexpr size_t kRoomReportBytes = 44;
inline void wire_put32(uint8_t *p, uint32_t v) {
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}
inline uint32_t wire_get32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
inline void encode_room_report(const RoomReport &s, uint8_t (&p)[kRoomReportBytes]) {
    p[0] = 2; p[1] = 1; // schema 2, bedroom 1; one physical zone
    p[2] = s.motion; p[3] = static_cast<uint8_t>(s.occupancy);
    p[4] = s.light; p[5] = s.fan; p[6] = 0; p[7] = 0;
    wire_put32(p + 8, s.boot_id); wire_put32(p + 12, s.uptime_ms);
    wire_put32(p+16,s.last_command); p[20]=s.mode; p[21]=s.auto_flags;
    p[22]=s.manual_flags; p[23]=0; wire_put32(p+24,s.grace_ms);
    wire_put32(p+28,s.timer_light_ms); wire_put32(p+32,s.timer_fan_ms);
    wire_put32(p+36,s.light_on_ms); wire_put32(p+40,s.fan_on_ms);
}
inline bool decode_room_report(const uint8_t *p, size_t n, RoomReport &s) {
    if (!p || n != kRoomReportBytes || p[0] != 2 || p[1] != 1 ||
        p[2] > 1 || p[3] > 3 || p[4] > 1 || p[5] > 1 || p[6] || p[7] ||
        p[20]>1 || p[21]>3 || p[22]>3 || p[23]) return false;
    s.motion = p[2]; s.occupancy = static_cast<Occupancy>(p[3]);
    s.light = p[4]; s.fan = p[5]; s.boot_id = wire_get32(p + 8); s.uptime_ms = wire_get32(p + 12);
    s.last_command=wire_get32(p+16); s.mode=p[20]; s.auto_flags=p[21]; s.manual_flags=p[22];
    s.grace_ms=wire_get32(p+24); s.timer_light_ms=wire_get32(p+28); s.timer_fan_ms=wire_get32(p+32);
    s.light_on_ms=wire_get32(p+36); s.fan_on_ms=wire_get32(p+40);
    return true;
}
inline const char *occupancy_name(Occupancy s) {
    switch (s) {
        case Occupancy::Occupied: return "OCCUPIED";
        case Occupancy::LikelyOccupied: return "LIKELY_OCCUPIED";
        case Occupancy::Vacant: return "VACANT";
        default: return "UNKNOWN";
    }
}
inline bool sequence_newer(uint32_t next, uint32_t last) {
    uint32_t delta = next - last;
    return delta != 0 && delta < 0x80000000u;
}
