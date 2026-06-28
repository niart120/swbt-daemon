# Release Candidate Validation

## 1. 概要

この work unit は、初回 Windows binary release candidate を `main` 取り込み後の codebase で検証する。

完了後は、`just verify`、Windows release package 作成、artifact smoke、license / notice 同梱確認、実機状態が記録され、M7 の tag / GitHub Release 判断へ進める。

## 2. 起点 / ユースケース

source:

- `spec/operations/release-build-and-publish.md` M6: release candidate validation。
- `work-units/complete/local_107/RELEASE_GITHUB_ACTIONS.md` の先送り事項 M6。
- PR #103 merge 後の `main` commit `a210a78b38643cd65f137de84dbedebcf9f1714f`。

use case:

- release operator は、tag push 前に main 上の release candidate が標準検証と package smoke を通ることを確認できる。
- reviewer は、artifact が必要な license / notice file を含むこと、実機未実行の扱い、BTstack を含む binary の license 境界を確認できる。
- release operator は、M7 の tag / GitHub Release publish を実行する前に、残る承認事項を明確にできる。

## 3. 対象範囲

- `just verify` を release candidate 状態で実行する。
- `just package-windows-release` を実行する。
- `scripts/smoke-windows-release.ps1` で package 内 executable を non-hardware smoke する。
- package manifest、SHA256 file、license / notice file の存在を確認する。
- package manifest の dirty 判定が Windows host + Dev Container の CRLF 表示差分だけで true にならないようにする。
- 実機未実行理由を記録する。

## 4. 対象外

- tag 作成、tag push。
- GitHub Release publish。
- 実機検証。
- release notes の最終文面。
- installer、package manager 配布。

## 5. 関連 spec / docs

- `spec/operations/release-build-and-publish.md`
- `spec/operations/release-license-boundary.md`
- `docs/development.md`
- `scripts/package-windows-release.sh`
- `scripts/smoke-windows-release.ps1`
- `work-units/complete/local_107/RELEASE_GITHUB_ACTIONS.md`

## 6. 根拠監査

not applicable。

この work unit は release candidate validation と package manifest の dirty 判定だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値を追加または変更しない。

## 7. 設計メモ

Windows filesystem を Dev Container へ mount した状態では、container 内の `git status --short` が CRLF 表示差分と submodule working tree 表示を modified として扱う場合がある。release package manifest の `worktree_dirty` は、release source に意味のある差分があるかを示す値であり、CRLF だけの表示差分で true にしない。

`scripts/package-windows-release.sh` は、superproject では `--ignore-cr-at-eol` と `--ignore-submodules=dirty` を使い、submodule は個別に safe.directory を追加してから CRLF を無視した diff / cached diff / untracked file を確認する。

## 8. 対象ファイル

- `scripts/package-windows-release.sh`
- `spec/operations/release-build-and-publish.md`
- `work-units/complete/local_108/RELEASE_CANDIDATE_VALIDATION.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | `just verify` が release candidate 状態で通る。 | regression | build/test | no |
| green | `just package-windows-release` が Windows release zip と `.sha256` を作る。 | regression | build/package | no |
| green | package manifest が release commit、version、target、preset、BTstack / toml11 submodule、notice file list を含む。 | regression | package | no |
| green | package manifest の `worktree_dirty` が clean release commit で `false` になる。 | regression | package | no |
| green | Windows package smoke が `swbt-daemon help`、`--backend noop`、`config --backend noop`、`swbt-debug-client` usage を確認する。 | regression | integration | no |
| green | package が `LICENSE`、`THIRD_PARTY_NOTICES.md`、BTstack license、BTstack 3rd-party notice、toml11 license を含む。 | regression | package/license | no |

## 10. 検証

- `just verify`: pass。format-check、clang-tidy、fresh debug build/test、ASan、Windows cross build が通った。
- `just package-windows-release`: pass。`dist/swbt-daemon-v0.1.0-windows-x86_64.zip` と `.sha256` を生成した。
- `Get-Content dist\swbt-daemon-v0.1.0-windows-x86_64\manifest.json`: pass。`version` は `0.1.0`、`target` は `windows-x86_64`、`build_preset` は `windows-mingw-release`、`swbt_commit` は release candidate commit、`includes_btstack` は `true`、`license_notice_files` は必要 notice を含む。
- `Get-Content dist\swbt-daemon-v0.1.0-windows-x86_64.zip.sha256`: pass。zip checksum file が生成されている。
- `scripts/smoke-windows-release.ps1 -PackageDir dist\swbt-daemon-v0.1.0-windows-x86_64`: pass。`swbt-daemon help`、`swbt-daemon --backend noop`、`swbt-daemon config --backend noop`、`swbt-debug-client` 引数なし usage / exit code `2` を確認した。
- `Test-Path vendor\btstack\LICENSE; Test-Path vendor\btstack\3rd-party\README.md; Test-Path vendor\toml11\LICENSE; Test-Path THIRD_PARTY_NOTICES.md`: pass。すべて `True`。
- `git ls-tree HEAD vendor/btstack vendor/toml11`: pass。superproject が参照する BTstack commit `075a0780f0fad7ff67d58ac19f46e8953656a752` と toml11 commit `be08ba2be2a964edcdb3d3e3ea8d100abc26f286` を確認した。

補足:

- `git submodule status -- vendor/btstack vendor/toml11`: not run to completion。Windows Git の `git-sh-setup` 解決で失敗したため、`git ls-tree HEAD ...` と package manifest の submodule status を確認した。
- GitHub Actions release workflow: not run。`v*` tag push が必要であり、tag push は M7 の明示承認まで実行しない。

## 11. 実機実行条件

not run。

この work unit では Bluetooth adapter open、HID advertising、Switch pairing、report loop を実行しない。M6 の実機状態は「未実行」として扱い、release notes で実機検証済みと書く場合は別途 `hardware-harness` の承認範囲で実機検証を実施し、`docs/hardware-test-log.md` に記録する必要がある。

## 12. 先送り事項

- M7: tag 名、annotated tag 作成、tag push、GitHub Actions release workflow 実行、draft GitHub Release の確認と publish。tag push と publish はユーザの明示承認を必要とする。
- 実機検証済み release として出す場合は、M7 前に別途 hardware-harness の承認範囲で実機検証を実行し、artifact hash と実機条件を記録する。

## 13. チェックリスト

- [x] work unit の source と use case が明確である。
- [x] release candidate の標準検証を実行した。
- [x] release package と checksum を検証した。
- [x] package smoke を実行した。
- [x] license / notice 同梱条件を確認した。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] 検証結果または未実行理由を記録した。
