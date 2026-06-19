# Just Task Runner Migration

## 1. 状態

current。

この spec は、Makefile によるタスクランナー運用を `just` に移すための移行計画である。
`work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md` で実装した結果、現在の実行経路は `spec/operations/development-tooling.md` に記録された `just` 前提の方針である。
`Makefile` は削除済みである。

## 2. 目的

開発環境の前提を「WSL2 + Dev Container」から「マルチプラットフォーム host + Dev Container」に広げる。

この方針では、Linux、macOS、Windows の host から同じタスク入口を使う。
Dev Container は、C toolchain、formatter、static analysis、sanitizer、Windows cross build を再現する標準実行環境として残す。
Windows native + WinUSB + 専用 USB Bluetooth ドングルの実機検証は、引き続き別の実機実行条件で扱う。

## 3. 適用範囲

- `Makefile` で定義していた標準タスクを `justfile` に移す。
- `debug`、`format`、`format-check`、`tidy`、`asan`、`windows-cross`、`verify`、`verify-ci` の名前と意味を維持する。
- host からの標準タスクは Dev Container CLI へ委譲する。
- Dev Container 内では `just` が CMake、CTest、format script、static analysis を直接実行する。
- CI と Git hooks の標準入口を `just` に切り替える。
- 公開 docs、agent guidance、project skills のコマンド例を `just` に切り替える。

次は対象外である。

- CMake presets の再設計。
- BTstack source selection の変更。
- Switch pairing、HID advertising、report loop の実機実行。
- Windows native 実機検証の自動化。

## 4. 決定事項

標準タスクランナーは `just` にする。
`just` は command runner であり、build graph を持つ build system として使わない。
CMake と CTest は引き続き build と test の正本である。

`justfile` は repository root に置く。
初期 recipe は既存 `Makefile` の public target と同じ名前を持つ。
`just --list` が開発者向けの標準ヘルプになる。

`just` は host 側と Dev Container 側の両方に置く。
host 側の `just` は、Linux、macOS、WSL2 shell、Windows native PowerShell から開発者が叩く外側の入口である。
Dev Container 側の `just` は、container 内の CMake、CTest、format、static analysis、cross build を実行する内側の入口である。

host 上の `just <recipe>` は、標準検証では host toolchain を使わない。
Dev Container 内であれば、recipe は `cmake`、`ctest`、`scripts/format.sh`、`scripts/check-format.sh` を直接実行する。
Linux、macOS、WSL2 shell、Windows native PowerShell の Dev Container 外実行では、recipe は `devcontainer up` と `devcontainer exec` で同名の in-container recipe を実行する。
この委譲先を container 内の `just` recipe に寄せることで、host recipe、container recipe、CI command の重複を避ける。

Windows host は、Windows native shell と WSL2 shell を分けて扱う。
Windows filesystem 上の repository は、Windows native PowerShell から `just` を実行する経路を標準入口に含める。
WSL2 filesystem 上の repository は、Windows native PowerShell から UNC path として扱わず、WSL2 shell 内で `just` を実行する経路を標準とする。
Windows native shell では POSIX shell の存在を前提にしない。
`justfile` は `windows-shell` と OS 別 private recipe を使い、Windows native shell の委譲処理を PowerShell で表現する。
Dev Container 内の recipe は Linux container 上で動くため、既存 shell scripts を継続利用してよい。
Windows filesystem checkout から Linux Dev Container へ bind mount する場合も shell scripts が実行できるよう、`justfile` と `*.sh` は `.gitattributes` で LF に固定する。
container 内 recipe は、Windows bind mount の ownership 差で Git が拒否しないよう、workspace を idempotent に `safe.directory` へ登録してから実行する。

`SWBT_DEVCONTAINER=1` は container 内判定として残す。
`SWBT_ALLOW_HOST_BUILD=1` と `-DSWBT_ALLOW_HOST_BUILD=ON` は、ユーザが明示した unsupported host build opt-in として残す。
この opt-in は、標準の `just` recipe が自動で付けない。

`Makefile` は compatibility shim として残さず削除する。
CI、hooks、README、AGENTS、project skills は `just` に切り替える。
completed work unit record と initial docs に残る過去の `make` command は historical evidence として残す。

## 5. 根拠

`just` 公式 manual は、`just` を project-specific command を保存して実行する command runner と説明している。
同 manual は、Linux、macOS、Windows、その他の Unix 系を追加依存なしでサポートすると説明している。
同 manual は、Ubuntu 24.04 derivatives では `apt install just`、Windows では `winget`、Scoop、Chocolatey、macOS では Homebrew や MacPorts などの導入経路を示している。
同 manual は、`windows-shell`、`[windows]`、`[unix]`、`os_family()` を提供しており、OS 別 recipe を書ける。

Dev Container CLI の docs は、`devcontainer up` で container を作成し、`devcontainer exec` で running container 内の command を実行できると説明している。
同 docs は npm package として `@devcontainers/cli` を install できると説明している。
VS Code Dev Containers docs は、Windows では Docker Desktop と WSL2 backend を使う構成、および WSL2 内に置いた source code を扱う構成を説明している。

参照:

- https://just.systems/man/en/
- https://just.systems/man/en/packages.html
- https://just.systems/man/en/settings.html
- https://just.systems/man/en/attributes.html
- https://just.systems/man/en/functions.html
- https://github.com/devcontainers/cli
- https://code.visualstudio.com/docs/devcontainers/devcontainer-cli
- https://code.visualstudio.com/docs/devcontainers/containers

この work unit は task runner と開発運用の計画である。
Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は変更しない。
根拠監査は not applicable とする。

## 6. 関連 work units

- `work-units/complete/local_002/DEVCONTAINER_VERIFICATION_POLICY.md`
- `work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md`
- `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md`

## 7. 未解決事項

- Git hooks を Windows native host でも同じ tracked hook で動かすか、hook body から OS 別 helper script に分岐するか。
- host 側 `just` がない場合に、README の install guidance だけで足りるか、bootstrap script を用意するか。

## 8. 移行手順

1. host 側の `just` を標準入口の前提として README / AGENTS に記録する。
2. Dev Container image に `just` を追加し、container 内 recipe と CI command で使えるようにする。
3. `justfile` を追加し、既存 `Makefile` の public target と同じ recipe を実装する。
4. Linux、macOS、WSL2 shell からの `just <recipe>` が Dev Container CLI へ委譲されることを設計する。
5. Dev Container 内の `just <recipe>` が CMake presets、CTest presets、format script、clang-tidy、Windows MinGW cross build を直接実行することを確認する。
6. Windows native PowerShell からの `just <recipe>` は、`work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md` で検証済みであり、Windows filesystem checkout の標準入口に含める。
7. `.github/workflows/ci.yml` の `runCmd` を `just verify-ci` に切り替える。
8. `.githooks/pre-commit` と `.githooks/pre-push` を `just` 呼び出しに切り替える。
9. `README.md`、`AGENTS.md`、`.githooks/README.md`、`.agents/skills/` のコマンド例を `just` に更新する。
10. `scripts/require-dev-environment.sh` の案内から Makefile 前提の文言を外す。
11. `Makefile` を削除する。
12. `spec/operations/development-tooling.md` を `just` 前提の current spec に更新し、この spec を current として整理する。
13. 置き換え済みの current spec が残らないことを確認する。

## 9. 現行記述の修正方針

| path | 現行の問題 | 修正方針 |
|---|---|---|
| `AGENTS.md` | 主開発環境を WSL2 + Dev Containers とし、標準入口を Makefile target としている。 | 主開発環境を Linux/macOS/Windows + Dev Container と書き、Windows は WSL2 shell と Windows native shell を分ける。標準入口は `just` recipe にする。 |
| `README.md` | WSL2 前提、`make` コマンド例、Makefile が host から Dev Container CLI へ委譲する説明がある。 | 開発者の host OS を限定しない説明に変え、command examples を `just debug` などにする。Windows は Windows filesystem checkout の Windows native PowerShell と WSL2 filesystem checkout の WSL2 shell を標準入口として分ける。 |
| `spec/operations/development-tooling.md` | Makefile target を標準検証入口としている。 | `just` 前提の current spec として更新する。 |
| `.github/workflows/ci.yml` | Dev Container 内で `make verify-ci` を実行している。 | Dev Container に `just` を入れた後、`runCmd: just verify-ci` に変える。 |
| `.githooks/pre-commit` | `make list-presets` と `make format-check` を呼んでいる。 | `just list-presets` と `just format-check` を呼ぶ。 |
| `.githooks/pre-push` | `make debug` と `make verify` を呼んでいる。 | `just debug` と `just verify` を呼ぶ。 |
| `.githooks/README.md` | hook の説明が Makefile 前提である。 | hook の標準入口を `just` に変更する。Windows native Git hooks は PowerShell helper の要否を検証してから標準化する。 |
| `Makefile` | task runner の正本になっている。 | `justfile` へ移行し、`Makefile` は削除する。 |
| `.devcontainer/Dockerfile` | `make` は入っているが `just` が入っていない。 | Ubuntu 24.04 の package として `just` を追加する。`make` は C ecosystem と vendor 参照用に残してよい。 |
| `scripts/require-dev-environment.sh` | host から Makefile targets を使うよう案内している。 | host から `just` recipes を使う案内に変える。 |
| `CMakeLists.txt` | Dev Container 外の direct CMake build を unsupported host build として止めている。 | 標準 build は引き続き Dev Container 内で行うため、guard は維持する。`just` 移行後も自動で `SWBT_ALLOW_HOST_BUILD` を付けない。 |
| `scripts/format.sh` と `scripts/check-format.sh` | Dev Container 内で実行するよう案内している。 | formatter は Dev Container 内の標準 toolchain を使うため、基本方針は維持する。 |
| `.agents/skills/tdd-workflow/SKILL.md` | TDD の標準コマンドが `make` である。 | `just build-debug`、`just test-debug`、`just asan`、`just windows-cross` に更新する。 |
| `.agents/skills/pr-merge-cleanup/SKILL.md` | PR 前検証の標準入口を Makefile target としている。 | PR 前検証の標準入口を `just` recipe または CI 結果にする。 |
| `.agents/skills/agentic-self-review/SKILL.md` | 標準検証を Makefile target としている。 | 標準検証を `just` recipe または CI 結果にする。 |
| `work-units/wip/*.md` | 未完了 record の検証計画に `make` コマンド例が残る。 | 対象 work unit を再開するときに `just` コマンドへ更新する。 |
| `work-units/complete/*.md` | 過去の検証記録に `make` コマンドが残る。 | historical evidence として残す。過去に実行した command は書き換えない。 |
| `spec/initial/*.md` | 初期方針として WSL2 + Dev Containers や raw CMake command が残る。 | initial docs は履歴として残す。現在方針とは限らないことを README で既に説明しているため、原則として書き換えない。 |

## 10. 検証方針

移行実装 work unit では、少なくとも次を確認する。

- `just --list` が標準 recipe を表示する。
- host 側の Linux、macOS、WSL2 shell に `just` があり、外側の入口として使える。
- Dev Container 内で `just list-presets` が成功する。
- Dev Container 内で `just format-check` が成功する。
- Dev Container 内で `just debug` が configure、build、CTest を実行する。
- Dev Container 内で `just asan` が sanitizer build と CTest を実行する。
- Dev Container 内で `just windows-cross` が MinGW cross build を実行する。
- Dev Container 内で `just verify-ci` が CI 相当の非実機検証を実行する。
- Linux、macOS、WSL2 shell から `just debug` が host toolchain を使わず Dev Container CLI へ委譲する。
- Windows native PowerShell から `just debug` が Dev Container CLI へ委譲できることを確認し、Windows filesystem checkout の標準入口に含める。
- `.githooks/pre-commit` が `just list-presets` と必要時の `just format-check` を実行する。
- `.githooks/pre-push` が通常 `just debug`、`SWBT_FULL_PRE_PUSH=1` で `just verify` を実行する。
- CI が Dev Container 内で `just verify-ci` を実行する。
- 置き換え済みの tooling spec が current location に残らず、必要に応じて `spec/archive/` に移されている。

実機検証は不要である。
この移行は task runner と docs の変更であり、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を含まない。
