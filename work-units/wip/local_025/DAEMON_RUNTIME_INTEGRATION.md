# Daemon Runtime Integration

## 1. 概要

stub の `swbt-daemon` を testable runtime core へ置き換えるための計画 record。

IPC server、state mailbox、BTstack bridge、periodic report adapter、shutdown cleanup を fake backend で統合する。

## 2. 対象範囲

- daemon runtime core を追加する。
- `apps/swbt-daemon/main.c` を runtime 起動の thin entrypoint にする。
- IPC server と state mailbox を runtime に接続する。
- BTstack HID registration、output report handler、input report timer adapter を fake backend integration test で接続する。
- shutdown 時に owner を解除し、neutral state を適用し、timer と IPC を停止する。
- fatal backend error の cleanup path を unit test で固定する。

## 3. 対象外

- 実機 Switch pairing の成功判定。
- HID advertising と report loop の manual bring-up。
- report period 比較。
- metrics と structured logging の詳細 schema。
- authentication token。
- 複数 controller 同時接続。
- binary release と installer。

## 4. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `work-units/complete/local_008/IPC_SESSION_CORE.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`
- `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md`
- `work-units/complete/local_012/BTSTACK_HID_DEVICE_REGISTRATION.md`
- `work-units/complete/local_013/BTSTACK_OUTPUT_REPORT_PARSER_BRIDGE.md`
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md`
- `work-units/wip/local_018/BTSTACK_PRODUCTION_ADAPTER.md`
- `work-units/wip/local_019/BTSTACK_OUTPUT_REPORT_CALLBACKS.md`
- `work-units/wip/local_022/SUBCOMMAND_REPLY_SEND_QUEUE.md`
- `work-units/wip/local_023/BTSTACK_INPUT_REPORT_TIMER_ADAPTER.md`
- `work-units/wip/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`
- `docs/hardware-test-log.md`

## 5. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| current daemon entrypoint | stub | implementation fact | `apps/swbt-daemon/main.c` | verified by inspection |
| daemon lifecycle order | 未確定 | design policy | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` | pending |
| real BTstack adapter open behavior | 未監査 | 根拠監査と実機実行条件が必要 | `vendor/btstack`, `docs/hardware-test-log.md` | pending |
| neutral cleanup through real HID disconnect | 未記録 | hardware observation required | `docs/hardware-test-log.md` | not run |

daemon lifecycle の実 BTstack backend 順序は未監査である。

fake backend integration test は real Bluetooth adapter 挙動を証明しない。

実機 pairing、advertising、report loop はこの record の時点で未実行である。

## 6. 設計メモ

- runtime core は dependency injection で IPC と BTstack backend を差し替えられる形にする。
- `main.c` は config 作成、runtime 起動、exit code 変換だけを担当する。
- shutdown path は normal stop、signal、IPC admin shutdown、backend fatal error を同じ cleanup primitive に集約する。
- cleanup では neutral state を mailbox に保存してから timer と transport を止める。
- production daemon が adapter を開く path は実機 gate なしに自動テストへ入れない。

## 7. 対象ファイル

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
- `work-units/wip/local_025/DAEMON_RUNTIME_INTEGRATION.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | runtime init rejects invalid config and leaves backends unopened | new | unit | no |
| todo | runtime start wires IPC, mailbox, HID registration, output handler, and report timer with fake backends | new | integration | no |
| todo | shutdown applies neutral state and stops timer, IPC, and backend resources once | edge | integration | no |
| todo | fatal backend error returns failure and still runs cleanup | edge | integration | no |
| todo | `swbt-daemon` entrypoint returns runtime exit code without opening hardware in tests | regression | integration | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、実装、build、CTest、実機検証は実行していない。

実装後は `make debug CTEST_ARGS="-R daemon_runtime_test"` を実行する。

entrypoint や platform branch に影響する場合は `make windows-cross` を実行する。

## 10. 実機実行条件

fake backend integration test では実機検証は不要である。

production daemon が Bluetooth adapter を開く run は実機作業として扱う。

Switch pairing、HID advertising、periodic report loop、output report handling は実機作業として扱う。

実機作業はユーザの明示承認を必要とする。

実機作業は専用 USB Bluetooth ドングルを使う。

実機作業は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定して実行する。

実機結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、driver、BTstack commit、swbt commit、Switch firmware、report period、結果、cleanup を記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] red を確認した。
- [ ] daemon runtime core を追加した。
- [ ] fake backend integration test を追加した。
- [ ] `make debug` を実行した。
- [ ] sanitizer または cross build の必要性を判断した。
- [ ] 実機状態を記録した。
