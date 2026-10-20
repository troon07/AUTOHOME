"""Unit tests for the Deterministic Safety Kernel."""

import pytest
from simulator.safety_kernel import SafetyKernel

def test_whitelist_validation():
    kernel = SafetyKernel(min_dwell_seconds=2.0)

    # Valid command
    ok, reason, state = kernel.authorize_and_execute("bedroom", "light", "ON")
    assert ok is True
    assert state["is_on"] is True

    # Invalid room
    ok, reason, state = kernel.authorize_and_execute("dungeon", "light", "ON")
    assert ok is False
    assert "Invalid room" in reason

    # Invalid device
    ok, reason, state = kernel.authorize_and_execute("bedroom", "plasma_cannon", "ON")
    assert ok is False
    assert "Invalid device" in reason

    # Invalid action
    ok, reason, state = kernel.authorize_and_execute("bedroom", "light", "EXPLODE")
    assert ok is False
    assert "Invalid action" in reason

def test_dwell_time_anti_cycling():
    kernel = SafetyKernel(min_dwell_seconds=5.0)

    # Turn ON at t=100
    ok, reason, state = kernel.authorize_and_execute("bedroom", "light", "ON", current_time=100.0)
    assert ok is True
    assert state["is_on"] is True

    # Rapid toggle OFF at t=102 (only 2s elapsed, minimum is 5s)
    ok, reason, state = kernel.authorize_and_execute("bedroom", "light", "OFF", current_time=102.0)
    assert ok is False
    assert "Dwell time safety limit" in reason

    # Try toggle OFF at t=106 (6s elapsed -> authorized!)
    ok, reason, state = kernel.authorize_and_execute("bedroom", "light", "OFF", current_time=106.0)
    assert ok is True
    assert state["is_on"] is False

def test_manual_override_lockout():
    kernel = SafetyKernel(min_dwell_seconds=1.0, manual_hold_seconds=300.0)

    # User manually switches off device at t=100
    ok, reason, state = kernel.authorize_and_execute("bedroom", "light", "OFF", is_manual=True, current_time=100.0)
    assert ok is True

    # Automatic sensor attempts to switch ON at t=150 (during 300s manual hold)
    ok, reason, state = kernel.authorize_and_execute("bedroom", "light", "ON", is_manual=False, current_time=150.0)
    assert ok is False
    assert "Manual override active" in reason

    # After hold period expires (t=405), automation can actuate
    ok, reason, state = kernel.authorize_and_execute("bedroom", "light", "ON", is_manual=False, current_time=405.0)
    assert ok is True
    assert state["is_on"] is True

def test_max_continuous_run_cutoff():
    kernel = SafetyKernel(min_dwell_seconds=1.0, max_on_duration_seconds=7200.0) # 2 hour max

    # Turn ON at t=0
    kernel.authorize_and_execute("living_room", "fan", "ON", current_time=0.0)
    dev = kernel.get_or_create_device("living_room", "fan")
    assert dev.is_on is True

    # Check at 1 hour (t=3600) -> remains on
    cutoffs = kernel.evaluate_periodic_safety(current_time=3600.0)
    assert len(cutoffs) == 0
    assert dev.is_on is True

    # Check at 2.5 hours (t=9000) -> auto-cutoff safety triggers!
    cutoffs = kernel.evaluate_periodic_safety(current_time=9000.0)
    assert "living_room:fan" in cutoffs
    assert dev.is_on is False
