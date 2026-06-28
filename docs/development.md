# Development

この文書は、`swbt-daemon` の開発、検証、repository 構成を扱う。利用者向けの概要と起動方法は root [README.md](../README.md) に置く。

## 開発環境

主開発環境は Linux、macOS、Windows host + Dev Containers とする。

Windows filesystem 上の repository では、Windows native PowerShell から `just` を実行し、`justfile` が Dev Container CLI へ委譲する経路を標準入口に含める。WSL2 filesystem 上の repository では、WSL2 shell 内で `just` を実行する。

Windows native PowerShell 経路は [work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md](../work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md) で検証済みである。

Dev Container には次のツールを含める。

- just
- CMake
- Ninja
- C / C++ compiler
- MinGW
- libusb development headers
- formatter / static analysis tools

host 側には `just` と Dev Container CLI が必要である。Dev Container CLI は VS Code Dev Containers または `@devcontainers/cli` で用意する。

```console
git submodule update --init --recursive
```

Dev Container 外の host build は通常の検証経路に含めない。host build を明示的に許可する場合だけ、`SWBT_ALLOW_HOST_BUILD=1` または `-DSWBT_ALLOW_HOST_BUILD=ON` を使う。

`.devcontainer/` を変更した後は、必要に応じて次を実行する。

```console
just devcontainer-rebuild
```

## ビルドとテスト

通常の debug configure / test target build / test:

```console
just debug
```

`just debug` は linux-debug の unit test executable target を build して CTest を実行する。`format-check` と `clang-tidy` は実行しない。

test executable だけの build:

```console
just build-tests-debug
```

daemon executable だけの debug build:

```console
just build-daemon-debug
```

debug client executable だけの debug build:

```console
just build-debug-client
```

sanitizer:

```console
just asan
```

Windows cross build:

```console
just windows-cross
```

Windows release build:

```console
just build-windows-release
just release-build
```

Windows release package:

```console
just package-windows-release
scripts/smoke-windows-release.ps1
```

全体検証:

```console
just verify
```

`just verify` は `format-check`、`clang-tidy`、fresh configure からの debug test target build/test、ASan、Windows cross build を実行する。

CI は `.devcontainer/devcontainer.json` を使い、Dev Container 内で `just verify-ci` を実行する。

CMake / Ninja 生成物の削除:

```console
just clean
```

`just clean` は Git の除外対象である `build/` と `cmake-build-*` だけを削除する。`tmp/`、Dev Container、submodule には触れない。

format と lint:

```console
just format
just format-check
just tidy
```

formatter と linter の選定理由は [spec/operations/development-tooling.md](../spec/operations/development-tooling.md) に記録している。

## Git Hooks

Git hooks は `.githooks/` に置く。clone 後、次のコマンドを一度だけ実行する。

```console
sh scripts/install-git-hooks.sh
```

Windows PowerShell では次を使える。

```console
scripts/install-git-hooks.ps1
```

hook の挙動:

- `pre-commit` は staged diff の whitespace、`just` 経由の CMake presets、staged C source の format を確認する。
- `commit-msg` は Conventional Commits の形式と subject 末尾句点なしを確認する。
- `pre-push` は `just verify` を実行する。
- `SWBT_FAST_PRE_PUSH=1` を指定すると `just debug` だけを実行する。
- `SWBT_SKIP_HOOKS=1` は hook 全体を明示的にスキップする。

## BTstack 依存

BTstack は `vendor/btstack` の Git submodule として扱う。`vendor/btstack` を直接編集しない。

現在の CMake 構成は submodule がなくても configure が通る。実際の Bluetooth 機能には `vendor/btstack` のソースファイルが必要である。

BTstack を含む binary や source distribution を MIT-only artifact と表現しない。release artifact と bundled notice policy は [spec/operations/release-license-boundary.md](../spec/operations/release-license-boundary.md) に記録している。

## アーキテクチャ概要

想定している実行モデル:

```text
client application
  CLI / tests / future language bindings
  |
  | JSON Lines over local IPC
  v
swbt-daemon
  - IPC transport / JSON codec
  - application-owned owner / latest controller state
  - Switch Pro Controller protocol
  - daemon host lifecycle / shutdown ordering
  - BTstack HID Device adapter
  - WinUSB / libusb production adapter backend
  |
  v
Nintendo Switch
```

主な責務境界:

- `swbt_domain_t` は single active owner、latest controller state、sequence、rumble、status counters を所有する。
- `swbt/control` は public C ABI と daemon IPC から来る操作を domain / runtime へ渡す。
- `swbt/ipc` は transport、framing、JSON codec、client identity、command mapping を扱う。
- `swbt_runtime_host_t` は HID registration、output handler、report timer、neutral shutdown、runtime resource status を扱う。
- `swbt_daemon_process_t` は IPC runner と runtime host の composition root であり、startup / shutdown / cleanup ordering を構成する。
- `swbt_daemon_production_runner_t` は production lifecycle をまとめ、BTstack-facing 操作は `swbt_btstack_production_ports_t` 経由で呼ぶ。
- BTstack production implementation は pinned BTstack の callback、HID send、run loop、HCI transport を扱い、domain の内部状態を直接持たない。

current architecture policy は [spec/architecture/daemon-architecture-cutover.md](../spec/architecture/daemon-architecture-cutover.md) に置く。

## リポジトリ構成

```text
api/                    public C ABI
apps/swbt-daemon/       daemon 実行ファイル
apps/swbt-debug-client/ debug IPC client
cmake/                  ビルド補助と toolchain
docs/                   プロジェクトメモと実機ログ
swbt/control/           public C ABI / IPC operation boundary
swbt/domain/            owner、controller state、rumble、status counters
swbt/runtime/           HID runtime host、report timer、neutral shutdown
swbt/daemon/            daemon process、config、IPC runner、production runner
swbt/ipc/               JSON Lines over local TCP IPC
swbt/support/           version、diagnostics、logging、metrics
swbt/switch/            Switch controller protocol code
swbt/btstack_bridge/    BTstack integration boundary
tests/                  C unit tests
vendor/btstack/         BTstack submodule
vendor/toml11/          toml11 submodule
spec/                   stable specs と development journal
work-units/             work unit records
.agents/skills/         project-local Codex skills
```

## リリース作業

Release build、package layout、license / notice gate は [spec/operations/release-build-and-publish.md](../spec/operations/release-build-and-publish.md) と [spec/operations/release-license-boundary.md](../spec/operations/release-license-boundary.md) に従う。

`.github/workflows/release.yml` は `v*` tag push で `just verify-ci`、`just package-windows-release`、Windows package smoke を実行し、draft GitHub Release へ zip と checksum を添付する。

tag push と GitHub Release publish は別の承認境界として扱う。tag push は release workflow を起動して draft release を作る。draft release の assets、checksum、license / notice、実機状態または未実行理由を確認してから publish する。
