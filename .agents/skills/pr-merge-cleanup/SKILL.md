---
name: pr-merge-cleanup
description: "swbt-daemon の GitHub Flow ワークフロー。default branch 以外の branch の push、project template に沿った PR 作成、CI/status 確認、repo またはユーザが承認した方法での merge、local default branch の同期、local/remote work branch の削除を扱う。Codex が PR、merge、branch publish、merge 後 cleanup、swbt changes の main 反映を求められたときに使う。"
---

# PR merge cleanup（PR 公開と後片付け）

完了した swbt branch を GitHub 経由で公開し、local state を片付けるときに、この skill を使う。

## 前提条件

- current branch が default branch ではない。
- `git status --short` が clean である。
- 必要な commit がすでに存在する。
- 変更が自明でない場合は work-unit self review が完了している。
- Hardware section と source-audit section が事実に沿っている。
- check と repository policy が許す場合に、ユーザが merge を承認している。

## 手順

1. context を確認する。
   - `git branch --show-current`
   - `git remote get-url origin`
   - `origin/HEAD` または repository metadata から default branch を確認する。
2. default branch 上なら停止する。
3. worktree が dirty なら停止する。
4. PR commit log を作る。
   - `git log --oneline <default>..HEAD`
5. `.github/PULL_REQUEST_TEMPLATE.md` を埋める。
6. branch を push する。
7. GitHub app または `gh pr create` で PR を作る。
8. GitHub app または `gh pr checks` で CI/status を確認する。
9. required check が failing または still running なら停止する。
10. repository またはユーザが承認した方法で merge する。
11. default branch を fetch して同期する。
    - `git fetch --prune origin`
    - `git switch <default>`
    - `git pull --ff-only origin <default>`
12. default branch が clean で、expected head にいることを確認する。
13. local と remote の work branch を削除する。

## Merge 方針

- repository に設定された project policy を優先する。
- project policy がない場合は、merge、squash、rebase のどれを使うか先に確認する。
- branch に複数 commit があるだけで squash しない。commit history が work-unit evidence を持つ場合がある。
- merge commit が disabled で、PR body が merge-commit history を前提にしている場合は停止する。

## PR Body の必須項目

PR body には次を含める。

- 概要。
- 関連 issue、spec、journal entry。
- behavior、structure、docs、workflow ごとに整理した変更。
- Source Audit（根拠監査）の状態。
- テスト command と結果。
- 実機状態または not-run reason。
- BTstack / License impact。
- チェックリスト。

## 停止条件

- worktree が dirty である。
- default branch。
- required check failure。
- mergeability が不明。
- hardware-gated change に対する hardware approval がない。
- protocol または BTstack facts に対する source audit がない。
- PR template section に実質的な欠落がある。
- merge 後に local default branch を fast-forward できない。

## 最終報告

次を報告する。

```text
PR:
merge method:
merge commit:
synced default branch:
deleted branches:
gates:
hardware:
source audit:
follow-up:
```
