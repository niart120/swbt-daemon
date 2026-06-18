# BTstack Backend Build Verification

## 1. 概要

Phase 4: BTstack bridge の `libusb build` と `windows-winusb MinGW build` を完了する work unit。

既存の CMake presets / Makefile target / BTstack source selection を使い、Linux `libusb` build/test と Windows `windows-winusb` MinGW cross build が通ることを確認する。

## 2. 対象範囲

- `make debug` で `linux-debug` / `SWBT_BACKEND=libusb` が configure、build、CTest まで通ることを確認する。
- `make windows-cross` で `windows-mingw-debug` / `SWBT_BACKEND=windows-winusb` が configure、build まで通ることを確認する。
- Phase 4 TODO の `libusb build` と `windows-winusb MinGW build` を完了状態にする。
- build matrix の検証結果を reference に記録する。

## 3. 対象外

- BTstack source selection の新規変更。
- `vendor/btstack` の変更。
- Windows native 実行。
- WinUSB driver assignment。
- Bluetooth dongle / Switch pairing / report loop 実機検証。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/references/btstack-source-selection.md`
- `spec/references/btstack-backend-build-matrix.md`
- `CMakePresets.json`
- `Makefile`

## 5. 根拠監査

### 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| Linux backend | `libusb` | implementation fact | `CMakePresets.json` `linux-debug`; `make debug` output | verified |
| Windows backend | `windows-winusb` | implementation fact | `CMakePresets.json` `windows-mingw-debug`; `make windows-cross` output | verified |
| BTstack source selection | backend-specific source/include/link selection | source fact and implementation fact | `spec/references/btstack-source-selection.md`; `tests/cmake/btstack_sources_test.cmake` | recorded |

### 未解決事項

- Windows native 実機検証は未実行であり、Phase 5 の対象である。
- `make windows-cross` は cross build 成功を示すが、Windows 上の runtime behavior は証明しない。

## 6. 設計メモ

- この work unit は code change を伴わない build verification である。
- `make verify` はこの build matrix を含むが、Phase 4 TODO の残項目を明示的に完了するため targeted command を個別に実行した。

## 7. 対象ファイル

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/references/README.md`
- `spec/references/btstack-source-selection.md`
- `spec/references/btstack-backend-build-matrix.md`
- `work-units/complete/local_016/BTSTACK_BACKEND_BUILD_VERIFICATION.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | `make debug` completes Linux `libusb` configure/build/test | verification | build | no |
| refactor-done | `make windows-cross` completes Windows MinGW `windows-winusb` configure/build | verification | build | no |

## 9. 検証

- `make debug` は pass。
  - `SWBT_BACKEND=libusb`。
  - BTstack selected sources: 182。
  - linux-debug CTest 13/13 passed。
- `make windows-cross` は pass。
  - `SWBT_BACKEND=windows-winusb`。
  - BTstack selected sources: 173。
  - `swbt-daemon.exe` linked。
- `make verify` は pass。
  - format-check pass。
  - clang-tidy preset build pass。
  - linux-debug CTest 13/13 passed。
  - linux-asan CTest 13/13 passed。
  - windows-mingw-debug cross build pass。

## 10. 実機実行条件

実機検証は不要。

この work unit は build verification のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 11. チェックリスト

- [x] `make debug` を実行した。
- [x] `make windows-cross` を実行した。
- [x] `make verify` を実行した。
- [x] build matrix の結果を記録した。
- [x] 実機状態を記録した。
- [x] Phase 4 TODO を更新した。
