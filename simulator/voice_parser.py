"""
ClawGrid V2.0 Local Voice & Intent Parser
Offline deterministic grammar-slot parser with disambiguation.
Schema: {intent, room, device, action, value, confidence, requires_confirmation, clarification_prompt}
"""

import re
from typing import Dict, Any, Optional, Tuple

WAKE_WORDS = ["hey claw", "hi claw", "claw", "hi lexi", "computer", "alexa"]
VALID_ROOMS = ["bedroom", "living_room", "kitchen", "office"]
ROOM_ALIASES = {
    "bedroom": ["bedroom", "bed room", "bed"],
    "living_room": ["living room", "living", "hall", "lounge"],
    "kitchen": ["kitchen", "cooking area"],
    "office": ["office", "study", "work room"]
}

DEVICE_ALIASES = {
    "light": ["light", "lights", "lamp", "bulbs", "led"],
    "fan": ["fan", "ceiling fan", "cooler", "ventilator"],
    "heater_mock": ["heater", "ac"]
}

def levenshtein(s1: str, s2: str) -> int:
    if len(s1) < len(s2):
        return levenshtein(s2, s1)
    if len(s2) == 0:
        return len(s1)
    prev = list(range(len(s2) + 1))
    for i, c1 in enumerate(s1):
        curr = [i + 1]
        for j, c2 in enumerate(s2):
            ins = prev[j + 1] + 1
            dels = curr[j] + 1
            subs = prev[j] + (c1 != c2)
            curr.append(min(ins, dels, subs))
        prev = curr
    return prev[-1]

def fuzzy_match(token: str, candidates: list, threshold: float = 0.70) -> Tuple[Optional[str], float]:
    best_candidate = None
    best_score = 0.0
    token = token.lower().strip()
    for cand in candidates:
        cand_lower = cand.lower().strip()
        if token == cand_lower:
            return cand, 1.0
        # Check substring containment
        if token in cand_lower or cand_lower in token:
            score = 0.85
        else:
            max_l = max(len(token), len(cand_lower))
            dist = levenshtein(token, cand_lower)
            score = 1.0 - (dist / max_l) if max_l > 0 else 0.0

        if score > best_score:
            best_score = score
            best_candidate = cand

    if best_score >= threshold:
        return best_candidate, best_score
    return None, 0.0

class VoiceParser:
    def __init__(self):
        pass

    def check_wake_word(self, text: str) -> Tuple[bool, str]:
        text_lower = text.strip().lower()
        for ww in WAKE_WORDS:
            if text_lower.startswith(ww):
                remainder = text_lower[len(ww):].strip()
                remainder = re.sub(r'^[,\s]+', '', remainder)
                return True, remainder
        return False, text_lower

    def parse_command(self,
                      raw_speech: str,
                      local_room_context: Optional[str] = None,
                      local_room_is_occupied: bool = False) -> Dict[str, Any]:
        has_wake, cmd_text = self.check_wake_word(raw_speech)
        text = cmd_text.lower().strip()

        result: Dict[str, Any] = {
            "has_wake_word": has_wake,
            "raw_text": raw_speech,
            "intent": "UNKNOWN",
            "action": None,
            "room": None,
            "device": None,
            "value": 100,
            "confidence": 0.0,
            "requires_clarification": False,
            "clarification_prompt": None,
            "requires_confirmation": False
        }

        if not text:
            result["intent"] = "EMPTY"
            return result

        # Check Help / Cancel / Status intents
        if any(w in text for w in ["help", "what can you do", "commands"]):
            result["intent"] = "HELP"
            result["confidence"] = 0.99
            result["spoken_response"] = "You can ask me to turn on or off any room or device, like 'turn on bedroom' or 'switch off fan in the kitchen'."
            return result

        if any(w in text for w in ["cancel", "stop", "abort", "never mind"]):
            result["intent"] = "CANCEL"
            result["confidence"] = 0.99
            result["spoken_response"] = "Action cancelled."
            return result

        if any(w in text for w in ["status", "report", "what's the state", "how is everything"]):
            result["intent"] = "STATUS_QUERY"
            result["confidence"] = 0.95
            result["spoken_response"] = "All systems nominal. ClawGrid edge nodes are active and operating offline."
            return result

        # Tokenize user speech into words
        clean_text = re.sub(r'[^\w\s]', ' ', text)
        words = clean_text.split()

        # Step 1: Detect Action (with phonetic/typo tolerance like 'of' -> 'off')
        action = None
        action_confidence = 0.0
        
        on_phrases = ["turn on", "switch on", "power on", "enable", "start", "activate", "open", "on", "onn"]
        off_phrases = ["turn off", "switch off", "power off", "shut down", "disable", "kill", "close", "off", "of", "offf"]
        toggle_phrases = ["toggle", "flip", "alternate", "switch"]

        for w in words:
            if w in ["of", "off", "offf", "shut", "kill", "close"]:
                action = "OFF"
                action_confidence = 0.95
                break
            elif w in ["on", "onn", "open", "start", "activate"]:
                action = "ON"
                action_confidence = 0.95
                break
            elif w in ["toggle", "flip"]:
                action = "TOGGLE"
                action_confidence = 0.90
                break

        if not action:
            # Check multi-word phrase matching
            if re.search(r'\b(turn on|switch on|power on)\b', text):
                action = "ON"
                action_confidence = 0.95
            elif re.search(r'\b(turn off|switch off|power off|shut off)\b', text):
                action = "OFF"
                action_confidence = 0.95

        if not action:
            result["intent"] = "UNRECOGNIZED_ACTION"
            result["spoken_response"] = "I didn't catch the action. Please say turn on, or turn off."
            return result

        result["action"] = action
        result["intent"] = "DEVICE_CONTROL"

        # Step 2: Detect Room with Fuzzy Matching (handles 'bedrrom', 'livin', 'kitchin', etc.)
        detected_room = None
        room_confidence = 0.0

        for canon_room, aliases in ROOM_ALIASES.items():
            for alias in aliases:
                # Check direct substring
                if alias in text:
                    detected_room = canon_room
                    room_confidence = 0.98
                    break
                # Check fuzzy match against each word
                for w in words:
                    cand, score = fuzzy_match(w, [alias], threshold=0.75)
                    if cand and score > room_confidence:
                        detected_room = canon_room
                        room_confidence = score
            if detected_room and room_confidence > 0.90:
                break

        # Disambiguation logic
        if not detected_room:
            if local_room_context and local_room_is_occupied:
                detected_room = local_room_context
                room_confidence = 0.88
            else:
                result["requires_clarification"] = True
                result["clarification_prompt"] = "Which room would you like to control?"
                result["spoken_response"] = "Which room would you like to control?"
                result["confidence"] = 0.50
                return result

        result["room"] = detected_room

        # Step 3: Detect Device with Fuzzy Matching ('lite' -> 'light', 'fen' -> 'fan')
        detected_device = None
        device_confidence = 0.0

        for canon_dev, aliases in DEVICE_ALIASES.items():
            for alias in aliases:
                if alias in text:
                    detected_device = canon_dev
                    device_confidence = 0.95
                    break
                for w in words:
                    cand, score = fuzzy_match(w, [alias], threshold=0.75)
                    if cand and score > device_confidence:
                        detected_device = canon_dev
                        device_confidence = score
            if detected_device and device_confidence > 0.90:
                break

        # If user didn't specify a device (e.g. "of bedrrom", "turn on bedroom"):
        if not detected_device:
            detected_device = "ALL"
            result["device"] = "ALL"
            result["confidence"] = round((action_confidence + room_confidence) / 2.0, 2)
            result["spoken_response"] = f"Turning {action.lower()} all devices in the {detected_room.replace('_', ' ')}."
        else:
            result["device"] = detected_device
            result["confidence"] = round((action_confidence + room_confidence + device_confidence) / 3.0, 2)
            result["spoken_response"] = f"Turning {action.lower()} the {detected_device} in the {detected_room.replace('_', ' ')}."

        return result
