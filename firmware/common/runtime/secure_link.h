#pragma once
#include <esp_now.h>
#include <mbedtls/md.h>
#include "private_config.h"
#include "clawgrid_protocol.h"

// CCMP unicast plus a mandatory application tag; plaintext/spoofed-MAC frames
// cannot authorize actions. CRC remains only accidental-corruption detection.
static constexpr size_t kAuthBytes = 16;
inline bool secure_peer(const uint8_t *mac) {
    if (esp_now_set_pmk(kPmk) != ESP_OK) return false;
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr,mac,6); memcpy(peer.lmk,kLmk,16);
    peer.channel=CLAWGRID_WIFI_CHANNEL; peer.ifidx=WIFI_IF_STA; peer.encrypt=true;
    return esp_now_add_peer(&peer)==ESP_OK;
}
inline bool auth_tag(const uint8_t *p, size_t n, uint8_t *out) {
    return mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
        kHmacKey,sizeof(kHmacKey),p,n,out)==0;
}
inline bool secure_decode(const uint8_t *mac, const uint8_t *expected,
    const uint8_t *data, size_t n, clawgrid_header_t &h, const uint8_t *&payload) {
    if (memcmp(mac,expected,6) || n<CLAWGRID_HEADER_SIZE+kAuthBytes) return false;
    uint8_t digest[32];
    if (!auth_tag(data,n-kAuthBytes,digest)) return false;
    uint8_t difference=0;
    for (size_t i=0;i<kAuthBytes;++i) difference |= digest[i]^data[n-kAuthBytes+i];
    return difference==0 && clawgrid_deserialize(data,n-kAuthBytes,&h,&payload);
}
inline bool secure_send(const uint8_t *mac, clawgrid_msg_type_t type, uint16_t src,
    uint16_t dst, uint32_t sequence, const uint8_t *payload, size_t bytes) {
    uint8_t packet[CLAWGRID_MAX_PACKET], digest[32];
    if (bytes>CLAWGRID_MAX_PAYLOAD-kAuthBytes) return false;
    size_t n=clawgrid_serialize(type,src,dst,sequence,0,payload,bytes,packet,sizeof(packet)-kAuthBytes);
    if (!n || !auth_tag(packet,n,digest)) return false;
    memcpy(packet+n,digest,kAuthBytes);
    return esp_now_send(mac,packet,n+kAuthBytes)==ESP_OK;
}
