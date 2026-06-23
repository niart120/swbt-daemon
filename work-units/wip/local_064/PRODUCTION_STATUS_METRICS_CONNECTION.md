# Production Status Metrics Connection

## 1. 概要

既存 status metrics schema に production path の観測点を接続する work unit。

`local_039` は IPC status schema と unavailable state を定義した。`local_058` では、report tick と IPC rejected / coalesced metrics の production 計測点が不完全であることが先送り事項として残った。

この work unit は schema 再設計ではなく、既存 fields が production path でどの条件により増えるか、または未観測として表現するかを固定する。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md` の先送り事項: report tick と IPC rejected/coalesced metrics を production path へ接続するか、未観測として表現する。
- `work-units/complete/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md` の status metrics schema。
- `spec/protocols/daemon-ipc-v1.md` の metrics fields。
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md` の `swbt_app_t` authoritative state。

use case:

- actor: debug client、maintainer、future observer client。
- 入力または状態: production daemon running、set_state accepted / rejected、coalesced updates、report tick success / failure、noop backend unavailable hardware state。
- 期待する観測結果: `get_status` の metrics counters は production path の実際の観測点から更新される。未観測値は measured value として返さない。
- 制約: protocol v1 の既存 field name と unit を壊さない。実機由来 metrics は hardware observation なしに measured と表現しない。
- 対象外: new metrics schema、external telemetry、high-frequency event stream、実機 report-rate measurement。

source から use case への変換:

`local_039` は schema contract を作った。後続では、architecture cutover 後の production path でその counters がどこから増えるかを inventory し、接続または未観測表現を test で固定する。

## 3. 対象範囲

- `swbt_metrics_t` の current fields と production caller を棚卸しする。
- report tick success / failure が production path で metrics に反映されるか確認する。
- IPC rejected / coalesced counters が application command path で期待どおり増えるか確認する。
- noop backend と production backend の hardware unavailable / available 表現を再確認する。
- field を追加せずに済むか、追加が必要なら protocol docs と tests を更新する。

## 4. 対象外

- actual report rate / jitter の実機測定。
- metrics schema の大幅な再設計。
- external telemetry backend。
- dashboard / GUI。
- parser fuzz、slow client hardening。

## 5. 関連 spec / docs

- `spec/protocols/daemon-ipc-v1.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/complete/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`
- `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md`
- `docs/status.md`

## 6. 根拠監査

not applicable。

この work unit は metrics wiring と status 表現を扱う。Switch protocol byte、BTstack source selection、report period、WinUSB/libusb fact を追加または変更しない。実機測定値を追加する場合は別 work unit とし、hardware observation として扱う。

## 7. 設計メモ

- metrics field name と unit は `daemon-ipc-v1.md` の current contract を優先する。
- production path で観測できない値は `hardware_status:"unavailable"` のままにする。
- report tick は fake timestamp 由来でも増える可能性があるため、hardware observation と混同しない。
- rejected / coalesced は owner policy と stale sequence handling の結果として確認する。

## 8. 対象ファイル

- `swbt/core/metrics.*`
- `swbt/application/app.*`
- `swbt/ipc/ipc_adapter.*`
- `swbt/ipc/ipc_json.*`
- `swbt/daemon/production_backend.*`
- `tests/report_metrics_test.c`
- `tests/ipc_json_test.c`
- `tests/daemon_production_backend_test.c`
- `spec/protocols/daemon-ipc-v1.md`
- `work-units/wip/local_064/PRODUCTION_STATUS_METRICS_CONNECTION.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | production path report tick success increments status report tick counters without implying hardware measurement | regression | integration | no |
| todo | production path report send failure increments send failure counters and keeps cleanup behavior | regression | integration | no |
| todo | rejected IPC state update increments rejected metrics without changing controller state | regression | unit | no |
| todo | coalesced state update count is either connected or documented as unavailable / zero by design | characterization | unit | no |
| todo | status response preserves existing metrics field names and units | regression | unit | no |

## 10. 検証

- red: `just build-debug`
  - result: expected failure。`tests/daemon_production_backend_test.c` が未実装の `report_tick_observer` / `report_tick_context` / `SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_OK` を参照し、build が失敗した。
- green: `$env:CTEST_ARGS='-R daemon_production_backend_test'; just test-debug`
  - result: pass。`daemon_production_backend_test` 1/1 passed。

item 1 での棚卸し結果:

- `swbt_metrics_record_report_tick` と `swbt_app_record_report_tick` は既存 API として存在していた。
- production report timer adapter は periodic scheduler tick の `now_us` と send result を同時に持つ唯一の境界である。
- production backend は timer adapter の `report_tick_observer` を `swbt_app_record_report_tick` に接続する。
- `hardware_status`、`actual_report_rate_hz`、`jitter_max_us` は実機観測なしでは更新しない。

## 11. 実機実行条件

通常は実機不要。production fake adapter と IPC JSON tests で閉じる。

actual report rate、jitter、adapter driver state などを measured value として扱う場合は実機 work unit に切り出す。実行時は専用 USB Bluetooth dongle、WinUSB、明示承認、`docs/hardware-test-log.md` 記録を必須にする。

## 12. 先送り事項

none。起票時点の先送り事項は、この record の source として取り込んだ。

## 13. チェックリスト

- [x] source を `local_039` と `local_058` から特定した。
- [x] use case を existing metrics schema の production connection として定義した。
- [x] current metrics caller を棚卸しした。
- [x] red test を追加した。
- [ ] green 実装または未観測判断を記録した。
- [x] targeted CTest を実行した。
- [ ] protocol docs の更新要否を判定した。
