# Just Task Runner Migration

## 1. 概要

Makefile によるタスクランナー運用を `just` に移した work unit。

標準入口は repository root の `justfile` とし、host 側 `just` は Dev Container CLI へ委譲する。
Dev Container 内の `just` は CMake presets、CTest presets、format script、clang-tidy、sanitizer、Windows MinGW cross build を直接実行する。

`Makefile` は compatibility shim として残さず削除した。
Windows native PowerShell からの `just` 実行可否は `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md` に分離し、未検証のまま標準入口に含めない。

## 2. 対象範囲

- `just` 採用の目的を整理する。
- `Makefile` で提供していた public target 相当の recipe を `justfile` に実装する。
- Dev Container を標準実行環境として残す。
- Linux、macOS、WSL2 shell からの host recipe が Dev Container CLI へ委譲する構造を実装する。
- Dev Container image、CI、Git hooks、README、AGENTS、project skills を `just` 前提に更新する。
- 現行記述の修正方針を `spec/archive/just-task-runner-migration.md` に記録する。
- current tooling policy を `spec/operations/development-tooling.md` に記録する。
- Windows native PowerShell 経路を `local_032` に分離し、標準入口に含めない判断を記録する。

## 3. 対象外

- CMake presets の変更。
- Switch pairing、HID advertising、report loop、Bluetooth adapter 操作。
- Windows native PowerShell からの `just` 実行可否の最終判断。
- Windows native 実機検証。

## 4. 関連 spec / docs

- `spec/archive/just-task-runner-migration.md`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_002/DEVCONTAINER_VERIFICATION_POLICY.md`
- `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md`
- `AGENTS.md`
- `README.md`
- `.github/workflows/ci.yml`
- `.githooks/README.md`
- `.githooks/pre-commit`
- `.githooks/pre-push`
- `.devcontainer/Dockerfile`
- `justfile`
- `scripts/require-dev-environment.sh`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/pr-merge-cleanup/SKILL.md`
- `.agents/skills/agentic-self-review/SKILL.md`

## 5. 根拠監査

not applicable。

この work unit は task runner と開発環境方針を扱う。
Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は変更しない。

`just` の platform support と導入経路は、公式 manual を確認した。

- https://just.systems/man/en/
- https://just.systems/man/en/packages.html
- https://just.systems/man/en/settings.html
- https://just.systems/man/en/attributes.html
- https://just.systems/man/en/functions.html
- https://github.com/devcontainers/cli
- https://code.visualstudio.com/docs/devcontainers/devcontainer-cli
- https://code.visualstudio.com/docs/devcontainers/containers

## 6. 設計メモ

`just` は標準タスクランナーとして採用した。
CMake と CTest は build と test の正本として残した。
Dev Container は再現可能な C toolchain 環境として残した。

host 側の `just` は、Linux、macOS、WSL2 shell から叩く外側の入口である。
Dev Container 外の recipe は host toolchain を使わず、Dev Container CLI の `devcontainer up` と `devcontainer exec` に委譲する。
委譲先では container 内の `_*-in-container` recipe を実行する。

Dev Container 側の `just` は、CMake presets、CTest presets、format script、clang-tidy、Windows MinGW cross build を直接実行する。
`CTEST_ARGS` は environment variable として `test-debug` と `asan` に渡す。

Windows host は、WSL2 shell 内実行と Windows native PowerShell 実行を分けて扱う。
この環境では WSL2 shell からの `just debug` を検証した。
Windows native PowerShell からの `just` は、`local_032` で path quoting、environment variable、Dev Container CLI、Docker Desktop、WSL2 backend を確認するまで標準入口に含めない。

既存の completed work unit record と initial docs に残る `make` command は historical evidence として残す。
未完了 record は、対象 work unit を再開するときに `just` command へ更新する。

## 7. 対象ファイル

- `justfile`
- `.devcontainer/Dockerfile`
- `.github/workflows/ci.yml`
- `.githooks/README.md`
- `.githooks/pre-commit`
- `.githooks/pre-push`
- `README.md`
- `AGENTS.md`
- `scripts/require-dev-environment.sh`
- `spec/archive/just-task-runner-migration.md`
- `spec/operations/development-tooling.md`
- `spec/operations/README.md`
- `work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md`
- `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/pr-merge-cleanup/SKILL.md`
- `.agents/skills/agentic-self-review/SKILL.md`
- `Makefile` deletion

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | `just` 移行の目的、範囲、対象外が spec に記録されている | new | docs | no |
| refactor-done | Makefile から `justfile` へ移す recipe 群が spec に記録されている | new | docs | no |
| refactor-done | マルチプラットフォーム host + Dev Container の方針が spec に記録されている | new | docs | no |
| refactor-done | 方針と反する現行記述と修正方針が spec に記録されている | new | docs | no |
| refactor-done | host 側 `just` の install guidance が README / AGENTS に記録されている | new | docs | no |
| refactor-done | Dev Container image に `just` が入っている | new | workflow | no |
| refactor-done | `justfile` が既存 Makefile public target 相当の recipe を提供する | new | workflow | no |
| refactor-done | WSL2 shell から `just debug` が Dev Container CLI へ委譲する | new | workflow | no |
| deferred | Windows native PowerShell からの `just debug` 実行可否を `local_032` で確認する | verification | workflow | no |
| refactor-done | CI と Git hooks が `just` recipe を標準入口にする | regression | workflow | no |
| refactor-done | README、AGENTS、project skills が `just` 前提に更新されている | regression | docs | no |
| refactor-done | 置き換え済みの tooling spec が current spec として更新済みである | regression | docs | no |

TDD status:

- item: `justfile` が既存 Makefile public target 相当の recipe を提供する
- state: refactor-done
- commands:
  - `just --list`: red。`error: No justfile found`。
  - `just --list`: green。public recipe 一覧を表示した。
  - `env SWBT_DEVCONTAINER=1 just list-presets`: green。container 内分岐で `cmake --list-presets` を実行した。
  - `just list-presets`: green。host から Dev Container CLI へ委譲した。
  - `just debug`: refactor-done。host から Dev Container CLI へ委譲し、linux-debug configure/build と CTest 13/13 が通った。
- notes: 初回の `just list-presets` は既存 container に `just` がなく失敗した。`.devcontainer/Dockerfile` 更新後に `just devcontainer-rebuild` を実行して解消した。

## 9. 検証

- `git branch --show-current`: `docs/just-task-runner-migration`。
- `git status --short`: 開始時は clean。
- `command -v just`: `/usr/bin/just`。
- `just --list`: red。`No justfile found`。
- `just --list`: pass。`debug`、`format-check`、`tidy`、`asan`、`windows-cross`、`verify`、`verify-ci` などの public recipe を表示した。
- `env SWBT_DEVCONTAINER=1 just list-presets`: pass。
- `just list-presets`: 初回は既存 Dev Container に `just` がなく `env: 'just': No such file or directory` で fail。
- `just devcontainer-rebuild`: pass。Dev Container image に Ubuntu package `just 1.21.0-1` を install し、container を再作成した。
- `just list-presets`: pass。host から Dev Container CLI へ委譲し、container 内で `cmake --list-presets` を実行した。
- `just verify-ci`: pass。Dev Container 内で `format-check`、`linux-clang-tidy` configure/build、`linux-debug` configure/build/CTest、`linux-asan` configure/build/CTest、`windows-mingw-debug` configure/build を実行した。`linux-debug` と `linux-asan` は CTest 13/13 pass。
- `just debug`: pass。WSL2 shell から Dev Container CLI へ委譲し、`linux-debug` CTest 13/13 pass。
- `.githooks/pre-commit`: pass。`git diff --cached --check` と `just list-presets` を実行した。
- `.githooks/pre-push`: pass。`just debug` を実行し、CTest 13/13 pass。
- `git diff --check`: pass。
- `just --list`: pass。移動後の worktree でも public recipe 一覧を表示した。

未実行:

- macOS host からの `just debug` は、この環境に macOS host がないため未実行。
- Windows native PowerShell からの `just debug` は `local_032` に分離したため未実行。

## 10. 実機実行条件

実機検証は不要。

この work unit は task runner、Dev Container、CI、hooks、docs だけを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。
専用 USB Bluetooth ドングル、WinUSB driver assignment、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1` は不要である。

## 11. チェックリスト

- [x] `just` 移行計画を operations spec に記録した。
- [x] マルチプラットフォーム host + Dev Container の方針を記録した。
- [x] Makefile 前提と WSL2 前提の現行記述を調査した。
- [x] 現行記述の修正方針を整理した。
- [x] Windows host の WSL2 shell 経路と Windows native shell 経路を分けた。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] host 側 `just` の install guidance を README / AGENTS に記録した。
- [x] Dev Container image に `just` を追加した。
- [x] `justfile` を追加した。
- [x] `Makefile` を削除した。
- [x] CI と Git hooks を `just` に切り替えた。
- [x] README、AGENTS、project skills を `just` 前提に更新した。
- [x] 置き換え済みの tooling spec を current spec として更新した。
- [x] WSL2 shell からの host 委譲を検証した。
- [x] Windows native PowerShell からの `just` 実行可否を `local_032` に分離し、未検証のまま標準入口に含めない判断を記録した。
