# swbt-daemon エージェントガイド

## 対話

- ユーザとの対話は日本語で行う。
- 技術文書と回答は事実ベースで簡潔に書く。
- 実機未検証、根拠監査未完了、BTstack のライセンス境界などの不確実性は明示する。

## 表記方針

- 固有名詞、ファイル名、コマンド名、環境変数、API 名、protocol ID、status 値は英語のまま残してよい。
- branch、commit、merge、build、test、driver、adapter、backend、firmware など、Git や開発環境の一般語は、文脈に応じてブランチ、コミット、マージ、ビルド、テスト、ドライバー、アダプター、バックエンド、ファームウェアと書く。
- work unit、source audit、hardware gate、requirements、verification、evidence、cleanup など、プロジェクト運用の概念は、作業単位、根拠監査、実機実行条件、要件、検証、根拠、後片付けのように日本語で書く。
- 「run」「record」「check」「update」のような動作は英単語を名詞化せず、実行する、記録する、確認する、更新するのような日本語の用言で書く。

## プロジェクト概要

`swbt-daemon` は、Nintendo Switch に Bluetooth Classic HID Device として Pro Controller 相当に見える daemon を実装する C/CMake プロジェクトである。

daemon は Bluetooth アダプター、BTstack run loop、Switch Pro Controller protocol、HID report scheduler、local IPC server を所有する。
クライアントは daemon IPC にコントローラー状態スナップショットを送る。

## 現在の範囲

- C11 / CMake / Ninja を主経路にする。
- BTstack は `vendor/btstack` submodule として固定する。
- 自前コードは `api/`, `apps/`, `swbt/`, `tests/`, `docs/` に置く。
- daemon IPC は JSON Lines over local IPC を主経路にする。
- C ABI は daemon 内部、単体テスト、将来の代替組み込み経路として扱う。
- Windows native + 専用 USB Bluetooth ドングル + WinUSB を実機検証の主経路にする。

## 対象外

初期段階では次を実装しない。

- BTstack 本体の広範囲改変。
- `vendor/btstack` の直接編集。
- daemon protocol としての `tap`, `duration_ms`, `sequence`, `at_ms`。
- 複数 controller 同時接続。
- Joy-Con L/R、NFC/IR MCU、amiibo 対応。
- Python / C# / GUI client。
- binary release。

## リポジトリ構成

```text
api/                    public C ABI surface
apps/swbt-daemon/       daemon 実行ファイル
cmake/                  ビルド補助と toolchain
docs/                   プロジェクトメモと実機ログ
swbt/core/              プロジェクト共通 utility
swbt/switch/            Switch controller protocol code
swbt/btstack_bridge/    BTstack integration boundary
tests/                  C 単体テスト
vendor/btstack/         BTstack submodule
spec/                   作業仕様と開発ジャーナル
.agents/skills/         project-local Codex skills
```

## 開発環境

- 主開発環境は WSL2 + Dev Containers とする。
- `.devcontainer/Dockerfile` は Ubuntu 24.04、CMake、Ninja、clang、clang-format、clang-tidy、mingw-w64、libusb headers、valgrind を含む再現環境である。
- `.devcontainer/devcontainer.json` は VS Code C/C++ と CMake Tools extension を推奨し、container user は `ubuntu` とする。
- Linux native build、sanitizer、unit test、Windows MinGW cross build は Dev Container 内で再現できる前提にする。
- Windows native は WinUSB ドライバー、Bluetooth ドングル、Switch pairing、latency / report rate 実測のための実機検証環境として別扱いにする。
- host OS へ個別 toolchain を手作業で入れることを通常の前提にしない。

## BTstack 方針

- BTstack は `vendor/btstack` submodule の固定済み source として扱う。
- `vendor/btstack` を直接編集しない。
- BTstack に patch が必要な場合は、まず `swbt/btstack_bridge/` で吸収できないか検討する。
- fork や upstream patch が必要になった場合は、理由、対象コミット、代替案を `docs/upstream-btstack.md` と作業仕様に記録する。
- BTstack を含む binary / release を MIT-only artifact と表現しない。
- ライセンスや notice に触れる変更では `THIRD_PARTY_NOTICES.md` を確認する。

## Daemon IPC 方針

- daemon protocol は最新状態スナップショットを受け取る。
- daemon は時間指定 macro executor ではない。
- `tap`, `duration_ms`, `sequence`, `at_ms` は client-side helper の責務とする。
- owner disconnect、heartbeat timeout、daemon shutdown では neutral state を優先する。
- IPC parser と BTstack report scheduler の責務を分ける。

## 実機安全境界

- 実機コマンド、pairing、HID advertising、report loop は人間の明示承認なしに実行しない。
- 実機検証は専用 USB Bluetooth ドングルで行い、普段使いの内蔵 Bluetooth や常用ドングルを使わない。
- Windows 実機検証では Zadig / WinUSB の割り当て状態を記録する。
- 実機テストは `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` のような明示条件を必要とする設計にする。
- 実機結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を記録する。

## 根拠監査

Switch protocol、BTstack source selection、report timing、HID descriptor、subcommand、SPI address、rumble packet などの値を追加・変更するときは `.agents/skills/source-audit` を使う。

記録では、文献値、upstream 実装値、swbt 実装値、実機観測値、推定、未検証仮説を分ける。

## C / CMake ルール

- C standard は C11 とする。
- public header は `api/`、internal header は `swbt/` 配下に置く。
- CMake presets を主経路にする。
- compiler warning と sanitizer helper は `cmake/` 配下の既存関数を使う。
- CMake 側で BTstack source list を追加する場合は根拠監査を先に行う。

## テストと検証

通常のローカル検証:

```console
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

sanitizer:

```console
cmake --preset linux-asan
cmake --build --preset linux-asan
ctest --preset linux-asan
```

Windows cross build:

```console
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
```

変更範囲に応じて、targeted CTest、sanitizer、cross build、実機未実行理由を報告する。

## ドキュメントルール

- 作業仕様は `spec/wip/local_{nnn}/FEATURE_NAME.md` に作る。
- 完了した作業仕様は `spec/complete/local_{nnn}/FEATURE_NAME.md` へ移す。
- 初期構想と長期方針は `spec/initial/` に置く。
- 小さい設計観測や先送り事項は `spec/dev-journal.md` に記録する。
- 実機観測は `docs/hardware-test-log.md` に記録する。
- `tmp/` は一時検討や移行中メモに限定し、恒久情報は `spec/` または `docs/` へ昇格する。

## Agent Skills

- `source-audit`: Switch HID / BTstack / 実機根拠を監査する。
- `hardware-harness`: Switch pairing や Bluetooth ドングル実機検証の安全境界を確認する。
- `spec-format`: 作業単位仕様を作成・更新する。
- `tdd-workflow`: CMake / CTest 前提で TDD を進める。
- `dev-journal`: `spec/dev-journal.md` に観測や先送り事項を記録する。
- `agentic-self-review`: PR 前や handoff 前に判定結果を整理する。
- `pr-merge-cleanup`: PR 作成、マージ、既定ブランチ同期、ブランチの後片付けを行う。

## Git / PR

- 変更を伴う作業では開始時にブランチと `git status --short` を確認する。
- 既定ブランチへの直接コミットは、ユーザの明示指示がある場合を除き避ける。
- PR では `.github/PULL_REQUEST_TEMPLATE.md` に従い、テスト、実機、根拠監査、BTstack / License impact を明記する。

## Commit ルール

Conventional Commits に準拠する。

```text
<type>(<scope>): <subject>
```

type は `feat` / `fix` / `docs` / `style` / `refactor` / `perf` / `test` / `build` / `ci` / `chore` / `revert` を使う。
subject は日本語で記述し、末尾句点は付けない。
