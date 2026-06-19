# BTstack Output Report Callbacks

## 1. 概要

`local_013` で追加した output report handler を BTstack Classic HID Device の
DATA / SET_REPORT callback に接続する work unit。

BTstack callback は user context を持たないため、この work unit では単一 daemon instance 前提の active handler trampoline を追加する。

## 2. 起点 / ユースケース

source:

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md` は BTstack output report parser 接続の production callback 登録を後続項目として残していた。
- `work-units/complete/local_013/BTSTACK_OUTPUT_REPORT_PARSER_BRIDGE.md` は DATA callback payload を既存 parser へ渡す handler までを完了し、BTstack への production 登録は対象外にした。
- `work-units/complete/local_018/BTSTACK_PRODUCTION_ADAPTER.md` は HID registration の production adapter を追加したが、output report callback registration は対象外にした。

use case:

- actor: daemon runtime。
- 入力または状態: BTstack Classic HID Device が DATA または SET_REPORT callback を呼ぶ。
- 期待する観測結果: registered trampoline が `swbt_btstack_output_report_handler_handle` へ `hid_cid`、report type、report ID または full payload を渡し、非 output report と parse failure は dispatch しない。
- 制約: Switch pairing、HID advertising、report loop、subcommand reply send queue はこの work unit では扱わない。
- 対象外: 実機 Switch が DATA と SET_REPORT のどちらを使うかの断定。

source から use case への変換:

DATA callback は BTstack が report ID を分離して渡すため、既存 handler へ `report_id` をそのまま渡す。
SET_REPORT callback は separated report ID 引数を持たないため、既存 handler へ `report_id = 0` を渡し、payload を full report として扱う。

## 3. 対象範囲

- `hid_device_register_report_data_callback` の production 登録を追加する。
- `hid_device_register_set_report_callback` の production 登録を追加する。
- BTstack callback から `swbt_btstack_output_report_handler_handle` へ接続する trampoline を追加する。
- DATA callback で受け取る `hid_cid`、`report_type`、`report_id`、payload を既存 handler に渡す。
- SET_REPORT callback で受け取る full payload を `report_id = 0` として既存 handler に渡す。
- callback registration、dispatch、unregister、invalid register を unit test で固定する。

## 4. 対象外

- Switch subcommand reply send queue の実装。
- Periodic input report scheduler の BTstack timer production registration。
- daemon IPC thread と BTstack thread の mailbox 実装。
- Switch pairing、HID advertising、report loop 実機実行。
- 実機 Switch が DATA と SET_REPORT のどちらを使うかの断定。
- unsupported output report の実機 acceptability 判断。
- `vendor/btstack` の変更。

## 5. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/btstack-output-report-parser-bridge.md`
- `spec/references/btstack-output-report-callbacks.md`
- `spec/references/switch-subcommand-core.md`
- `work-units/complete/local_013/BTSTACK_OUTPUT_REPORT_PARSER_BRIDGE.md`
- `work-units/complete/local_018/BTSTACK_PRODUCTION_ADAPTER.md`
- `swbt/btstack_bridge/README.md`

## 6. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| HID report data callback signature | `callback(hid_cid, report_type, report_id, report_size, report)` | source fact | `spec/references/btstack-output-report-callbacks.md` | recorded |
| report data callback registration | `hid_device_register_report_data_callback` | source fact | `spec/references/btstack-output-report-callbacks.md` | recorded |
| SET_REPORT callback signature | `callback(hid_cid, report_type, report_size, report)` | source fact | `spec/references/btstack-output-report-callbacks.md` | recorded |
| SET_REPORT payload shape | separated report ID 引数なしの full payload | source fact | `spec/references/btstack-output-report-callbacks.md` | recorded |
| BTstack DATA report payload shape | report ID may be separated before callback | source fact | `spec/references/btstack-output-report-parser-bridge.md` | recorded |
| 実機 Switch output report transport | 未確定 | 実機根拠なし | `docs/hardware-test-log.md` | hardware-gated |
| callback thread ownership | BTstack-owning thread | design policy | `swbt/btstack_bridge/README.md` | adapter must preserve boundary |

実機 Switch が DATA と SET_REPORT のどちらで output report を送るかは未検証である。

unsupported output report の実機 acceptability は未検証である。

## 7. 設計メモ

- BTstack callback には user context がないため、production registration は単一 active handler を保持する。
- 複数 controller 同時接続は対象外である。global trampoline は単一 daemon instance 前提に限定する。
- callback は BTstack-owning thread で実行される前提とし、IPC socket へ直接 write しない。
- parsed output report は既存 handler の同期 callback へ渡す。send reply path は別 work unit に残す。
- DATA callback と SET_REPORT callback の両方で既存 `swbt_btstack_output_report_handler_t` を再利用し、parser 挙動を重複させない。
- `unregister` は active handler を解除し、BTstack 側へ `NULL` callback を登録する。BTstack は `NULL` を dummy callback に置き換える。

## 8. 対象ファイル

- `swbt/btstack_bridge/output_report_callbacks.h`
- `swbt/btstack_bridge/output_report_callbacks.c`
- `swbt/btstack_bridge/output_report_handler.h`
- `swbt/btstack_bridge/output_report_handler.c`
- `tests/btstack_output_report_callbacks_test.c`
- `CMakeLists.txt`
- `spec/references/btstack-output-report-callbacks.md`
- `work-units/complete/local_019/BTSTACK_OUTPUT_REPORT_CALLBACKS.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | production registration installs BTstack DATA callback trampoline | new | unit | no |
| refactor-done | production registration installs BTstack SET_REPORT callback trampoline | new | unit | no |
| refactor-done | DATA callback dispatches output reports through existing handler | new | unit | no |
| refactor-done | SET_REPORT callback dispatches full report payload through existing handler | characterization | unit | no |
| refactor-done | non-output report type and parse failure are not dispatched | regression | unit | no |
| refactor-done | callback context preserves `hid_cid` and report ID handling | regression | unit | no |

## 10. 検証

TDD red / green:

- red: `just build-debug` は `btstack_bridge/output_report_callbacks.h` missing で compile 失敗した。
- green: `just build-debug` は成功した。
- green: `just test-debug` は 17/17 passed。`btstack_output_report_callbacks_test` を含む。
- refactor: `just format` は成功した。

最終検証:

- `just verify` は成功した。内容は format-check、clang-tidy、linux-debug build/test 17/17、linux-asan build/test 17/17、windows-mingw cross build。
- `vendor/btstack` は変更していない。

Test Desiderata review:

- 価値: BTstack callback registration と DATA / SET_REPORT dispatch の production 境界を unit test で固定できる。
- 独立性: BTstack callback registration symbols は test stub で置き換え、Bluetooth adapter、Switch 実機、BTstack run loop に依存しない。
- 残るリスク: 実機 Switch がどちらの callback path を使うか、実 BTstack run loop 上の callback timing、subcommand reply 送信はこの test では証明しない。

## 11. 実機実行条件

この work unit では実機を実行しない。

この work unit は production callback registration と unit test を主対象にし、Switch pairing、HID advertising、report loop は実行しない。

実機 Switch の DATA / SET_REPORT 経路を確認する場合は、人間の明示承認を必須とする。

実機確認では専用 USB Bluetooth ドングルを使い、普段使いの内蔵 Bluetooth や常用ドングルを使わない。

実機確認では `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を必須条件にする。

実機確認結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を記録する。

## 12. 先送り事項

- subcommand reply send queue への接続。
- 実機 Switch が DATA と SET_REPORT のどちらを使うかの確認。
- unsupported output report の実機 acceptability 確認。
- 実 BTstack run loop 上の callback timing と daemon mailbox 境界の確認。

## 13. チェックリスト

- [x] work unit record を新形式へ更新した。
- [x] 根拠監査を完了した。
- [x] output report callback production registration を実装した。
- [x] SET_REPORT callback の扱いを記録した。
- [x] unit test を追加した。
- [x] 検証コマンドを実行した。
- [x] 実機状態を記録した。
