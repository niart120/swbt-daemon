# BTstack Output Report Parser Bridge

## 1. 概要

Phase 4: BTstack bridge の Output Report parser 接続を追加する work unit。

BTstack Classic HID report data callback の `report_type` / `report_id` / payload を、既存の `swbt_switch_parse_output_report` が期待する report ID 先頭の byte sequence に変換して dispatch する。

## 2. 対象範囲

- BTstack HID report type の project-local mirror constant を追加する。
- Output report handler を追加し、BTstack callback payload を Switch output report parser へ接続する。
- BTstack が report ID を分離して渡した場合に report ID を payload 先頭へ再結合する。
- Output report type 以外を無視する。
- parser failure を dispatch せず error として返す。
- Phase 4 TODO の Output Report parser 接続を完了状態にする。

## 3. 対象外

- `hid_device_register_report_data_callback` への production adapter 登録。
- `hid_device_register_set_report_callback` の接続。
- Subcommand `0x21` reply。
- Periodic `0x30` input report。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/references/btstack-output-report-parser-bridge.md`
- `spec/references/switch-subcommand-core.md`
- `work-units/complete/local_012/BTSTACK_HID_DEVICE_REGISTRATION.md`

## 5. 根拠監査

### 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| BTstack HID output report type | `2` | source fact | `vendor/btstack/src/btstack_hid.h:100-105` | stable BTstack API at pinned commit |
| BTstack report data callback shape | `cid`, `report_type`, `report_id`, `report_size`, `report` | source fact | `vendor/btstack/src/classic/hid_device.h:112-116` | stable BTstack API at pinned commit |
| BTstack DATA report payload shape | report ID may be separated before callback | source fact | `vendor/btstack/src/classic/hid_device.c:620-639` | bridge must rejoin report ID |
| swbt output report parser shape | report ID is byte 0 | implementation fact | `swbt/switch/switch_subcommand.c`; `spec/references/switch-subcommand-core.md` | existing implementation contract |

### 未解決事項

- 実機 Switch の exact transport path は未検証である。
- SET_REPORT callback 接続は後続判断に残す。

## 6. 設計メモ

- handler callback は parsed report を同期的に受け取る。`subcommand_data` pointer は handler 内部 scratch または caller payload を指すため、callback は必要な data をその場で消費する。
- report ID が `0` の場合、payload は既存 parser が期待する full report として扱う。
- report ID が `1..255` の場合、handler 内部 scratch に report ID と payload を再結合する。

## 7. 対象ファイル

- `swbt/btstack_bridge/output_report_handler.h`
- `swbt/btstack_bridge/output_report_handler.c`
- `tests/btstack_output_report_handler_test.c`
- `spec/references/btstack-output-report-parser-bridge.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/complete/local_013/BTSTACK_OUTPUT_REPORT_PARSER_BRIDGE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | report ID separated by BTstack is rejoined before Switch parser dispatch | new | unit | no |
| refactor-done | report ID 0 uses payload as full output report | new | unit | no |
| refactor-done | non-output report type is ignored without dispatch | edge | unit | no |
| refactor-done | parser failure returns error without dispatch | edge | unit | no |
| refactor-done | invalid arguments and oversized reconstruction are rejected | edge | unit | no |

## 9. 検証

- red: `make debug CTEST_ARGS="-R btstack_output_report_handler_test"` は missing `btstack_bridge/output_report_handler.h` のため compile で失敗した。
- green: `make debug CTEST_ARGS="-R btstack_output_report_handler_test"` は 1/1 passed。
- standard verification: `make verify` は pass。
  - format-check pass。
  - clang-tidy preset build pass。
  - linux-debug CTest 11/11 passed。
  - linux-asan CTest 11/11 passed。
  - windows-mingw-debug cross build pass。

## 10. 実機実行条件

実機検証は不要。

この work unit は BTstack callback payload shape と既存 parser の adapter unit test のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 11. チェックリスト

- [x] red を確認した。
- [x] Output Report parser bridge を追加した。
- [x] unit test を追加した。
- [x] `make debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] 実機状態を記録した。
