"""Unit tests for the Room State Engine."""

import pytest
from simulator.safety_kernel import SafetyKernel
from simulator.room_state import RoomStateEngine, OccupancyState, EvidenceSource

def test_single_pir_localization_boundary():
    """Verify that 1 PIR attached to Bedroom does NOT falsely localize other rooms."""
    kernel = SafetyKernel(min_dwell_seconds=1.0)
    engine = RoomStateEngine(kernel, vacancy_grace_seconds=120.0, likely_occupied_window_seconds=30.0)

    # Register Bedroom with PIR-1, and Living Room with NO sensor (None)
    engine.register_room("bedroom", pir_sensor_id="PIR_1", auto_light=True)
    engine.register_room("living_room", pir_sensor_id=None, auto_light=True)

    # Initially: bedroom is VACANT, living_room is UNKNOWN
    assert engine.rooms["bedroom"].state == OccupancyState.VACANT
    assert engine.rooms["living_room"].state == OccupancyState.UNKNOWN
    assert engine.rooms["living_room"].confidence == 0.0

    # Motion detected on PIR-1
    actions = engine.process_sensor_event("PIR_1", "MOTION", True, current_time=100.0)

    # Bedroom transitions to OCCUPIED and light turns ON
    assert engine.rooms["bedroom"].state == OccupancyState.OCCUPIED
    assert engine.rooms["bedroom"].confidence == 0.98
    assert len(actions) == 1
    assert actions[0]["authorized"] is True
    assert actions[0]["room"] == "bedroom"
    assert actions[0]["device"] == "light"

    # CRITICAL CHECK: Living Room remains UNKNOWN! (No hallucinated multi-room presence)
    assert engine.rooms["living_room"].state == OccupancyState.UNKNOWN
    assert engine.rooms["living_room"].confidence == 0.0

def test_vacancy_grace_countdown_and_auto_off():
    """Verify transition OCCUPIED -> LIKELY_OCCUPIED -> VACANT and device turn off."""
    kernel = SafetyKernel(min_dwell_seconds=1.0)
    engine = RoomStateEngine(kernel, vacancy_grace_seconds=100.0, likely_occupied_window_seconds=20.0)
    engine.register_room("bedroom", pir_sensor_id="PIR_1", auto_light=True)

    # Motion at t=0
    engine.process_sensor_event("PIR_1", "MOTION", True, current_time=0.0)
    assert engine.rooms["bedroom"].state == OccupancyState.OCCUPIED
    dev = kernel.get_or_create_device("bedroom", "light")
    assert dev.is_on is True

    # At t=10s: still in active window -> OCCUPIED
    engine.tick_vacancy_timers(current_time=10.0)
    assert engine.rooms["bedroom"].state == OccupancyState.OCCUPIED

    # At t=30s: past 20s active window -> transitions to LIKELY_OCCUPIED (light stays ON during grace)
    engine.tick_vacancy_timers(current_time=30.0)
    assert engine.rooms["bedroom"].state == OccupancyState.LIKELY_OCCUPIED
    assert dev.is_on is True

    # At t=105s: past 100s vacancy grace -> transitions to VACANT, light turns OFF
    actions = engine.tick_vacancy_timers(current_time=105.0)
    assert engine.rooms["bedroom"].state == OccupancyState.VACANT
    assert len(actions) == 1
    assert actions[0]["action"] == "OFF"
    assert dev.is_on is False
