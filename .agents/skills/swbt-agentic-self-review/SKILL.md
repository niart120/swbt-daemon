---
name: swbt-agentic-self-review
description: "PR、マージ、handoff の前に swbt-daemon の変更をゲート単位で自己レビューする。Codex が要件の充足範囲、根拠監査の状態、CMake/CTest 結果、sanitizer/cross-build 結果、実機実行または未実行理由、BTstack/license impact、non-goals、remaining risks、作業単位完了を裏付ける根拠を整理するときに使う。"
---

# swbt 自己レビュー

swbt 作業単位の完了宣言、PR 作成、大きな変更の handoff の前に、このスキルを使う。

## レビュー手順

1. 対象作業単位、ユーザの意図、non-goals を特定する。
2. 変更ファイルを仕様と `AGENTS.md` に照らして確認する。
3. protocol、BTstack、backend、実機事実の根拠監査状態を記録する。
4. 検証コマンドと正確な結果を記録する。
5. 実機状態は自動テストと分けて記録する。
6. BTstack license / notice impact を記録する。
7. 弱い根拠、間接的な根拠、欠けている根拠は not proven として扱う。

## Gate Table（判定表）

次のゲートを使う。

| ゲート | 結果 | 根拠 |
|---|---|---|
| Requirements | pass/fail/not proven | 仕様とユーザ目的の充足範囲 |
| Non-goals | pass/fail | scope が広がっていないこと |
| Source Audit | pass/not applicable/not run | protocol/BTstack facts の根拠 |
| Tests | pass/fail/not run | CTest または targeted command |
| Static / Build | pass/fail/not run | CMake configure/build、sanitizer、cross build |
| Hardware | pass/fail/not run/not applicable | 承認、アダプター、結果、または理由 |
| BTstack / License | pass/not applicable/not proven | submodule untouched、notices checked |
| Integration Review | pass/fail/not proven | 境界に照らした diff review |

## 指摘を先に置く

問題がある場合は、summary より前に指摘を列挙する。

```markdown
### 指摘

| 重要度 | 指摘 | 根拠 | 推奨対応 |
|---|---|---|---|
```

問題がない場合でも、残るリスクと未実行ゲートを列挙する。

## 報告テンプレート

```markdown
## swbt 自己レビュー

### 作業単位
- 対象:
- 意図:
- 対象外:

### 指摘
| 重要度 | 指摘 | 根拠 | 推奨対応 |
|---|---|---|---|

### Gate
| ゲート | 結果 | 根拠 |
|---|---|---|

### 検証
- コマンド:
- not run:

### 根拠 / 実機
- 根拠監査:
- 実機:
- BTstack/license:

### 次
- 追加確認:
- open risk:
```

## ルール

- テストが通っただけで作業単位を complete にしない。根拠を要件と対応させる。
- 単体テストの成功で実機未実行状態を隠さない。
- 明示的な根拠監査と license review なしに、`vendor/btstack` の変更を安全扱いしない。
