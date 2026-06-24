# Pre-Push Verify Default

## 1. 概要

この work unit は、ローカル push 前に `just verify` が走らない可能性を減らすため、`pre-push` hook の既定を `just debug` から `just verify` へ変える。

完了後は、通常の push で `format-check`、`clang-tidy`、debug build/test、ASan、Windows cross build が走る。例外的に軽い push 前確認だけにしたい場合は、`SWBT_FAST_PRE_PUSH=1` で `just debug` を実行できる。

## 2. 起点 / ユースケース

source:

- User request: `verify` がローカルで走らない可能性が気になるため、push 時に `verify` を掛ける方が良い。
- Existing contract: `pre-push` は通常 `just debug` で、`SWBT_FULL_PRE_PUSH=1` のときだけ `just verify` を実行していた。
- CI contract: GitHub Actions は Dev Container 内で `just verify-ci` を実行し、`verify-ci` は `verify` と同じ `_verify-in-container` を使う。

use case:

- actor: contributor / maintainer。
- input/state: local branch を remote へ push する。
- expected observation: 通常の `pre-push` で `just verify` が実行され、formatter / linter / sanitizer / cross build の問題が push 前に止まる。
- constraints: 緊急時や調査中に hook を完全に無効化する `SWBT_SKIP_HOOKS=1` は残す。debug-only の軽量確認は明示的な opt-in にする。

source から use case への判断:

- `just verify` はこの repo の広い非実機 gate であり、CI と同じ範囲をローカル push 前に走らせる方が失敗の発見が早い。
- ただし `verify` は重いので、既定化に伴う escape hatch として `SWBT_FAST_PRE_PUSH=1` を追加する。これは hook skip ではなく、`just debug` だけに範囲を狭める選択肢である。

## 3. 対象範囲

- `.githooks/pre-push` の既定を `just verify` にする。
- `SWBT_FAST_PRE_PUSH=1` で `just debug` を実行する分岐を追加する。
- README、`.githooks/README.md`、`AGENTS.md`、`spec/operations/development-tooling.md` の hook 契約を更新する。
- CI は `just verify-ci` のまま据え置く。

## 4. 対象外

- CI workflow の実行 command 変更。
- `just verify` / `just verify-ci` の中身変更。
- `pre-commit` の実行範囲変更。
- 実機検証 gate の追加。

## 5. 関連 spec / docs

- `.githooks/pre-push`
- `.githooks/README.md`
- `README.md`
- `AGENTS.md`
- `spec/operations/development-tooling.md`
- `.github/workflows/ci.yml`
- `justfile`

## 6. 根拠監査

not applicable。

この work unit は Git hook と task runner の運用契約を扱う。Switch HID report bytes、BTstack source selection、report period、subcommand、SPI、rumble、descriptor data、WinUSB/libusb の実装値は変更しない。

## 7. 設計メモ

`pre-push` の通常経路は `just verify` にする。`SWBT_FAST_PRE_PUSH=1` の場合だけ `just debug` を実行する。`SWBT_SKIP_HOOKS=1` は hook 全体の明示スキップとして残す。

CI は `just verify-ci` を実行し続ける。`justfile` では `_verify-ci-in-container` が `_verify-in-container` の alias なので、CI の実行範囲は `format-check`、`tidy`、`debug`、`asan`、`windows-cross` のままである。

## 8. 対象ファイル

- `.githooks/pre-push`
- `.githooks/README.md`
- `README.md`
- `AGENTS.md`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_069/PRE_PUSH_VERIFY_DEFAULT.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | default `pre-push` path executes `just verify` | regression | tooling | no |
| done | `SWBT_FAST_PRE_PUSH=1` path executes `just debug` and does not skip hooks entirely | regression | tooling | no |
| done | CI remains `just verify-ci` and therefore runs the same non-hardware gate as `verify` | characterization | tooling | no |

## 10. 検証

- `sh -n .githooks/pre-push`: pass。
- `rg -n "SWBT_FAST_PRE_PUSH|SWBT_FULL_PRE_PUSH|just verify|just debug|verify-ci" .githooks README.md AGENTS.md spec/operations/development-tooling.md work-units/wip/local_069 .github/workflows/ci.yml justfile`: pass。active docs / hook は `SWBT_FAST_PRE_PUSH` を案内し、`SWBT_FULL_PRE_PUSH` はこの record の historical source にだけ残る。
- `.githooks/pre-push` content inspection: default branch executes `just verify`; `SWBT_FAST_PRE_PUSH=1` branch executes `just debug`; `SWBT_SKIP_HOOKS=1` still exits before either gate.
- `.github/workflows/ci.yml` content inspection: CI remains `runCmd: just verify-ci`。
- `justfile` content inspection: `_verify-ci-in-container: _verify-in-container`。CI scope remains `format-check`、`tidy`、`debug`、`asan`、`windows-cross`。
- `git diff --check`: pass。CRLF 変換警告のみ。

## 11. 実機実行条件

実機不要。

Bluetooth adapter、Switch pairing、HID advertising、report loop を実行しない Git hook / CI gate の変更である。

## 12. 先送り事項

なし。

## 13. チェックリスト

- [x] `pre-push` の既定が `just verify` になっている。
- [x] `SWBT_FAST_PRE_PUSH=1` の例外経路が docs/spec に記録されている。
- [x] CI の実行対象が `just verify-ci` のままであることを記録する。
- [x] 検証結果と実機未実行理由を記録する。
