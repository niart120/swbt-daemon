---
name: tdd-test-list
description: "swbt-daemon の source / use case から観測可能な TDD Test List を作成または更新する skill。Codex が TDD の最初の分析、テストシナリオ、expected behavior、edge case、regression、characterization、hardware-gated item を実装前に洗い出すときに使う。work unit record の起点 / ユースケースから test item を作る。"
---

# TDD Test List

実装設計ではなく、観測可能な振る舞いの分析として TDD Test List を作る。

## Inputs

- ユーザ要求、roadmap TODO、journal entry、deferred item、bug、実機観測、根拠監査 finding、既存 spec の未解決事項。
- work unit record の `起点 / ユースケース`。
- 関連 spec の behavior、制約、対象外。
- protocol、BTstack、report timing、WinUSB/libusb に依存する場合は、根拠監査状態。
- 実機に依存する場合は、hardware-gated であること。

## Process

1. source をそのまま item にせず、actor または境界、入力または状態、期待する観測結果へ変換する。
2. new、regression、edge、error handling、characterization、hardware-gated を分けて列挙する。
3. 各 item に、入力または state、期待する観測結果、test layer、実機要否を含める。
4. 実装方法、内部構造、抽象化案、file list は test item に混ぜない。必要なら設計メモへ分離する。
5. 不確実な値や実機未検証の値は、確定値ではなく仮説または characterization として記録する。
6. 次に実行する 1 item を選ぶ。小さく、自動化でき、失敗理由が明確になる item を優先する。

## swbt Item Patterns

- controller state validation: input state と expected neutral / clamped / rejected result を分ける。
- report packing: button、stick、battery、connection info など観測単位を分け、byte 値は根拠監査状態を添える。
- subcommand response: request、state、expected response fields、unsupported handling を分ける。
- SPI / rumble: address、length、packet shape、未検証仮説を明示する。
- JSON Lines IPC: input line、owner/session state、expected daemon state、error response を分ける。
- scheduler / heartbeat: elapsed time、owner disconnect、neutral fallback、report cadence を分ける。

## Selection Rules

- 最初の item は、最小の public behavior か characterization を優先する。
- 新しい protocol fact が必要な item は、根拠監査が済むまで `deferred` または characterization にする。
- 実機がないと成功条件を観測できない item は、unit で代替できる境界と hardware-gated item に分ける。
- 1 item が parser、state mutation、scheduler output を同時に含む場合は分割する。

## List Format

work unit record では次の形を使う。

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | observable behavior | new/regression/edge/characterization | unit/integration/hardware | no/yes |

status は `todo`、`red`、`green`、`refactor-done`、`deferred` を使う。

## Quality Gate

- 各 item は assertion または観測可能な docs check に変換できる。
- 1 item が複数の独立した期待結果を含む場合は分割する。
- 実機なしで検証できない item は `hardware-gated` として明示する。
- 期待値を実行結果から丸写しする必要がある item は characterization として扱い、根拠を記録する。
- `deferred` にする item は、work unit record の `先送り事項` または先送り不要とした理由へ辿れるようにする。

## Output

work unit record へ、更新した table と次に扱う 1 item を記録する。

```text
TDD status:
- source:
- use case:
- item:
- state: todo
- commands: none
- notes: selected by tdd-test-list; rationale=<短い理由>
```
