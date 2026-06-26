# Build Test Targets

## 1. 概要

`just debug` と `just verify` の debug gate が、CTest 実行に必要な unit test executable だけを build できるようにする。

これにより、daemon executable、debug client executable、共有 C ABI library のような test 実行に不要な top-level target を debug gate から外し、失敗時の追跡性を落とさず build 対象を狭める。

## 2. 起点 / ユースケース

source:

- ユーザ要求, 2026-06-27: 外側の gate 並列化は失敗追跡が不便になるため採用せず、代わりに build target 整理を進める。
- `work-units/complete/local_075/BUILD_CONFIGURATION_OPTIMIZATION.md`: CTest 既定 8 並列、ローカル増分 debug、target 指定 build を導入済み。

use case:

- developer が `just debug` を実行したとき、linux-debug の configure 後、CTest に必要な unit test executable だけが build される。
- developer が test executable build だけを明示したいとき、`just build-tests-debug` を実行できる。
- pre-push / CI の debug gate は fresh configure boundary を維持したまま、test target build と CTest を実行する。

source から use case へ変換した判断:

- 外側の gate 並列化は採用しない。独立 gate を同時実行すると失敗時のログ追跡が難しくなるため、この work unit では build target の縮小だけを扱う。

## 3. 対象範囲

- CMake に unit test executable 集約 target を追加する。
- `just build-tests-debug` を追加する。
- `just debug` と `just verify` 内の debug gate が、all target ではなく unit test 集約 target を build するようにする。
- 開発ツール spec に target 分担を記録する。
- README と AGENTS の標準コマンド説明を、debug test target build に合わせて更新する。

## 4. 対象外

- `tidy`、`debug`、`asan`、`windows-cross` の外側並列実行。
- BTstack source selection の削減。
- test executable 定義の helper 化や CMakeLists 全体の広い整理。
- CI workflow の追加変更。

## 5. 関連 spec / docs

- `spec/operations/development-tooling.md`
- `README.md`
- `AGENTS.md`
- `work-units/complete/local_075/BUILD_CONFIGURATION_OPTIMIZATION.md`

## 6. 根拠監査

not applicable。

この work unit は build target と task runner の整理だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は変更しない。

## 7. 設計メモ

- CMake target 名は `swbt_unit_tests` とする。
- `swbt_unit_tests` は executable test target だけに依存する custom target とする。CTest script test は build artifact を持たないため含めない。
- `build-debug` は従来どおり linux-debug の all target build として残す。test 向けの狭い build は `build-tests-debug` に分ける。
- `test-debug` は CTest 実行だけを行う既存の役割を維持する。build を伴う fast loop は `debug` が担う。

## 8. 対象ファイル

- `CMakeLists.txt`
- `justfile`
- `spec/operations/development-tooling.md`
- `README.md`
- `AGENTS.md`
- `work-units/complete/local_076/BUILD_TEST_TARGETS.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | `cmake --build --preset linux-debug --target swbt_unit_tests` builds the CTest executable targets without requiring app-only targets | regression | build | no |
| green | `just build-tests-debug` builds the unit test aggregate target through the standard Dev Container entrypoint | new | build | no |
| green | `just debug` still configures, builds the test target, and runs CTest successfully | regression | integration | no |
| green | `just verify` still keeps the fresh debug boundary and passes the non-hardware gate | regression | integration | no |

## 10. 検証

- `just --list`: pass。`build-tests-debug` recipe が一覧に出ることを確認した。
- `git diff --check`: pass。LF/CRLF warning のみ。
- `Select-String -Path README.md,AGENTS.md,spec/operations/development-tooling.md,work-units/complete/local_076/BUILD_TEST_TARGETS.md -Pattern "debug build/test","build/test only"`: pass。README / AGENTS / tooling spec / work unit record に旧表現が残っていないことを確認した。
- `just build-tests-debug`: pass。Dev Container 経由で linux-debug を configure し、`swbt_unit_tests` target を build した。build は 121 steps で、test executable とその依存 target だけを要求した。
- `just debug`: pass。linux-debug configure、`swbt_unit_tests` target build、CTest 41/41 pass。CTest real time は 1.75 sec。
- `just verify`: pass。`format-check`、`tidy`、fresh debug target build/test、ASan build/test、Windows cross build を確認した。

## 11. 実機実行条件

実機実行は不要。

この work unit は build target と task runner の変更だけであり、Bluetooth adapter open、Switch pairing、HID advertising、report loop を開始しない。

## 12. 先送り事項

none。

## 13. チェックリスト

- [x] CMake aggregate target を追加した。
- [x] `just build-tests-debug` を追加した。
- [x] `just debug` / verify debug gate が test target build を使う。
- [x] spec を更新した。
- [x] README / AGENTS を更新した。
- [x] 検証結果を記録した。
- [x] 実機未実行理由を記録した。
