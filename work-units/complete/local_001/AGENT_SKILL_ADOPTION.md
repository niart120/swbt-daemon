# Agent Skill Adoption

## 1. 概要

swbt-daemon の初期運用基盤を整えた work unit。

root `AGENTS.md`、project-local Codex skills、work unit record / spec 構成、
Git hooks、PR template、CI、formatter / linter 方針、README の開発手順を追加した。

## 2. 対象範囲

- 日本語の agent 運用ルールと用語整理。
- `work unit record`、`spec`、`journal entry` の配置ルール。
- `source-audit`、`hardware-harness`、`work-unit-record`、`spec-page`、`tdd-workflow`、
  `dev-journal`、`agentic-self-review`、`pr-merge-cleanup` の skill 導入。
- `.clang-format`、`.clang-tidy`、format scripts、Git hooks、GitHub Actions の導入。
- Dev Container を主経路にし、host build を明示 opt-in にする CMake guard。
- 初期設計メモを `spec/initial/` に移し、開発 tooling spec を追加。

## 3. 対象外

- Switch pairing、Bluetooth HID advertising、report loop の実装。
- HID descriptor、subcommand response、SPI payload、rumble packet の実装。
- BTstack source list の CMake 取り込み。
- `vendor/btstack` の変更。
- 実機検証。

## 4. 関連 spec / docs

- `AGENTS.md`
- `README.md`
- `.github/PULL_REQUEST_TEMPLATE.md`
- `spec/operations/development-tooling.md`
- `spec/references/switch-hid-initial-source-audit.md`
- `spec/dev-journal.md`
- `docs/hardware-test-log.md`
- `docs/upstream-btstack.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/initial/BTSTACK_SWITCH_DEVELOPMENT_PLAN.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/SWBT_DAEMON_INITIAL_DIRECTION.md`

## 5. 根拠監査

`spec/initial/` には Switch HID report、subcommand、report period、BTstack port の
初期設計値が含まれるため、`spec/references/switch-hid-initial-source-audit.md` に
documentation-level の根拠監査を追加した。

判定:

- docs / initial planning の根拠整理: pass。
- protocol implementation の根拠: not complete。
- 実機観測: not run。
- `vendor/btstack`: untouched。

## 6. 設計メモ

- Dev Container と CI を標準実行環境にし、host build は `SWBT_ALLOW_HOST_BUILD=1`
  または `-DSWBT_ALLOW_HOST_BUILD=ON` で明示 opt-in にした。
- Git hooks は `.githooks/` を正本にし、clone 後の install script を用意した。
- PR template は根拠監査、実機状態、BTstack / License impact を必須項目にした。
- `spec/references/` は規範ではなく、upstream 調査と根拠要約を置く場所として整理した。

## 7. 対象ファイル

- `.agents/skills/**`
- `.clang-format`
- `.clang-tidy`
- `.devcontainer/devcontainer.json`
- `.githooks/**`
- `.github/PULL_REQUEST_TEMPLATE.md`
- `.github/workflows/ci.yml`
- `AGENTS.md`
- `CMakeLists.txt`
- `CMakePresets.json`
- `README.md`
- `docs/**`
- `scripts/**`
- `spec/**`
- `work-units/**`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | `cmake --list-presets` で主要 preset が読める | regression | build | no |
| green | host build opt-in 付き `linux-debug` configure が成功する | regression | build | no |
| green | `linux-debug` build が成功する | regression | build | no |
| green | `ctest --preset linux-debug` で `swbt_smoke_test` が通る | regression | unit | no |
| deferred | `scripts/check-format.sh` | regression | workflow | no |
| deferred | `linux-clang-tidy` | regression | static | no |
| deferred | `linux-asan` | regression | sanitizer | no |
| deferred | `windows-mingw-debug` | regression | cross-build | no |

## 9. 検証

- `git diff --check origin/main..HEAD`: pass。
- `cmake --list-presets`: pass。`linux-debug`、`linux-asan`、`linux-clang-tidy`、`windows-mingw-debug` を確認。
- `SWBT_ALLOW_HOST_BUILD=1 cmake --fresh --preset linux-debug`: pass。
- `cmake --build --preset linux-debug`: pass。
- `ctest --preset linux-debug`: pass。`1/1` tests passed。
- `SWBT_ALLOW_HOST_BUILD=1 git push -u origin chore/agent-skill-adoption`: pass。pre-push hook 内で `linux-debug` configure/build/test が再実行され、`1/1` tests passed。

not run:

- `scripts/check-format.sh`: local environment に `clang-format` が無いため未実行。CI `quality` job の対象。
- `linux-clang-tidy`: local environment に `clang` / `clang-tidy` が無いため未実行。CI `quality` job の対象。
- `linux-asan`: local environment に `clang` が無いため未実行。CI `linux-asan` job の対象。
- `windows-mingw-debug`: local environment に `x86_64-w64-mingw32-gcc` が無いため未実行。CI `windows-mingw` job の対象。

## 10. 実機実行条件

この work unit は docs、build workflow、agent 運用基盤の変更であり、Switch pairing、
HID advertising、report loop を実行しない。

実機検証は not applicable。

将来の実機 work unit では `hardware-harness` に従い、専用 USB Bluetooth ドングル、
WinUSB 割り当て、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、
`docs/hardware-test-log.md` への記録を必要条件にする。

## 11. チェックリスト

- [x] 変更範囲が初期 agent / docs / workflow 整備に収まっている。
- [x] Non-goals を実装していない。
- [x] `vendor/btstack` を変更していない。
- [x] protocol implementation を追加していない。
- [x] documentation-level の根拠監査を記録した。
- [x] local で可能な build / test を記録した。
- [x] sanitizer / clang-tidy / Windows cross build の未実行理由を記録した。
- [x] 実機未実行理由を記録した。
