# Release GitHub Actions

## 1. 概要

この work unit は、tag-driven release workflow を追加した。workflow は `v*` tag push を入口にして、既存 CI 相当の非実機検証、Windows release package 作成、Windows runner 上の package smoke、draft GitHub Release への artifact 添付まで進む。

この work unit では tag を作成しない。GitHub Release publish も実行しない。

## 2. 起点 / ユースケース

source:

- `spec/operations/release-build-and-publish.md` M5: tag-driven GitHub Actions を追加する。
- `work-units/complete/local_106/RELEASE_PACKAGE_AND_SMOKE.md` の先送り事項 M5。
- `.github/workflows/ci.yml`: 既存 CI は Dev Container 内で `just verify-ci` を実行する。

use case:

- release operator は、承認済み tag push 後に GitHub Actions で release package を作れる。
- reviewer は、GitHub Release に添付される zip と checksum が Windows smoke 後の artifact であることを確認できる。
- release operator は、公開前に draft release を確認し、M6 / M7 の判断を分けられる。

## 3. 対象範囲

- `.github/workflows/release.yml` を追加する。
- tag `v*` push だけを workflow trigger にする。
- Ubuntu runner + Dev Container で `just verify-ci` と `just package-windows-release` を実行する。
- Windows runner で checksum と `scripts/smoke-windows-release.ps1` を実行する。
- draft GitHub Release へ zip と `.sha256` を添付する。
- docs / spec へ workflow の現在位置を記録する。

## 4. 対象外

- tag 作成、tag push。
- GitHub Release publish。
- 実機検証。
- release notes の最終文面。
- package manager 配布。

## 5. 関連 spec / docs

- `spec/operations/release-build-and-publish.md`
- `spec/operations/release-license-boundary.md`
- `docs/development.md`
- `.github/workflows/release.yml`

## 6. 根拠監査

not applicable。

この work unit は GitHub Actions orchestration だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値を追加または変更しない。

## 7. 設計メモ

release workflow は `.github/workflows/ci.yml` と同じ Dev Container image / cache 経路を使う。tag workflow では GHCR へ Dev Container image を push しない。`devcontainers/ci` の `push: filter` は `refs/heads/main` だけに限定する。

Windows smoke は package zip を `actions/download-artifact@v4` で受け取り、checksum を確認してから zip を展開する。展開後の package directory に対して `scripts/smoke-windows-release.ps1` を実行する。

GitHub Release は draft として作る。既存 release が draft の場合は artifact を `--clobber` で差し替える。既存 release が公開済みの場合は上書きを拒否する。

## 8. 対象ファイル

- `.github/workflows/release.yml`
- `docs/development.md`
- `spec/operations/release-build-and-publish.md`
- `work-units/complete/local_107/RELEASE_GITHUB_ACTIONS.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | release workflow は `push.tags: v*` だけを trigger にする。 | new | ci | no |
| green | release workflow は Dev Container で `just verify-ci` と `just package-windows-release` を実行する。 | new | ci | no |
| green | Windows smoke job は checksum を確認し、package を展開して `scripts/smoke-windows-release.ps1` を実行する。 | new | ci | no |
| green | draft release job は smoke 成功後に zip と `.sha256` を draft GitHub Release へ添付する。 | new | ci | no |

## 10. 検証

- `gh release create --help`: pass。`--draft` と `--verify-tag` の意味を確認した。
- `gh release upload --help`: pass。`--clobber` を確認した。
- `gh release view --help`: pass。`--json isDraft` と `--jq` を確認した。
- `rg -n "push:|tags:|just verify-ci|just package-windows-release|smoke-windows-release.ps1|gh release create|gh release upload|--draft|--verify-tag|isDraft" .github\workflows\release.yml`: pass。workflow の主要経路を確認した。
- `rg -n "workflow_dispatch|schedule:|pull_request:|branches:" .github\workflows\release.yml`: pass。対象外 trigger なし。
- PowerShell checksum fragment を生成済み `dist/swbt-daemon-v0.1.0-windows-x86_64.zip` に対して実行: pass。
- `rg -n '[ \t]+$' .github\workflows\release.yml docs\development.md spec\operations\release-build-and-publish.md work-units\complete\local_107\RELEASE_GITHUB_ACTIONS.md`: pass。trailing whitespace なし。
- `git diff --check -- .github\workflows\release.yml docs\development.md spec\operations\release-build-and-publish.md work-units\complete\local_107\RELEASE_GITHUB_ACTIONS.md`: pass。whitespace error なし。

未実行:

- `.github/workflows/release.yml` の GitHub Actions 実行。`v*` tag push が必要であり、tag push は M7 の明示承認まで実行しない。
- `actionlint`。ローカル環境で `actionlint` が見つからなかった。

## 11. 実機実行条件

not applicable。

この work unit では Bluetooth adapter open、HID advertising、Switch pairing、report loop を実行しない。

## 12. 先送り事項

- M6: release candidate validation で `just verify`、package smoke、license / notice check、実機状態を main 上で記録する。
- M7: 明示承認後に annotated tag と GitHub Release publish を実行する。

## 13. チェックリスト

- [x] work unit の source と use case が明確である。
- [x] tag-driven release workflow を追加した。
- [x] package / smoke / draft release の job 順序を記録した。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] 検証結果または未実行理由を記録した。
