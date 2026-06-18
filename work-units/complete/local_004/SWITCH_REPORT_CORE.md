# Switch Report Core

## 1. 概要

Phase 2: Switch protocol core のうち、`swbt/switch/switch_report.*` と golden packet test を実装する work unit。

`swbt_state_t` の最新状態スナップショットから、Nintendo Switch Pro Controller 相当の standard full input report `0x30` を生成する。

## 2. 対象範囲

- input report `0x30` の基本 layout を定義する。
- Pro Controller 相当の button bit と standard input report 3 bytes の対応を定義する。
- 12-bit stick raw value を standard input report の 3 bytes へ pack する。
- `swbt_state_t` の accel / gyro 値を 6-Axis payload 3 frames に little-endian で pack する。
- golden packet unit test を追加する。

## 3. 対象外

- HID descriptor。
- Output report parser。
- Subcommand reply builder。
- SPI flash payload。
- Rumble packet parser。
- Calibration data の取得と補正。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-hid-initial-source-audit.md`
- `spec/references/switch-report-core.md`

## 5. 根拠監査

recorded。

Switch HID report bytes と packing を実装するため、`source-audit` に従って `spec/references/switch-report-core.md` に根拠を記録した。
実機 acceptability は未検証であり、この work unit では source fact と inference に基づく packet builder までを扱う。

## 6. 設計メモ

- `swbt_switch_build_standard_full_report()` は HIDP prefix を含めず、report ID を byte 0 に置く。
- `battery_connection` と `vibrator_report` は builder option として caller が渡す。
- `swbt_state_t` の stick 値は 0..4095 の raw 12-bit 値として扱う。
- calibration は SPI work unit または report scheduler integration で扱う。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/switch/switch_controller_state.h`
- `swbt/switch/switch_report.h`
- `swbt/switch/switch_report.c`
- `tests/switch_report_test.c`
- `spec/references/switch-report-core.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/complete/local_004/SWITCH_REPORT_CORE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | `swbt_state_t` から `0x30` standard full report の report ID、timer、status、button bytes、stick bytes、6-Axis payload を生成する | new | unit | no |
| refactor-done | report buffer が小さい場合に explicit error を返す | edge | unit | no |
| refactor-done | NULL argument に explicit error を返す | edge | unit | no |
| refactor-done | Phase 2 TODO の `switch_report.*` と golden packet test を完了状態に更新する | regression | docs | no |

## 9. 検証

TDD status:

- item: `swbt_state_t` から `0x30` standard full report の report ID、timer、status、button bytes、stick bytes、6-Axis payload を生成する
- state: refactor-done
- commands:
  - red: `make debug CTEST_ARGS="-R switch_report_test"` failed as expected because `switch/switch_report.h` did not exist.
  - green: `make debug CTEST_ARGS="-R switch_report_test"` passed.
  - refactor: `make debug` passed.
- notes: report ID included at byte 0; HIDP prefix is not included.

Verification:

- `make verify`: passed. format-check、clang-tidy、debug、ASan/UBSan、Windows MinGW cross build が完了した。
- `make debug`: passed. `swbt_smoke_test`, `btstack_sources_cmake_test`, `switch_report_test` passed.
- `make asan`: passed. `swbt_smoke_test`, `btstack_sources_cmake_test`, `switch_report_test` passed under ASan/UBSan.
- `make windows-cross`: passed. MinGW debug build completed for `swbt-daemon.exe`, `swbt_smoke_test.exe`, and `switch_report_test.exe`.

## 10. 実機実行条件

実機検証は不要。

この work unit は packet builder の unit test のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。
実機 acceptability は BTstack bridge / hardware work unit で確認する。

## 11. チェックリスト

- [x] 根拠監査を記録した。
- [x] red を確認した。
- [x] `switch_report.*` を追加した。
- [x] golden packet unit test を追加した。
- [x] `make debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] Phase 2 TODO を更新した。
- [x] 実機状態を記録した。
