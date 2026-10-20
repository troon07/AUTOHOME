# Hardware status on 20 October 2026

Node B has returned on COM7 (MAC e0:72:a1:f5:bc:b0), ESP32-S3 N16R8. It is flashed with the current native speech/API/observation-monitor image. Node A COM5 (MAC e0:72:a1:f6:a9:68) remains unavailable; its updated native image compiled but has not been flashed. Nothing is wired. Physical outputs remain inhibited. No phone settings or Wi-Fi network changes were made.

Current B-only campaign:32/32 API/shadow rows passed, including23 monitor cases. Raw trace: `benchmarks/raw/hardware/single_b_20261020T080754Z.json`. Current host regression:83 passed. A separate 18-input B speech run completed with matching PCM CRCs but no hits on16 TTS commands; two negatives gave no command. Useful recognition is still an open functional issue, and a real-voice diagnostic sample has been requested.

The historical paired run passed81/81 controller checks and1000/1000 commands on older images. Those data are retained and do not certify the latest pair. Recovery final verification, physical feedback, energy, UPS, power cycling, RF coexistence and long-duration tests remain incomplete or unmeasured.

Port8765 is the offline interface preview. Port8766 is the active serial byte bridge to B. Stop only that owned serial bridge before any COM7 flash/serial test. When A returns, `python scripts/resume_validation.py --deploy-staged` deploys both reviewed images with MAC checks and preserved NVS, then runs paired tests. It does not enable physical outputs. Expect audio failure until recognition is repaired.

[Current evidence](../benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md) · [Review response](DEEP_REVIEW_RESPONSE.md) · [Handoff](../NEXT_CHAT_HANDOFF.md).
