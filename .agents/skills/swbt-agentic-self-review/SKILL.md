---
name: swbt-agentic-self-review
description: "PR、merge、handoff の前に swbt-daemon の変更を gate 単位で自己 review する。Codex が要件の充足範囲、source audit の状態、CMake/CTest 結果、sanitizer/cross-build 結果、hardware run または not-run reason、BTstack/license impact、non-goals、remaining risks、work unit 完了を裏付ける根拠を整理するときに使う。"
---

# swbt agentic self review（自己レビュー）

swbt work unit の完了宣言、PR 作成、大きな変更の handoff の前に、この skill を使う。

## レビュー手順

1. 対象 work unit、ユーザの意図、non-goals を特定する。
2. 変更ファイルを spec と `AGENTS.md` に照らして確認する。
3. protocol、BTstack、backend、hardware facts の source audit status を記録する。
4. verification command と正確な結果を記録する。
5. hardware status は automated test と分けて記録する。
6. BTstack license / notice impact を記録する。
7. 弱い根拠、間接的な根拠、欠けている根拠は not proven として扱う。

## Gate Table（判定表）

次の gate を使う。

| gate | result | evidence |
|---|---|---|
| Requirements | pass/fail/not proven | spec とユーザ目的の充足範囲 |
| Non-goals | pass/fail | scope が広がっていないこと |
| Source Audit | pass/not applicable/not run | protocol/BTstack facts の根拠 |
| Tests | pass/fail/not run | CTest または targeted command |
| Static / Build | pass/fail/not run | CMake configure/build、sanitizer、cross build |
| Hardware | pass/fail/not run/not applicable | 承認、adapter、結果、または理由 |
| BTstack / License | pass/not applicable/not proven | submodule untouched、notices checked |
| Integration Review | pass/fail/not proven | 境界に照らした diff review |

## 指摘を先に置く

問題がある場合は、summary より前に findings を列挙する。

```markdown
### 指摘

| severity | finding | evidence | recommendation |
|---|---|---|---|
```

問題がない場合でも、residual risk と未実行 gate を列挙する。

## 報告テンプレート

```markdown
## swbt self review（自己レビュー）

### 作業単位
- 対象:
- 意図:
- 対象外:

### 指摘
| 重要度 | 指摘 | 根拠 | 推奨対応 |
|---|---|---|---|

### Gate
| gate | result | 根拠 |
|---|---|---|

### 検証
- command:
- not run:

### 根拠 / 実機
- source audit:
- hardware:
- BTstack/license:

### 次
- follow-up:
- open risk:
```

## ルール

- test が通っただけで work unit を complete にしない。根拠を requirements と対応させる。
- unit test の成功で hardware not-run status を隠さない。
- 明示的な source audit と license review なしに、`vendor/btstack` の変更を安全扱いしない。
