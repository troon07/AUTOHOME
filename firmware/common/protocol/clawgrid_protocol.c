/* Canonical big-endian wire format; no packed-struct aliasing or allocation. */
#include "clawgrid_protocol.h"
#include <string.h>

static uint16_t crc_update(uint16_t crc, const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; ++b)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
    }
    return crc;
}
uint16_t clawgrid_crc16(const uint8_t *data, size_t length) {
    return crc_update(0xffff, data, length);
}
static void put16(uint8_t *p, uint16_t v) { p[0] = v >> 8; p[1] = v; }
static uint16_t get16(const uint8_t *p) { return ((uint16_t)p[0] << 8) | p[1]; }
static bool valid_type(uint8_t t) {
    return (t >= MSG_TYPE_HELLO && t <= MSG_TYPE_MODEL_RESULT) || t == MSG_TYPE_ERROR;
}

size_t clawgrid_serialize(clawgrid_msg_type_t type, uint16_t src, uint16_t dst,
                         uint32_t seq, uint8_t flags, const uint8_t *payload,
                         uint16_t payload_len, uint8_t *out, size_t capacity) {
    if (!out || payload_len > CLAWGRID_MAX_PAYLOAD ||
        (payload_len && !payload) || capacity < CLAWGRID_HEADER_SIZE + (size_t)payload_len ||
        !valid_type((uint8_t)type) || (flags & ~7u)) return 0;
    put16(out, CLAWGRID_MAGIC_WORD);
    out[2] = CLAWGRID_PROTOCOL_VERSION; out[3] = (uint8_t)type;
    out[4] = flags; out[5] = 0;
    put16(out + 6, src); put16(out + 8, dst);
    put16(out + 10, (uint16_t)(seq >> 16)); put16(out + 12, (uint16_t)seq);
    put16(out + 14, payload_len);
    if (payload_len) memcpy(out + CLAWGRID_HEADER_SIZE, payload, payload_len);
    uint16_t crc = crc_update(0xffff, out, 16);
    crc = crc_update(crc, out + CLAWGRID_HEADER_SIZE, payload_len);
    put16(out + 16, crc);
    return CLAWGRID_HEADER_SIZE + payload_len;
}

bool clawgrid_deserialize(const uint8_t *in, size_t length,
                         clawgrid_header_t *header, const uint8_t **payload) {
    if (payload) *payload = NULL;
    if (!in || !header || length < CLAWGRID_HEADER_SIZE ||
        length > CLAWGRID_MAX_PACKET) return false;
    uint16_t payload_len = get16(in + 14);
    if (get16(in) != CLAWGRID_MAGIC_WORD || in[2] != CLAWGRID_PROTOCOL_VERSION ||
        !valid_type(in[3]) || (in[4] & ~7u) || in[5] != 0 ||
        payload_len > CLAWGRID_MAX_PAYLOAD || length != CLAWGRID_HEADER_SIZE + (size_t)payload_len)
        return false;
    uint16_t crc = crc_update(0xffff, in, 16);
    crc = crc_update(crc, in + CLAWGRID_HEADER_SIZE, payload_len);
    if (crc != get16(in + 16)) return false;
    header->magic = get16(in); header->version = in[2]; header->msg_type = in[3];
    header->flags = in[4]; header->reserved = in[5];
    header->src_node = get16(in + 6); header->dst_node = get16(in + 8);
    header->seq_num = ((uint32_t)get16(in + 10) << 16) | get16(in + 12);
    header->payload_len = payload_len; header->crc16 = crc;
    if (payload && payload_len) *payload = in + CLAWGRID_HEADER_SIZE;
    return true;
}
