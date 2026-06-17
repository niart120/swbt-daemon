---
name: tdd-workflow
description: "CMake、Ninja、CTest、sanitizer preset、作業単位のテスト一覧を使う swbt-daemon C11 code の標準 TDD ワークフロー。Codex が TDD、TDD test list 作成、red/green/refactor、C unit test 追加、Switch packet behavior の characterise、protocol/IPC behavior の小さな実装を求められたときに使う。"
---

# TDD ワークフロー

swbt の小さな挙動変更を一つずつ進めるときに、この skill を使う。

## 前提条件

- ユーザが明示しない限り、既定ブランチでは作業しない。
- `git status --short` を確認し、ユーザの変更を保持する。
- 挙動が作業単位仕様に属する場合は `spec-format` を使う。
- protocol または BTstack fact を hard-code する前に `source-audit` を使う。
- 実機承認を必要とするテストの前に `hardware-harness` を使う。

## Test List（テスト一覧）

coding の前に、仕様の TDD Test List から観測可能な項目を一つ選ぶ。
list がない場合は、次を含む項目を作る。

- input または state。
- 期待する観測結果。
- test layer。
- 実機が必要かどうか。

テスト項目に実装詳細を入れない。

## Red

最小の関連 C test を追加または更新する。
期待する失敗を示す最も狭いコマンドを実行する。

典型的なコマンド:

```console
cmake --build --preset linux-debug
ctest --preset linux-debug -R <test-name> --output-on-failure
```

失敗が期待する挙動ではなく build、collection、environment problem によるものなら、red として数えない。

## Green

選んだ項目と関連する既存テストを通すために必要な最小挙動を実装する。

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
観測可能な挙動変更と構造変更は分ける。

refactor 後に同じテストを実行する。
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

仕様または handoff に次を更新する。

```text
TDD status:
- item:
- state: red | green | refactor-done | deferred
- commands:
- notes:
```
