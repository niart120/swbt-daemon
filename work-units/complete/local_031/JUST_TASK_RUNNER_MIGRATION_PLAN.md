# Just Task Runner Migration Plan

## 1. 概要

Makefile によるタスクランナー運用を `just` に移す計画を作成する work unit。

目的は、開発環境の説明を「WSL2 + Dev Container」から「マルチプラットフォーム host + Dev Container」に広げることである。
この work unit では移行計画と現行記述の修正方針を文書化し、`justfile` の実装は行わない。

## 2. 対象範囲

- `just` 採用の目的を整理する。
- `Makefile` から `justfile` へ移す recipe 群を整理する。
- Dev Container を標準実行環境として残す範囲を整理する。
- Linux、macOS、Windows host からの実行入口を整理する。
- 方針と反する現行記述を調査する。
- 現行記述の修正方針を `spec/operations/just-task-runner-migration.md` に記録する。

## 3. 対象外

- `justfile` の追加。
- `Makefile` の削除または shim 化。
- CI、Git hooks、README、AGENTS、project skills の実更新。
- CMake presets の変更。
- 実機検証。

## 4. 関連 spec / docs

- `spec/operations/just-task-runner-migration.md`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_002/DEVCONTAINER_VERIFICATION_POLICY.md`
- `AGENTS.md`
- `README.md`
- `.github/workflows/ci.yml`
- `.githooks/README.md`
- `.githooks/pre-commit`
- `.githooks/pre-push`
- `.devcontainer/Dockerfile`
- `Makefile`
- `scripts/require-dev-environment.sh`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/pr-merge-cleanup/SKILL.md`
- `.agents/skills/agentic-self-review/SKILL.md`

## 5. 根拠監査

not applicable。

この work unit は task runner と開発環境方針の文書化だけを扱う。
Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は変更しない。

`just` の platform support と導入経路は、公式 manual を確認した。

- https://just.systems/man/en/
- https://just.systems/man/en/packages.html
- https://just.systems/man/en/settings.html
- https://just.systems/man/en/attributes.html
- https://just.systems/man/en/functions.html

## 6. 設計メモ

`just` は標準タスクランナーとして採用する。
CMake と CTest は build と test の正本として残す。
Dev Container は再現可能な C toolchain 環境として残す。

host からの標準 recipe は host toolchain を使わず Dev Container CLI へ委譲する。
Dev Container 内の recipe は CMake presets、CTest presets、format script、clang-tidy、Windows MinGW cross build を直接実行する。

Windows host では POSIX shell を前提にしない。
`justfile` 実装時は `windows-shell`、`[windows]`、`[unix]`、または OS 別 helper script を使う。

既存の completed work unit record に残る `make` command は historical evidence として残す。
未完了 record は、対象 work unit を再開するときに `just` command へ更新する。

## 7. 対象ファイル

- `spec/operations/just-task-runner-migration.md`
- `spec/operations/README.md`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | `just` 移行の目的、範囲、対象外が spec に記録されている | new | docs | no |
| green | Makefile から `justfile` へ移す recipe 群が spec に記録されている | new | docs | no |
| green | マルチプラットフォーム host + Dev Container の方針が spec に記録されている | new | docs | no |
| green | 方針と反する現行記述と修正方針が spec に記録されている | new | docs | no |
| green | 根拠監査と実機状態が work unit record に記録されている | regression | docs | no |

## 9. 検証

- `git branch --show-current`: `main`。
- `git status --short`: 開始時は clean。
- `rg -n "Makefile|make |just|WSL2|Dev Container|devcontainer|host build|pre-push|pre-commit|verify-ci" ...`: `README.md`、`AGENTS.md`、`spec/operations/development-tooling.md`、`.github/workflows/ci.yml`、`.githooks/`、`Makefile`、`.agents/skills/`、`scripts/require-dev-environment.sh` に対象記述があることを確認した。
- `git diff --check`: pass。
- `rg -n "Just Task Runner Migration|現行記述の修正方針|マルチプラットフォーム host"`: pass。
- `rg -n "[ \t]+$" spec/operations/just-task-runner-migration.md work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md`: no matches。
- `rg -n "\t" spec/operations/just-task-runner-migration.md work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md`: no matches。

`justfile` は未追加であるため、`just --list` と `just` recipe の実行はこの work unit では実行していない。
コード、CMake、CI、hook は変更していないため、`make debug`、`make verify`、`just verify` は実行していない。

## 10. 実機実行条件

実機検証は不要。

この work unit は docs と計画だけを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 11. チェックリスト

- [x] `just` 移行計画を operations spec に記録した。
- [x] マルチプラットフォーム host + Dev Container の方針を記録した。
- [x] Makefile 前提と WSL2 前提の現行記述を調査した。
- [x] 現行記述の修正方針を整理した。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
