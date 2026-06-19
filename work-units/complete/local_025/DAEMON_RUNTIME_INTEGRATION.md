# Daemon Runtime Integration

## 1. 概要

stub の `swbt-daemon` を testable runtime core へ置き換える work unit。

IPC server、state mailbox、BTstack bridge、periodic report adapter、shutdown cleanup を fake backend で統合する。

この work unit は runtime lifecycle と依存注入境界を固定する。実 Bluetooth adapter、Switch pairing、HID advertising、report loop は起動しない。

## 2. 起点 / ユースケース

source:

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` は daemon が IPC で controller state snapshot を受け取り、BTstack 側で periodic input report を送る方針を示している。
- `work-units/complete/local_022/SUBCOMMAND_REPLY_SEND_QUEUE.md` は output report 由来の subcommand reply queue を追加した。
- `work-units/complete/local_023/BTSTACK_INPUT_REPORT_TIMER_ADAPTER.md` は BTstack timer と HID interrupt send 境界を追加した。
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md` は IPC session と latest-state mailbox の接続を追加した。

use case:

- actor: daemon entrypoint。
- 入力または状態: runtime config、IPC backend、BTstack HID registration backend、output report handler backend、input report timer backend。
- 期待する観測結果: runtime start は IPC session と mailbox を接続し、HID 登録、output handler、report timer を順に起動する。stop は neutral state を保存して、起動済み resource を一度だけ停止する。
- 制約: 実 Bluetooth adapter を開かず、fake backend で lifecycle を検証する。
- 対象外: Switch pairing 成功、HID advertising、実 report loop、metrics schema。

source から use case への変換:

既存の個別 component を `main.c` へ直接詰め込むのではなく、runtime core に lifecycle と cleanup primitive を置く。unit test は fake backend で lifecycle と mailbox 接続を観測し、実機作業が必要な production backend open は後続 work unit に残す。

## 3. 対象範囲

- daemon runtime core を追加する。
- `apps/swbt-daemon/main.c` を runtime 起動の thin entrypoint にする。
- IPC server と state mailbox を runtime に接続する。
- BTstack HID registration、output report handler、input report timer adapter を fake backend integration test で接続する。
- shutdown 時に owner を解除し、neutral state を適用し、timer と IPC を停止する。
- fatal backend error の cleanup path を unit test で固定する。

## 4. 対象外

- 実機 Switch pairing の成功判定。
- HID advertising と report loop の manual bring-up。
- report period 比較。
- metrics と structured logging の詳細 schema。
- authentication token。
- 複数 controller 同時接続。
- binary release と installer。

## 5. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `work-units/complete/local_008/IPC_SESSION_CORE.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`
- `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md`
- `work-units/complete/local_012/BTSTACK_HID_DEVICE_REGISTRATION.md`
- `work-units/complete/local_013/BTSTACK_OUTPUT_REPORT_PARSER_BRIDGE.md`
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md`
- `work-units/complete/local_018/BTSTACK_PRODUCTION_ADAPTER.md`
- `work-units/complete/local_019/BTSTACK_OUTPUT_REPORT_CALLBACKS.md`
- `work-units/complete/local_022/SUBCOMMAND_REPLY_SEND_QUEUE.md`
- `work-units/complete/local_023/BTSTACK_INPUT_REPORT_TIMER_ADAPTER.md`
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`
- `docs/hardware-test-log.md`

## 6. 根拠監査

この work unit は Switch protocol bytes、BTstack source selection、HID descriptor 値、report period 値を追加または変更しない。

`source-audit` が必要な新しい upstream fact はない。runtime lifecycle は swbt 側の設計境界として unit test で固定する。

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| current daemon entrypoint | runtime entrypoint | implementation fact | `apps/swbt-daemon/main.c` | verified by test |
| daemon lifecycle order | IPC start, HID register, output handler start, report timer start | design policy | this work unit | tested with fake backend |
| real BTstack adapter open behavior | 未監査 | 根拠監査と実機実行条件が必要 | `vendor/btstack`, `docs/hardware-test-log.md` | pending |
| neutral cleanup through real HID disconnect | 未記録 | hardware observation required | `docs/hardware-test-log.md` | not run |

daemon lifecycle の実 BTstack backend 順序は未監査である。

fake backend integration test は real Bluetooth adapter 挙動を証明しない。

実機 pairing、advertising、report loop はこの record の時点で未実行である。

## 7. 設計メモ

- runtime core は dependency injection で IPC と BTstack backend を差し替えられる形にする。
- `main.c` は config 作成、runtime 起動、exit code 変換だけを担当する。
- shutdown path は normal stop、signal、IPC admin shutdown、backend fatal error を同じ cleanup primitive に集約する。
- cleanup では neutral state を mailbox に保存してから timer と transport を止める。
- production daemon が adapter を開く path は実機 gate なしに自動テストへ入れない。

## 8. 対象ファイル

- `CMakeLists.txt`
- `apps/swbt-daemon/main.c`
- `swbt/daemon/runtime.h`
- `swbt/daemon/runtime.c`
- `swbt/daemon/config.h`
- `swbt/daemon/config.c`
- `swbt/core/state_mailbox.h`
- `swbt/core/state_mailbox.c`
- `swbt/btstack_bridge/input_report_timer_adapter.h`
- `swbt/btstack_bridge/input_report_timer_adapter.c`
- `tests/daemon_runtime_test.c`
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | runtime init rejects invalid config and leaves backends unopened | new | unit | no |
| done | runtime start wires IPC, mailbox, HID registration, output handler, and report timer with fake backends | new | integration | no |
| done | shutdown applies neutral state and stops timer, IPC, and backend resources once | edge | integration | no |
| done | fatal backend error returns failure and still runs cleanup | edge | integration | no |
| done | `swbt-daemon` entrypoint returns runtime exit code without opening hardware in tests | regression | integration | no |

TDD status:

- source: daemon runtime lifecycle と fake backend integration。
- use case: entrypoint から runtime core を起動し、IPC session と mailbox、HID 登録、output handler、report timer を接続する。
- item: `swbt-daemon` entrypoint returns runtime exit code without opening hardware in tests。
- state: done。
- commands:
  - red: `just build-debug` は missing `daemon/config.h` のため compile 失敗。
  - green: `just debug` は `apps/swbt-daemon/main.c` の `NULL` include 漏れで一度 compile 失敗した後、`stddef.h` 追加で pass。CTest 22/22。
  - refactor: runtime が `swbt_btstack_output_report_handler_t` を所有し、fake backend に handler pointer を渡す形へ整理した。
  - final: `just verify` は pass。format check、clang-tidy、linux debug tests、ASan/UBSan tests、Windows MinGW cross build が通った。

Test desiderata:

- purpose: runtime lifecycle と cleanup ordering を、実機を開かない fake backend integration test で固定する。
- key trade-offs: deterministic / fast / specific を優先した。production BTstack open、HID advertising、Switch pairing は predictive ではないため、この work unit の自動テストに入れていない。
- risks: 実 adapter での advertising、pairing、report loop、HID disconnect cleanup は未検証である。
- action: 実機境界は hardware gate 付きの後続 work unit で扱う。

## 10. 検証

実行済み:

- red: `just build-debug` は missing `daemon/config.h` のため compile 失敗。
- green: `just debug` pass、CTest 22/22。
- first verify: `just verify` は clang-tidy の `clang-analyzer-core.CallAndMessage` で失敗。テスト内の function pointer に null 分岐を追加して修正した。
- final verify: `just verify` pass。format check、clang-tidy、linux debug tests、ASan/UBSan tests、Windows MinGW cross build が通った。

## 11. 実機実行条件

fake backend integration test では実機検証は不要である。

production daemon が Bluetooth adapter を開く run は実機作業として扱う。

Switch pairing、HID advertising、periodic report loop、output report handling は実機作業として扱う。

実機作業はユーザの明示承認を必要とする。

実機作業は専用 USB Bluetooth ドングルを使う。

実機作業は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定して実行する。

実機結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、driver、BTstack commit、swbt commit、Switch firmware、report period、結果、cleanup を記録する。

この work unit では実機コマンドを実行していない。理由は、fake backend で runtime lifecycle と cleanup を検証し、Bluetooth adapter、Switch pairing、HID advertising、report loop を開始していないためである。

## 12. 先送り事項

- 観測: production daemon が実 Bluetooth adapter を開く順序と失敗時の Bluetooth stack cleanup は、fake backend test だけでは証明できない。
  先送り理由: adapter open、HID advertising、pairing は実機実行条件に入る。
  次の置き場: 実機 bring-up work unit または `docs/hardware-test-log.md`。
- 観測: metrics と structured logging の schema は runtime lifecycle から独立して決める必要がある。
  先送り理由: この work unit は lifecycle と cleanup を固定する。
  次の置き場: `local_026`。

## 13. チェックリスト

- [x] work unit record を作成した。
- [x] work unit record を新形式へ更新した。
- [x] red を確認した。
- [x] daemon runtime core を追加した。
- [x] fake backend integration test を追加した。
- [x] `just` 経由の検証を実行した。
- [x] sanitizer または cross build の必要性を判断した。
- [x] 実機状態を記録した。
