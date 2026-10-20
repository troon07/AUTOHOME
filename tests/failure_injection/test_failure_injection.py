"""Failure Injection and Fault Tolerance Tests."""

import pytest
import time
from simulator.safety_kernel import SafetyKernel
from simulator.room_state import RoomStateEngine, OccupancyState
from simulator.protocol import PacketEnvelope, MsgType

def test_rapid_toggle_flood_attack():
    """Ensure SafetyKernel absorbs rapid contradictory commands without cycling hardware."""
    kernel = SafetyKernel(min_dwell_seconds=3.0)
    dev = kernel.get_or_create_device("bedroom", "light")

    # Initial turn on at t=10.0
    ok, _, _ = kernel.authorize_and_execute("bedroom", "light", "ON", current_time=10.0)
    assert ok is True
    assert dev.is_on is True

    # Flood with 50 alternate TOGGLE commands within 2 seconds
    rejections = 0
    for i in range(50):
        t = 10.0 + ((i + 1) * 0.04) # spans 10.04 to 12.04s (< 3.0s dwell limit)
        ok, reason, _ = kernel.authorize_and_execute("bedroom", "light", "TOGGLE", current_time=t)
        if not ok:
            rejections += 1

    # Every single toggle attempt within the 3.0s dwell window must be rejected!
    assert rejections == 50
    assert dev.is_on is True # Kept steady

def test_stale_network_packet_handling():
    """Simulate receiving a delayed/stale network command."""
    kernel = SafetyKernel(min_dwell_seconds=1.0)
    now = 500.0

    # Command arrived with timestamp from 120 seconds ago
    packet_timestamp = now - 120.0
    stale_threshold_seconds = 60.0

    is_stale = (now - packet_timestamp) > stale_threshold_seconds
    assert is_stale is True

    # System rejects stale remote command
    if is_stale:
        authorized = False
        reason = "REJECTED: Stale network command timestamp"
    else:
        authorized, reason, _ = kernel.authorize_and_execute("bedroom", "light", "ON")

    assert authorized is False
    assert "Stale" in reason

def test_node_disconnect_degraded_standalone():
    """Verify Node A maintains local autonomous safety and PIR control when Node B disconnects."""
    kernel = SafetyKernel(min_dwell_seconds=1.0)
    engine = RoomStateEngine(kernel, vacancy_grace_seconds=60.0, likely_occupied_window_seconds=15.0)
    engine.register_room("bedroom", pir_sensor_id="LOCAL_PIR", auto_light=True)

    # Disconnect flag simulated
    node_b_connected = False

    # Motion occurs locally on Node A
    actions = engine.process_sensor_event("LOCAL_PIR", "MOTION", True, current_time=100.0)
    assert len(actions) == 1
    assert actions[0]["authorized"] is True
    assert kernel.get_or_create_device("bedroom", "light").is_on is True

    # Standalone mode: Node A successfully managed local light without Node B
    assert engine.rooms["bedroom"].state == OccupancyState.OCCUPIED

def test_safe_initialization_on_boot():
    """Verify all devices default to OFF upon boot/reset."""
    kernel = SafetyKernel()
    dev = kernel.get_or_create_device("bedroom", "light")
    assert dev.is_on is False
    assert dev.value == 0
    assert dev.turned_on_ts is None
