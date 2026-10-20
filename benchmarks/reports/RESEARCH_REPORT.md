# ClawGrid V2.0 Analytical Simulation (Not a Hardware Research Result)

All values below are `SIMULATED` by a fixed cost model. No neural layers, activation tensors, or physical ESP-NOW links were executed. These numbers cannot establish hardware speedup. See `docs/RESEARCH_DIRECTION.md`.

## 1. Scenario and Assumptions
**Primary Question**: *Under what memory, tensor-size, compute, and communication conditions does cooperative inference across two ESP32-S3-N16R8 nodes outperform single-node inference or independent task placement for a local smart-home assistant?*

### Outputs of the assumed cost model (not empirical conclusions):
1. Mode B uses hard-coded voice, state, and network timing assumptions. It is a different workload from Mode A token generation, so their latencies do not form a valid speedup comparison.
2. Under the chosen simulated constants, Mode C is slower than Mode A. Test the hypothesis with a real quantized model and device traces.

## 2. Quantitative Results Matrix

| Model Name | Parameters | Mode A (Single Node) | Mode B (Parallel Tasks) | Mode C (Split-Model) | Wire Overhead | Split Speedup | Provenance |
|---|---|---|---|---|---|---|---|
| **Nano-MLP (1.0M)** | 1,000,000 | 33.33 ms | 27.41 ms | 37.81 ms | 250 B | 0.882x | `SIMULATED` |
| **TinyStories (3.2M)** | 3,200,000 | 106.67 ms | 27.41 ms | 115.63 ms | 500 B | 0.923x | `SIMULATED` |
| **Micro-Transformer (15M)** | 15,000,000 | 500.0 ms | 27.41 ms | 513.45 ms | 750 B | 0.974x | `SIMULATED` |

## 3. Proposed Hardware Architecture
- **Automation & Voice**: Prototype task-parallel placement. Node A currently handles one PIR and local SafetyKernel actuation. WakeNet + MultiNet and actuation latency remain to be verified on hardware.
- **Model choice**: Prefer constrained offline command recognition for actuation and evaluate a small temporal occupancy predictor separately. No generative model is deployed.
