# Just Debug Tidy Contract

## 1. 概要

この work unit は、`just debug` が formatter / linter を実行しないという報告を確認し、`just` recipe と Git hooks の運用契約を明文化する。

完了後は、`debug` が linux-debug の configure / build / CTest 用 fast loop であり、`format-check` と `clang-tidy` は `verify` / `verify-ci` に含まれることを、README、hook 表示、operations spec から確認できる状態にする。

## 2. 起点 / ユースケース

source:

- User request: `just debug` だと tidy、つまり formatter / linter が走らないという報告を受けたため、`just` 周りを精査する。
- Audit finding: `justfile` の `_debug-in-container` は `_configure-debug-in-container`、`_build-debug-in-container`、`_test-debug-in-container` だけを呼ぶ。`_verify-in-container` は `_format-check-in-container`、`_tidy-in-container`、`_debug-in-container`、`_asan-in-container`、`_windows-cross-in-container` を順に呼ぶ。

use case:

- actor: maintainer / contributor。
- input/state: 変更後に `just debug` または Git hook の `pre-push` default を実行する。
- expected observation: `debug` は build/test only であり、formatter / linter まで必要なときは `just verify` または `SWBT_FULL_PRE_PUSH=1` を使うと判断できる。
- constraints: 既存の fast debug loop と default pre-push の重さは変えない。

source から use case への判断:

- `just debug` に `format-check` と `tidy` を追加すると、`verify` との責務差が薄くなり、default pre-push が重くなる。今回の問題は実行内容そのものより、名前と説明から期待される範囲が曖昧な点である。

## 3. 対象範囲

- `justfile` の recipe コメントを明確にする。
- README と `.githooks/README.md` で `debug` / `verify` / `SWBT_FULL_PRE_PUSH=1` の違いを明記する。
- `pre-push` hook の表示メッセージで default と full gate の範囲を示す。
- `spec/operations/development-tooling.md` に `debug` と `verify` の契約を安定判断として記録する。

## 4. 対象外

- `just debug` の実行内容変更。
- `pre-push` の default を `just verify` に変えること。
- CMake presets、CTest presets、CI workflow の再設計。
- formatter / clang-tidy の check set 変更。

## 5. 関連 spec / docs

- `justfile`
- `.githooks/pre-push`
- `.githooks/README.md`
- `README.md`
- `AGENTS.md`
- `spec/operations/development-tooling.md`

## 6. 根拠監査

not applicable。

この work unit は task runner と documentation の運用契約を扱う。Switch HID report bytes、BTstack source selection、report period、subcommand、SPI、rumble、descriptor data、WinUSB/libusb の実装値は変更しない。

## 7. 設計メモ

`debug` は名前どおり debug preset の configure / build / CTest に閉じる。formatter / linter / sanitizer / Windows cross build を含む gate は `verify` とする。

`pre-push` の default は既存どおり `just debug` にする。full gate を push 前に実行したい場合は `SWBT_FULL_PRE_PUSH=1` を使う。

## 8. 対象ファイル

- `justfile`
- `.githooks/pre-push`
- `.githooks/README.md`
- `README.md`
- `AGENTS.md`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_068/JUST_DEBUG_TIDY_CONTRACT.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | `just debug` の dry-run / recipe definition から formatter / linter が含まれないことを確認できる | characterization | tooling | no |
| done | `just verify` の recipe definition から format-check と tidy が含まれることを確認できる | characterization | tooling | no |
| done | docs と hook 表示が default pre-push と full gate の違いを明記する | regression | docs/tooling | no |

## 10. 検証

- `just --dry-run debug`: pass。top-level recipe は `_run-or-delegate debug` を呼ぶ。`justfile` の `_debug-in-container` は `_configure-debug-in-container`、`_build-debug-in-container`、`_test-debug-in-container` だけを実行する。
- `just --dry-run verify`: pass。top-level recipe は `_run-or-delegate verify` を呼ぶ。`justfile` の `_verify-in-container` は `_format-check-in-container`、`_tidy-in-container`、`_debug-in-container`、`_asan-in-container`、`_windows-cross-in-container` を実行する。
- `just --list`: pass。`debug` の説明に `Does not run format-check or clang-tidy` が表示され、`verify` の説明に `format-check, clang-tidy, debug, ASan, and Windows cross build` が表示される。
- `rg -n "just debug|format-check|clang-tidy|SWBT_FULL_PRE_PUSH|just verify" README.md AGENTS.md .githooks/README.md .githooks/pre-push spec/operations/development-tooling.md justfile`: pass。docs / hook 表示 / spec に default と full gate の違いが出ている。
- `git diff --check`: pass。CRLF 変換警告のみ。

## 11. 実機実行条件

実機不要。

Bluetooth adapter、Switch pairing、HID advertising、report loop を実行しない task runner / docs の精査である。

## 12. 先送り事項

なし。

## 13. チェックリスト

- [x] `debug` / `verify` の実行範囲を確認する。
- [x] docs と hook 表示を更新する。
- [x] operations spec と work unit record を相互に辿れる状態にする。
- [x] 検証結果と実機未実行理由を記録する。
