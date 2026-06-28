# Release Workflow Smoke Exit Code

## 1. 概要

この work unit は、`v0.1.0` tag push 後の release workflow で Windows package smoke が成功表示後に失敗扱いになった問題を修正する。

原因は、`scripts/smoke-windows-release.ps1` が `swbt-debug-client` の usage 表示を exit code `2` として期待どおり受け入れた後、PowerShell の `$LASTEXITCODE` を戻していなかったことである。GitHub Actions の `pwsh` step は最後の native command exit code を step 終了コードとして扱うため、`release package smoke passed` を表示した後に step が失敗した。

## 2. 起点 / ユースケース

source:

- `v0.1.0` tag push による release workflow run `28329586952`。
- `smoke-windows` job `83925522573` が `release package smoke passed` 表示後に exit code `1` で失敗した。
- `work-units/complete/local_107/RELEASE_GITHUB_ACTIONS.md` の M7: tag-driven workflow を実行する。

use case:

- release operator は、`swbt-debug-client` の引数なし usage / exit code `2` を期待値として検証しつつ、smoke script 全体は成功として終了できる。
- GitHub Actions の `smoke-windows` job は、checksum 検証と package smoke が成功したときに後続の `draft-release` job へ進む。

## 3. 対象範囲

- `scripts/smoke-windows-release.ps1` の expected exit code 処理を修正する。
- `v0.1.0` release workflow failure の原因と検証を記録する。

## 4. 対象外

- release package layout の変更。
- GitHub Actions workflow 構造の変更。
- tag 作成、tag push、GitHub Release publish。
- 実機検証。

## 5. 関連 spec / docs

- `.github/workflows/release.yml`
- `scripts/smoke-windows-release.ps1`
- `work-units/complete/local_107/RELEASE_GITHUB_ACTIONS.md`
- `work-units/complete/local_108/RELEASE_CANDIDATE_VALIDATION.md`

## 6. 根拠監査

not applicable。

この work unit は PowerShell smoke script の終了コード処理だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値を追加または変更しない。

## 7. 設計メモ

`Invoke-ExpectExit` は、呼び出した executable の exit code が期待値と一致すれば成功扱いにする。その後に `$global:LASTEXITCODE = 0` を設定し、期待済みの非ゼロ終了コードが script 全体の失敗として残らないようにする。

`exit 0` で script 全体を終了させる方法は避ける。ローカルで script を呼ぶ場合に PowerShell session 全体の終了へ寄りすぎるため、期待値検証の責務を持つ helper 内で `$LASTEXITCODE` を正規化する。

## 8. 対象ファイル

- `scripts/smoke-windows-release.ps1`
- `spec/operations/release-build-and-publish.md`
- `work-units/complete/local_110/RELEASE_WORKFLOW_SMOKE_EXIT_CODE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | package smoke は `swbt-debug-client` 引数なし usage / exit code `2` を期待値として扱い、script 全体を exit code `0` で終了する。 | regression | integration | no |
| green | `scripts/smoke-windows-release.ps1` は package 内の required files を確認し、`swbt-daemon` noop 起動と config 表示を検証する。 | regression | integration | no |

## 10. 検証

- `scripts/smoke-windows-release.ps1 -PackageDir dist\swbt-daemon-v0.1.0-windows-x86_64`: pass。`swbt-daemon help`、`swbt-daemon --backend noop`、`swbt-daemon config --backend noop`、`swbt-debug-client` usage / exit code `2` を確認し、script 全体は exit code `0` で終了した。
- `git diff --check -- scripts\smoke-windows-release.ps1 spec\operations\release-build-and-publish.md work-units\complete\local_110\RELEASE_WORKFLOW_SMOKE_EXIT_CODE.md`: pass。whitespace error なし。

## 11. 実機実行条件

not applicable。

この work unit では Bluetooth adapter open、HID advertising、Switch pairing、report loop を実行しない。

## 12. 先送り事項

- `v0.1.0` tag は既に remote に存在する。修正を main に取り込んだ後、同じ tag で release workflow を再実行するには tag の付け直しが必要である。remote tag の削除または更新は、ユーザの明示承認後に実施する。

## 13. チェックリスト

- [x] work unit の source と use case が明確である。
- [x] release workflow failure の原因を記録した。
- [x] smoke script の終了コード処理を修正した。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] 検証結果または未実行理由を記録した。
