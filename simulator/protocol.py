"""
ClawGrid V2.0 Protocol Engine
Handles binary packet framing, CRC16-CCITT, versioned envelope, and semantic messages.
"""

import struct
from enum import IntEnum
from typing import Optional, Tuple, Dict, Any

MAGIC_HEADER = 0x4347 # 'C', 'G'
MAGIC_WORD = 0x4347
PROTOCOL_VERSION = 0x02
MAX_PAYLOAD = 220

class MsgType(IntEnum):
    HELLO = 0x01
    HEARTBEAT = 0x02
    SENSOR_EVENT = 0x03
    ROOM_STATE = 0x04
    DEVICE_COMMAND = 0x05
    COMMAND_ACK = 0x06
    NODE_STATUS = 0x07
    MODEL_TASK = 0x08
    MODEL_RESULT = 0x09
    ERROR = 0x0F

class MsgFlags(IntEnum):
    NONE = 0x00
    ACK_REQUIRED = 0x01
    RETRY = 0x02
    URGENT = 0x04

def crc16_ccitt(data: bytes, poly: int = 0x1021, init: int = 0xFFFF) -> int:
    crc = init
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ poly) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc

class PacketEnvelope:
    HEADER_FORMAT = "!HBBBBHHIHH" # Magic(2), Ver(1), Type(1), Flags(1), Res(1), Src(2), Dst(2), Seq(4), Len(2), CRC(2)
    HEADER_SIZE = 18

    def __init__(self,
                 msg_type: MsgType,
                 src_node: int,
                 dst_node: int,
                 seq_num: int,
                 payload: bytes = b"",
                 flags: int = MsgFlags.NONE):
        self.magic = MAGIC_WORD
        self.version = PROTOCOL_VERSION
        self.msg_type = msg_type
        self.flags = flags
        self.src_node = src_node
        self.dst_node = dst_node
        self.seq_num = seq_num
        self.payload = payload
        self.crc = 0

    def serialize(self) -> bytes:
        payload_len = len(self.payload)
        if payload_len > MAX_PAYLOAD:
            raise ValueError("Payload exceeds protocol limit")
        if self.flags & ~7:
            raise ValueError("Unsupported flags")
        MsgType(self.msg_type)
        # Pack header with CRC = 0 to calculate CRC over header[:16] + payload
        temp_header = struct.pack(
            self.HEADER_FORMAT,
            self.magic,
            self.version,
            int(self.msg_type),
            self.flags,
            0, # reserved
            self.src_node,
            self.dst_node,
            self.seq_num,
            payload_len,
            0 # CRC placeholder
        )
        data_to_crc = temp_header[:16] + self.payload
        self.crc = crc16_ccitt(data_to_crc)
        final_header = temp_header[:16] + struct.pack("!H", self.crc)
        return final_header + self.payload

    @classmethod
    def deserialize(cls, data: bytes) -> Tuple[Optional['PacketEnvelope'], Optional[str]]:
        if len(data) < cls.HEADER_SIZE:
            return None, "Packet too short (less than header size)"
        
        magic, version, msg_type, flags, res, src, dst, seq, payload_len, crc = struct.unpack(
            cls.HEADER_FORMAT, data[:cls.HEADER_SIZE]
        )

        if magic != MAGIC_WORD:
            return None, f"Invalid magic word: 0x{magic:04X}"
        if version != PROTOCOL_VERSION:
            return None, f"Unsupported protocol version: 0x{version:02X}"
        if res != 0 or flags & ~7:
            return None, "Unsupported reserved field or flags"
        if payload_len > MAX_PAYLOAD:
            return None, "Payload exceeds protocol limit"
        if len(data) < cls.HEADER_SIZE + payload_len:
            return None, f"Payload truncated: expected {payload_len}, got {len(data) - cls.HEADER_SIZE}"
        if len(data) != cls.HEADER_SIZE + payload_len:
            return None, "Unexpected trailing bytes"

        payload = data[cls.HEADER_SIZE : cls.HEADER_SIZE + payload_len]
        data_to_crc = data[:16] + payload
        expected_crc = crc16_ccitt(data_to_crc)
        if crc != expected_crc:
            return None, f"CRC mismatch: got 0x{crc:04X}, expected 0x{expected_crc:04X}"

        try:
            mtype = MsgType(msg_type)
        except ValueError:
            return None, f"Unknown message type: {msg_type}"

        pkt = cls(mtype, src, dst, seq, payload, flags)
        pkt.crc = crc
        return pkt, None
