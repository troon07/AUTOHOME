"""Unit tests for the Neural Embedding & Contextual Reasoning Agent."""

import pytest
from simulator.neural_embedding_agent import NeuralReasoningAgent

def test_scene_reasoning_going_to_sleep():
    agent = NeuralReasoningAgent()
    res = agent.parse_and_reason("I am going to sleep")
    assert res["intent"] == "SCENE_SLEEP"
    assert res["is_scene"] is True
    assert "Good night" in res["spoken_response"]

def test_implicit_reasoning_temperature():
    agent = NeuralReasoningAgent()
    res = agent.parse_and_reason("It is too hot in here", active_room="living_room")
    assert res["action"] == "ON"
    assert res["device"] == "fan"
    assert res["room"] == "living_room"
    assert "fan in the living room" in res["spoken_response"]

def test_implicit_reasoning_darkness():
    agent = NeuralReasoningAgent()
    res = agent.parse_and_reason("It is too dark in here", active_room="office")
    assert res["action"] == "ON"
    assert res["device"] == "light"
    assert res["room"] == "office"
    assert "Illuminating the office" in res["spoken_response"]

def test_fuzzy_typo_of_bedrrom():
    agent = NeuralReasoningAgent()
    res = agent.parse_and_reason("of bedrrom")
    assert res["action"] == "OFF"
    assert res["room"] == "bedroom"
    assert res["device"] == "ALL"
    assert "Turning off all devices in the bedroom" in res["spoken_response"]

def test_multiturn_conversational_memory():
    agent = NeuralReasoningAgent()
    # Turn 1: User gives incomplete command
    res1 = agent.parse_and_reason("turn on the light", active_room=None)
    assert res1["requires_clarification"] is True
    assert "Which room" in res1["clarification_prompt"]

    # Turn 2: User responds only with room name
    res2 = agent.parse_and_reason("bedroom")
    assert res2["action"] == "ON"
    assert res2["device"] == "light"
    assert res2["room"] == "bedroom"
    assert "Turning on the light in the bedroom" in res2["spoken_response"]

def test_implicit_too_bright():
    agent = NeuralReasoningAgent()
    res = agent.parse_and_reason("too bright", active_room="bedroom")
    assert res["action"] == "OFF"
    assert res["device"] == "light"
    assert res["room"] == "bedroom"
    assert "lights in the bedroom" in res["spoken_response"]

def test_implicit_cold_off_fan():
    agent = NeuralReasoningAgent()
    res = agent.parse_and_reason("cold", active_room="bedroom")
    assert res["action"] == "OFF"
    assert res["device"] == "fan"
    assert res["room"] == "bedroom"
    assert "fan in the bedroom" in res["spoken_response"]

def test_active_room_guessing():
    agent = NeuralReasoningAgent()
    # When active_room is known, "turn on light" shouldn't ask "which room", it should directly guess the active room
    res = agent.parse_and_reason("turn on light", active_room="living_room")
    assert res["requires_clarification"] is False
    assert res["action"] == "ON"
    assert res["device"] == "light"
    assert res["room"] == "living_room"

def test_mismatch_repair_flow():
    agent = NeuralReasoningAgent()
    # Step 1: Agent acts on default bedroom
    res1 = agent.parse_and_reason("turn on light", active_room="bedroom")
    assert res1["room"] == "bedroom"

    # Step 2: User complains "y not still on"
    res2 = agent.parse_and_reason("y not still on")
    assert res2["intent"] == "REPAIR"
    assert res2["requires_clarification"] is True
    assert "Which room are you in" in res2["spoken_response"]

    # Step 3: User answers "living room"
    res3 = agent.parse_and_reason("living room")
    assert res3["action"] == "ON"
    assert res3["device"] == "light"
    assert res3["room"] == "living_room"
    assert "Switching location to living room" in res3["spoken_response"]
