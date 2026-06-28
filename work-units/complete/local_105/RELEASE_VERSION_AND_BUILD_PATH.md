# Release Version And Build Path

## 1. 概要

この work unit は、初回 Windows release build の入口を追加する。CMake project version を runtime version の正本にし、Debug では `-dev` suffix を維持し、Release preset では suffix なしの version を使う。

完了後は、`windows-mingw-release` preset と `just release-build` / `just build-windows-release` で Windows release executable を build できる状態になる。

## 2. 起点 / ユースケース

source:

- `spec/operations/release-build-and-publish.md` M3: version の正本と Release build preset / `just` recipe を追加する。
- `work-units/complete/local_102/RELEASE_READINESS_PLAN.md` の先送り事項 M3。
- `work-units/complete/local_104/README_USER_DEVELOPMENT_DOC_SPLIT.md` の先送り事項 M3。

use case:

- release operator は、CMake preset と `just` recipe から Windows release executable を作れる。
- reviewer は、CMake project version、runtime daemon version、artifact version の関係を確認できる。
- Debug build の既存 unit test は、runtime version `0.1.0-dev` を維持する。

## 3. 対象範囲

- `swbt_get_version_string()` を compile definition から返せるようにする。
- CMake cache に `SWBT_VERSION_SUFFIX` を追加し、`SWBT_VERSION_STRING` は `PROJECT_VERSION` と suffix から configure 時に組み立てる。
- `windows-mingw-release` configure / build preset を追加する。
- `just` に Windows release build recipe を追加する。

## 4. 対象外

- package artifact 作成。
- checksum / manifest 作成。
- GitHub Actions release workflow 実装。
- tag 作成、tag push、GitHub Release publish。
- 実機検証。

## 5. 関連 spec / docs

- `spec/operations/release-build-and-publish.md`
- `spec/operations/release-license-boundary.md`
- `docs/development.md`
- `CMakeLists.txt`
- `CMakePresets.json`
- `justfile`
- `swbt/support/swbt_version.c`

## 6. 根拠監査

not applicable。

この work unit は build configuration と runtime version string だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値を追加または変更しない。

## 7. 設計メモ

`project(swbt_daemon VERSION 0.1.0)` を version の正本にする。Debug 系 preset は既定の `SWBT_VERSION_SUFFIX=-dev` を使い、Release preset は `SWBT_VERSION_SUFFIX=""` を設定する。

初回 release artifact は Windows x86_64 zip だけを既定候補にしているため、この work unit では `windows-mingw-release` だけを追加する。

## 8. 対象ファイル

- `CMakeLists.txt`
- `CMakePresets.json`
- `justfile`
- `swbt/support/swbt_version.c`
- `docs/development.md`
- `work-units/complete/local_105/RELEASE_VERSION_AND_BUILD_PATH.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | Debug build の runtime version は既存どおり `0.1.0-dev` を返す。 | regression | unit | no |
| green | Release preset は `SWBT_VERSION_SUFFIX` を空にし、runtime version と artifact version を一致させる。 | new | build | no |
| green | `windows-mingw-release` preset が CMake から読める。 | new | build | no |
| green | `just release-build` と `just build-windows-release` が recipe 一覧に出る。 | new | build | no |

## 10. 検証

- `cmake --list-presets`: pass。`windows-mingw-release` が表示された。
- `just --list`: pass。`configure-windows-release`、`build-windows-release`、`release-build` が表示された。
- `just build-windows-release`: pass。Dev Container 内で `windows-mingw-release` を configure し、`swbt-daemon.exe` と `swbt-debug-client.exe` を build した。BTstack third-party warning は出たが build は成功した。
- `just debug`: pass。59 tests passed。Debug runtime version `0.1.0-dev` の既存 test も通った。
- `just format-check`: pass。
- `rg -n '[ \t]+$' CMakeLists.txt CMakePresets.json justfile swbt\support\swbt_version.c docs\development.md work-units\complete\local_105\RELEASE_VERSION_AND_BUILD_PATH.md`: pass。行末空白なし。`rg` は該当なしのため exit code `1`、stdout なし。
- `git diff --check -- CMakeLists.txt CMakePresets.json justfile swbt\support\swbt_version.c docs\development.md work-units\complete\local_105\RELEASE_VERSION_AND_BUILD_PATH.md`: pass。whitespace error なし。PowerShell checkout の CRLF 変換警告だけを出した。

## 11. 実機実行条件

not applicable。

この work unit では Bluetooth adapter open、HID advertising、Switch pairing、report loop を実行しない。

## 12. 先送り事項

- M4: release package、manifest、checksum、artifact smoke を実装する。
- M5: GitHub Actions release workflow から `just release-build` を呼ぶ。

## 13. チェックリスト

- [x] work unit の source と use case が明確である。
- [x] version の正本を CMake project version に寄せた。
- [x] Windows release preset を追加した。
- [x] `just` の release build recipe を追加した。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] 検証結果または未実行理由を記録した。
