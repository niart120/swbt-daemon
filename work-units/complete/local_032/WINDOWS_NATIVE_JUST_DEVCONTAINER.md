# Windows Native Just Devcontainer

## 1. 概要

Windows native PowerShell から `just` recipe を実行し、Dev Container CLI 経由で container 内 recipe へ委譲できることを確認した work unit。

`work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md` では、Windows host を WSL2 shell 経路と Windows native shell 経路に分けた。
この work unit では、Windows filesystem 上の checkout では Windows native PowerShell の `just` を標準入口に含め、WSL2 filesystem 上の checkout は WSL2 shell 内の `just` を標準入口にする、と判断した。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md` の未解決事項。
- `spec/archive/just-task-runner-migration.md` の Windows native PowerShell 経路確認。
- ユーザ要求: `$tdd-workflow に従って work-units\wip\local_032 を完了させる。`

use case:

- actor: Windows native PowerShell の開発者。
- 入力 / 状態: repository が Windows filesystem 上の checkout にあり、Windows host に `just`、Dev Container CLI、Docker Desktop WSL2 backend がある。
- 期待する観測結果: `just` public recipe が POSIX `sh` を前提にせず、Dev Container CLI の `up` と `exec` を通じて container 内 recipe を実行する。`CTEST_ARGS` は container 内 CTest に渡る。
- 制約: Dev Container 外の host CMake / compiler / CTest を標準検証に使わない。WSL2 filesystem 上の checkout は Windows native PowerShell から UNC path として扱わず、WSL2 shell 内の `just` を使う。
- 対象外: Windows native daemon、WinUSB ドングル、Switch pairing、HID advertising、report loop。

## 3. 対象範囲

- Windows native PowerShell で `just --version` が実行できる前提を確認する。
- Windows native PowerShell で `devcontainer --version`、`devcontainer up`、`devcontainer exec` が使える前提を確認する。
- Docker Desktop と WSL2 backend の前提を確認する。
- repository が Windows filesystem にある場合の workspace path の扱いを確認する。
- repository が WSL2 filesystem にある場合の標準入口を WSL2 shell に寄せる判断を記録する。
- 既存 `justfile` を Windows native PowerShell 経路でも使えるか、`windows-shell`、`[windows]` attribute、または PowerShell helper script が必要か判断する。
- `CTEST_ARGS` などの environment variable と argument の受け渡しを確認する。
- Windows native PowerShell から `just debug` または同等の確認用 recipe が host toolchain を使わず Dev Container CLI へ委譲することを確認する。

## 4. 対象外

- `just` 移行全体の実装。
- Linux、macOS、WSL2 shell 経路の検証。
- CMake presets の変更。
- Switch pairing、HID advertising、report loop、Bluetooth adapter 操作。
- Windows native daemon と WinUSB ドングルの実機検証。
- Windows native Git hooks の追加検証。

## 5. 関連 spec / docs

- `spec/operations/development-tooling.md`
- `spec/archive/just-task-runner-migration.md`
- `work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md`
- `.devcontainer/devcontainer.json`
- `.devcontainer/Dockerfile`
- `justfile`
- `README.md`
- `AGENTS.md`
- https://just.systems/man/en/
- https://just.systems/man/en/settings.html
- https://just.systems/man/en/attributes.html
- https://github.com/devcontainers/cli
- https://code.visualstudio.com/docs/devcontainers/devcontainer-cli
- https://code.visualstudio.com/docs/devcontainers/containers

## 6. 根拠監査

not applicable。

この work unit は Windows native shell からの task runner と Dev Container CLI 委譲の検証だけを扱う。
Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は変更しない。

## 7. 設計メモ

初期 red では、Windows native PowerShell から `just list-presets` を実行すると、`justfile` の `set shell := ["sh", "-eu", "-c"]` により `sh` が見つからず失敗した。

`justfile` は、Linux / macOS / WSL2 shell / Dev Container 内では POSIX shell recipe を維持し、Windows native PowerShell では `set windows-shell` と PowerShell 用 private recipe で Dev Container CLI を呼ぶ形にした。
PowerShell helper script は追加しなかった。

Windows filesystem checkout では、repository root を `devcontainer up --workspace-folder` と `devcontainer exec --workspace-folder` に渡す経路が通る。
WSL2 filesystem checkout は Windows native PowerShell から `\\wsl$` / `\\wsl.localhost` path を渡す標準経路にせず、WSL2 shell 内で repository を開いて `just` を実行する経路に寄せる。

Windows filesystem checkout では shell scripts が CRLF になると Linux container 内の shebang 実行が失敗するため、`.gitattributes` で `justfile` と `*.sh` を LF に固定した。
また、Windows bind mount では Git が repository ownership を dubious と判定することがあるため、container 内 recipe の共通前処理で workspace を `safe.directory` に idempotent に登録する。

## 8. 対象ファイル

- `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md`
- `spec/operations/development-tooling.md`
- `spec/archive/just-task-runner-migration.md`
- `work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md`
- `.gitattributes`
- `.codex/rules/default.rules`
- `justfile`
- `README.md`
- `AGENTS.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | Windows native PowerShell can run `just --version` | verification | workflow | no |
| green | Windows native PowerShell can run `devcontainer --version` | verification | workflow | no |
| green | Docker Desktop with WSL2 backend is visible to Dev Container CLI | verification | workflow | no |
| green | Windows filesystem repository path is passed to `devcontainer up --workspace-folder` without quoting failure | verification | workflow | no |
| green | WSL2 filesystem repository path is assigned to WSL2 shell rather than Windows native PowerShell standard entry | edge | workflow | no |
| green | `devcontainer exec --workspace-folder <repo> ...` runs a simple command inside the project container | verification | workflow | no |
| green | Windows native `just` recipe propagates environment variables and arguments needed by test recipes | verification | workflow | no |
| green | Windows native `just debug` delegates to Dev Container without host CMake/toolchain use | verification | workflow | no |
| green | failure mode gives actionable guidance when Windows native prerequisites are missing | edge | workflow | no |
| green | Windows filesystem checkout keeps `justfile` and shell scripts LF for Linux container execution | regression | workflow | no |
| green | container recipes register the workspace as Git `safe.directory` before git-backed scripts run | regression | workflow | no |
| green | project-local Codex rules allow documented non-hardware `just` recipes and idempotent `just devcontainer-up` without allowing direct Docker/devcontainer, rebuild, or hardware commands | regression | workflow | no |

TDD status:

- source: `work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md` の未解決事項とユーザ要求。
- use case: Windows filesystem checkout の Windows native PowerShell から `just` public recipe を実行し、Dev Container 内 recipe へ委譲する。
- item: Windows native `just debug` delegates to Dev Container without host CMake/toolchain use。
- state: green。
- commands: `just list-presets`, `just debug`, `$env:CTEST_ARGS='-N'; just test-debug`, `devcontainer exec --workspace-folder . pwd`。
- notes: read `tdd-workflow`, `tdd-test-list`, `tdd-one-cycle`, `work-unit-record`。初期 red は `just list-presets` が `sh` 不在で失敗したこと。green では `set windows-shell` と Windows private recipe を追加した。

## 10. 検証

検証環境:

- OS: Microsoft Windows NT 10.0.26200.0。
- PowerShell: 7.6.2。
- repository path: repository root（repo-relative `.`）。
- `just`: `PATH 上の just.exe`, version `1.53.0`。
- Dev Container CLI: `PATH 上の devcontainer.ps1`, version `0.87.0`。
- Docker: Docker Desktop `4.78.0`, client/server `29.5.3`, context `desktop-linux`。
- Docker engine: Linux `x86_64`, kernel `6.6.87.2-microsoft-standard-WSL2`。
- WSL: version `2.6.3.0`、kernel `6.6.87.2-1`。
- submodule: `vendor/btstack` initialized at `075a0780f0fad7ff67d58ac19f46e8953656a752` before final `just debug`。

commands:

- `just --version`: pass, `just 1.53.0`。
- `devcontainer --version`: pass, `0.87.0`。
- `docker version`: pass, Docker Desktop Linux engine was reachable through context `desktop-linux`。
- `docker info`: pass, WSL2 kernel and Docker Desktop Linux engine confirmed。
- initial `just list-presets`: expected red, failed because Windows native `just` could not find `sh`。
- final `just --list`: pass, public recipes remained `asan`, `build-debug`, `configure-debug`, `debug`, `default`, `devcontainer-rebuild`, `devcontainer-up`, `format`, `format-check`, `help`, `list-presets`, `test-debug`, `tidy`, `verify`, `verify-ci`, `windows-cross`。
- final `just list-presets`: pass, Dev Container CLI returned container `0d255ddc1b4f8193df4d1a915f21ebb8a04a936a56fde45b2e761ded55256178` and ran `cmake --list-presets` in Dev Container workspace root。
- first `just debug` after justfile fix: failed at CMake configure because `vendor/btstack` was not initialized. This was not a Windows native delegation failure.
- `git submodule update --init --recursive`: pass after running through Git Bash with elevated filesystem access; `vendor/btstack` checked out at `075a0780f0fad7ff67d58ac19f46e8953656a752`。
- final `just debug`: pass. Dev Container内で `cmake --fresh --preset linux-debug`、`cmake --build --preset linux-debug`、`ctest --preset linux-debug --output-on-failure` が実行され、13/13 tests passed。
- `$env:CTEST_ARGS='-N'; just test-debug`: pass. Container 内 CTest が 13 tests を listing し、argument propagation を確認した。
- `devcontainer exec --workspace-folder . pwd`: pass, output was Dev Container workspace root。
- `$env:DEVCONTAINER_CLI='swbt-missing-devcontainer-cli'; just list-presets`: expected failure, exit 127 with `devcontainer CLI was not found. Install the Dev Containers CLI or open this repository in the Dev Container.`。
- first `just verify`: failed at `scripts/check-format.sh` with `sh: 1: scripts/check-format.sh: not found` because Windows checkout had CRLF shell scripts. Added `.gitattributes` and normalized `justfile` / `scripts/*.sh` working tree to LF。
- second `just verify`: failed at `scripts/check-format.sh` with Git dubious ownership for Dev Container workspace root. Added container recipe preparation that registers the workspace as `safe.directory` when needed。
- final `just verify`: pass. Windows native PowerShell delegated to Dev Container and ran `format-check`, `tidy`, `debug` with 13/13 tests, `asan` with 13/13 tests, and `windows-cross` successfully。
- `.codex/rules/default.rules` check: pass. Replaced stale Makefile allow rule with documented non-hardware `just` recipes and `just devcontainer-up`, kept `just devcontainer-rebuild`, direct Docker/devcontainer, hardware-facing, and destructive commands outside the allowlist。
- `just devcontainer-up`: pass. Existing Dev Container returned success with container `0d255ddc1b4f8193df4d1a915f21ebb8a04a936a56fde45b2e761ded55256178`, confirming the allowed recipe is idempotent for the current workspace state。

## 11. 実機実行条件

実機検証は不要。

この work unit は Windows native shell と Dev Container CLI の開発環境検証であり、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を含まない。
専用 USB Bluetooth ドングル、WinUSB driver assignment、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1` は不要である。

## 12. 先送り事項

none。

Windows native Git hooks の追加検証はこの work unit の対象外であり、現時点で新しい follow-up source は作らない。

## 13. チェックリスト

- [x] Windows native PowerShell の `just` install path を確認した。
- [x] Windows native PowerShell の Dev Container CLI install path を確認した。
- [x] Docker Desktop と WSL2 backend の状態を確認した。
- [x] Windows filesystem repository path の扱いを確認した。
- [x] WSL2 filesystem repository path の扱いを確認した。
- [x] Windows recipe を `justfile` だけで書けるか、PowerShell helper が必要か判断した。
- [x] Windows native `just` から Dev Container 内 command を実行した。
- [x] Windows native `just debug` を標準入口に含めるか、WSL2 shell 経路に限定するか判断した。
- [x] Windows filesystem checkout の LF / shell script 実行境界を固定した。
- [x] Windows bind mount の Git dubious ownership に対する container 内前処理を追加した。
- [x] `.codex/rules` の非実機 `just` recipe allowlist を更新した。
- [x] README、AGENTS、移行 spec に判断を反映した。
- [x] 検証結果または未実行理由を記録した。
- [x] 実機状態を記録した。
