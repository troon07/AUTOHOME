# Hardware research test results

Evidence: `benchmarks\raw\hardware\research_20261020T033845Z.json`. Source: **HARDWARE_MEASURED**.

Bare boards, no sensors/loads. HTTP tests use on-board loopback; they do not validate a phone browser or external network load.

| Test | Result | Source |
|---|---|---|
| COM5_runtime_selftest | PASS | HARDWARE_MEASURED |
| COM5_psram_probe | PASS | HARDWARE_MEASURED |
| COM7_runtime_selftest | PASS | HARDWARE_MEASURED |
| COM7_psram_probe | PASS | HARDWARE_MEASURED |
| authenticated_room_telemetry | PASS | HARDWARE_MEASURED |
| http_loopback_suite | PASS | HARDWARE_MEASURED |
| http_state | PASS | HARDWARE_MEASURED |
| http_memory | PASS | HARDWARE_MEASURED |
| http_unauthorized_control | PASS | HARDWARE_MEASURED |
| http_wrong_token | PASS | HARDWARE_MEASURED |
| http_unauthorized_transaction | PASS | HARDWARE_MEASURED |
| http_authorized_transaction | PASS | HARDWARE_MEASURED |
| http_parser_preview | PASS | HARDWARE_MEASURED |
| http_parser_ambiguity | PASS | HARDWARE_MEASURED |
| http_bad_control | PASS | HARDWARE_MEASURED |
| http_overflow_control | PASS | HARDWARE_MEASURED |
| http_oversized_body | PASS | HARDWARE_MEASURED |
| http_unknown_route | PASS | HARDWARE_MEASURED |
| http_authorized_control_queue | PASS | HARDWARE_MEASURED |
| away_scene | PASS | HARDWARE_MEASURED |
| light_on | PASS | HARDWARE_MEASURED |
| rapid_off_denied | PASS | HARDWARE_MEASURED |
| light_off_after_dwell | PASS | HARDWARE_MEASURED |
| sleep_scene | PASS | HARDWARE_MEASURED |
| away_after_sleep | PASS | HARDWARE_MEASURED |
| enable_fan_automation | PASS | HARDWARE_MEASURED |
| set_vacancy_grace | PASS | HARDWARE_MEASURED |
| reject_short_grace | PASS | HARDWARE_MEASURED |
| reject_timer_overflow | PASS | HARDWARE_MEASURED |
| home_scene | PASS | HARDWARE_MEASURED |
| fan_on | PASS | HARDWARE_MEASURED |
| fan_timer | PASS | HARDWARE_MEASURED |
| fan_reported_on_before_timer | PASS | HARDWARE_MEASURED |
| fan_off_after_timer | PASS | HARDWARE_MEASURED |
| timer_cancel | PASS | HARDWARE_MEASURED |
| resume_light_auto | PASS | HARDWARE_MEASURED |
| restore_fan_policy | PASS | HARDWARE_MEASURED |
| restore_light_policy | PASS | HARDWARE_MEASURED |
| restore_grace | PASS | HARDWARE_MEASURED |
| all_off_scene | PASS | HARDWARE_MEASURED |
| noop_ack_trials | PASS | HARDWARE_MEASURED |
| lost_ack_retry | PASS | HARDWARE_MEASURED |
| retry_uses_cached_result | PASS | HARDWARE_MEASURED |
| bad_auth_tag_rejected | PASS | HARDWARE_MEASURED |
| changed_duplicate_payload_rejected | PASS | HARDWARE_MEASURED |
| duplicate_replied_without_reexecution | PASS | HARDWARE_MEASURED |
| cached_ack_survives_altered_replay | PASS | HARDWARE_MEASURED |
| wrong_boot_rejected | PASS | HARDWARE_MEASURED |
| application_outage_timeout | PASS | HARDWARE_MEASURED |
| stale_state_becomes_unknown | PASS | HARDWARE_MEASURED |
| stale_control_rejected | PASS | HARDWARE_MEASURED |
| telemetry_recovers_after_outage | PASS | HARDWARE_MEASURED |
| control_recovers_after_outage | PASS | HARDWARE_MEASURED |
| COM5_memory_headroom | PASS | HARDWARE_MEASURED |
| COM5_bounded_queue | PASS | HARDWARE_MEASURED |
| COM7_memory_headroom | PASS | HARDWARE_MEASURED |
| COM7_bounded_queue | PASS | HARDWARE_MEASURED |

## Runtime memory observations

| Port | Samples | Min internal free B | Min largest internal block B | Min PSRAM free B | Min stack free B | Max RX drops | Max TX enqueue failures |
|---|---:|---:|---:|---:|---:|---:|---:|
| COM5 | 64 | 290292 | 278516 | 8374767 | 5932 | 0 | 0 |
| COM7 | 63 | 273976 | 262132 | 8373003 | 3308 | 0 | 0 |

## Idempotent OFF command acknowledgement RTT

These are independent command IDs issued sequentially in one bench run. The output was already off. Timing includes Node B queueing, authenticated radio request, SafetyKernel processing and acknowledgement; no physical switching or AI inference is measured.

| Metric | Value |
|---|---:|
| Submitted trials | 1000 |
| Completed with expected outcome | 1000 |
| Failed / missing outcomes | 0 |
| USB diagnostic recovery queries | 5 |
| n | 1000.000 |
| mean (microseconds) | 10337.330 |
| median (microseconds) | 11251.500 |
| p95 (microseconds) | 15291.250 |
| p99 (microseconds) | 25323.470 |
| minimum (microseconds) | 5273.000 |
| maximum (microseconds) | 41303.000 |
| Bootstrap 95% CI for mean (microseconds) | 10118.097 to 10563.124 |
| Wilson 95% completion interval | 0.99617 to 1.00000 |

Bootstrap: 2,000 resamples, seed 20261020. The interval assumes exchangeable samples; autocorrelation and one-session bias limit inference. No cross-device/population generalization is established.

## Publication limits

Speech accuracy, occupancy prediction, electrical energy, physical relay behavior, external interference, power-cycle recovery, long-duration stability, and single-vs-dual-model comparisons remain pending. See `docs/PUBLICATION_TEST_MATRIX.md`.
