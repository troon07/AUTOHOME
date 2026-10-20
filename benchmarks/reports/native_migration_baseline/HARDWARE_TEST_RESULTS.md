# Hardware research test results

Evidence: `benchmarks\raw\hardware\research_20261020T042308Z.json`. Source: **HARDWARE_MEASURED**.

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
| http_rtos_telemetry | PASS | HARDWARE_MEASURED |
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
| COM5_ControlTask_progress | PASS | HARDWARE_MEASURED |
| COM5_ControlTask_watchdog | PASS | HARDWARE_MEASURED |
| COM5_ControlTask_stack_margin | PASS | HARDWARE_MEASURED |
| COM5_ControlTask_diagnostic_queue | PASS | HARDWARE_MEASURED |
| COM5_LinkTask_progress | PASS | HARDWARE_MEASURED |
| COM5_LinkTask_watchdog | PASS | HARDWARE_MEASURED |
| COM5_LinkTask_stack_margin | PASS | HARDWARE_MEASURED |
| COM5_LinkTask_diagnostic_queue | PASS | HARDWARE_MEASURED |
| COM7_LinkTask_progress | PASS | HARDWARE_MEASURED |
| COM7_LinkTask_watchdog | PASS | HARDWARE_MEASURED |
| COM7_LinkTask_stack_margin | PASS | HARDWARE_MEASURED |
| COM7_LinkTask_diagnostic_queue | PASS | HARDWARE_MEASURED |

## Runtime memory observations

| Port | Samples | Min internal free B | Min largest internal block B | Min PSRAM free B | Min stack free B | Max RX drops | Max TX enqueue failures |
|---|---:|---:|---:|---:|---:|---:|---:|
| COM5 | 41 | 270000 | 258036 | 8374767 | 5400 | 0 | 0 |
| COM7 | 39 | 260580 | 249844 | 8373003 | 3480 | 0 | 0 |

## FreeRTOS task measurements

| Port / task | Core | Priority | Period ms | Max execution us | Max release lateness us | Deadline misses | Min stack free B | Watchdog |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| COM5 / ControlTask | 1 | 5 | 5 | 311 | 1 | 0 | 4140 | registered |
| COM5 / LinkTask | 0 | 3 | 5 | 1206 | 2 | 0 | 4152 | registered |
| COM7 / LinkTask | 0 | 3 | 5 | 1031 | 975 | 0 | 3600 | registered |

The configured five-millisecond period is a software target. Execution maxima and missed deadlines are observations, not a proven WCET bound. Watchdog registration is verified; an induced watchdog reboot/power-cycle recovery test is still pending.

## Idempotent OFF command acknowledgement RTT

These are independent command IDs issued sequentially in one bench run. The output was already off. Timing includes Node B queueing, authenticated radio request, SafetyKernel processing and acknowledgement; no physical switching or AI inference is measured.

| Metric | Value |
|---|---:|
| Submitted trials | 1000 |
| Completed with expected outcome | 1000 |
| Failed / missing outcomes | 0 |
| USB diagnostic recovery queries | 1 |
| n | 1000.000 |
| mean (microseconds) | 34376.461 |
| median (microseconds) | 23919.000 |
| p95 (microseconds) | 78844.700 |
| p99 (microseconds) | 137829.670 |
| minimum (microseconds) | 15616.000 |
| maximum (microseconds) | 186790.000 |
| Bootstrap 95% CI for mean (microseconds) | 33002.892 to 35903.231 |
| Wilson 95% completion interval | 0.99617 to 1.00000 |

Bootstrap: 2,000 resamples, seed 20261020. The interval assumes exchangeable samples; autocorrelation and one-session bias limit inference. No cross-device/population generalization is established.

## Publication limits

Speech accuracy, occupancy prediction, electrical energy, physical relay behavior, external interference, power-cycle recovery, long-duration stability, and single-vs-dual-model comparisons remain pending. See `docs/PUBLICATION_TEST_MATRIX.md`.
