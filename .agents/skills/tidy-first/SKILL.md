---
name: tidy-first
description: "swbt-daemon で behavior change と structure change を分離し、構造変更を先に行うか、green 後に行うか、後回しにするか判断する skill。Codex が refactor、cleanup、設計整理、実装前の下準備、TDD green 後の構造変更判断を行うときに使う。"
---

# Tidy First

振る舞い変更と構造変更を分け、構造変更をいつ行うか判断する。

## Classify

| 種別 | 判断基準 |
|---|---|
| behavior change | 外部から観測できる結果、IPC、HID report、public C ABI、ログ、エラー、BTstack 連携境界が変わる |
| structure change | 観測可能な振る舞いを変えず、読みやすさや変更容易性だけを変える |

Switch protocol bytes、BTstack source selection、report timing、WinUSB/libusb behavior に触れる作業は、見た目が小さくても behavior change として扱い、必要なら `source-audit` を使う。

## Decision

- tidy first: 小さな構造変更で次の behavior change が明確に小さくなり、既存 test または characterization がある。
- tidy after: green 後に重複や読みにくさが明確になり、次の TDD cycle を助ける。
- tidy later: 今の item に不要で、範囲またはリスクが大きい。
- do not tidy: 抽象化の根拠が弱い、未検証 protocol / BTstack / 実機 sequence に触る、または speculative な整理である。

## swbt Examples

- tidy first: duplicated byte packing helper を、既存 report packing test がある状態で private helper へ抽出する。
- tidy after: green 後に subcommand response builder の重複 field initialization をまとめる。
- tidy later: daemon runtime と IPC parser の責務分離を、今の parser item とは別 work unit に送る。
- do not tidy: 実機未検証の report period や BTstack callback order を「整理」として変える。

## Rules

- 構造変更は観測可能な振る舞いを変えない。
- behavior change と structure change は、差分、説明、検証を分ける。
- 大きい抽象化、責務移動、外部 API 変更は tidy ではなく設計変更として work unit record または spec に記録する。
- green にする途中で tidy を混ぜない。green 後、または behavior change 前の明確な準備として扱う。

## Verification

構造変更の前後で、同じ検証コマンドを実行する。リスクに応じて次を使う。

```console
just build-debug
CTEST_ARGS="-R <test-name>" just test-debug
just test-debug
just asan
just windows-cross
```

docs / skill guidance の構造変更では、同じ `rg`、`git diff --check`、skill validation を前後で使う。

## Output

work unit record または handoff に分類と判断を残す。

```text
Tidy status:
- classification: behavior change | structure change
- decision: tidy first | tidy after | tidy later | do not tidy
- reason:
- verification:
```
