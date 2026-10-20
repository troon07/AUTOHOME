"""End-to-End Integration Tests for ClawGrid V2.0."""

import pytest
from simulator.safety_kernel import SafetyKernel
from simulator.room_state import RoomStateEngine, OccupancyState
from simulator.voice_parser import VoiceParser
from simulator.protocol import PacketEnvelope, MsgType, MsgFlags
from simulator.cooperative_model import CooperativeInferenceSimulator

def test_voice_command_to_safety_actuation_pipeline():
    """Verify raw speech -> parse intent -> safety kernel -> state update -> network ACK."""
    kernel = SafetyKernel(min_dwell_seconds=1.0)
    engine = RoomStateEngine(kernel, vacancy_grace_seconds=120.0)
    engine.register_room("bedroom", pir_sensor_id="PIR_1", auto_light=True)
    parser = VoiceParser()

    # User speaks to Node A
    speech = "Hey Claw, turn on the bedroom light"
    parsed = parser.parse_command(speech)
    assert parsed["has_wake_word"] is True
    assert parsed["action"] == "ON"
    assert parsed["room"] == "bedroom"
    assert parsed["device"] == "light"
    assert parsed["requires_clarification"] is False

    # Pass proposed intent to Deterministic Safety Kernel
    authorized, reason, dev_state = kernel.authorize_and_execute(
        room=parsed["room"],
        device_id=parsed["device"],
        action=parsed["action"],
        value=parsed["value"],
        is_manual=True,
        current_time=200.0
    )
    assert authorized is True
    assert dev_state["is_on"] is True

    # Record voice interaction as occupancy evidence
    engine.record_voice_interaction("bedroom", current_time=200.0)
    assert engine.rooms["bedroom"].state == OccupancyState.OCCUPIED

    # Build network ACK envelope to send back or broadcast to Node B
    ack_pkt = PacketEnvelope(
        msg_type=MsgType.COMMAND_ACK,
        src_node=0x01, # Node A
        dst_node=0x02, # Node B
        seq_num=100,
        payload=b'{"status":"AUTHORIZED","room":"bedroom","device":"light","state":"ON"}',
        flags=MsgFlags.NONE
    )
    raw_bytes = ack_pkt.serialize()
    deserialized, err = PacketEnvelope.deserialize(raw_bytes)
    assert err is None
    assert deserialized.msg_type == MsgType.COMMAND_ACK

def test_cooperative_inference_research_comparison():
    """Benchmark Mode A (Single Node) vs Mode B (Parallel) vs Mode C (Split-Model)."""
    sim = CooperativeInferenceSimulator()

    # Evaluate 3.2M parameter model (typical nano-LLM / TinyStories)
    result = sim.simulate_model_execution(
        param_count=3_200_000,
        layers=12,
        hidden_dim=256,
        seq_len=1
    )
    assert result["source"] == "SIMULATED"
    assert "mode_a_single_node" in result
    assert "mode_b_parallel_tasks" in result
    assert "mode_c_split_model" in result

    # Check that Mode B parallel responsibilities is fast (~27ms)
    assert result["mode_b_parallel_tasks"]["latency_ms"] < 50.0

    # For MCU with 1.2 Mbps wireless link, transferring intermediate activations (~256 bytes)
    # has measurable network overhead; check that split model report correctly labels helped/hurt
    assert "helped_or_hurt" in result["mode_c_split_model"]
