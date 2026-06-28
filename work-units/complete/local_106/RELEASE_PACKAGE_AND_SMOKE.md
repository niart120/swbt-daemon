# Release Package And Smoke

## 1. 概要

この work unit は、Windows release build の出力を初回 binary artifact layout にまとめ、zip、SHA256 checksum、manifest を作る経路を追加した。あわせて、package 内 executable を Windows host で non-hardware smoke する script を追加した。

完了後は、`just package-windows-release` で `dist/swbt-daemon-v<version>-windows-x86_64.zip` と `.sha256` を作り、`scripts/smoke-windows-release.ps1` で package directory の内容と non-hardware executable smoke を確認できる。

## 2. 起点 / ユースケース

source:

- `spec/operations/release-build-and-publish.md` M4: package layout、checksum、manifest、artifact smoke を実装する。
- `spec/operations/release-license-boundary.md`: 初回 Windows binary artifact の layout と notice 同梱条件。
- `work-units/complete/local_103/RELEASE_ARTIFACT_LICENSE_BOUNDARY.md` の先送り事項 M3 / M4。
- `work-units/complete/local_105/RELEASE_VERSION_AND_BUILD_PATH.md` の先送り事項 M4。

use case:

- release operator は、Release build 後に Windows zip artifact と checksum を作れる。
- reviewer は、artifact に必要な executable、README、license、third-party notice、manifest が入っていることを確認できる。
- Windows host または Windows runner は、artifact 内 executable で hardware-free smoke を実行できる。

## 3. 対象範囲

- `scripts/package-windows-release.sh` を追加する。
- `scripts/smoke-windows-release.ps1` を追加する。
- `just package-windows-release` を追加する。
- `dist/` を Git ignore に追加する。
- `docs/development.md` に package / smoke entry point を追加する。

## 4. 対象外

- GitHub Actions release workflow 実装。
- GitHub Release upload。
- tag 作成、tag push、GitHub Release publish。
- 実機検証。
- installer、package manager 配布。

## 5. 関連 spec / docs

- `spec/operations/release-build-and-publish.md`
- `spec/operations/release-license-boundary.md`
- `docs/development.md`
- `scripts/package-windows-release.sh`
- `scripts/smoke-windows-release.ps1`
- `justfile`

## 6. 根拠監査

not applicable。

この work unit は package layout、checksum、manifest、non-hardware smoke だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値を追加または変更しない。

## 7. 設計メモ

zip 作成は Dev Container 内の `cmake -E tar --format=zip` を使う。Ubuntu package の `zip` を追加しない。

executable smoke は Windows binary を実行する必要があるため、PowerShell script として分ける。Linux / Dev Container では package layout と checksum までを確認し、Windows host / runner で executable smoke を実行する。

## 8. 対象ファイル

- `.gitignore`
- `justfile`
- `docs/development.md`
- `scripts/package-windows-release.sh`
- `scripts/smoke-windows-release.ps1`
- `work-units/complete/local_106/RELEASE_PACKAGE_AND_SMOKE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | `just package-windows-release` が Windows release build から zip と `.sha256` を作る。 | new | build | no |
| green | package layout が release license boundary の executable、README、license、notice、manifest を含む。 | new | integration | no |
| green | Windows package smoke が `swbt-daemon help`、`--backend noop`、`config --backend noop`、`swbt-debug-client` usage を確認する。 | new | integration | no |
| green | `dist/` が Git 管理対象に入らない。 | regression | docs/build | no |

## 10. 検証

- `just --list`: pass。`package-windows-release` が recipe に含まれる。
- `just package-windows-release`: pass。`dist/swbt-daemon-v0.1.0-windows-x86_64.zip` と `dist/swbt-daemon-v0.1.0-windows-x86_64.zip.sha256` が生成された。
- `Get-Content dist\swbt-daemon-v0.1.0-windows-x86_64\manifest.json`: pass。`version` は `0.1.0`、`target` は `windows-x86_64`、`build_preset` は `windows-mingw-release`、`includes_btstack` は `true`、`license_notice_files` は `LICENSE`、`THIRD_PARTY_NOTICES.md`、BTstack license、BTstack 3rd-party notice、toml11 license を含む。
- `Get-Content dist\swbt-daemon-v0.1.0-windows-x86_64.zip.sha256`: pass。`da0a9869137cb2dc2a596eeb2bef1346a63a853d1f8176ecfe5f45900b94f1c1  swbt-daemon-v0.1.0-windows-x86_64.zip` を確認した。
- `scripts/smoke-windows-release.ps1 -PackageDir dist\swbt-daemon-v0.1.0-windows-x86_64`: pass。`swbt-daemon help`、`swbt-daemon --backend noop`、`swbt-daemon config --backend noop`、`swbt-debug-client` 引数なし usage / exit code `2` を確認した。
- `rg -n '[ \t]+$' .gitignore justfile docs\development.md scripts\package-windows-release.sh scripts\smoke-windows-release.ps1 spec\operations\release-build-and-publish.md work-units\complete\local_106\RELEASE_PACKAGE_AND_SMOKE.md`: pass。trailing whitespace なし。
- `git diff --check -- .gitignore justfile docs\development.md scripts\package-windows-release.sh scripts\smoke-windows-release.ps1 spec\operations\release-build-and-publish.md work-units\complete\local_106\RELEASE_PACKAGE_AND_SMOKE.md`: pass。whitespace error なし。

## 11. 実機実行条件

not applicable。

この work unit では Bluetooth adapter open、HID advertising、Switch pairing、report loop を実行しない。

## 12. 先送り事項

- M5: GitHub Actions release workflow から `just package-windows-release` と `scripts/smoke-windows-release.ps1` を呼ぶ。

## 13. チェックリスト

- [x] work unit の source と use case が明確である。
- [x] package script を追加した。
- [x] Windows package smoke script を追加した。
- [x] package artifact と checksum を検証した。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] 検証結果または未実行理由を記録した。
