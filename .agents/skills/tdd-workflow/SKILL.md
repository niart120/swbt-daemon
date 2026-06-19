---
name: tdd-workflow
description: "CMake、Ninja、CTest、sanitizer preset、work unit record の source / use case / TDD Test List を使う swbt-daemon の TDD orchestration skill。Codex が TDD、ユースケースからの TDD test list 作成、red/green/refactor、tidy-first 判断、Test Desiderata review、C unit test 追加、Switch packet behavior の characterization、protocol/IPC behavior の実装を求められたときに使う。必要に応じて tdd-test-list、tdd-one-cycle、tidy-first、test-desiderata-review に分岐する。"
---

# TDD ワークフロー

swbt の挙動変更を一つずつ進めるときに、この skill を使う。TDD は source から直接始めない。source を use case または観測したい振る舞いへ変換し、その use case から TDD Test List を作る。work unit、spec、TDD Test List の作成順序は `spec/operations/work-unit-spec-tdd-flow.md` に従う。

この skill は入口だけを担当する。実際の作業では、次の分割 skill を必要な順に読む。

| 作業 | 読む skill |
|---|---|
| source / use case から TDD Test List を作る、または見直す | `../tdd-test-list/SKILL.md` |
| TDD Test List の 1 item を red / green / refactor で進める | `../tdd-one-cycle/SKILL.md` |
| behavior change と structure change を分ける | `../tidy-first/SKILL.md` |
| 追加・変更した test の価値や脆さを確認する | `../test-desiderata-review/SKILL.md` |

## 前提条件

- ユーザが明示しない限り、既定ブランチでは作業しない。
- `git status --short` を確認し、ユーザの変更を保持する。
- 挙動が work unit に属する場合は `work-unit-record` を使う。
- work unit record に source と use case がない場合は、実装前に追記する。
- protocol または BTstack fact を hard-code する前に `source-audit` を使う。
- 実機承認を必要とするテストの前に `hardware-harness` を使う。
- 標準の configure / build / test / format / static analysis / cross build は `just` recipe を使う。
- Dev Container 外の host では、`justfile` が Dev Container CLI へ委譲する。

## Workflow

1. work unit record の `起点 / ユースケース` を読む。source と use case が不足していれば、実装前に record を更新する。
2. TDD Test List がない、または use case と対応していない場合は `../tdd-test-list/SKILL.md` を読む。
3. Test List から次に扱う item を 1 つだけ選ぶ。小さく、自動化でき、失敗理由が明確になる item を優先する。
4. `../tdd-one-cycle/SKILL.md` を読んで red / green / refactor を 1 cycle だけ進める。
5. 構造変更の扱いに迷う場合、または green 後に整理が必要な場合は `../tidy-first/SKILL.md` を読む。
6. test が複数の期待、characterization、hardware-gated、壊れやすい assertion を含む場合は `../test-desiderata-review/SKILL.md` を読む。
7. work unit record の TDD Test List、TDD status、検証、先送り事項を更新する。

## Routing Rules

- TDD Test List の作成だけを求められた場合は、`tdd-test-list` だけで止めてよい。
- 既存 item を 1 つ進める場合は、`tdd-one-cycle` へ直接進む。ただし source / use case と item の対応が曖昧なら先に `tdd-test-list` を読む。
- refactor、cleanup、下準備が中心の場合は、`tidy-first` を読んで behavior change と structure change を分類してから差分を作る。
- test の品質、脆さ、characterization、実機 gated test が焦点の場合は、`test-desiderata-review` を読む。
- Switch protocol、BTstack source selection、report timing、WinUSB/libusb fact を新しく扱う場合は、TDD cycle より前に `source-audit` を読む。
- 実機 command、pairing、HID advertising、report loop を伴う場合は、TDD cycle より前に `hardware-harness` を読む。

## TDD に向く対象

- controller state validation。
- button と stick の report packing。
- subcommand parser と response builder。
- SPI flash read response。
- rumble packet parser。
- JSON Lines IPC parser。
- owner acquire / release / disconnect neutral。

## 状態更新

work unit record または handoff に次を更新する。

```text
TDD status:
- source:
- use case:
- item:
- state: red | green | refactor-done | deferred
- commands:
- notes:
```

`notes` には、読んだ分割 skill、追加した Test List item、tidy 判断、Test Desiderata review の trade-off、実機未実行理由を必要に応じて残す。
