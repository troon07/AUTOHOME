"""
ClawGrid V2.0 Room State Engine & Automation Policy
Maintains per-room occupancy, evidence tracking, vacancy grace timers,
and coordinates with the Safety Kernel for physical actuation.
"""

from enum import Enum
import time
from typing import Dict, Any, List, Optional, Tuple
from simulator.safety_kernel import SafetyKernel

class OccupancyState(str, Enum):
    OCCUPIED = "OCCUPIED"
    LIKELY_OCCUPIED = "LIKELY_OCCUPIED"
    VACANT = "VACANT"
    UNKNOWN = "UNKNOWN"

class EvidenceSource(str, Enum):
    PIR_MOTION = "PIR_MOTION"
    VOICE_COMMAND = "VOICE_COMMAND"
    MANUAL_SWITCH = "MANUAL_SWITCH"
    DOOR_SENSOR = "DOOR_SENSOR"
    TIMEOUT = "TIMEOUT"

class RoomRecord:
    def __init__(self, name: str, pir_sensor_id: Optional[str] = None):
        self.name = name
        self.pir_sensor_id = pir_sensor_id # None if no local PIR attached!
        self.state: OccupancyState = OccupancyState.UNKNOWN if pir_sensor_id is None else OccupancyState.VACANT
        self.confidence: float = 0.0 if pir_sensor_id is None else 0.95
        self.last_evidence_ts: Optional[float] = None
        self.last_transition_ts: float = 0.0
        self.last_evidence_source: Optional[EvidenceSource] = None
        self.auto_light_enabled: bool = True
        self.auto_fan_enabled: bool = False

class RoomStateEngine:
    def __init__(self,
                 safety_kernel: SafetyKernel,
                 vacancy_grace_seconds: float = 120.0,
                 likely_occupied_window_seconds: float = 30.0):
        self.safety_kernel = safety_kernel
        self.vacancy_grace_seconds = vacancy_grace_seconds
        self.likely_occupied_window_seconds = likely_occupied_window_seconds
        self.rooms: Dict[str, RoomRecord] = {}
        self.sensor_to_room_map: Dict[str, str] = {}
        self.event_log = []

    def register_room(self, name: str, pir_sensor_id: Optional[str] = None, auto_light: bool = True, auto_fan: bool = False):
        record = RoomRecord(name, pir_sensor_id)
        record.auto_light_enabled = auto_light
        record.auto_fan_enabled = auto_fan
        self.rooms[name] = record
        if pir_sensor_id:
            self.sensor_to_room_map[pir_sensor_id] = name

    def process_sensor_event(self,
                             sensor_id: str,
                             event_type: str,
                             value: Any,
                             current_time: Optional[float] = None) -> List[Dict[str, Any]]:
        now = current_time if current_time is not None else time.time()
        actions_taken = []

        # Find room mapped to this sensor
        target_room_name = self.sensor_to_room_map.get(sensor_id)
        if not target_room_name:
            # Unmapped sensor event: cannot localize room!
            self.event_log.append({
                "timestamp": now,
                "event": "UNMAPPED_SENSOR",
                "sensor_id": sensor_id
            })
            return actions_taken

        room = self.rooms[target_room_name]

        if event_type == "MOTION" and value:
            # Motion confirmed in target room
            prev_state = room.state
            room.state = OccupancyState.OCCUPIED
            room.confidence = 0.98
            room.last_evidence_ts = now
            room.last_evidence_source = EvidenceSource.PIR_MOTION
            if prev_state != OccupancyState.OCCUPIED:
                room.last_transition_ts = now

            # Policy: Activate lighting if configured
            if room.auto_light_enabled:
                ok, reason, dev_state = self.safety_kernel.authorize_and_execute(
                    room=room.name, device_id="light", action="ON", value=100, is_manual=False, current_time=now
                )
                actions_taken.append({"action": "ON", "device": "light", "room": room.name, "authorized": ok, "reason": reason})

            if room.auto_fan_enabled:
                ok, reason, dev_state = self.safety_kernel.authorize_and_execute(
                    room=room.name, device_id="fan", action="ON", value=100, is_manual=False, current_time=now
                )
                actions_taken.append({"action": "ON", "device": "fan", "room": room.name, "authorized": ok, "reason": reason})

        return actions_taken

    def record_voice_interaction(self, room_name: str, current_time: Optional[float] = None):
        now = current_time if current_time is not None else time.time()
        if room_name in self.rooms:
            room = self.rooms[room_name]
            room.state = OccupancyState.OCCUPIED
            room.confidence = 0.95
            room.last_evidence_ts = now
            room.last_evidence_source = EvidenceSource.VOICE_COMMAND

    def tick_vacancy_timers(self, current_time: Optional[float] = None) -> List[Dict[str, Any]]:
        """Periodically evaluates grace periods and transitions rooms to LIKELY_OCCUPIED and VACANT."""
        now = current_time if current_time is not None else time.time()
        actions_taken = []

        for name, room in self.rooms.items():
            if room.last_evidence_ts is None:
                continue

            elapsed = now - room.last_evidence_ts

            if elapsed < self.likely_occupied_window_seconds:
                # Remains OCCUPIED
                pass
            elif elapsed < self.vacancy_grace_seconds:
                # Transition to LIKELY_OCCUPIED
                if room.state == OccupancyState.OCCUPIED:
                    room.state = OccupancyState.LIKELY_OCCUPIED
                    room.confidence = 0.65
                    room.last_transition_ts = now
            else:
                # Full grace period elapsed -> transition to VACANT
                if room.state != OccupancyState.VACANT:
                    room.state = OccupancyState.VACANT
                    room.confidence = 0.95
                    room.last_transition_ts = now
                    room.last_evidence_source = EvidenceSource.TIMEOUT

                    # Policy: Automatically turn off light & fan
                    if room.auto_light_enabled:
                        ok, reason, dev_state = self.safety_kernel.authorize_and_execute(
                            room=room.name, device_id="light", action="OFF", is_manual=False, current_time=now
                        )
                        actions_taken.append({"action": "OFF", "device": "light", "room": room.name, "authorized": ok, "reason": reason})

                    if room.auto_fan_enabled:
                        ok, reason, dev_state = self.safety_kernel.authorize_and_execute(
                            room=room.name, device_id="fan", action="OFF", is_manual=False, current_time=now
                        )
                        actions_taken.append({"action": "OFF", "device": "fan", "room": room.name, "authorized": ok, "reason": reason})

        # Also trigger periodic safety cutoffs in safety kernel
        self.safety_kernel.evaluate_periodic_safety(now)

        return actions_taken
