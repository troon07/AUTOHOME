"""
ClawGrid V2.0 Local Host Server
Serves the responsive web dashboard and REST API for offline local control,
multi-room state simulation, and research benchmarking.
"""

import os
import sys
import json
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from fastapi import FastAPI, Request
from fastapi.staticfiles import StaticFiles
from fastapi.responses import HTMLResponse, JSONResponse
from pydantic import BaseModel
from typing import Optional

from simulator.safety_kernel import SafetyKernel
from simulator.room_state import RoomStateEngine, OccupancyState
from simulator.voice_parser import VoiceParser
from simulator.cooperative_model import CooperativeInferenceSimulator
from simulator.neural_embedding_agent import NeuralReasoningAgent

app = FastAPI(title="ClawGrid V2.0 Local Assistant")

# Core Engine Instances
kernel = SafetyKernel(min_dwell_seconds=2.0, max_on_duration_seconds=14400.0)
engine = RoomStateEngine(kernel, vacancy_grace_seconds=120.0, likely_occupied_window_seconds=30.0)
parser = VoiceParser()
neural_agent = NeuralReasoningAgent()
sim = CooperativeInferenceSimulator()

# Register Rooms (Bedroom with physical PIR_1, others with virtual or door sensors)
engine.register_room("bedroom", pir_sensor_id="PIR_1", auto_light=True, auto_fan=True)
engine.register_room("living_room", pir_sensor_id=None, auto_light=True, auto_fan=False)
engine.register_room("kitchen", pir_sensor_id=None, auto_light=True, auto_fan=False)
engine.register_room("office", pir_sensor_id=None, auto_light=True, auto_fan=False)

# Mount Dashboard Static Files
STATIC_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "dashboard", "static")
if os.path.exists(STATIC_DIR):
    app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")

@app.get("/", response_class=HTMLResponse)
async def get_index():
    index_file = os.path.join(STATIC_DIR, "index.html")
    if os.path.exists(index_file):
        with open(index_file, "r", encoding="utf-8") as f:
            return f.read()
    return "<h1>ClawGrid V2.0 Dashboard</h1>"

@app.get("/api/state")
async def get_state():
    engine.tick_vacancy_timers()
    rooms_data = []
    for name, r in engine.rooms.items():
        light = kernel.get_or_create_device(name, "light")
        fan = kernel.get_or_create_device(name, "fan")
        rooms_data.append({
            "name": name,
            "state": r.state.value,
            "confidence": r.confidence,
            "sensor_id": r.pir_sensor_id,
            "light_on": light.is_on,
            "fan_on": fan.is_on
        })
    return {
        "rooms": rooms_data,
        "logs": kernel.audit_log[-15:], # Last 15 events
        "timestamp": time.time()
    }

class DeviceRequest(BaseModel):
    room: str
    device: str
    action: str
    value: int = 100
    is_manual: bool = True

@app.post("/api/device")
async def control_device(req: DeviceRequest):
    ok, reason, state = kernel.authorize_and_execute(
        room=req.room,
        device_id=req.device,
        action=req.action,
        value=req.value,
        is_manual=req.is_manual
    )
    return {"authorized": ok, "reason": reason, "state": state}

class SensorRequest(BaseModel):
    sensor_id: str
    event_type: str = "MOTION"
    value: bool = True

@app.post("/api/sensor_event")
async def trigger_sensor(req: SensorRequest):
    actions = engine.process_sensor_event(req.sensor_id, req.event_type, req.value)
    return {"success": True, "actions": actions}

class VoiceRequest(BaseModel):
    text: str
    local_room_context: Optional[str] = "bedroom"

@app.post("/api/voice")
async def handle_voice(req: VoiceRequest):
    # Find room with active presence
    active_room = "bedroom"
    for r_name, r_rec in engine.rooms.items():
        if r_rec.state == OccupancyState.OCCUPIED:
            active_room = r_name
            break

    # Neural Embedding & Contextual Reasoning Model executes
    parsed = neural_agent.parse_and_reason(
        req.text,
        active_room=active_room,
        current_time=time.time()
    )

    if parsed["requires_clarification"]:
        return parsed

    # Handle Smart Scene Routines
    if parsed.get("is_scene"):
        if parsed["intent"] == "SCENE_SLEEP":
            for r in engine.rooms.keys():
                kernel.authorize_and_execute(room=r, device_id="light", action="OFF", is_manual=True)
            kernel.authorize_and_execute(room="bedroom", device_id="fan", action="ON", is_manual=True)
            parsed["authorized"] = True
            return parsed
        elif parsed["intent"] == "SCENE_AWAY":
            for r in engine.rooms.keys():
                kernel.authorize_and_execute(room=r, device_id="light", action="OFF", is_manual=True)
                kernel.authorize_and_execute(room=r, device_id="fan", action="OFF", is_manual=True)
            parsed["authorized"] = True
            return parsed

    # Handle Direct & Fuzzy Device Actions
    if parsed["intent"] == "DEVICE_CONTROL" and parsed["action"] and parsed["room"]:
        target_devices = ["light", "fan"] if parsed["device"] == "ALL" else [parsed["device"]]
        all_authorized = True
        last_reason = "AUTHORIZED"
        last_state = None

        for d in target_devices:
            ok, reason, dev_state = kernel.authorize_and_execute(
                room=parsed["room"],
                device_id=d,
                action=parsed["action"],
                value=parsed.get("value", 100),
                is_manual=True
            )
            if not ok:
                all_authorized = False
                last_reason = reason
            last_state = dev_state

        parsed["authorized"] = all_authorized
        parsed["reason"] = last_reason
        parsed["device_state"] = last_state

        if all_authorized:
            engine.record_voice_interaction(parsed["room"])

    return parsed

@app.get("/api/benchmark")
async def run_benchmark():
    # 3.2M parameter model evaluation
    res = sim.simulate_model_execution(param_count=3_200_000, layers=12, hidden_dim=256, seq_len=1)
    
    # Save raw benchmark result
    raw_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), "benchmarks", "raw", "latest_benchmark.json")
    with open(raw_path, "w", encoding="utf-8") as f:
        json.dump(res, f, indent=2)

    return res

if __name__ == "__main__":
    import uvicorn
    import socket
    import threading

    def get_local_ip():
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect(("8.8.8.8", 80))
            ip = s.getsockname()[0]
            s.close()
            return ip
        except Exception:
            return "127.0.0.1"

    lan_ip = get_local_ip()
    key_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), "key.pem")
    cert_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), "cert.pem")

    has_ssl = os.path.exists(key_path) and os.path.exists(cert_path)
    if has_ssl:
        def run_https():
            uvicorn.run(app, host="0.0.0.0", port=8443, ssl_keyfile=key_path, ssl_certfile=cert_path, log_level="warning")
        threading.Thread(target=run_https, daemon=True).start()

    print("\n=======================================================")
    print("  ClawGrid V2.0 Local Assistant & Edge AI Host         ")
    print("=======================================================")
    print(f"  [Standard HTTP]  http://127.0.0.1:8000")
    print(f"  [Phone Browser]  http://{lan_ip}:8000")
    if has_ssl:
        print(f"  [Secure Mic SSL] https://{lan_ip}:8443")
    print("=======================================================\n")
    uvicorn.run(app, host="0.0.0.0", port=8000)
