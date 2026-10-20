"""Unit tests for the Voice & Intent Parser."""

import pytest
from simulator.voice_parser import VoiceParser

def test_wake_word_extraction():
    parser = VoiceParser()
    has_wake, rem = parser.check_wake_word("hey claw turn on the light")
    assert has_wake is True
    assert rem == "turn on the light"

    has_wake, rem = parser.check_wake_word("hi lexi, switch off fan")
    assert has_wake is True
    assert rem == "switch off fan"

    has_wake, rem = parser.check_wake_word("turn on bedroom light")
    assert has_wake is False
    assert rem == "turn on bedroom light"

def test_explicit_command_parsing():
    parser = VoiceParser()
    res = parser.parse_command("hey claw, please turn on the bedroom light")
    assert res["intent"] == "DEVICE_CONTROL"
    assert res["action"] == "ON"
    assert res["room"] == "bedroom"
    assert res["device"] == "light"
    assert res["confidence"] >= 0.90
    assert res["requires_clarification"] is False

    res2 = parser.parse_command("switch off the ceiling fan in the kitchen")
    assert res2["action"] == "OFF"
    assert res2["room"] == "kitchen"
    assert res2["device"] == "fan"

def test_disambiguation_with_active_local_context():
    parser = VoiceParser()
    # Speaker says "turn on the lamp" without specifying room, but Bedroom is currently occupied
    res = parser.parse_command("turn on the lamp", local_room_context="bedroom", local_room_is_occupied=True)
    assert res["action"] == "ON"
    assert res["device"] == "light"
    assert res["room"] == "bedroom"
    assert res["requires_clarification"] is False

def test_disambiguation_when_ambiguous():
    parser = VoiceParser()
    # Speaker says "turn on the light", no local room is occupied -> MUST ASK FOR CLARIFICATION!
    res = parser.parse_command("turn on the light", local_room_context=None, local_room_is_occupied=False)
    assert res["requires_clarification"] is True
    assert "Which room" in res["clarification_prompt"]
    assert res["room"] is None

def test_help_and_cancel_intents():
    parser = VoiceParser()
    res_help = parser.parse_command("hey claw help")
    assert res_help["intent"] == "HELP"

    res_cancel = parser.parse_command("hey claw abort")
    assert res_cancel["intent"] == "CANCEL"
