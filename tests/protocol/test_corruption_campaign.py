import random
import pytest
from simulator.protocol import PacketEnvelope, MsgType

# Each single-bit fault must be rejected across representative payload sizes.
# This exercises the Python implementation; C golden vectors run on the boards.
def test_single_bit_faults():
    rng=random.Random(20261020)
    for length in (0,1,16,44,128,220):
        payload=bytes(rng.randrange(256) for _ in range(length))
        packet=PacketEnvelope(MsgType.HEARTBEAT,1,2,17,payload).serialize()
        for bit in range(len(packet)*8):
            damaged=bytearray(packet); damaged[bit//8]^=1<<(bit%8)
            assert PacketEnvelope.deserialize(bytes(damaged))[0] is None
