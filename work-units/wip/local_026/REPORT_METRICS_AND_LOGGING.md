# Report Metrics And Logging

## 1. 概要

report loop、IPC update、daemon lifecycle の metrics と logging を追加するための計画 record。

initial design の metrics 名は候補として扱い、実装で記録する値と実機で観測した値を分ける。

## 2. 対象範囲

- in-process metrics struct と update API を追加する。
- report tick interval と send result を fake clock unit test で記録する。
- IPC state update の accepted、rejected、coalesced count を記録する。
- startup log と shutdown log の sink を追加する。
- hardware-derived metrics は未実行として区別できる出力にする。

## 3. 対象外

- HID report 内への latency 計測データ埋め込み。
- 外部 telemetry backend。
- stable IPC metrics protocol。
- report period の tuning。
- Switch 実機での report rate 判定。
- GUI や dashboard。

## 4. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`
- `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md`
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md`
- `work-units/wip/local_023/BTSTACK_INPUT_REPORT_TIMER_ADAPTER.md`
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`
- `work-units/wip/local_025/DAEMON_RUNTIME_INTEGRATION.md`
- `docs/hardware-test-log.md`

## 5. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| metrics schema | 未確定 | design policy | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` | pending |
| report interval metrics | fake clock only | implementation fact after tests | `tests/report_metrics_test.c` | pending |
| real report rate and jitter | 未記録 | hardware observation required | `docs/hardware-test-log.md` | not run |
| selected adapter and driver log fields | 未記録 | backend fact and hardware observation required | runtime backend, `docs/hardware-test-log.md` | pending |

`actual_report_rate_hz` や jitter を実機事実として断定しない。

startup log に出す adapter identity と driver state は backend 実装または実機記録で裏付ける。

metrics 名は initial design 由来の候補であり、stable protocol surface ではない。

## 6. 設計メモ

- metrics update API は runtime から呼び、global mutable state に依存しない。
- logger は sink callback を受け取り、unit test では memory sink を使う。
- report interval は fake monotonic timestamp から計算し、real clock 精度はこの work unit で保証しない。
- state update coalescing は mailbox metadata から記録する。
- unavailable hardware fields は空文字ではなく unavailable state として表現する。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/core/metrics.h`
- `swbt/core/metrics.c`
- `swbt/core/logging.h`
- `swbt/core/logging.c`
- `swbt/core/state_mailbox.h`
- `swbt/core/state_mailbox.c`
- `swbt/btstack_bridge/input_report_scheduler.h`
- `swbt/daemon/runtime.h`
- `swbt/daemon/runtime.c`
- `tests/report_metrics_test.c`
- `work-units/wip/local_026/REPORT_METRICS_AND_LOGGING.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | metrics init returns zero counters and unavailable hardware fields | new | unit | no |
| todo | report tick events update interval average, max, and send counters from fake timestamps | new | unit | no |
| todo | state update accepted, rejected, and coalesced events update separate counters | new | unit | no |
| todo | startup and shutdown log sink emits configured fields without claiming hardware observations | new | unit | no |
| todo | metrics snapshot does not mutate live counters | edge | unit | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、実装、build、CTest、実機検証は実行していない。

実装後は `make debug CTEST_ARGS="-R report_metrics_test"` を実行する。

runtime へ接続した場合は `make debug CTEST_ARGS="-R daemon_runtime_test"` を実行する。

## 10. 実機実行条件

通常の metrics unit test では実機検証は不要である。

real report rate、jitter、adapter identity、driver state、disconnect、reconnect を測る作業は実機作業として扱う。

実機作業はユーザの明示承認を必要とする。

実機作業は専用 USB Bluetooth ドングルを使う。

実機作業は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定して実行する。

実機結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、driver、BTstack commit、swbt commit、Switch firmware、report period、metrics、結果、cleanup を記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] red を確認した。
- [ ] metrics core を追加した。
- [ ] logging sink を追加した。
- [ ] unit test を追加した。
- [ ] `make debug` を実行した。
- [ ] sanitizer または cross build の必要性を判断した。
- [ ] 実機状態を記録した。
