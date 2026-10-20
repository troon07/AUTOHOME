# ClawGrid V2.0 Agent Guidelines & Execution Protocol

## Purpose
This document provides guidelines for AI agents working in this repository to maintain architectural integrity, safety boundaries, and reproducible verification.

## Non-Negotiable Rules
1. **Home Automation Comes First**: The primary product is room-aware lighting and fan control and local offline voice assistance. Distributed AI is the research contribution.
2. **Deterministic Safety Kernel**: Never bypass `SafetyKernel`. AI / NLP / network outputs are strictly proposed intents.
3. **Low-Voltage Prototype Only**: Mains AC voltage switching is prohibited in this phase.
4. **Data Integrity**: Never fabricate latency, memory, accuracy, or benchmark figures. Always label data `SIMULATED`, `HOST_MEASURED`, or `HARDWARE_MEASURED`.
5. **No Phone Modification**: The user's phone must remain untouched. Use only standard web browser client access.
6. **Single PIR Reality**: One PIR sensor can only localize its assigned zone. Never describe one PIR as a multi-room positioning system.
