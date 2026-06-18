# Switch Rumble Core

## 1. 概要

Phase 2: Switch protocol core のうち、`swbt/switch/switch_rumble.*` と raw rumble state unit test を実装する work unit。

Switch output report から分離された 8-byte rumble payload を、controller input state と混ぜずに保持する。

## 2. 対象範囲

- rumble payload size と neutral payload を定義する。
- raw rumble state を初期化する。
- raw rumble payload と update timestamp を state に反映する。
- neutral payload / state 判定を unit test で確認する。
- NULL argument に explicit error を返す。

## 3. 対象外

- Rumble frequency/amplitude semantic decode。
- Actuator-safe amplitude conversion。
- Rumble output scheduling。
- Client IPC event 化。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-hid-initial-source-audit.md`
- `spec/references/switch-rumble-core.md`

## 5. 根拠監査

recorded。

Rumble payload size と neutral payload を実装するため、`source-audit` に従って `spec/references/switch-rumble-core.md` に根拠を記録した。
実機 effect は未検証であり、この work unit では raw state update と neutral 判定までを扱う。

## 6. 設計メモ

- `switch_subcommand.*` が output report から取り出した 8-byte rumble payload を入力にする。
- `swbt_switch_rumble_state_t` は raw bytes、updated flag、updated timestamp だけを持つ。
- rumble の意味解釈は後続 work unit に残す。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/switch/switch_rumble.h`
- `swbt/switch/switch_rumble.c`
- `swbt/switch/switch_subcommand.h`
- `tests/switch_rumble_test.c`
- `spec/references/switch-rumble-core.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/complete/local_007/SWITCH_RUMBLE_CORE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | rumble state initializes to neutral raw payload and `updated=false` | new | unit | no |
| refactor-done | active raw payload updates raw bytes and timestamp | new | unit | no |
| refactor-done | neutral payload and state can be detected | edge | unit | no |
| refactor-done | NULL argument に explicit error を返す | edge | unit | no |
| refactor-done | Phase 2 TODO の `switch_rumble.*` を完了状態に更新する | regression | docs | no |

## 9. 検証

TDD status:

- item: rumble state initializes to neutral raw payload and active raw payload updates raw bytes and timestamp
- state: refactor-done
- commands:
  - red: `make debug CTEST_ARGS="-R switch_rumble_test"` failed as expected because `swbt/switch/switch_rumble.c` did not exist.
  - green: `make debug CTEST_ARGS="-R switch_rumble_test"` passed.
  - refactor: `make verify` passed.
- notes: implementation stores raw bytes only; it does not decode frequency/amplitude.

Verification:

- `make verify`: passed. format-check、clang-tidy、debug、ASan/UBSan、Windows MinGW cross build が完了した。
- `make debug`: passed. `swbt_smoke_test`, `btstack_sources_cmake_test`, `switch_report_test`, `switch_subcommand_test`, `switch_spi_test`, `switch_rumble_test` passed.
- `make asan`: passed. `swbt_smoke_test`, `btstack_sources_cmake_test`, `switch_report_test`, `switch_subcommand_test`, `switch_spi_test`, `switch_rumble_test` passed under ASan/UBSan.
- `make windows-cross`: passed. MinGW debug build completed for `swbt-daemon.exe`, `swbt_smoke_test.exe`, `switch_report_test.exe`, `switch_subcommand_test.exe`, `switch_spi_test.exe`, and `switch_rumble_test.exe`.

## 10. 実機実行条件

実機検証は不要。

この work unit は raw rumble state の unit test のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。
実機 effect は BTstack bridge / hardware work unit で確認する。

## 11. チェックリスト

- [x] 根拠監査を記録した。
- [x] red を確認した。
- [x] `switch_rumble.*` を追加した。
- [x] raw rumble state unit test を追加した。
- [x] `make debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] Phase 2 TODO を更新した。
- [x] 実機状態を記録した。
