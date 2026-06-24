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
- Windows native PowerShell と WSL2 shell の標準入口の使い分け。

次は対象外である。

- CMake presets の再設計。
- BTstack source selection。
- Windows native daemon と WinUSB ドングルの実機検証。
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
- `clean`

Dev Container 内では、recipe が CMake、CTest、format script、clang-tidy、Windows MinGW cross build を直接実行する。
Dev Container 外の Linux、macOS、WSL2 shell では、recipe が Dev Container CLI の `devcontainer up` と `devcontainer exec` に委譲し、container 内の同等 recipe を実行する。
Windows filesystem 上の checkout では、Windows native PowerShell の `just` も同じく Dev Container CLI へ委譲する。
host 側 recipe は `SWBT_ALLOW_HOST_BUILD=1` を自動で付けない。

`just debug` は linux-debug preset の configure、build、CTest を実行する fast debug loop である。`just debug` は `format-check` と `clang-tidy` を実行しない。

formatter / linter を含む非実機 gate は `just verify` と `just verify-ci` である。`just verify` は `format-check`、`tidy`、`debug`、`asan`、`windows-cross` を順に実行する。

Windows native CI job は追加しない。現在の CI gate は Ubuntu runner 上の Dev Container で `just verify-ci` を実行する。
Windows native PowerShell entrypoint は local gate として扱い、Windows filesystem checkout で `just list-presets` を実行して Dev Container 委譲と CMake preset 読み取りを最小確認する。
広い確認が必要な場合は、同じ Windows native PowerShell 入口で `just verify` を実行する。

`just clean` は host 側 workspace の Git 除外対象である CMake build 出力だけを削除する。対象は `build/` と `cmake-build-*` であり、`tmp/`、Dev Container、submodule には触れない。

Windows では、Windows filesystem 上の repository は Windows native PowerShell から `just` を実行する経路を標準入口に含める。
WSL2 filesystem 上の repository は、Windows native PowerShell から UNC path として扱わず、WSL2 shell 内で `just` を実行する経路を標準とする。
Windows native PowerShell 経路は `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md` で検証済みである。

Dev Container image は `just`、CMake、Ninja、clang、clang-format、clang-tidy、mingw-w64、libusb headers、valgrind を含む。
Dev Container 定義を変更した後の再作成は `just devcontainer-rebuild` で行う。
Windows filesystem checkout から Linux Dev Container へ bind mount するため、`justfile` と shell scripts は `.gitattributes` で LF に固定する。
container 内 recipe は、Git が Windows bind mount を dubious ownership として拒否しないよう、実行前に workspace を idempotent に `safe.directory` へ登録する。

Dev Container 外で host build する場合は、ユーザが明示的に `SWBT_ALLOW_HOST_BUILD=1` または `-DSWBT_ALLOW_HOST_BUILD=ON` を付けて実行する。
この host build は unsupported opt-in であり、primary verification として扱わない。

formatter は `clang-format` を使う。
初期ルールは `.clang-format` に置く。
format check は `just format-check`、自動整形は `just format` で実行する。
format script は `api/`、`apps/`、`swbt/`、`tests/` の tracked C source/header と、まだ commit 前の untracked C source/header を対象にする。
`.gitignore` などで除外される生成物や一時ファイルは formatter の対象にしない。

linter と static analysis は `clang-tidy` を使う。
初期ルールは `.clang-tidy` に置く。
`clang-tidy` は `just tidy` で実行する。

Git hooks は `.githooks/` を正本とする。
`pre-commit` は staged diff の whitespace、`just list-presets`、staged C source がある場合の `just format-check` を確認する。
`pre-push` は通常 `just verify` を実行する。`just verify` は `format-check`、`tidy`、`debug`、`asan`、`windows-cross` を含む。
軽い push 前確認だけにしたい場合は `SWBT_FAST_PRE_PUSH=1` を使い、`just debug` だけを実行する。`SWBT_FAST_PRE_PUSH=1` は hook skip ではなく build/test only の明示 opt-in である。
hook 全体を明示的にスキップする場合は `SWBT_SKIP_HOOKS=1` を使う。

CI では Dev Container 内で `just verify-ci` を実行する。
CI job は `SWBT_RUN_HARDWARE=0` と `SWBT_HARDWARE_APPROVED=0` を明示し、実機承認を必要とする処理を既定で開始しない。

## 5. 根拠

`just` 公式 manual は、`just` を project-specific command を保存して実行する command runner と説明している。
同 manual は Linux、macOS、Windows の導入経路と OS 別 recipe の機能を説明している。

Dev Container CLI の docs は、`devcontainer up` で container を作成し、`devcontainer exec` で running container 内の command を実行できると説明している。
VS Code Dev Containers docs は、Windows では Docker Desktop と WSL2 backend を使う構成、および WSL2 内に置いた source code を扱う構成を説明している。

参照:

- `justfile`
- `.gitattributes`
- `.codex/rules/default.rules`
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
- `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md`
- `work-units/complete/local_041/FORMAT_UNTRACKED_C_SOURCES.md`
- `work-units/complete/local_068/JUST_DEBUG_TIDY_CONTRACT.md`
- `work-units/complete/local_069/PRE_PUSH_VERIFY_DEFAULT.md`

## 7. 未解決事項

- host 側 `just` がない場合に README の install guidance だけで足りるか、bootstrap script を用意するかは未決定である。
- Windows native Git hooks を追加で実行確認するかは未決定である。
