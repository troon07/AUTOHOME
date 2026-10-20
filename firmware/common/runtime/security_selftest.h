#pragma once
#include "secure_link.h"
#include "control_wire.h"
inline bool security_selftest() {
    // Independent SHA256-HMAC known-answer vector (key="key").
    const uint8_t expected[32]={0xf7,0xbc,0x83,0xf4,0x30,0x53,0x84,0x24,0xb1,0x32,0x98,0xe6,0xaa,0x6f,0xb1,0x43,0xef,0x4d,0x59,0xa1,0x49,0x46,0x17,0x59,0x97,0x47,0x9d,0xbc,0x2d,0x1a,0x3c,0xd8};
    const char *message="The quick brown fox jumps over the lazy dog";
    uint8_t digest[32];
    if(mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),(const uint8_t*)"key",3,(const uint8_t*)message,strlen(message),digest) || memcmp(digest,expected,32)) return false;
    ControlCommand c; c.boot=0x01020304; c.id=1; c.action=2;
    uint8_t payload[kControlBytes],packet[CLAWGRID_MAX_PACKET]; encode_control(c,payload);
    ControlCommand d;
    if(!decode_control(payload,sizeof(payload),d) || d.boot!=c.boot || d.action!=2) return false;
    ControlAck ack; ack.id=1; memcpy(ack.request,payload,sizeof(payload));
    uint8_t ack_bytes[kAckBytes]; encode_ack(ack,ack_bytes); ControlAck decoded;
    if(!decode_ack(ack_bytes,sizeof(ack_bytes),decoded) || memcmp(decoded.request,payload,sizeof(payload))) return false;
    size_t n=clawgrid_serialize(MSG_TYPE_DEVICE_COMMAND,2,1,1,0,payload,sizeof(payload),packet,sizeof(packet));
    if(!auth_tag(packet,n,digest)) return false;
    memcpy(packet+n,digest,kAuthBytes); n+=kAuthBytes;
    clawgrid_header_t h; const uint8_t *p=nullptr;
    if(!secure_decode(kNodeB,kNodeB,packet,n,h,p)) return false;
    packet[n-1]^=1;
    if(secure_decode(kNodeB,kNodeB,packet,n,h,p)) return false;
    packet[n-1]^=1;
    if(secure_decode(kNodeA,kNodeB,packet,n,h,p) || secure_decode(kNodeB,kNodeB,packet,n-1,h,p)) return false;
    return !secure_decode(kNodeB,kNodeB,packet,n-kAuthBytes,h,p);
}
