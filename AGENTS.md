# swbt-daemon Agent Guide

## Communication

- ユーザとの対話は日本語で行う。
- 技術文書と回答は事実ベースで簡潔に書く。
- 実機未検証、source audit 未完了、BTstack license 境界などの不確実性は明示する。

## Project Overview

`swbt-daemon` は、Nintendo Switch に Bluetooth Classic HID Device として Pro Controller 相当に見える daemon を実装する C/CMake プロジェクトである。

daemon は Bluetooth adapter、BTstack run loop、Switch Pro Controller protocol、HID report scheduler、local IPC server を所有する。クライアントは daemon IPC に controller state snapshot を送る。

## Current Scope

- C11 / CMake / Ninja を主経路にする。
- BTstack は `vendor/btstack` submodule として pin する。
- 自前コードは `api/`, `apps/`, `swbt/`, `tests/`, `docs/` に置く。
- daemon IPC は JSON Lines over local IPC を主経路にする。
- C ABI は daemon 内部、単体テスト、将来の代替組み込み経路として扱う。
- Windows native + 専用 USB Bluetooth dongle + WinUSB を実機検証の主経路にする。

## Non-goals

初期段階では次を実装しない。

- BTstack 本体の広範囲改変。
- `vendor/btstack` の直接編集。
- daemon protocol としての `tap`, `duration_ms`, `sequence`, `at_ms`。
- 複数 controller 同時接続。
- Joy-Con L/R、NFC/IR MCU、amiibo 対応。
- Python / C# / GUI client。
- binary release。

## Repository Layout

```text
api/                    public C ABI surface
apps/swbt-daemon/       daemon executable
cmake/                  build helpers and toolchains
docs/                   project notes and hardware logs
swbt/core/              project core utilities
swbt/switch/            Switch controller protocol code
swbt/btstack_bridge/    BTstack integration boundary
tests/                  C unit tests
vendor/btstack/         BTstack submodule
spec/                   work-unit specs and dev journal
.agents/skills/         project-local Codex skills
```

## Development Environment

- 主開発環境は WSL2 + Dev Containers とする。
- `.devcontainer/Dockerfile` は Ubuntu 24.04、CMake、Ninja、clang、clang-format、clang-tidy、mingw-w64、libusb headers、valgrind を含む再現環境である。
- `.devcontainer/devcontainer.json` は VS Code C/C++ と CMake Tools extension を推奨し、container user は `ubuntu` とする。
- Linux native build、sanitizer、unit test、Windows MinGW cross build は Dev Container 内で再現できる前提にする。
- Windows native は WinUSB driver、Bluetooth dongle、Switch pairing、latency / report rate 実測のための実機検証環境として別扱いにする。
- host OS へ個別 toolchain を手作業で入れることを通常の前提にしない。

## BTstack Policy

- BTstack は `vendor/btstack` submodule の pinned source として扱う。
- `vendor/btstack` を直接編集しない。
- BTstack に patch が必要な場合は、まず `swbt/btstack_bridge/` で吸収できないか検討する。
- fork や upstream patch が必要になった場合は、理由、対象 commit、代替案を `docs/upstream-btstack.md` と作業仕様に記録する。
- BTstack を含む binary / release を MIT-only artifact と表現しない。
- license / notice に触れる変更では `THIRD_PARTY_NOTICES.md` を確認する。

## Daemon IPC Policy

- daemon protocol は latest state snapshot を受け取る。
- daemon は時間指定 macro executor ではない。
- `tap`, `duration_ms`, `sequence`, `at_ms` は client-side helper の責務とする。
- owner disconnect、heartbeat timeout、daemon shutdown では neutral state を優先する。
- IPC parser と BTstack report scheduler の責務を分ける。

## Hardware Safety

- 実機 command、pairing、HID advertising、report loop は人間の明示承認なしに実行しない。
- 実機検証は専用 USB Bluetooth dongle で行い、普段使いの内蔵 Bluetooth や常用 dongle を使わない。
- Windows 実機検証では Zadig / WinUSB の割り当て状態を記録する。
- hardware test は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` のような明示条件を必要とする設計にする。
- 実機結果は `docs/hardware-test-log.md` に OS、dongle VID/PID、driver、BTstack commit、swbt commit、Switch firmware、report period、結果を記録する。

## Source Audit

Switch protocol、BTstack source selection、report timing、HID descriptor、subcommand、SPI address、rumble packet などの値を追加・変更するときは `.agents/skills/swbt-source-audit` を使う。

記録では、文献値、upstream 実装値、swbt 実装値、実機観測値、推定、未検証仮説を分ける。

## C / CMake Rules

- C standard は C11 とする。
- public headers は `api/`、internal headers は `swbt/` 配下に置く。
- CMake presets を主経路にする。
- compiler warnings と sanitizer helper は `cmake/` 配下の既存関数を使う。
- CMake 側で BTstack source list を追加する場合は source audit を先に行う。

## Testing And Verification

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

変更範囲に応じて、targeted CTest、sanitizer、cross build、hardware not-run reason を報告する。

## Documentation Rules

- 作業仕様は `spec/wip/local_{nnn}/FEATURE_NAME.md` に作る。
- 完了した作業仕様は `spec/complete/local_{nnn}/FEATURE_NAME.md` へ移す。
- 初期構想と長期方針は `spec/initial/` に置く。
- 小さい設計観測や先送り事項は `spec/dev-journal.md` に記録する。
- 実機観測は `docs/hardware-test-log.md` に記録する。
- `tmp/` は一時検討や移行中メモに限定し、恒久情報は `spec/` または `docs/` へ昇格する。

## Agent Skills

- `swbt-source-audit`: Switch HID / BTstack / hardware evidence を監査する。
- `swbt-hardware-harness`: Switch pairing や Bluetooth dongle 実機検証の安全境界を確認する。
- `swbt-spec-format`: work-unit 仕様を作成・更新する。
- `swbt-tdd-workflow`: CMake / CTest 前提で TDD を進める。
- `swbt-dev-journal`: `spec/dev-journal.md` に観測や先送り事項を記録する。
- `swbt-agentic-self-review`: PR 前や handoff 前に gate 結果を整理する。
- `pr-merge-cleanup`: PR 作成、merge、default branch 同期、branch cleanup を行う。

## Git / PR

- 変更を伴う作業では開始時に branch と `git status --short` を確認する。
- default branch への直接 commit は、ユーザの明示指示がある場合を除き避ける。
- PR では `.github/PULL_REQUEST_TEMPLATE.md` に従い、Testing、Hardware、Source Audit、BTstack / License impact を明記する。

## Commit Rules

Conventional Commits に準拠する。

```text
<type>(<scope>): <subject>
```

type は `feat` / `fix` / `docs` / `style` / `refactor` / `perf` / `test` / `build` / `ci` / `chore` / `revert` を使う。subject は日本語で記述し、末尾句点は付けない。
