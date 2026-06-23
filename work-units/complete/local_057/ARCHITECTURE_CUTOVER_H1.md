# Architecture Cutover H1

## 1. 概要

`local_056` の software cutover 後に残った Hardware Gate H1 を実行し、architecture cutover 後の production path が専用 USB Bluetooth dongle と Switch で Button A、owner disconnect neutral、shutdown trailing neutral を維持することを確認した work unit。

この work unit では H1 実行中に shutdown trailing neutral の失敗を観測したため、旧 runtime / session / mailbox は復活させず、new path の shutdown scheduling と timer adapter を修正した。最終 H1 は pass である。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-23: architecture cutover を完遂する。
- `spec/architecture/daemon-architecture-cutover.md`: Hardware Gate H1 を rearchitecture completion condition として採用した current spec。
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`: software cutover 完了後、H1 が未実行として残っていた。

use case:

- actor: hardware operator、maintainer、reviewer。
- 入力または状態: `codex/architecture-cutover` の software gate 済み build、CSR8510 A10、WinUSB、Switch2、production backend。
- 期待する観測結果: Button A report が HCI dump に出る。owner disconnect 後に neutral report が出る。再接続後の Button A と daemon shutdown trailing neutral が確認できる。current connection の transport anomaly は 0 件である。
- 制約: pairing、HID advertising、report loop、shutdown 実機操作は明示承認後だけ実行する。内蔵 Bluetooth と常用 dongle は対象にしない。
- 対象外: heartbeat timeout、explicit release、stick 全方向、report period 比較、sleep / resume、USB removal / reinsertion、bonded reconnect persistence、初回 pairing 詳細。

source から use case への変換:

`local_056` は software cutover を証明したが、採用 spec の H1 pass 条件は満たしていなかった。H1 は追加の architecture refactor ではなく、cutover 後の production path が実機で Button A と neutral fail-safe を維持するかを確認する gate として扱った。

## 3. 対象範囲

- H1 実行前の承認範囲、adapter identity、driver state、BTstack commit、swbt source snapshot を記録する。
- `SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1` を明示して production daemon を起動する。
- Button A、owner disconnect neutral、client reconnect 後 Button A、daemon shutdown trailing neutral を確認する。
- H1 failure で見つかった shutdown neutral の欠落を new path で修正する。
- HCI dump と daemon trace を artifact として残す。
- `docs/hardware-test-log.md` に H1 result を記録する。
- H1 pass 後に `spec/architecture/daemon-architecture-cutover.md` と `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md` の H1 状態を更新する。

## 4. 対象外

- Switch HID report byte、subcommand byte、SPI data、rumble packet の新規値。
- report period の比較。
- bonded reconnect persistence。
- adapter removal / reinsertion recovery。
- sleep / resume recovery。
- status / observability protocol の拡張。
- Windows native CI。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`
- `docs/hardware-test-log.md`
- `docs/status.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`

## 6. 根拠監査

条件付き。Switch protocol byte、report period、HID descriptor、subcommand、SPI、rumble packet は変更していない。

BTstack run loop callback と Windows 実装の扱いは、固定 submodule の source fact として確認した。

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| main thread callback API | `btstack_run_loop_execute_on_main_thread` | source fact | `vendor/btstack/src/btstack_run_loop.h:355` at `075a0780f0fad7ff67d58ac19f46e8953656a752` | stable source fact |
| Windows main-thread callback dispatch | callback list に登録し、Windows event で run loop を起こす | source fact | `vendor/btstack/platform/windows/btstack_run_loop_windows.c:170` and `:180` at `075a0780f0fad7ff67d58ac19f46e8953656a752` | stable source fact |
| H1 final artifact | trailing neutral before HCI power-off | hardware observation | `tmp/hardware/local_057/20260623-105416-architecture-cutover-h1` | pass |

## 7. 設計メモ

- H1 failure 時にも旧 runtime、旧 session、旧 mailbox、旧 backend table は復活させない。
- Windows console handler から直接 BTstack send / power-off へ進めず、`btstack_run_loop_execute_on_main_thread` 経由で shutdown neutral を BTstack run loop 側へ寄せた。
- `swbt_btstack_input_report_timer_adapter_send_neutral_now` は即時送信に失敗した場合、urgent neutral を pending にし、次の CAN_SEND で periodic report より先に送る。
- final H1 では seq3 を PowerShell の直接 IPC client で `acquire` / `set_state` し、connection を保持したまま shutdown した。`swbt-debug-client` の job 起動と HCI dump polling では、shutdown 前に L2CAP close へ進む timing failure があった。
- raw HCI dump は artifact として保持し、repository へ全文転記しない。durable docs には判定に必要な line order と count だけを残す。

## 8. 対象ファイル

- `swbt/btstack_bridge/input_report_timer_adapter.c`
- `swbt/btstack_bridge/input_report_timer_adapter.h`
- `swbt/btstack_bridge/production_adapter.h`
- `swbt/btstack_bridge/production_btstack.c`
- `swbt/daemon/production_backend.c`
- `swbt/daemon/production_backend.h`
- `tests/btstack_input_report_timer_adapter_test.c`
- `tests/daemon_production_backend_test.c`
- `docs/hardware-test-log.md`
- `docs/status.md`
- `README.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`
- H1 実行 artifact: `tmp/hardware/local_057/20260623-105416-architecture-cutover-h1`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | H1 preflight records dedicated adapter identity, WinUSB driver assignment, swbt source snapshot, BTstack commit, Switch firmware baseline, report period, and artifact paths | hardware-gated | hardware | yes |
| done | Button A report is observed in HCI dump before owner disconnect | hardware-gated | hardware | yes |
| done | owner disconnect produces a neutral report after the last non-neutral report | hardware-gated | hardware | yes |
| done | reconnect after owner disconnect can send Button A again | hardware-gated | hardware | yes |
| done | daemon shutdown sends trailing neutral before HCI power-off / transport stop | hardware-gated | hardware | yes |
| done | current connection transport anomalies are 0 and cleanup result is recorded | hardware-gated | hardware | yes |
| done | failed immediate neutral send is retried as urgent neutral before periodic report | regression | unit | no |
| done | production shutdown request is scheduled onto the BTstack main thread and pending neutral completes on CAN_SEND | regression | integration | no |

TDD status:

- source: `spec/architecture/daemon-architecture-cutover.md` Hardware Gate H1 and `local_056` deferred item。
- use case: production path keeps Button A and neutral fail-safe behavior after architecture cutover。
- item: H1 preflight、software fix、hardware execution。
- state: done。
- commands:
  - `just debug`: pass。38/38 tests。
  - `just windows-cross`: pass。Windows MinGW build 更新。
  - H1 final run: pass。artifact `tmp/hardware/local_057/20260623-105416-architecture-cutover-h1`。
- notes: failed H1 attempts `20260623-104806`, `20260623-105050`, `20260623-105219` は pass に含めない。最終 pass は direct IPC held client と shutdown trailing neutral の HCI line order で判定した。

## 10. 検証

- `just debug`: pass。38/38 tests。`daemon_production_backend_test` は main-thread shutdown scheduling と pending neutral CAN_SEND completion を含む。
- `just windows-cross`: pass。H1 実行用 `build/windows-mingw-debug/swbt-daemon.exe` と `swbt-debug-client.exe` を更新した。
- H1 final run: pass。`tmp/hardware/local_057/20260623-105416-architecture-cutover-h1`。
- H1 daemon trace: `production: shutdown requested`、`production: shutdown neutral send`、`production: neutral send adapter ok`、`production: shutdown neutral send ok`、`btstack: hci power off` の順。
- H1 HCI dump: Button A `080000` `109` 件、neutral `000000` `205` 件。末尾は line `953` Button A、line `954` trailing neutral、line `955` `hci_power_control: 0`。
- H1 current connection anomalies: `invalid size` `0` 件、`non-registered handle` `0` 件。BTstack USB scan の `usb_open: Device open failed` は `16` 件あり、対象外 device / interface 探索時の記録として H1 transport failure から分けた。
- `just verify`: pass。format check、clang-tidy、debug CTest、ASan CTest、Windows MinGW cross build を含む。

## 11. 実機実行条件

承認済み範囲:

- 対象 adapter は CSR8510 A10 専用 USB Bluetooth dongle。内蔵 Bluetooth と常用 dongle は対象外。
- Windows native で対象 adapter の driver は WinUSB。
- pairing / HID advertising / periodic input report loop / owner disconnect / daemon shutdown を含む H1 scope。
- `SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1` の使用。
- artifact 出力先 `tmp/hardware/local_057/...`。

記録した項目:

- OS / environment。
- dongle VID/PID。
- driver。
- backend。
- BTstack commit。
- swbt source snapshot。
- Switch firmware baseline。
- report period。
- daemon command / client command。
- result。
- cleanup。

## 12. 先送り事項

- none。H1 は pass 済みであり、`local_056` からの hardware-gated deferred item はこの record で閉じる。

## 13. チェックリスト

- [x] H1 実行範囲の承認を得た。
- [x] adapter identity と WinUSB driver assignment を記録した。
- [x] software gate 済み source snapshot / branch を記録した。
- [x] daemon trace と HCI dump artifact path を決めた。
- [x] Button A report を確認した。
- [x] owner disconnect neutral を確認した。
- [x] reconnect 後 Button A を確認した。
- [x] shutdown trailing neutral を確認した。
- [x] current connection anomaly 0 件を確認した。
- [x] cleanup result を記録した。
- [x] `docs/hardware-test-log.md` を更新した。
- [x] `spec/architecture/daemon-architecture-cutover.md` と `local_056` の H1 状態を更新した。
