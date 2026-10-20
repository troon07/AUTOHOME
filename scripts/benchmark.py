"""
ClawGrid V2.0 Benchmark Runner
Executes comprehensive research comparison across Mode A, Mode B, and Mode C.
Outputs raw JSON, raw CSV, and research report.
Strictly labels all data with authoritative provenance tags.
"""

import os
import sys
import csv
import json
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from simulator.cooperative_model import CooperativeInferenceSimulator

def run_suite():
    sim = CooperativeInferenceSimulator()
    models = [
        {"name": "Nano-MLP (1.0M)", "params": 1_000_000, "layers": 8, "hidden": 128},
        {"name": "TinyStories (3.2M)", "params": 3_200_000, "layers": 12, "hidden": 256},
        {"name": "Micro-Transformer (15M)", "params": 15_000_000, "layers": 16, "hidden": 512}
    ]

    all_results = []
    csv_rows = []

    print("=========================================================================")
    print("  ClawGrid V2.0 Cooperative Computing Research Benchmark Suite          ")
    print("=========================================================================")

    for m in models:
        print(f"\n[Benchmarking] Model: {m['name']} (Params: {m['params']:,}, Hidden: {m['hidden']}, Layers: {m['layers']})")
        res = sim.simulate_model_execution(
            param_count=m["params"],
            layers=m["layers"],
            hidden_dim=m["hidden"],
            seq_len=1
        )
        res["model_name"] = m["name"]
        all_results.append(res)

        mode_a = res["mode_a_single_node"]
        mode_b = res["mode_b_parallel_tasks"]
        mode_c = res["mode_c_split_model"]

        print(f"  -> Mode A (Single Node):     Latency = {mode_a['latency_ms']:.1f} ms | Throughput = {mode_a['tokens_per_second']} tok/s")
        print(f"  -> Mode B (Parallel Tasks):  Latency = {mode_b['latency_ms']:.1f} ms | Semantic packet = {mode_b['semantic_packet_bytes']} B")
        print(f"  -> Mode C (Split-Model):     Latency = {mode_c['latency_ms']:.1f} ms | Wire = {mode_c['total_wire_bytes']} B | Speedup = {mode_c['speedup_vs_single_node']}x")
        print(f"     Verdict: {mode_c['helped_or_hurt']}")

        csv_rows.append({
            "model_name": m["name"],
            "params": m["params"],
            "mode_a_latency_ms": mode_a["latency_ms"],
            "mode_a_tokens_per_sec": mode_a["tokens_per_second"],
            "mode_b_latency_ms": mode_b["latency_ms"],
            "mode_c_latency_ms": mode_c["latency_ms"],
            "mode_c_wire_bytes": mode_c["total_wire_bytes"],
            "mode_c_speedup": mode_c["speedup_vs_single_node"],
            "verdict": mode_c["helped_or_hurt"],
            "source_tag": res["source"]
        })

    # Save Raw JSON
    raw_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "benchmarks", "raw")
    os.makedirs(raw_dir, exist_ok=True)
    json_path = os.path.join(raw_dir, "benchmark_results.json")
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(all_results, f, indent=2)
    print(f"\n[Output] Saved raw JSON to {json_path}")

    # Save Raw CSV
    csv_path = os.path.join(raw_dir, "benchmark_summary.csv")
    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(csv_rows[0].keys()))
        writer.writeheader()
        writer.writerows(csv_rows)
    print(f"[Output] Saved raw CSV to {csv_path}")

    # Generate Markdown Research Report
    report_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "benchmarks", "reports")
    os.makedirs(report_dir, exist_ok=True)
    report_path = os.path.join(report_dir, "RESEARCH_REPORT.md")
    
    with open(report_path, "w", encoding="utf-8") as f:
        f.write("# ClawGrid V2.0 Analytical Simulation (Not a Hardware Research Result)\n\n")
        f.write("All values below are `SIMULATED` by a fixed cost model. No neural layers, activation tensors, or physical ESP-NOW links were executed. These numbers cannot establish hardware speedup. See `docs/RESEARCH_DIRECTION.md`.\n\n")
        f.write("## 1. Scenario and Assumptions\n")
        f.write("**Primary Question**: *Under what memory, tensor-size, compute, and communication conditions does cooperative inference across two ESP32-S3-N16R8 nodes outperform single-node inference or independent task placement for a local smart-home assistant?*\n\n")
        f.write("### Outputs of the assumed cost model (not empirical conclusions):\n")
        f.write("1. Mode B uses hard-coded voice, state, and network timing assumptions. It is a different workload from Mode A token generation, so their latencies do not form a valid speedup comparison.\n")
        f.write("2. Under the chosen simulated constants, Mode C is slower than Mode A. Test the hypothesis with a real quantized model and device traces.\n\n")
        f.write("## 2. Quantitative Results Matrix\n\n")
        f.write("| Model Name | Parameters | Mode A (Single Node) | Mode B (Parallel Tasks) | Mode C (Split-Model) | Wire Overhead | Split Speedup | Provenance |\n")
        f.write("|---|---|---|---|---|---|---|---|\n")
        for r in csv_rows:
            f.write(f"| **{r['model_name']}** | {r['params']:,} | {r['mode_a_latency_ms']} ms | {r['mode_b_latency_ms']} ms | {r['mode_c_latency_ms']} ms | {r['mode_c_wire_bytes']} B | {r['mode_c_speedup']}x | `{r['source_tag']}` |\n")
        f.write("\n## 3. Proposed Hardware Architecture\n")
        f.write("- **Automation & Voice**: Prototype task-parallel placement. Node A currently handles one PIR and local SafetyKernel actuation. WakeNet + MultiNet and actuation latency remain to be verified on hardware.\n")
        f.write("- **Model choice**: Prefer constrained offline command recognition for actuation and evaluate a small temporal occupancy predictor separately. No generative model is deployed.\n")

    print(f"[Output] Generated Research Report at {report_path}")

if __name__ == "__main__":
    run_suite()
