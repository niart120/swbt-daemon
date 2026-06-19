# Windows Native Just Devcontainer

## 1. 概要

Windows native PowerShell から `just` recipe を実行し、Dev Container CLI 経由で container 内 recipe へ委譲できるかを確認する work unit。

`work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md` では、Windows host を WSL2 shell 経路と Windows native shell 経路に分けた。
この work unit は、未検証の Windows native shell 経路を標準入口に含められるか判断するための記録である。

## 2. 対象範囲

- Windows native PowerShell で `just --version` が実行できる前提を確認する。
- Windows native PowerShell で `devcontainer --version`、`devcontainer up`、`devcontainer exec` が使える前提を確認する。
- Docker Desktop と WSL2 backend の前提を確認する。
- repository が Windows filesystem にある場合と WSL2 filesystem にある場合の workspace path の扱いを確認する。
- 既存 `justfile` を Windows native PowerShell 経路でも使えるか、`windows-shell`、`[windows]` attribute、または PowerShell helper script が必要か判断する。
- `CTEST_ARGS` などの environment variable と argument の受け渡しを確認する。
- Windows native PowerShell から `just debug` または同等の確認用 recipe が host toolchain を使わず Dev Container CLI へ委譲することを確認する。

## 3. 対象外

- `just` 移行全体の実装。
- Linux、macOS、WSL2 shell 経路の検証。
- CMake presets の変更。
- Switch pairing、HID advertising、report loop、Bluetooth adapter 操作。
- Windows native daemon と WinUSB ドングルの実機検証。

## 4. 関連 spec / docs

- `spec/operations/just-task-runner-migration.md`
- `work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md`
- `.devcontainer/devcontainer.json`
- `.devcontainer/Dockerfile`
- `justfile`
- future Windows helper script if needed
- https://just.systems/man/en/
- https://just.systems/man/en/settings.html
- https://just.systems/man/en/attributes.html
- https://github.com/devcontainers/cli
- https://code.visualstudio.com/docs/devcontainers/devcontainer-cli
- https://code.visualstudio.com/docs/devcontainers/containers

## 5. 根拠監査

not applicable。

この work unit は Windows native shell からの task runner と Dev Container CLI 委譲の検証だけを扱う。
Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は変更しない。

## 6. 設計メモ

Windows native PowerShell 経路は、未検証のまま標準入口にしない。
まず確認用 recipe または helper script で、path quoting、environment variable、exit code、Dev Container CLI の workspace 解決を確認する。

この環境のように repository が WSL2 filesystem にある場合、Windows native PowerShell から見える path は `\\wsl$` または `\\wsl.localhost` 系になる可能性がある。
Dev Container CLI と Docker Desktop がその path を workspace folder として扱えるかは検証が必要である。

Windows native PowerShell 経路が不安定な場合、標準入口は WSL2 shell 内の `just` に寄せる。
その場合でも README と AGENTS には、Windows native PowerShell 経路が未対応または optional であることを明記する。

## 7. 対象ファイル

- `work-units/wip/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md`
- `spec/operations/just-task-runner-migration.md`
- `work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md`
- `justfile`
- future Windows helper script if needed
- future README / AGENTS updates

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | Windows native PowerShell can run `just --version` | verification | workflow | no |
| todo | Windows native PowerShell can run `devcontainer --version` | verification | workflow | no |
| todo | Docker Desktop with WSL2 backend is visible to Dev Container CLI | verification | workflow | no |
| todo | repository path is passed to `devcontainer up --workspace-folder` without quoting failure | verification | workflow | no |
| todo | `devcontainer exec --workspace-folder <repo> ...` runs a simple command inside the project container | verification | workflow | no |
| todo | Windows native `just` recipe propagates environment variables and arguments needed by test recipes | verification | workflow | no |
| todo | Windows native `just debug` or equivalent confirmation recipe delegates to Dev Container without host CMake/toolchain use | verification | workflow | no |
| todo | failure mode gives actionable guidance when Windows native prerequisites are missing | edge | workflow | no |

## 9. 検証

この record 作成時点では、Windows native PowerShell での `just`、Dev Container CLI、Docker Desktop、WSL2 backend、workspace path の動作は未実行である。

この repository 上では次だけを確認した。

- `git branch --show-current`: `docs/just-task-runner-migration`。
- `git status --short`: 開始時は clean。

完了時には、Windows native host の OS version、PowerShell version、`just` version、Dev Container CLI version、Docker Desktop version、WSL2 backend の有効状態、repository の置き場所、実行 command、結果をこの record に追記する。

## 10. 実機実行条件

実機検証は不要。

この work unit は Windows native shell と Dev Container CLI の開発環境検証であり、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を含まない。
専用 USB Bluetooth ドングル、WinUSB driver assignment、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1` は不要である。

## 11. チェックリスト

- [ ] Windows native PowerShell の `just` install path を確認した。
- [ ] Windows native PowerShell の Dev Container CLI install path を確認した。
- [ ] Docker Desktop と WSL2 backend の状態を確認した。
- [ ] Windows filesystem repository path の扱いを確認した。
- [ ] WSL2 filesystem repository path の扱いを確認した。
- [ ] Windows recipe を `justfile` だけで書けるか、PowerShell helper が必要か判断した。
- [ ] Windows native `just` から Dev Container 内 command を実行した。
- [ ] Windows native `just debug` を標準入口に含めるか、WSL2 shell 経路に限定するか判断した。
- [ ] README、AGENTS、移行 spec に判断を反映した。
- [ ] 検証結果または未実行理由を記録した。
- [ ] 実機状態を記録した。
