# swbt-daemon

`swbt-daemon` は、Nintendo Switch に Bluetooth Classic HID Device として接続し、Pro Controller 相当のコントローラーとして認識されることを目指す daemon です。
daemon は local IPC interface を公開し、デバッグ用ツール、テスト実行環境、将来追加する言語 binding から使うことを想定しています。

## 状態

現在の状態表は [docs/status.md](docs/status.md) に置きます。巨大な実機ログは証跡として [docs/hardware-test-log.md](docs/hardware-test-log.md) を参照します。

### 確認済み

| 項目 | 現在の状態 | 根拠 |
|---|---|---|
| 既定起動 | `SWBT_DAEMON_BACKEND` 未指定では noop backend で起動し、Bluetooth アダプターを開かない。 | `apps/swbt-daemon/main.c` |
| production 起動条件 | `SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1` をそろえた場合だけ production backend が実機経路へ進む。 | `apps/swbt-daemon/main.c`, `swbt/daemon/production_backend.c` |
| 既知の対応構成 | Windows native、CSR8510 A10、WinUSB、Switch 2 firmware `22.1.0`（実機ログ表記は Switch2）、production backend。 | `docs/hardware-test-log.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` |
| Switch-facing 挙動 | pairing、HID L2CAP open、subcommand reply、`0x21` reply 後の Switch UI 入力反映を観測済み。 | `docs/hardware-test-log.md` |
| neutral fail-safe | owner disconnect、heartbeat timeout、shutdown で neutral report へ戻ることを実機観測済み。architecture cutover 後の H1 でも owner disconnect neutral と shutdown trailing neutral を確認済み。 | `docs/hardware-test-log.md`, `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md` |
| architecture cutover | `swbt_app_t`、`swbt_daemon_host_t`、能力別 port group を持つ `swbt_btstack_production_adapter_t` を唯一の実装経路にし、旧 session、mailbox、runtime、production backend ops、`swbt_core` を削除済み。 | `spec/architecture/daemon-architecture-cutover.md`, `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`, `work-units/complete/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md` |

### 未確認

| 項目 | 現在の扱い |
|---|---|
| 初代Switch各モデル | 実機ログなし。 |
| 他のUSBドングル | CSR8510 A10 以外の実機ログなし。 |
| Linux実機経路 | build / test の経路はあるが、Linux + libusb の実機実行は未記録。 |
| daemon再起動後の bonded reconnect | persistence と再接続成功を確認した実機ログなし。 |
| 厳密な遅延・jitter・取りこぼし率 | report period comparison は画面遷移ベースの粗い受理確認であり、測定値ではない。 |

### 未実装

| 項目 | 現在の扱い |
|---|---|
| daemon protocol の時間指定 macro | `tap`、`duration_ms`、`sequence`、`at_ms` は daemon protocol に含めない。 |
| 複数 controller 同時接続 | 初期範囲外。 |
| NFC/IR MCU / amiibo の意味処理 | `0x21` reply は bring-up sequence 用の応答であり、NFC/IR の意味状態は未実装。 |
| rumble 周波数 / 振幅の意味変換 | raw payload の扱いに留まり、actuator-safe conversion は未実装。 |

### production 起動条件

実機へ触れる production 起動では、専用 USB Bluetooth ドングルを使い、内蔵 Bluetooth や普段使いのドングルを対象にしません。Windows native では検証対象ドングルの driver が WinUSB に割り当たっていることを確認してから実行します。

| 環境変数 | 値 | 役割 |
|---|---|---|
| `SWBT_DAEMON_BACKEND` | `production` | noop backend ではなく production backend を選ぶ。 |
| `SWBT_RUN_HARDWARE` | `1` | 実機実行を意図した起動であることを明示する。 |
| `SWBT_HARDWARE_APPROVED` | `1` | 人間が対象 adapter と実行範囲を承認済みであることを明示する。 |
| `SWBT_IPC_PORT` | 例: `37637` | IPC client が接続する固定 port。未指定時の既定は `0`。 |
| `SWBT_REPORT_PERIOD_US` | 例: `8000` | input report period。既定は `8000`。 |
| `SWBT_DEVICE_INFO_PROFILE` | 例: `swbt-pro` | device info profile。未指定時も `swbt-pro` を使う。 |

## アーキテクチャ

想定している実行モデルは次のとおりです。

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

このモデルでは、責務を次のように分けます。production backend は実装済みですが、実機へ触れる起動は明示的な環境変数を必要条件にします。

- `swbt_app_t` は single active owner、latest controller state、sequence、rumble、status counters を所有します。
- IPC 層は transport、framing、JSON codec、client identity、application command mapping を扱います。
- `swbt_daemon_host_t` は application、IPC、BTstack adapter、report timer、startup / shutdown / cleanup ordering を構成します。
- BTstack production adapter は pinned BTstack の callback、HID send、run loop、HCI transport を扱い、application の内部状態を直接持ちません。
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

通常の debug configure / build / test:

```bash
just debug
```

`just debug` は `format-check` と `clang-tidy` を実行しません。formatter / linter を含める場合は `just verify` を使います。

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

`just verify` は `format-check`、`clang-tidy`、debug build/test、ASan、Windows cross build を実行します。

Git hooks は `.githooks/` に置いています。
clone 後、次のコマンドを一度だけ実行して有効化してください。

```bash
sh scripts/install-git-hooks.sh
```

hooks の挙動は次のとおりです。

- `pre-commit` は staged diff の whitespace、`just` 経由の CMake presets、staged C source の format を確認します。
- `commit-msg` は Conventional Commits の形式と subject 末尾句点なしを確認します。
- `pre-push` は `just verify` を実行します。`format-check`、`clang-tidy`、debug build/test、ASan、Windows cross build を含みます。
- `SWBT_FAST_PRE_PUSH=1` を指定すると `just debug` だけを実行します。これは build/test only で、`format-check` と `clang-tidy` は含みません。
- `SWBT_SKIP_HOOKS=1` は hook 全体を明示的にスキップします。

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
apps/swbt-debug-client/ debug IPC client
cmake/                  ビルド補助と toolchain
docs/                   プロジェクトメモと実機ログ
swbt/core/              version、logging、diagnostics、metrics
swbt/application/       owner、controller state、rumble、status counters
swbt/daemon/            daemon host、config、IPC runner、production backend
swbt/ipc/               JSON Lines over local TCP IPC
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
