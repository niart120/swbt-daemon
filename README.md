# swbt-daemon

`swbt-daemon` は、Nintendo Switch に Bluetooth Classic HID Device として接続し、Pro Controller 相当に見えることを目指す daemon です。
daemon は local IPC interface を公開し、デバッグ用ツール、テスト実行環境、将来の各言語 binding から使うことを想定しています。

## 状態

このリポジトリは初期開発段階です。
daemon はまだ Switch と pairing できず、実機挙動も未検証です。
現時点のリポジトリは、最小構成、ビルド入口、ライセンス方針、設計メモを置く段階にとどまります。

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

daemon は Bluetooth アダプター、BTstack run loop、Switch protocol state、HID report scheduler を所有します。
client は controller state snapshot 全体を送ります。
daemon protocol は `tap`、`duration_ms`、`sequence`、`at_ms` のような時間指定 command を含みません。

## 開発環境

主開発環境は WSL2 + Dev Containers です。
Dev Container は CMake、Ninja、コンパイラー、MinGW、libusb 開発ヘッダー、解析ツールを含みます。

サポート対象のローカル build は Dev Container 内で実行します。
通常の host build は、手元の未管理 toolchain に結果が依存することを避けるため、既定で止めます。
CI は例外として許可します。
Dev Container 外で明示的に build する場合は、`SWBT_ALLOW_HOST_BUILD=1` または `-DSWBT_ALLOW_HOST_BUILD=ON` を指定します。

実機検証は Windows native build で行う想定です。
その場合は専用 USB Bluetooth ドングルを Zadig で WinUSB に割り当てます。

通常の build:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

sanitizer build:

```bash
cmake --preset linux-asan
cmake --build --preset linux-asan
ctest --preset linux-asan
```

Windows cross build:

```bash
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
```

Git hooks は `.githooks/` に置いています。
clone 後に一度だけ次を実行してください。

```bash
sh scripts/install-git-hooks.sh
```

`pre-commit` は staged diff の whitespace、CMake presets、staged C source の format を確認します。
`pre-push` は `linux-debug` の fresh configure、ビルド、テストを実行します。
`SWBT_FULL_PRE_PUSH=1` を指定すると、format check、clang-tidy、sanitizer、Windows cross build も実行します。

format と lint のコマンド:

```bash
scripts/format.sh
scripts/check-format.sh
cmake --fresh --preset linux-clang-tidy
cmake --build --preset linux-clang-tidy
```

## BTstack 依存

BTstack は `vendor/btstack` の Git submodule として扱います。

```bash
git submodule update --init --recursive
```

現在の CMake 構成は submodule がなくても configure できます。
実際の Bluetooth 機能には `vendor/btstack` のソースファイルが必要になります。

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
