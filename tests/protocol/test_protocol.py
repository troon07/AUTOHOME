"""Unit tests for the ClawGrid Protocol Engine."""

import pytest
from simulator.protocol import PacketEnvelope, MsgType, MsgFlags, crc16_ccitt

def test_packet_serialization_deserialization():
    payload = b"CLAWGRID_TEST_PAYLOAD"
    pkt = PacketEnvelope(
        msg_type=MsgType.DEVICE_COMMAND,
        src_node=0x01,
        dst_node=0x02,
        seq_num=42,
        payload=payload,
        flags=MsgFlags.ACK_REQUIRED
    )
    raw_bytes = pkt.serialize()

    # Deserialization check
    parsed, err = PacketEnvelope.deserialize(raw_bytes)
    assert err is None
    assert parsed is not None
    assert parsed.msg_type == MsgType.DEVICE_COMMAND
    assert parsed.src_node == 0x01
    assert parsed.dst_node == 0x02
    assert parsed.seq_num == 42
    assert parsed.payload == payload
    assert parsed.flags == MsgFlags.ACK_REQUIRED

def test_corrupted_crc_rejection():
    pkt = PacketEnvelope(
        msg_type=MsgType.HEARTBEAT,
        src_node=0x01,
        dst_node=0x02,
        seq_num=10,
        payload=b"health:ok"
    )
    raw = bytearray(pkt.serialize())

    # Corrupt one payload byte
    raw[-1] ^= 0xFF

    parsed, err = PacketEnvelope.deserialize(bytes(raw))
    assert parsed is None
    assert "CRC mismatch" in err

def test_truncated_packet():
    pkt = PacketEnvelope(
        msg_type=MsgType.SENSOR_EVENT,
        src_node=0x01,
        dst_node=0x02,
        seq_num=15,
        payload=b"1234567890"
    )
    raw = pkt.serialize()

    # Truncate
    parsed, err = PacketEnvelope.deserialize(raw[:14])
    assert parsed is None
    assert "less than header size" in err

def test_invalid_magic_and_version():
    raw = bytearray(PacketEnvelope(MsgType.HELLO, 1, 2, 1).serialize())
    # Alter magic
    raw[0] = 0x00
    parsed, err = PacketEnvelope.deserialize(bytes(raw))
    assert parsed is None
    assert "Invalid magic" in err
