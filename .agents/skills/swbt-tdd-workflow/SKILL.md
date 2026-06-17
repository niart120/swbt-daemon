---
name: swbt-tdd-workflow
description: "CMake、Ninja、CTest、sanitizer preset、work-unit test list を使う swbt-daemon C11 code の標準 TDD ワークフロー。Codex が TDD、TDD test list 作成、red/green/refactor、C unit test 追加、Switch packet behavior の characterise、protocol/IPC behavior の小さな実装を求められたときに使う。"
---

# swbt TDD workflow

swbt の小さな behavior change を一つずつ進めるときに、この skill を使う。

## 前提条件

- ユーザが明示しない限り、default branch では作業しない。
- `git status --short` を確認し、ユーザの変更を保持する。
- behavior が work-unit spec に属する場合は `swbt-spec-format` を使う。
- protocol または BTstack fact を hard-code する前に `swbt-source-audit` を使う。
- hardware-gated test の前に `swbt-hardware-harness` を使う。

## Test List（テスト一覧）

coding の前に、spec の TDD Test List から observable item を一つ選ぶ。
list がない場合は、次を含む item を作る。

- input または state。
- 期待する observable result。
- test layer。
- hardware が必要かどうか。

test item に implementation detail を入れない。

## Red

最小の関連 C test を追加または更新する。
期待する失敗を示す最も狭い command を実行する。

典型的な command:

```console
cmake --build --preset linux-debug
ctest --preset linux-debug -R <test-name> --output-on-failure
```

失敗が期待する behavior ではなく build、collection、environment problem によるものなら、red として数えない。

## Green

選んだ item と関連する既存 test を通すために必要な最小 behavior を実装する。

実行する。

```console
cmake --build --preset linux-debug
ctest --preset linux-debug -R <test-name> --output-on-failure
```

変更が shared protocol code に触れる場合は、full debug preset も実行する。

```console
ctest --preset linux-debug --output-on-failure
```

## Refactor

green になってから refactor する。
observable behavior change と structure change は分ける。

refactor 後に同じ test を実行する。
触った code のリスクに応じて sanitizer または cross build を追加する。

```console
cmake --preset linux-asan
cmake --build --preset linux-asan
ctest --preset linux-asan --output-on-failure
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
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

spec または handoff に次を更新する。

```text
TDD status:
- item:
- state: red | green | refactor-done | deferred
- commands:
- notes:
```
