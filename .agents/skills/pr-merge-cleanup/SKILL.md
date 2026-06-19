---
name: pr-merge-cleanup
description: "swbt-daemon の作業ブランチを GitHub PR 経由で merge commit として既定ブランチへ取り込み、PR template、CI/status、work unit / 根拠監査 / 実機 / BTstack license gate、既定ブランチ同期、ブランチ後片付けまで扱う。Codex が PR 作成、マージ、branch publish、PR 後片付け、swbt changes の main 反映を求められたときに使う。"
---

# PR Merge Cleanup

作業ブランチの変更を GitHub PR 経由で既定ブランチに取り込み、local 同期と不要ブランチ削除まで行う。
swbt-daemon の project policy は merge commit 固定である。
remote head branch は merge 時の GitHub / `gh` option で削除し、後片付けでは local branch の削除と remote 削除済み確認だけを行う。

## 前提条件

- GitHub remote が設定済みである。
- current branch が既定ブランチではない。
- 作業ブランチで必要な commit が完了している。
- `git status --short` が clean である。
- 変更が自明でない場合は work unit のセルフRvが完了している。
- 実機 section と根拠監査 section が事実に沿っている。
- PR 前の標準検証は `just` recipe または CI 結果で確認する。Codex は自分の判断で `SWBT_ALLOW_HOST_BUILD=1` を付けない。
- check と project policy が許す場合に、ユーザがマージを承認している。

## マージ方針

| 条件 | 方法 |
|---|---|
| 既定 | merge commit を作る。`gh` を使う場合は `gh pr merge --merge --delete-branch` を使う。 |
| GitHub app を使う | merge method は `merge` にし、remote head branch の自動削除を有効にする。 |
| repository が merge commit を禁止 | 停止し、許可されている merge method と影響を報告する。 |
| ユーザが squash / rebase を求めた | project policy から外れるため、理由と影響を確認してから進める。 |

merge commit 固定の理由:

- 個別 commit が work unit の根拠や判断を持つ場合がある。
- PR body の Commit Log と実際の履歴を一致させやすい。
- root `AGENTS.md` の Conventional Commits 運用と相性がよい。

## Skill Coordination

PR 作成前に、変更内容に応じて次の結果を PR 本文へ反映する。

| 条件 | 使う skill | PR へ残すもの |
|---|---|---|
| work unit がある | `work-unit-record` | work unit record path、scope、non-goals、TDD Test List、検証結果。 |
| 大きな変更または完了宣言を伴う | `agentic-self-review` | 要件、non-goals、根拠監査、test/build、実機、BTstack/license、remaining risk。 |
| spec を追加または更新した | `spec-page` | spec path、状態、関連 work unit、未解決事項。 |
| TDD で実装した | `tdd-workflow` | 対象 item、red / green / refactor 状態、実行 command。 |
| Switch protocol / BTstack / backend fact に触れた | `source-audit` | 参照元、事実 / 推定 / 未検証事項、根拠監査 status。 |
| 実機が関係する | `hardware-harness` | 承認範囲、adapter、log path、cleanup、未実行理由。 |
| 後回し判断がある | `dev-journal` | 記録した項目と path。 |

該当しない skill は起動しない。
起動しなかった場合でも、高リスク領域では PR の根拠監査、実機、BTstack / License section に理由を残す。

## 手順

1. ブランチと repository context を確認する。
   - `git branch --show-current`
   - `git remote get-url origin`
   - `origin/HEAD` または repository metadata から既定ブランチを確認する。
   - `git status --short`
2. 既定ブランチ上なら停止する。
3. worktree が dirty なら停止する。
4. PR commit log を作る。
   - `git log --oneline <default>..HEAD`
5. 変更範囲から必要な skill coordination を確認する。
6. `.github/PULL_REQUEST_TEMPLATE.md` に沿って PR body を作る。
7. ブランチを push する。
   - `git push -u origin <branch>`
8. GitHub app または `gh pr create` で PR を作る。
9. GitHub app または `gh pr checks` で CI/status を確認する。
10. required check が failing または still running なら停止する。
11. ユーザがマージを承認したら、merge commit でマージし、remote head branch を merge option で削除する。
    - `gh pr merge --merge --delete-branch`
12. 既定ブランチを fetch して同期する。
    - `git fetch --prune origin`
    - `git switch <default>`
    - `git pull --ff-only origin <default>`
13. 既定ブランチが clean で、expected head にいることを確認する。
14. local 作業ブランチが残っていれば削除する。
    - `git branch -d <branch>`
15. remote 作業ブランチが削除済みであることを確認する。残っている場合は自動削除失敗として報告し、勝手に追加削除しない。

## PR Body の必須項目

PR 本文には次を含める。

- 概要。
- 関連 issue、work unit record、spec、journal entry。
- 挙動、構造、docs、workflow ごとに整理した変更。
- Commit Log。
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
- merge commit が repository で許可されていない。
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
BTstack/license:
follow-up:
```
