# BTstack Source Selection

## 1. 概要

Phase 1: BTstack source selection を完了する work unit。

`vendor/btstack` の固定済み `v1.8.2` source を直接編集せず、swbt 側の CMake で Linux `libusb` と Windows `windows-winusb` backend の source selection を明示する。

## 2. 対象範囲

- `vendor/btstack/port/libusb` と `vendor/btstack/port/windows-winusb` の upstream CMake / docs を調査する。
- `vendor/btstack/src`、`vendor/btstack/src/classic`、platform source の取り込み単位を CMake に定義する。
- `cmake/btstack_sources.cmake` を追加する。
- `vendor/btstack` を直接変更しないことを確認する。
- CTest で backend ごとの必須 source と include / link 境界を確認する。

## 3. 対象外

- BTstack HID Device bridge の実装。
- Switch HID descriptor、input report、subcommand response の実装。
- 実機 pairing、advertising、report loop。
- BTstack submodule の更新または fork。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DEVELOPMENT_PLAN.md`
- `spec/references/switch-hid-initial-source-audit.md`
- `spec/references/btstack-source-selection.md`
- `docs/upstream-btstack.md`
- `THIRD_PARTY_NOTICES.md`

## 5. 根拠監査

pass。

BTstack source selection は `spec/references/btstack-source-selection.md` に記録した。
upstream `vendor/btstack` source の CMakeLists と README を根拠に、swbt 側の CMake source family、backend transport、include / link 境界、port `main.c` 除外を分けて記録している。

## 6. 設計メモ

- `port/*/main.c` は upstream example entry point であり、swbt-daemon の `main` と衝突するため source selection から外す。
- Phase 1 では source selection を CMake module と CTest で固定する。
- BTstack bridge から実際に API を呼ぶ work unit で、必要に応じて static library 化と link を進める。

## 7. 対象ファイル

- `CMakeLists.txt`
- `cmake/btstack_sources.cmake`
- `tests/cmake/btstack_sources_test.cmake`
- `spec/references/btstack-source-selection.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/complete/local_003/BTSTACK_SOURCE_SELECTION.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | `libusb` backend selection が HCI、L2CAP、Classic HID、libusb transport、POSIX run loop を含み Windows transport と port main を除外する | new | cmake/ctest | no |
| refactor-done | `windows-winusb` backend selection が HCI、L2CAP、Classic HID、WinUSB transport、Windows run loop を含み libusb transport と port main を除外する | new | cmake/ctest | no |
| green | source selection の根拠監査を `spec/references/` と work unit record に記録する | new | docs | no |
| green | Phase 1 TODO を完了状態に更新する | regression | docs | no |

## 9. 検証

- `make debug CTEST_ARGS="-R btstack_sources_cmake_test"`: red。`cmake/btstack_sources.cmake` 未追加のため CTest が fail。
- `make debug CTEST_ARGS="-R btstack_sources_cmake_test"`: pass。`btstack_sources_cmake_test` が 1/1 passed。
- `make format-check`: pass。
- `make debug`: pass。`swbt_smoke_test` と `btstack_sources_cmake_test` が 2/2 passed。
- `make windows-cross`: pass。`windows-mingw-debug` configure で `BTstack backend: windows-winusb`、`BTstack selected sources: 173` を確認。
- `make verify`: pass。format check、clang-tidy build、debug build/test、ASan/UBSan build/test、Windows MinGW cross build が通過。

TDD status:
- item: backend ごとの BTstack source selection を CTest で確認する。
- state: refactor-done。
- commands:
  - `make debug CTEST_ARGS="-R btstack_sources_cmake_test"`
  - `make verify`
- notes: Linux `libusb` configure で `BTstack selected sources: 182` を確認。CTest script は `libusb` と `windows-winusb` の両方を検査する。

## 10. 実機実行条件

実機検証は不要。

この work unit は CMake source selection と docs のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 11. チェックリスト

- [x] upstream source selection の根拠を記録した。
- [x] `cmake/btstack_sources.cmake` を追加した。
- [x] `libusb` backend の必須 source selection を CTest で確認した。
- [x] `windows-winusb` backend の必須 source selection を CTest で確認した。
- [x] `vendor/btstack` を直接変更していない。
- [x] 検証結果または未実行理由を記録した。
- [x] 実機状態を記録した。
