---
name: tdd-one-cycle
description: "swbt-daemon の TDD Test List から 1 item だけを選び、CMake/CTest/just で red、green、必要な refactor を 1 cycle 進める skill。Codex が失敗する C test を追加し、最小実装で通し、refactor-done / refactor-skipped を含む work unit record の TDD status を更新するときに使う。"
---

# TDD One Cycle

TDD Test List の 1 item だけを対象に、red、green、必要な refactor を実行する。

## Preconditions

- 変更を伴う cycle は、既定ブランチではなく作業ブランチ上で行う。
- dirty worktree では既存変更の由来を確認し、ユーザ変更を破棄しない。
- work unit record の source、use case、TDD Test List が今回の item と対応している。
- protocol または BTstack fact を hard-code する前に `source-audit` を使う。
- 実機 item を実行する前に `hardware-harness` を使う。

## Red

1. 対象 item を 1 つだけ選び、期待結果を assertion または観測可能な check として先に明確にする。
2. 関連 C test、docs check、または skill validation の最小差分を追加する。
3. 最小範囲の command を実行し、期待した理由で失敗することを確認する。

典型コマンド:

```console
just build-debug
CTEST_ARGS="-R <test-name>" just test-debug
```

docs / skill guidance の item では、該当する `rg`、`git diff --check`、または skill validation を red の観測として使ってよい。失敗理由が build、collection、environment problem の場合は TDD の red として扱わない。

red を確認したら work unit record の対象 item を `red` に更新する。red の command と、期待した失敗理由を短く残す。

## Green

1. 今の item と関連する既存契約を通す実装にする。必要な構造変更は入れてよいが、選んだ item 以外の新しい振る舞いは混ぜない。
2. 途中で別の振る舞いに気づいたら、実装へ混ぜず TDD Test List または先送り事項に追加する。
3. 対象 command と関連 command を実行し、green を確認する。

典型コマンド:

```console
just build-debug
CTEST_ARGS="-R <test-name>" just test-debug
```

shared protocol code、IPC parser、scheduler、public C ABI に触れる場合は full debug preset も実行する。

```console
just test-debug
```

docs / skill guidance の item では、該当する file path、routing、frontmatter、validation command を最小範囲で通す。C code に触れていない場合、CMake / CTest は不要理由を work unit record に書く。

## Refactor

1. green の後だけ実行する。green baseline の command と結果を確認してから始める。
2. green 後に構造変更を行う、または行わない判断を記録する必要がある場合は `../refactor-after-green/SKILL.md` を読む。
3. behavior change と structure change を分ける必要がある場合は `../tidy-first/SKILL.md` を使う。
4. formatter / linter の実行だけを refactor 本体として扱わない。構造変更がない、または行うべきでない場合は `refactor-skipped` と記録する。
5. refactor 後は同じ command を再実行し、リスクに応じて sanitizer または cross build を追加する。

```console
just asan
just windows-cross
```

## Scope Control

- 1 cycle で扱う item は 1 つだけにする。
- green の途中で見つけた別 behavior は、追加実装せず TDD Test List に戻す。
- protocol fact、BTstack source、実機 observation をついでに確定しない。必要なら `source-audit` または `hardware-harness` の別 item にする。
- unrelated formatting、large rename、spec 整理を混ぜない。必要な場合は `tidy-first` で分ける。

## Status Update

work unit record に対象 item の状態を `red`、`green`、`refactor-done`、`refactor-skipped`、`deferred` のいずれかで反映する。実行した command、失敗理由、追加した Test List item、refactor の判断、実機未実行理由も記録する。

```text
TDD status:
- source:
- use case:
- item:
- state: red | green | refactor-done | refactor-skipped | deferred
- commands:
- notes:
```
