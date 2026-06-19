# BTstack Output Report Callbacks

## 1. 概要

local_013 で追加した output report handler を BTstack Classic HID Device callback に接続するための計画 record。

DATA callback と SET_REPORT callback の扱いは、実装前の根拠監査と実機未検証状態を分けて記録する。

## 2. 対象範囲

- `hid_device_register_report_data_callback` の production 登録を追加する。
- BTstack callback から `swbt_btstack_output_report_handler_handle` へ接続する trampoline を追加する。
- callback で受け取る `cid`、`report_type`、`report_id`、payload を既存 handler に渡す。
- SET_REPORT callback を登録するか、後続に残すか、同じ handler に流すかを実装前に根拠監査で判断する。
- callback registration と dispatch 挙動を unit test で固定する。

## 3. 対象外

- Switch subcommand reply send queue の実装。
- Periodic input report scheduler の BTstack timer production registration。
- daemon IPC thread と BTstack thread の mailbox 実装。
- Switch pairing、HID advertising、report loop 実機実行。
- 実機 Switch が DATA と SET_REPORT のどちらを使うかの断定。
- `vendor/btstack` の変更。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/btstack-output-report-parser-bridge.md`
- `spec/references/switch-subcommand-core.md`
- `work-units/complete/local_013/BTSTACK_OUTPUT_REPORT_PARSER_BRIDGE.md`
- `work-units/complete/local_018/BTSTACK_PRODUCTION_ADAPTER.md`
- `swbt/btstack_bridge/README.md`

## 5. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| HID report data callback signature | `callback(cid, report_type, report_id, report_size, report)` | source fact | `spec/references/btstack-output-report-parser-bridge.md` | recorded |
| report data callback registration | `hid_device_register_report_data_callback` | source fact | `spec/references/btstack-output-report-parser-bridge.md` | recorded |
| BTstack DATA report payload shape | report ID may be separated before callback | source fact | `spec/references/btstack-output-report-parser-bridge.md` | recorded |
| SET_REPORT callback API and call path | 未定 | 未監査 | TBD | 実装前に根拠監査が必要 |
| 実機 Switch output report transport | 未定 | 実機根拠なし | `docs/hardware-test-log.md` | 実機未実行 |
| callback thread ownership | BTstack-owning thread | design policy | `swbt/btstack_bridge/README.md` | adapter must preserve boundary |

SET_REPORT callback の必要性と exact API は実装前に `source-audit` で確認する。

実機 Switch が DATA と SET_REPORT のどちらで output report を送るかは未検証である。

unsupported output report の実機 acceptability は未検証である。

## 6. 設計メモ

- BTstack callback には user context がない可能性があるため、production registration 側で単一 daemon instance の dispatch context を明示的に管理する。
- 複数 controller 同時接続は対象外であり、global trampoline を使う場合も単一接続前提を record に残す。
- callback は BTstack-owning thread で実行される前提とし、IPC socket へ直接 write しない。
- parsed output report は同期 callback または後続 mailbox 境界へ渡し、send reply path は別 work unit に残す。
- DATA callback と SET_REPORT callback の両方を扱う場合も、既存 `swbt_btstack_output_report_handler_t` を再利用して parser 挙動を重複させない。

## 7. 対象ファイル

- `swbt/btstack_bridge/output_report_callbacks.h`
- `swbt/btstack_bridge/output_report_callbacks.c`
- `swbt/btstack_bridge/output_report_handler.h`
- `swbt/btstack_bridge/output_report_handler.c`
- `tests/btstack_output_report_callbacks_test.c`
- `CMakeLists.txt`
- `spec/references/btstack-output-report-callbacks.md`
- `work-units/wip/local_019/BTSTACK_OUTPUT_REPORT_CALLBACKS.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | production registration installs BTstack report data callback trampoline | new | unit | no |
| todo | DATA callback dispatches output reports through existing handler | new | unit | no |
| todo | non-output report type and parse failure are not dispatched | regression | unit | no |
| todo | SET_REPORT audit outcome is represented by explicit behavior or deferred state | characterization | unit | no |
| todo | callback context preserves `hid_cid` and report ID handling | regression | unit | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、実装、build、CTest、実機検証は実行していない。

実装後は `make debug CTEST_ARGS="-R btstack_output_report_callbacks_test"` を実行する。

Windows callback registration の compile/link 境界が変わる場合は `make windows-cross` を実行する。

変更範囲に応じて `make asan` または `make verify` を実行する。

## 10. 実機実行条件

この work unit record 作成時点では実機を実行しない。

この work unit は production callback registration と unit test を主対象にし、Switch pairing、HID advertising、report loop は実行しない。

実機 Switch の DATA / SET_REPORT 経路を確認する場合は、人間の明示承認を必須とする。

実機確認では専用 USB Bluetooth ドングルを使い、普段使いの内蔵 Bluetooth や常用ドングルを使わない。

実機確認では `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を必須条件にする。

実機確認結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] 根拠監査を完了した。
- [ ] output report callback production registration を実装した。
- [ ] SET_REPORT callback の扱いを記録した。
- [ ] unit test を追加した。
- [ ] 検証コマンドを実行した。
- [ ] 実機状態を記録した。
