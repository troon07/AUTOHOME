> The configurations below are planned/historical analytical comparisons, not executed neural benchmarks. Current measured allocation is speech/API on B, deterministic control on A. Only the retained controller HIL session has a completed hardware dataset; audio/single-versus-dual/split experiments remain incomplete. See [current evidence](../benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md) and [publication matrix](PUBLICATION_TEST_MATRIX.md).

# ClawGrid V2.0 Benchmark Methodology & Integrity Standards

## 1. Experimental Configurations

| Config ID | Description | Role Allocation |
|---|---|---|
| **A (Single Node)** | Baseline: Node A runs local voice + intent + automation end-to-end; Node B idle | Establishes single-core/single-board latency & memory baseline |
| **B (Parallel Tasks)** | Node A handles audio capture & voice parsing; sends semantic event to Node B for state coordination & web serving | Asynchronous distributed pipeline; zero tensor overhead |
| **C (Split Model)** | Partitioned neural model: Node A computes Layers 0..L/2; streams intermediate tensor activations via ESP-NOW; Node B computes Layers (L/2)+1..L | Tests cooperative deep learning speedup vs communication penalty |
| **D (Fault Injection)** | Node B rebooted or wireless packet loss (0%, 5%, 20%, 50%) injected | Validates fail-safe degradation, autonomous local control, and recovery |

## 2. Integrity & Attribution Protocol
Every output metric and chart must carry one of three authoritative source tags:
- `SIMULATED`: Generated via host multi-room simulator or mock networking.
- `HOST_MEASURED`: Measured on the developer host machine with native compiled binaries.
- `HARDWARE_MEASURED`: Measured on physical ESP32-S3 hardware via UART/JTAG timestamped telemetry.

Falsifying, rounding up, or fabricating benchmark results is strictly prohibited.
