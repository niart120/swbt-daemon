---
name: pr-merge-cleanup
description: "swbt-daemon の GitHub Flow ワークフロー。既定ブランチ以外のブランチの push、project template に沿った PR 作成、CI/status 確認、repo またはユーザが承認した方法でのマージ、local 既定ブランチの同期、local/remote 作業ブランチの削除を扱う。Codex が PR、マージ、branch publish、マージ後の後片付け、swbt changes の main 反映を求められたときに使う。"
---

# PR 公開と後片付け

完了した swbt ブランチを GitHub 経由で公開し、local state を片付けるときに、この skill を使う。

## 前提条件

- current branch が既定ブランチではない。
- `git status --short` が clean である。
- 必要な commit がすでに存在する。
- 変更が自明でない場合は work unit のセルフRvが完了している。
- 実機 section と根拠監査 section が事実に沿っている。
- check と repository policy が許す場合に、ユーザがマージを承認している。

## 手順

1. context を確認する。
   - `git branch --show-current`
   - `git remote get-url origin`
   - `origin/HEAD` または repository metadata から既定ブランチを確認する。
2. 既定ブランチ上なら停止する。
3. worktree が dirty なら停止する。
4. PR commit log を作る。
   - `git log --oneline <default>..HEAD`
5. `.github/PULL_REQUEST_TEMPLATE.md` を埋める。
6. ブランチを push する。
7. GitHub app または `gh pr create` で PR を作る。
8. GitHub app または `gh pr checks` で CI/status を確認する。
9. required check が failing または still running なら停止する。
10. repository またはユーザが承認した方法でマージする。
11. 既定ブランチを fetch して同期する。
    - `git fetch --prune origin`
    - `git switch <default>`
    - `git pull --ff-only origin <default>`
12. 既定ブランチが clean で、expected head にいることを確認する。
13. local と remote の作業ブランチを削除する。

## マージ方針

- repository に設定された project policy を優先する。
- project policy がない場合は、merge、squash、rebase のどれを使うか先に確認する。
- ブランチに複数 commit があるだけで squash しない。commit history が work unit の根拠を持つ場合がある。
- merge commit が disabled で、PR body が merge-commit history を前提にしている場合は停止する。

## PR Body の必須項目

PR 本文には次を含める。

- 概要。
- 関連 issue、work unit record、spec、journal entry。
- 挙動、構造、docs、workflow ごとに整理した変更。
- 根拠監査の状態。
- テストコマンドと結果。
- 実機状態または未実行理由。
- BTstack / License impact。
- チェックリスト。

## 停止条件

- worktree が dirty である。
- 既定ブランチ。
- required check failure。
- mergeability が不明。
- 実機承認を必要とする変更に対する承認がない。
- protocol または BTstack facts に対する根拠監査がない。
- PR template section に実質的な欠落がある。
- マージ後に local 既定ブランチを fast-forward できない。

## 最終報告

次を報告する。

```text
PR:
merge method:
merge commit:
synced default branch:
deleted branches:
checks:
hardware:
source audit:
follow-up:
```
