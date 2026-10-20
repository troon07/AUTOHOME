"""
ClawGrid V2.0 Cooperative Inference & Benchmark Engine
Evaluates:
- Mode A: Single-node baseline
- Mode B: Parallel independent tasks (Semantic event exchange)
- Mode C: True partitioned model-parallel inference (Intermediate activation streaming)
Strictly tags all results as SIMULATED or HOST_MEASURED.
"""

import time
import math
from typing import Dict, Any, List, Tuple
from simulator.protocol import PacketEnvelope, MsgType, MsgFlags

class CooperativeInferenceSimulator:
    def __init__(self,
                 cpu_mhz: float = 240.0,
                 esp_now_effective_kbps: float = 1200.0, # 1.2 Mbps realistic ESP-NOW payload rate
                 mac_cycles_int8: float = 2.5, # ESP32-S3 vector instruction efficiency for INT8 MAC
                 psram_penalty_factor: float = 3.2): # PSRAM access latency factor vs internal SRAM
        self.cpu_mhz = cpu_mhz
        self.esp_now_kbps = esp_now_effective_kbps
        self.mac_cycles_int8 = mac_cycles_int8
        self.psram_penalty = psram_penalty_factor

    def simulate_model_execution(self,
                                 param_count: int,
                                 layers: int,
                                 hidden_dim: int,
                                 seq_len: int = 1) -> Dict[str, Any]:
        """
        Simulates inference across Modes A, B, and C for a model with given parameters.
        Returns detailed latency breakdowns, memory requirements, and comparison.
        """
        # Flops per token approximately 2 * param_count
        mac_ops = param_count * seq_len
        base_cycles = mac_ops * self.mac_cycles_int8 * self.psram_penalty
        full_compute_time_ms = (base_cycles / (self.cpu_mhz * 1e6)) * 1000.0

        # Memory calculations
        weights_int8_bytes = param_count # 1 byte per weight in INT8
        kv_cache_bytes = 2 * layers * seq_len * hidden_dim * 2 # 2 bytes per float16/int16 cache
        activation_dim_bytes = hidden_dim * seq_len # Intermediate tensor at split point (INT8)

        # Mode A: Single Node
        # Entire model on Node A
        mode_a_time_ms = full_compute_time_ms
        mode_a_mem_node_a = weights_int8_bytes + kv_cache_bytes + activation_dim_bytes
        mode_a_mem_node_b = 0
        mode_a_net_bytes = 0
        mode_a_net_time_ms = 0.0

        # Mode B: Parallel Independent Tasks
        # Node A runs Voice AFE / Wake / MultiNet (0.2 MB weights), Node B runs State / Policy
        mode_b_node_a_compute_ms = 18.5 # Typical ESP-SR keyword spot latency
        mode_b_node_b_compute_ms = 4.2  # State engine evaluation latency
        # Network: single 32-byte semantic command packet
        semantic_packet_bytes = 32
        mode_b_net_time_ms = (semantic_packet_bytes * 8 / (self.esp_now_kbps * 1000)) * 1000 + 4.5 # 4.5ms MAC ACK latency
        mode_b_total_latency_ms = mode_b_node_a_compute_ms + mode_b_net_time_ms + mode_b_node_b_compute_ms

        # Mode C: Split-Model Partitioned Inference (Layer 0..L/2 on Node A, Layer (L/2)+1..L on Node B)
        node_a_split_compute_ms = full_compute_time_ms * 0.5
        node_b_split_compute_ms = full_compute_time_ms * 0.5

        # Serialization & Quantization of intermediate tensor
        quant_cycles = activation_dim_bytes * 12 # Min/max search + scale calculation
        quant_time_ms = (quant_cycles / (self.cpu_mhz * 1e6)) * 1000.0

        # Chunking: ESP-NOW 250 byte frames. Reserve 24 bytes header -> 226 payload bytes
        chunk_payload_size = 220
        num_chunks = math.ceil(activation_dim_bytes / chunk_payload_size)
        total_transfer_bytes = num_chunks * 250
        # Transmit time: packet transmission + per-chunk inter-frame spacing (~2ms) + ACK
        tx_air_time_ms = (total_transfer_bytes * 8 / (self.esp_now_kbps * 1000)) * 1000
        packet_spacing_and_ack_ms = num_chunks * 2.8
        mode_c_net_time_ms = tx_air_time_ms + packet_spacing_and_ack_ms

        mode_c_total_latency_ms = node_a_split_compute_ms + quant_time_ms + mode_c_net_time_ms + quant_time_ms + node_b_split_compute_ms

        # Speedup analysis
        speedup_c_vs_a = mode_a_time_ms / mode_c_total_latency_ms if mode_c_total_latency_ms > 0 else 0

        return {
            "source": "SIMULATED",
            "model_params": param_count,
            "hidden_dim": hidden_dim,
            "layers": layers,
            "seq_len": seq_len,
            "tensor_split_bytes": activation_dim_bytes,
            "mode_a_single_node": {
                "latency_ms": round(mode_a_time_ms, 2),
                "node_a_psram_bytes": mode_a_mem_node_a,
                "node_b_psram_bytes": 0,
                "network_bytes_transferred": 0,
                "tokens_per_second": round(1000.0 / mode_a_time_ms, 2) if mode_a_time_ms > 0 else 0
            },
            "mode_b_parallel_tasks": {
                "latency_ms": round(mode_b_total_latency_ms, 2),
                "semantic_packet_bytes": semantic_packet_bytes,
                "network_time_ms": round(mode_b_net_time_ms, 2)
            },
            "mode_c_split_model": {
                "latency_ms": round(mode_c_total_latency_ms, 2),
                "node_a_compute_ms": round(node_a_split_compute_ms, 2),
                "node_b_compute_ms": round(node_b_split_compute_ms, 2),
                "quant_serialization_ms": round(quant_time_ms * 2, 2),
                "network_transfer_ms": round(mode_c_net_time_ms, 2),
                "chunks_transmitted": num_chunks,
                "total_wire_bytes": total_transfer_bytes,
                "speedup_vs_single_node": round(speedup_c_vs_a, 3),
                "helped_or_hurt": "HELPED" if speedup_c_vs_a > 1.0 else "HURT (Communication overhead exceeds compute savings)"
            }
        }
