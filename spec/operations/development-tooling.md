# Development Tooling

## 1. 状態

current。

この spec は、task runner、formatter、linter、Git hooks、CI quality job の選定理由と運用方針を定める。

対象は repository 全体の開発入口と、`api/`、`apps/`、`swbt/`、`tests/` にある自前の C source と header である。
`vendor/btstack/` は upstream source なので、formatter と linter の対象にしない。

## 2. 目的

Linux、macOS、Windows host から同じ標準タスク入口を使い、実際の C toolchain は Dev Container 内で再現する。

host OS の未管理 toolchain、system include、tool version の差で configure、build、test、format、static analysis、cross build の結果が揺れることを避ける。

## 3. 適用範囲

- host と Dev Container 内の標準 command entry。
- CMake presets、CTest presets、format script、clang-tidy、sanitizer、Windows cross build の実行入口。
- Git hooks と GitHub Actions CI の検証入口。
- host build opt-in の扱い。

次は対象外である。

- CMake presets の再設計。
- BTstack source selection。
- Windows native PowerShell からの `just` 実行可否の最終判断。
- Switch pairing、HID advertising、report loop、Bluetooth adapter 操作。

## 4. 決定事項

標準タスクランナーは `just` とする。
repository root の `justfile` を標準入口にし、`just --list` を開発者向けの recipe 一覧にする。

標準 recipe は次を提供する。

- `list-presets`
- `configure-debug`
- `build-debug`
- `test-debug`
- `debug`
- `format`
- `format-check`
- `tidy`
- `asan`
- `windows-cross`
- `verify`
- `verify-ci`
- `devcontainer-up`
- `devcontainer-rebuild`

Dev Container 内では、recipe が CMake、CTest、format script、clang-tidy、Windows MinGW cross build を直接実行する。
Dev Container 外の Linux、macOS、WSL2 shell では、recipe が Dev Container CLI の `devcontainer up` と `devcontainer exec` に委譲し、container 内の同等 recipe を実行する。
host 側 recipe は `SWBT_ALLOW_HOST_BUILD=1` を自動で付けない。

Windows では、WSL2 shell 内で repository を開いて `just` を実行する経路を標準とする。
Windows native PowerShell からの `just` 実行は `work-units/wip/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md` で検証するまで標準入口に含めない。

`Makefile` は標準入口から外し、削除する。
completed work unit record や initial docs に残る過去の `make` command は historical evidence として残す。

Dev Container image は `just`、CMake、Ninja、clang、clang-format、clang-tidy、mingw-w64、libusb headers、valgrind を含む。
Dev Container 定義を変更した後の再作成は `just devcontainer-rebuild` で行う。

Dev Container 外で host build する場合は、ユーザが明示的に `SWBT_ALLOW_HOST_BUILD=1` または `-DSWBT_ALLOW_HOST_BUILD=ON` を付けて実行する。
この host build は unsupported opt-in であり、primary verification として扱わない。

formatter は `clang-format` を使う。
初期ルールは `.clang-format` に置く。
format check は `just format-check`、自動整形は `just format` で実行する。

linter と static analysis は `clang-tidy` を使う。
初期ルールは `.clang-tidy` に置く。
`clang-tidy` は `just tidy` で実行する。

Git hooks は `.githooks/` を正本とする。
`pre-commit` は staged diff の whitespace、`just list-presets`、staged C source がある場合の `just format-check` を確認する。
`pre-push` は通常 `just debug` を実行し、`SWBT_FULL_PRE_PUSH=1` のとき `just verify` を実行する。

CI では Dev Container 内で `just verify-ci` を実行する。

## 5. 根拠

`just` への移行判断と現行記述の修正方針は `spec/operations/just-task-runner-migration.md` に記録した。

`just` 公式 manual は、`just` を project-specific command を保存して実行する command runner と説明している。
同 manual は Linux、macOS、Windows の導入経路と OS 別 recipe の機能を説明している。

Dev Container CLI の docs は、`devcontainer up` で container を作成し、`devcontainer exec` で running container 内の command を実行できると説明している。
VS Code Dev Containers docs は、Windows では Docker Desktop と WSL2 backend を使う構成、および WSL2 内に置いた source code を扱う構成を説明している。

参照:

- `justfile`
- `.devcontainer/Dockerfile`
- `.github/workflows/ci.yml`
- `.githooks/`
- https://just.systems/man/en/
- https://just.systems/man/en/packages.html
- https://just.systems/man/en/settings.html
- https://just.systems/man/en/attributes.html
- https://just.systems/man/en/functions.html
- https://github.com/devcontainers/cli
- https://code.visualstudio.com/docs/devcontainers/devcontainer-cli
- https://code.visualstudio.com/docs/devcontainers/containers

この spec は task runner と開発運用を扱う。
Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は変更しない。
根拠監査は not applicable とする。

## 6. 関連 work units

- `work-units/complete/local_002/DEVCONTAINER_VERIFICATION_POLICY.md`
- `work-units/complete/local_031/JUST_TASK_RUNNER_MIGRATION_PLAN.md`
- `work-units/wip/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md`

## 7. 未解決事項

- Windows native PowerShell からの `just` 実行可否、workspace path、environment variable、exit code の扱いは `work-units/wip/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md` で検証する。
- host 側 `just` がない場合に README の install guidance だけで足りるか、bootstrap script を用意するかは未決定である。
