---
name: test-desiderata-review
description: "Kent Beck の Test Desiderata に基づいて swbt-daemon の test の価値と trade-off をレビューする skill。Codex が CTest、characterization test、hardware-gated test、TDD で追加した test の品質、脆さ、実機未検証リスクを確認するときに使う。"
---

# Test Desiderata Review

テストを万能の点数表ではなく、目的に対する trade-off としてレビューする。

## Desiderata

主に次の性質を確認する。

| 性質 | 見ること |
|---|---|
| Isolated | 実行順や他 test の状態に依存しないか |
| Composable | 入力、状態、adapter、backend、実機条件を分けて検証できるか |
| Deterministic | 同じ条件で同じ結果になるか |
| Fast | 開発中に繰り返し実行できる速度か |
| Writable | 対象コードの価値に対して書くコストが過大でないか |
| Readable | 失敗時に意図と期待が読み取れるか |
| Behavioral | 実装構造ではなく観測可能な振る舞いに反応するか |
| Structure-insensitive | 内部構造の整理だけで壊れないか |
| Automated | 人手なしで実行できるか |
| Specific | 失敗原因が狭く分かるか |
| Predictive | green なら protocol / IPC / daemon behavior が実利用に近づくか |
| Inspiring | 通ったときに十分な信頼を与えるか |

## Review Process

1. test の目的を new、regression、edge、characterization、hardware-gated に分類する。
2. すべての性質を最大化しようとせず、今回の目的で重要な trade-off を 2 から 4 個に絞る。
3. 実機 test では predictive と fast / deterministic の衝突を明示する。
4. characterization test では根拠、入力 artifact、期待値の由来を確認する。
5. 実装詳細に依存する assertion は、必要性が説明できる場合だけ残す。
6. 根拠監査未完了、実機未検証、BTstack license / notice への影響があれば明示する。

## swbt Trade-Offs

- unit protocol test は fast / deterministic に寄せる。predictive が足りない場合は hardware-gated item を別に残す。
- BTstack bridge test は structure-insensitive と predictive が衝突しやすい。callback order や source selection の根拠を分ける。
- characterization test は readable / specific より evidence traceability を優先する場合がある。入力 artifact と期待値の由来を記録する。
- hardware-gated test は fast / automated を満たせない場合がある。承認条件、adapter、cleanup、実機未実行理由を明記する。
- docs / skill guidance の check は behavioral ではなく workflow contract の regression として扱う。

## Risk Signals

- assertion が private helper 名、内部配列順、偶然の formatting に依存している。
- 1 test が parser、state mutation、scheduler output、transport error を同時に検証している。
- 実機観測値を firmware、adapter、driver、commit なしに固定している。
- 根拠監査未完了の protocol byte を regression expected value として確定している。
- flaky さを避けるために test から重要な behavior を落としている。

## Output

レビュー結果は、問題の大きい順に示す。

| 指摘 | 関連する性質 | リスク | 対応 |
|---|---|---|---|
| 例: 実機結果を固定値として unit test に入れている | Deterministic / Predictive | firmware 差で brittle | characterization と根拠を分ける |

問題がない場合も、残る trade-off と未検証領域を明記する。

work unit record に残す場合は次の短い形でよい。

```text
Test desiderata:
- purpose:
- key trade-offs:
- risks:
- action:
```
