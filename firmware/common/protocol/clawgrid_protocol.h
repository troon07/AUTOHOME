/**
 * @file clawgrid_protocol.h
 * @brief ClawGrid V2.0 Inter-Node Binary Communication Protocol
 */

#ifndef CLAWGRID_PROTOCOL_H
#define CLAWGRID_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLAWGRID_MAGIC_WORD       0x4347  /* 'C', 'G' */
#define CLAWGRID_PROTOCOL_VERSION 0x02
#define CLAWGRID_HEADER_SIZE      18
#define CLAWGRID_MAX_PAYLOAD      220     /* Fits comfortably in ESP-NOW 250-byte MTU */
#define CLAWGRID_MAX_PACKET       (CLAWGRID_HEADER_SIZE + CLAWGRID_MAX_PAYLOAD)

typedef enum {
    MSG_TYPE_HELLO          = 0x01,
    MSG_TYPE_HEARTBEAT      = 0x02,
    MSG_TYPE_SENSOR_EVENT   = 0x03,
    MSG_TYPE_ROOM_STATE     = 0x04,
    MSG_TYPE_DEVICE_COMMAND = 0x05,
    MSG_TYPE_COMMAND_ACK    = 0x06,
    MSG_TYPE_NODE_STATUS    = 0x07,
    MSG_TYPE_MODEL_TASK     = 0x08,
    MSG_TYPE_MODEL_RESULT   = 0x09,
    MSG_TYPE_ERROR          = 0x0F
} clawgrid_msg_type_t;

typedef enum {
    FLAG_NONE         = 0x00,
    FLAG_ACK_REQUIRED = 0x01,
    FLAG_RETRY        = 0x02,
    FLAG_URGENT       = 0x04
} clawgrid_flags_t;

#pragma pack(push, 1)

typedef struct {
    uint16_t magic;          /* 0x4347 */
    uint8_t  version;        /* 0x02 */
    uint8_t  msg_type;       /* clawgrid_msg_type_t */
    uint8_t  flags;          /* clawgrid_flags_t */
    uint8_t  reserved;       /* 0x00 */
    uint16_t src_node;       /* Source Node ID */
    uint16_t dst_node;       /* Destination Node ID (0xFFFF = Broadcast) */
    uint32_t seq_num;        /* Monotonically increasing sequence number */
    uint16_t payload_len;    /* Payload length in bytes */
    uint16_t crc16;          /* CRC16-CCITT across header (bytes 0..15) + payload */
} clawgrid_header_t;

typedef struct {
    uint16_t model_id;
    uint8_t  partition_id;
    uint8_t  dtype;          /* 0=INT8, 1=UINT8, 2=FP16, 3=FP32 */
    uint16_t seq_len;
    uint16_t hidden_dim;
    float    quant_scale;
    int32_t  quant_zero_point;
    uint16_t chunk_index;
    uint16_t chunk_total;
} clawgrid_tensor_header_t;

#pragma pack(pop)

/* Function prototypes */
uint16_t clawgrid_crc16(const uint8_t *data, size_t length);
size_t clawgrid_serialize(clawgrid_msg_type_t type,
                          uint16_t src,
                          uint16_t dst,
                          uint32_t seq,
                          uint8_t flags,
                          const uint8_t *payload,
                          uint16_t payload_len,
                          uint8_t *out_buffer,
                          size_t max_out_len);

bool clawgrid_deserialize(const uint8_t *in_buffer,
                          size_t in_len,
                          clawgrid_header_t *out_header,
                          const uint8_t **out_payload);

#ifdef __cplusplus
}
#endif

#endif /* CLAWGRID_PROTOCOL_H */
