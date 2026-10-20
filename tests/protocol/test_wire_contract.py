import struct
import pytest
from simulator.protocol import PacketEnvelope, MsgType, MAX_PAYLOAD, crc16_ccitt

def test_firmware_golden_vector():
    raw = bytes.fromhex("434702020000000100020102030400030ba1616263")
    assert PacketEnvelope(MsgType.HEARTBEAT, 1, 2, 0x01020304, b"abc").serialize() == raw
    packet, error = PacketEnvelope.deserialize(raw)
    assert error is None and packet.payload == b"abc"
    assert packet.seq_num == 0x01020304

def test_maximum_frame_and_trailing_bytes():
    raw = PacketEnvelope(MsgType.MODEL_TASK, 1, 2, 1, bytes(MAX_PAYLOAD)).serialize()
    assert len(raw) == 238
    assert PacketEnvelope.deserialize(raw)[1] is None
    assert PacketEnvelope.deserialize(raw+b"x")[0] is None
    with pytest.raises(ValueError):
        PacketEnvelope(MsgType.MODEL_TASK, 1, 2, 1, bytes(MAX_PAYLOAD+1)).serialize()

@pytest.mark.parametrize("index,value", [(3,0xff),(4,0x80),(5,1)])
def test_unsupported_fields_rejected_even_with_valid_crc(index,value):
    raw = bytearray(PacketEnvelope(MsgType.HELLO,1,2,0).serialize())
    raw[index] = value
    raw[16:18] = struct.pack("!H",crc16_ccitt(raw[:16]))
    assert PacketEnvelope.deserialize(bytes(raw))[0] is None
