"""
ClawGrid V2.0 Deterministic Safety Kernel
The NON-BYPASSABLE authority for physical output transitions.
Enforces:
- Hysteresis / Minimum dwell time (anti-cycling)
- Maximum run duration limits
- Manual override lockouts
- Whitelisted room/device names
- Zero mains voltage enforcement
"""

import time
from typing import Dict, Any, Optional, Tuple, Set

VALID_ROOMS: Set[str] = {"bedroom", "living_room", "kitchen", "office"}
VALID_DEVICES: Set[str] = {"light", "fan", "lamp", "heater_mock"}
VALID_ACTIONS: Set[str] = {"ON", "OFF", "TOGGLE", "SET_VALUE"}

class DeviceState:
    def __init__(self, room: str, device_id: str):
        self.room = room
        self.device_id = device_id
        self.is_on: bool = False
        self.value: int = 0
        self.last_transition_ts: float = -999999.0
        self.turned_on_ts: Optional[float] = None
        self.manual_override: bool = False
        self.manual_override_ts: float = 0.0
        self.manual_override_duration: float = 300.0 # 5 minutes default

class SafetyKernel:
    def __init__(self,
                 min_dwell_seconds: float = 3.0,
                 max_on_duration_seconds: float = 14400.0, # 4 hours max
                 manual_hold_seconds: float = 300.0):
        self.min_dwell_seconds = min_dwell_seconds
        self.max_on_duration_seconds = max_on_duration_seconds
        self.manual_hold_seconds = manual_hold_seconds
        self._devices: Dict[str, DeviceState] = {}
        self.audit_log = []

    def _get_device_key(self, room: str, device_id: str) -> str:
        return f"{room}:{device_id}"

    def get_or_create_device(self, room: str, device_id: str) -> DeviceState:
        key = self._get_device_key(room, device_id)
        if key not in self._devices:
            self._devices[key] = DeviceState(room, device_id)
        return self._devices[key]

    def authorize_and_execute(self,
                              room: str,
                              device_id: str,
                              action: str,
                              value: int = 100,
                              is_manual: bool = False,
                              current_time: Optional[float] = None) -> Tuple[bool, str, Optional[Dict[str, Any]]]:
        now = current_time if current_time is not None else time.time()

        # Rule 1: Validation of Room and Device
        if room not in VALID_ROOMS:
            return False, f"REJECTED: Invalid room '{room}'", None
        if device_id not in VALID_DEVICES:
            return False, f"REJECTED: Invalid device '{device_id}'", None
        if action not in VALID_ACTIONS:
            return False, f"REJECTED: Invalid action '{action}'", None

        dev = self.get_or_create_device(room, device_id)

        # Calculate target state
        if action == "ON":
            target_on = True
            target_val = value if value > 0 else 100
        elif action == "OFF":
            target_on = False
            target_val = 0
        elif action == "TOGGLE":
            target_on = not dev.is_on
            target_val = 100 if target_on else 0
        elif action == "SET_VALUE":
            target_on = (value > 0)
            target_val = max(0, min(100, value))
        else:
            return False, f"REJECTED: Unknown action '{action}'", None

        # Check if already in requested state
        if dev.is_on == target_on and (not target_on or dev.value == target_val):
            if is_manual:
                dev.manual_override = True
                dev.manual_override_ts = now
            return True, "NOOP: Device already in requested state", {
                "room": room, "device_id": device_id, "is_on": dev.is_on, "value": dev.value
            }

        # Rule 2: Minimum Dwell Time (Anti-cycling / Hysteresis)
        elapsed = now - dev.last_transition_ts
        if elapsed < self.min_dwell_seconds:
            remaining = self.min_dwell_seconds - elapsed
            return False, f"REJECTED: Dwell time safety limit ({remaining:.1f}s remaining)", None

        # Rule 3: Manual Override Lockout
        if not is_manual and dev.manual_override:
            override_elapsed = now - dev.manual_override_ts
            if override_elapsed < self.manual_hold_seconds:
                rem_hold = self.manual_hold_seconds - override_elapsed
                return False, f"REJECTED: Manual override active ({rem_hold:.0f}s remaining)", None
            else:
                # Override expired
                dev.manual_override = False

        # Apply state transition safely
        prev_state = dev.is_on
        dev.is_on = target_on
        dev.value = target_val
        dev.last_transition_ts = now

        if target_on:
            dev.turned_on_ts = now
        else:
            dev.turned_on_ts = None

        if is_manual:
            dev.manual_override = True
            dev.manual_override_ts = now

        event = {
            "timestamp": now,
            "room": room,
            "device_id": device_id,
            "previous_on": prev_state,
            "new_on": target_on,
            "value": target_val,
            "is_manual": is_manual
        }
        self.audit_log.append(event)

        return True, "AUTHORIZED", {
            "room": room, "device_id": device_id, "is_on": dev.is_on, "value": dev.value
        }

    def evaluate_periodic_safety(self, current_time: Optional[float] = None):
        """Enforces max continuous run duration cutoff."""
        now = current_time if current_time is not None else time.time()
        cutoffs = []
        for key, dev in self._devices.items():
            if dev.is_on and dev.turned_on_ts is not None:
                duration = now - dev.turned_on_ts
                if duration > self.max_on_duration_seconds:
                    dev.is_on = False
                    dev.value = 0
                    dev.last_transition_ts = now
                    dev.turned_on_ts = None
                    dev.manual_override = False
                    cutoffs.append(key)
        return cutoffs
