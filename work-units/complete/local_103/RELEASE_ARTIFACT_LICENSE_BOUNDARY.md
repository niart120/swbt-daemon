# Release Artifact License Boundary

## 1. 概要

この work unit は、初回 binary release の artifact 名、内容、third-party notice 同梱条件、BTstack 表記を `spec/operations/release-license-boundary.md` に反映する。

完了後は、後続の README 分割、Release build、package 実装、GitHub Actions 実装が、同じ artifact / license boundary を参照できる状態になる。

## 2. 起点 / ユースケース

source:

- `spec/operations/release-build-and-publish.md` M1: 初回 binary artifact の名前、内容、notice 同梱方法、BTstack 表記を決める。
- `work-units/complete/local_102/RELEASE_READINESS_PLAN.md` の先送り事項 M1。
- `spec/operations/release-license-boundary.md` の未解決事項: 初回 binary release の artifact 名、同梱ファイル、publish 手順が未定。

use case:

- release operator は、初回 Windows binary zip の名前、top-level layout、同梱 license / notice file を確認できる。
- reviewer は、BTstack を含む artifact が MIT-only と表現されていないことを確認できる。
- package 実装 work unit は、artifact layout と manifest / checksum の要件を source として使える。

## 3. 対象範囲

- `spec/operations/release-license-boundary.md` を、初回 Windows binary artifact の policy に対応させる。
- `spec/operations/release-build-and-publish.md` の M4 smoke と未解決事項を M1 の決定に合わせる。
- BTstack / toml11 の notice file が repository 上に存在することを確認する。
- tag push と GitHub Release publish は、この work unit で実行しないことを明記する。

## 4. 対象外

- package artifact 作成。
- CMake / `justfile` の Release build 実装。
- GitHub Actions release workflow 実装。
- tag 作成、tag push、GitHub Release publish。
- 実機検証。
- package manager 配布、installer、commercial BTstack license 判断。

## 5. 関連 spec / docs

- `spec/operations/release-license-boundary.md`
- `spec/operations/release-build-and-publish.md`
- `THIRD_PARTY_NOTICES.md`
- `vendor/btstack/LICENSE`
- `vendor/btstack/3rd-party/README.md`
- `vendor/toml11/LICENSE`

## 6. 根拠監査

not applicable。

この work unit は release artifact の同梱方針と license / notice boundary だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値を追加または変更しない。

## 7. 設計メモ

初回 binary artifact は Windows x86_64 zip を既定候補にする。現時点の実機検証主経路が Windows native + WinUSB であり、Linux libusb 実機経路は未確認だからである。

`swbt-debug-client.exe` は診断用 client として artifact に含める。ただし安定した end-user automation API とは表現しない。non-hardware smoke は現行の引数なし usage 表示を使うか、M4 で `--help` を追加する。

## 8. 対象ファイル

- `spec/operations/release-license-boundary.md`
- `spec/operations/release-build-and-publish.md`
- `work-units/complete/local_103/RELEASE_ARTIFACT_LICENSE_BOUNDARY.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | 初回 Windows binary zip の artifact 名と layout が記録されている。 | new | docs | no |
| green | BTstack と toml11 の notice file 同梱条件が記録されている。 | new | docs | no |
| green | BTstack を含む binary artifact を MIT-only と表現しない release page wording が記録されている。 | regression | docs | no |
| green | tag push / GitHub Release publish がこの work unit では実行対象外である。 | regression | docs | no |
| green | `vendor/btstack/LICENSE`、`vendor/btstack/3rd-party/README.md`、`vendor/toml11/LICENSE` の存在を確認している。 | characterization | docs | no |

## 10. 検証

- `Test-Path vendor\btstack\LICENSE`: `True`
- `Test-Path vendor\btstack\3rd-party\README.md`: `True`
- `Test-Path vendor\toml11\LICENSE`: `True`
- stale boundary wording check: pass。M1 前の artifact 未定 / debug-client help 前提の記述が、対象 spec に残っていないことを `rg` で確認した。
- `rg -n '[ \t]+$' spec\operations\release-license-boundary.md spec\operations\release-build-and-publish.md work-units\complete\local_103\RELEASE_ARTIFACT_LICENSE_BOUNDARY.md`: pass。行末空白なし。`rg` は該当なしのため exit code `1`、stdout なし。
- `git diff --check -- spec\operations\README.md spec\operations\release-build-and-publish.md spec\operations\release-license-boundary.md work-units\complete\local_102\RELEASE_READINESS_PLAN.md work-units\complete\local_103\RELEASE_ARTIFACT_LICENSE_BOUNDARY.md`: pass。whitespace error なし。PowerShell checkout の CRLF 変換警告だけを出した。

## 11. 実機実行条件

not applicable。

この work unit では Bluetooth adapter open、HID advertising、Switch pairing、report loop を実行しない。

## 12. 先送り事項

- M2: 利用者向け README と開発者向け docs の分割で、artifact layout と license wording を反映する。
- M3 / M4: Release build と package 実装で、ここに記録した layout、manifest、checksum を実際の artifact にする。
- M5 / M7: tag-driven workflow と GitHub Release publish の draft / publish 方針を確定する。

## 13. チェックリスト

- [x] work unit の source と use case が明確である。
- [x] artifact 名と layout を記録した。
- [x] BTstack / toml11 notice 同梱条件を記録した。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] 検証結果または未実行理由を記録した。
