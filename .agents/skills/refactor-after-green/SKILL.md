---
name: refactor-after-green
description: "swbt-daemon の TDD green 後に、観測可能な振る舞いを変えずに小さな構造変更を行う skill。Codex が TDD one cycle の refactor phase、green 後の重複整理、命名改善、責務境界の整理、test fixture の読みやすさ改善、または refactor-done / refactor-skipped の判断を行うときに使う。"
---

# Refactor After Green

TDD の green 後に、今の item を保ったまま構造を整える。formatter や linter の実行は検証または機械的整形であり、この skill の refactor 本体ではない。

詳細な判断基準と swbt 固有の例が必要な場合は `references/swbt-refactoring-guidance.md` を読む。

## Preconditions

- green baseline の command と結果が分かっている。
- 変更対象の item が 1 つに絞られている。
- 観測可能な振る舞いを変えない。
- behavior change と structure change の分類に迷う場合は、先に `../tidy-first/SKILL.md` を読む。
- test の assertion が構造変更で壊れそうな場合は、`../test-desiderata-review/SKILL.md` を読む。

## Review Points

green 後に次を短く確認する。

- 同じ意味の分岐、初期化、byte packing、fixture setup が重複していないか。
- 関数名、変数名、test 名が、今回確定した振る舞いを表しているか。
- parser、state mutation、scheduler、BTstack bridge、IPC transport の責務が混ざっていないか。
- private helper 抽出で次の TDD item が小さくなるか。
- test が実装構造ではなく観測可能な振る舞いを読ませているか。
- `source-audit` または実機承認が必要な値へ触れていないか。

## Decision

- `refactor-done`: 小さな構造変更を行い、同じ検証 command が再び green になった。
- `refactor-skipped`: 見直したが、今の item で行う構造変更がない、または行うべきでない。
- `deferred`: 必要な整理は見つかったが、今の cycle の範囲を超えるため後続 source として残した。

`refactor-skipped` は失敗ではない。抽象化の根拠が弱い、差分が大きい、未検証 protocol / BTstack / 実機 sequence に触れる場合は、構造変更を入れない判断を明記する。

## Refactor

1. green baseline と同じ command を確認する。
2. Review Points から、今の item に効く構造変更だけを 1 つまたは少数選ぶ。
3. formatter / linter だけで終わらせず、構造変更がない場合は `refactor-skipped` とする。
4. 変更後に green baseline と同じ command を再実行する。
5. shared protocol code、IPC parser、scheduler、public C ABI に触れた場合は、リスクに応じて `just test-debug`、`just asan`、`just windows-cross` を追加する。

docs / skill guidance の item では、同じ `rg`、`git diff --check`、skill validation を再実行する。C code に触れていない場合、CMake / CTest は不要理由を記録する。

## Output

work unit record または handoff に次を残す。

```text
Refactor status:
- decision: refactor-done | refactor-skipped | deferred
- change:
- unchanged behavior:
- verification:
- notes:
```
