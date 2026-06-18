# Switch Subcommand Core

## 1. 概要

Phase 2: Switch protocol core のうち、`swbt/switch/switch_subcommand.*` と output report parser unit test を実装する work unit。

Switch から daemon が受け取る HID output report を、BTstack bridge や reply builder に渡せる構造へ安全に parse する。

## 2. 対象範囲

- output report `0x01` rumble + subcommand を parse する。
- output report `0x10` rumble only を parse する。
- packet counter、8 bytes rumble、subcommand ID、subcommand data span を取り出す。
- 初期 protocol core が扱う subcommand ID の classifier を追加する。
- 空 packet、短い packet、未対応 report ID、未知 subcommand classifier の unit test を追加する。

## 3. 対象外

- `0x21` subcommand reply builder。
- SPI flash read の address / size validation と payload 生成。
- Rumble packet の semantic decode。
- NFC/IR MCU output report `0x11` の処理。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-hid-initial-source-audit.md`
- `spec/references/switch-subcommand-core.md`

## 5. 根拠監査

recorded。

Switch output report bytes と subcommand ID を実装するため、`source-audit` に従って `spec/references/switch-subcommand-core.md` に根拠を記録した。
実機 acceptability は未検証であり、この work unit では source fact に基づく parser と classifier までを扱う。

## 6. 設計メモ

- parser は HIDP prefix を含まない packet を入力として扱う。
- subcommand data は caller の input buffer 内の span として返す。parser は ownership を持たない。
- `0x10` rumble only は subcommand を持たない output report として扱う。
- NFC/IR MCU report `0x11` はこの work unit では unsupported report id として返す。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/switch/switch_subcommand.h`
- `swbt/switch/switch_subcommand.c`
- `tests/switch_subcommand_test.c`
- `spec/references/switch-subcommand-core.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/complete/local_005/SWITCH_SUBCOMMAND_CORE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | `0x01` output report から packet counter、rumble bytes、subcommand ID、data span を取り出す | new | unit | no |
| refactor-done | `0x10` rumble only report を subcommand なしとして parse する | new | unit | no |
| refactor-done | 空 packet、短い packet、NULL argument に explicit error を返す | edge | unit | no |
| refactor-done | 未対応 report ID に explicit error を返す | edge | unit | no |
| refactor-done | 未知 subcommand と既知 subcommand を classifier で区別する | edge | unit | no |
| refactor-done | Phase 2 TODO の `switch_subcommand.*` を完了状態に更新する | regression | docs | no |

## 9. 検証

TDD status:

- item: `0x01` output report から packet counter、rumble bytes、subcommand ID、data span を取り出す
- state: refactor-done
- commands:
  - red: `make debug CTEST_ARGS="-R switch_subcommand_test"` failed as expected because `switch/switch_subcommand.h` did not exist.
  - green: `make debug CTEST_ARGS="-R switch_subcommand_test"` passed.
  - refactor: `make verify` passed.
- notes: parser input excludes HIDP prefix; `subcommand_data` is a span into caller-owned input buffer.

Verification:

- `make verify`: passed. format-check、clang-tidy、debug、ASan/UBSan、Windows MinGW cross build が完了した。
- `make debug`: passed. `swbt_smoke_test`, `btstack_sources_cmake_test`, `switch_report_test`, `switch_subcommand_test` passed.
- `make asan`: passed. `swbt_smoke_test`, `btstack_sources_cmake_test`, `switch_report_test`, `switch_subcommand_test` passed under ASan/UBSan.
- `make windows-cross`: passed. MinGW debug build completed for `swbt-daemon.exe`, `swbt_smoke_test.exe`, `switch_report_test.exe`, and `switch_subcommand_test.exe`.

## 10. 実機実行条件

実機検証は不要。

この work unit は output report parser の unit test のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。
実機での subcommand sequence と acceptability は BTstack bridge / hardware work unit で確認する。

## 11. チェックリスト

- [x] 根拠監査を記録した。
- [x] red を確認した。
- [x] `switch_subcommand.*` を追加した。
- [x] output report parser unit test を追加した。
- [x] `make debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] Phase 2 TODO を更新した。
- [x] 実機状態を記録した。
