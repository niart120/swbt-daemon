# swbt-daemon

`swbt-daemon` は、Nintendo Switch に Bluetooth Classic HID Device として接続し、Pro Controller 相当のコントローラーとして認識されることを目指す daemon です。
daemon は local IPC interface を公開し、デバッグ用ツール、テスト実行環境、将来追加する言語 binding から使うことを想定しています。

## 状態

現在の状態は次のとおりです。

- このリポジトリは初期開発段階です。
- daemon は Switch との pairing にはまだ対応しておらず、実機での挙動も未検証です。
- 現時点では、IPC server、JSON Lines protocol、Switch HID report core、BTstack bridge、runtime lifecycle、C unit tests を含みます。
- 現行の `apps/swbt-daemon/main.c` は noop backend で runtime lifecycle を起動して終了します。実 Bluetooth アダプター、実 IPC listener、Switch pairing、HID advertising、periodic report loop はまだ daemon 実行ファイルから起動しません。

## アーキテクチャ

想定している実行モデルは次のとおりです。

```text
client application
  CLI / tests / future language bindings
  |
  | JSON Lines over local IPC
  v
swbt-daemon
  - IPC server
  - single active owner
  - latest controller state
  - Switch Pro Controller protocol
  - BTstack HID Device bridge
  - WinUSB / libusb adapter backend
  |
  v
Nintendo Switch
```

このモデルでは、責務を次のように分けます。これは production backend を含む最終的な daemon 実行モデルであり、現行の daemon 実行ファイルは noop backend で起動します。

- daemon runtime は Bluetooth アダプター、BTstack run loop、Switch protocol state、HID report scheduler を所有する境界を定めます。
- クライアントはコントローラー状態の snapshot 全体を送ります。
- daemon protocol は `tap`、`duration_ms`、`sequence`、`at_ms` のような時間指定コマンドを含みません。

## 開発環境

主開発環境は Linux、macOS、Windows host + Dev Containers です。
Windows では、Windows filesystem 上の repository は Windows native PowerShell から `just` を実行して Dev Container CLI へ委譲する経路を標準入口に含めます。
WSL2 filesystem 上の repository は、WSL2 shell 内で `just` を実行する経路を標準とします。
Windows native PowerShell 経路は、`work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md` で検証しています。
Dev Container には主に次のツールを含めています。

- just
- CMake
- Ninja
- コンパイラー
- MinGW
- libusb 開発ヘッダー
- 解析ツール

build / test 環境の扱いは次のとおりです。

- ローカルでの configure、build、test、format、static analysis、cross build は、Dev Container 内で行うことを標準とします。
- Dev Container 外の host build は通常の検証経路に含めません。手元の未管理 toolchain によって結果が変わることを避けるためです。
- host で `just` recipes を実行した場合は、`justfile` が Dev Container CLI へ委譲します。
- `.devcontainer/` を変更した後は、必要に応じて `just devcontainer-rebuild` で既存コンテナを作り直します。
- CI も `.devcontainer/devcontainer.json` を使い、Dev Container 内で検証します。
- Dev Container 外で host build する場合は、ユーザが明示的に `SWBT_ALLOW_HOST_BUILD=1` または `-DSWBT_ALLOW_HOST_BUILD=ON` を付けて実行します。

host 側には `just` と Dev Container CLI が必要です。
Ubuntu 24.04 系では `sudo apt install just`、macOS では `brew install just`、Windows では `winget install Casey.Just`、Scoop、Chocolatey などを使えます。
Dev Container CLI は VS Code Dev Containers または `@devcontainers/cli` で用意します。

実機検証の前提は次のとおりです。

- Windows native build を使い、専用 USB Bluetooth ドングル経由で Switch と接続します。
- [Zadig](https://zadig.akeo.ie/) を使い、検証用ドングルのドライバーを WinUSB に差し替えてください。
- 内蔵 Bluetooth や普段使いのドングルのドライバーは差し替えないでください。

通常の configure / build / test:

```bash
just debug
```

sanitizer configure / build / test:

```bash
just asan
```

Windows cross build:

```bash
just windows-cross
```

全体検証:

```bash
just verify
```

Git hooks は `.githooks/` に置いています。
clone 後、次のコマンドを一度だけ実行して有効化してください。

```bash
sh scripts/install-git-hooks.sh
```

hooks の挙動は次のとおりです。

- `pre-commit` は staged diff の whitespace、`just` 経由の CMake presets、staged C source の format を確認します。
- `commit-msg` は Conventional Commits の形式と subject 末尾句点なしを確認します。
- `pre-push` は `just debug` を実行します。
- `SWBT_FULL_PRE_PUSH=1` を指定すると `just verify` を実行します。

format と lint のコマンド:

```bash
just format
just format-check
just tidy
```

formatter と linter の選定理由は `spec/operations/development-tooling.md` に記録しています。

## BTstack 依存

BTstack は `vendor/btstack` の Git submodule として扱います。

```bash
git submodule update --init --recursive
```

現在の CMake 構成は submodule がなくても configure が通ります。
実際の Bluetooth 機能には `vendor/btstack` のソースファイルが必要です。

## リポジトリ構成

```text
api/                    public C ABI
apps/swbt-daemon/       daemon 実行ファイル
cmake/                  ビルド補助と toolchain
docs/                   プロジェクトメモと実機ログ
swbt/core/              プロジェクト共通 utility
swbt/switch/            Switch controller protocol code
swbt/btstack_bridge/    BTstack integration boundary
tests/                  C unit tests
vendor/btstack/         BTstack submodule
spec/                   安定した spec と開発ジャーナル
work-units/             work unit record
.agents/skills/         project-local Codex skills
```

## License

Original swbt-daemon project files are licensed under the MIT License. See
`LICENSE`.

BTstack is a third-party dependency with its own license terms. Builds and
source distributions that include or link BTstack are also subject to the
BTstack license. Such builds are intended for personal, non-commercial use
unless a separate commercial BTstack license is obtained from BlueKitchen.

See `THIRD_PARTY_NOTICES.md`.
