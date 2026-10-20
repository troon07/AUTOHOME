#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Explicit bytes: no compiler padding or host endianness is stored in flash.
struct PolicyConfig {
    bool away=false, light_auto=true, fan_auto=false;
    uint32_t grace_ms=120000;
    bool operator==(const PolicyConfig &b) const {
        return away==b.away && light_auto==b.light_auto && fan_auto==b.fan_auto && grace_ms==b.grace_ms;
    }
    bool operator!=(const PolicyConfig &b) const { return !(*this==b); }
};
static constexpr size_t kPolicyRecordBytes=16;
inline uint32_t policy_crc(const uint8_t *p,size_t n) {
    uint32_t crc=~0u;
    while(n--) { crc^=*p++; for(int bit=0;bit<8;++bit) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1))); }
    return ~crc;
}
inline void policy_put32(uint8_t *p,uint32_t value) {
    for(int i=0;i<4;++i) p[i]=(uint8_t)(value>>(24-i*8));
}
inline uint32_t policy_get32(const uint8_t *p) {
    return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];
}
inline bool valid_policy(const PolicyConfig &c) { return c.grace_ms>=30000 && c.grace_ms<=1800000; }
inline void encode_policy(const PolicyConfig &c,uint8_t *p) {
    memset(p,0,kPolicyRecordBytes); memcpy(p,"CGP",3); p[3]=1;
    p[4]=c.away|(c.light_auto<<1)|(c.fan_auto<<2); policy_put32(p+8,c.grace_ms);
    policy_put32(p+12,policy_crc(p,12));
}
inline bool decode_policy(const uint8_t *p,size_t n,PolicyConfig &out) {
    if(!p || n!=kPolicyRecordBytes || memcmp(p,"CGP",3) || p[3]!=1 ||
       p[4]>7 || p[5] || p[6] || p[7] || policy_get32(p+12)!=policy_crc(p,12)) return false;
    PolicyConfig c; c.away=p[4]&1; c.light_auto=p[4]&2; c.fan_auto=p[4]&4;
    c.grace_ms=policy_get32(p+8); if(!valid_policy(c)) return false;
    out=c; return true;
}
