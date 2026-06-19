---
name: tdd-workflow
description: "CMake、Ninja、CTest、sanitizer preset、work unit record の source / use case / TDD Test List を使う swbt-daemon code の標準 TDD ワークフロー。Codex が TDD、ユースケースからの TDD test list 作成、red/green/refactor、C unit test 追加、Switch packet behavior の characterise、protocol/IPC behavior の実装を求められたときに使う。source には user request、roadmap TODO、journal entry、deferred item、bug、実機観測、根拠監査 finding、既存 spec の未解決事項を含む。"
---

# TDD ワークフロー

swbt の挙動変更を一つずつ進めるときに、この skill を使う。TDD は source から直接始めない。source を use case または観測したい振る舞いへ変換し、その use case から TDD Test List を作る。work unit、spec、TDD Test List の作成順序は `spec/operations/work-unit-spec-tdd-flow.md` に従う。

## 前提条件

- ユーザが明示しない限り、既定ブランチでは作業しない。
- `git status --short` を確認し、ユーザの変更を保持する。
- 挙動が work unit に属する場合は `work-unit-record` を使う。
- work unit record に source と use case がない場合は、実装前に追記する。
- protocol または BTstack fact を hard-code する前に `source-audit` を使う。
- 実機承認を必要とするテストの前に `hardware-harness` を使う。
- 標準の configure / build / test / format / static analysis / cross build は `just` recipe を使う。
- Dev Container 外の host では、`justfile` が Dev Container CLI へ委譲する。Codex は自分の判断で `SWBT_ALLOW_HOST_BUILD=1` を付けない。

## Test List（テスト一覧）

coding の前に、work unit record の `起点 / ユースケース` を読む。TDD Test List がない場合は、use case から作る。list がある場合は、各 item が use case に対応していることを確認する。

test item には次を含める。

- input または state。
- 期待する観測結果。
- test layer。
- 実機が必要かどうか。

テスト項目に実装詳細を入れない。roadmap TODO、journal entry、file list、実装都合だけを test item にしない。

TDD Test List から、今回扱う item を一つ選ぶ。

## Red

関連 C test を追加または更新する。
期待する失敗を示す最も狭いコマンドを実行する。

典型的なコマンド:

```console
just build-debug
CTEST_ARGS="-R <test-name>" just test-debug
```

失敗が期待する挙動ではなく build、collection、environment problem によるものなら、red として数えない。

## Green

選んだ項目と関連する既存テストを通すために必要な挙動を実装する。必要なら構造を大きく変えてよい。ただし、選んだ item 以外の新しい振る舞いを同じ cycle に混ぜない。

実行する。

```console
just build-debug
CTEST_ARGS="-R <test-name>" just test-debug
```

変更が shared protocol code に触れる場合は、full debug preset も実行する。

```console
just test-debug
```

## Refactor

green になってから refactor する。観測可能な挙動変更と構造変更は分ける。structure change は、差分、説明、検証を behavior change と分ける。

refactor 後に同じテストを実行する。触った code のリスクに応じて sanitizer または cross build を追加する。

```console
just asan
just windows-cross
```

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
