# Current constrained assistant

The text preview is a deterministic room/device/action parser. It is not a language model. The separate native speech build deploys ESP-SR 1.7.0 MultiNet6 English on Node B, with 16 ON/OFF phrases covering light/fan in bedroom, living room, kitchen and study.

Browser push-to-talk captures at most four seconds of mono 16 kHz PCM16LE and uploads raw bytes. A static SpeechTask runs all recognition on B. A result is an untrusted proposal; confirmation sends operation 6 for the appropriate virtual device through B's transaction manager and A's SafetyKernel. No voice recognition is executed on the laptop or in the cloud.

Model readiness was verified on B. Node B has returned. The diagnostic transport timeout was repaired, and all 18 submitted PCM checksums matched on B. The latest synthetic run detected 0/16 command clips; silence and noise yielded no command. This is an unresolved functional failure, not human accuracy or inference/control concurrency validation. A real-voice diagnostic sample has been requested. See [current evidence](../benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md).

The configuration has no AFE, WakeNet, continuous listening, arbitrary transcription, I2S/DMA microphone or generative NLU. It is a raw-PCM push-to-talk baseline. Espressif recommends combining MultiNet with its audio front end; measure the baseline before choosing that extension. [Official command documentation](https://docs.espressif.com/projects/esp-sr/en/latest/esp32/speech_command_recognition/README.html) states that MultiNet6 English accepts grapheme commands directly; phoneme requirements differ by model generation.

Use localhost for laptop microphone capture. Phone AP HTTP provides visual controls but is not a secure microphone context. The browser only requests access when its record button is pressed; no phone changes are required. Audio is cleared from the board clip buffer after processing; benchmark TTS clips are explicitly SIMULATED inputs, not human-acoustic evaluation.

[Simulation setup](SIMULATION_LAB.md) · [Model selection](MODEL_EVALUATION.md) · [Publication protocol](PUBLICATION_TEST_MATRIX.md).
