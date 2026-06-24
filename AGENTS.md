# swbt-daemon エージェントガイド

## 対話

- ユーザとの対話は日本語で行う。
- 技術文書と回答は事実ベースで簡潔に書く。
- 実機未検証、根拠監査未完了、BTstack のライセンス境界などの不確実性は明示する。

## 表記方針

- 固有名詞、ファイル名、コマンド名、環境変数、API 名、protocol ID、status 値は英語のまま残してよい。
- branch、commit、merge、build、test、driver、adapter、backend、firmware など、Git や開発環境の一般語は、文脈に応じてブランチ、コミット、マージ、ビルド、テスト、ドライバー、アダプター、バックエンド、ファームウェアと書く。
- source audit、hardware gate、requirements、verification、evidence、cleanup など、プロジェクト運用の概念は、根拠監査、実機実行条件、要件、検証、根拠、後片付けのように日本語で書く。
- `work unit`、`work unit record`、`spec` は別の概念として英語のまま使う。
- 「run」「record」「check」「update」のような動作は英単語を名詞化せず、実行する、記録する、確認する、更新するのような日本語の用言で書く。

## 運用概念

- `work unit` は作業管理上のまとまりである。PR、handoff、検証、完了判定の単位になるが、文書そのものではない。
- `work unit record` は 1 つの `work unit` の範囲、関連する `spec` や docs、TDD list、検証結果、実機状態、根拠監査状態、チェックリストを束ねる記録である。
- `spec` は安定した設計、protocol、挙動、方針を書く文書である。1 つの `work unit` だけに閉じるとは限らず、複数の `work unit record` から参照されてもよい。
- `journal entry` は `work unit` や `spec` にするほど固まっていない観測や先送り判断である。
- 通常は `work unit` を先に立てる。複数の work unit から参照する設計や protocol が必要になったときだけ、`spec` を作成または更新する。

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
spec/                   安定した spec と開発ジャーナル
work-units/             work unit record
.agents/skills/         project-local Codex skills
```

## 開発環境

- 主開発環境は Linux、macOS、Windows host + Dev Containers とする。
- Windows では、Windows filesystem 上の repository は Windows native PowerShell から `just` を実行して Dev Container CLI へ委譲する経路を標準入口に含める。WSL2 filesystem 上の repository は WSL2 shell 内で `just` を実行する経路を標準とする。
- Windows native PowerShell 経路は `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md` で検証済みであり、host 側の `just`、Dev Container CLI、Docker Desktop WSL2 backend を前提にする。
- `.devcontainer/Dockerfile` は Ubuntu 24.04、`just`、CMake、Ninja、clang、clang-format、clang-tidy、mingw-w64、libusb headers、valgrind を含む再現環境である。
- `.devcontainer/devcontainer.json` は VS Code C/C++ と CMake Tools extension を推奨し、container user は `ubuntu` とする。
- Linux native build、sanitizer、unit test、Windows MinGW cross build は Dev Container 内で再現できる前提にする。
- ローカルの標準 configure、build、test、format、static analysis、cross build は `just` recipe を入口にする。host からの `just` recipe は Dev Container CLI へ委譲する。
- ローカルの host build は既定で止める。ユーザが Dev Container 外で host build を明示的に許可した場合だけ `SWBT_ALLOW_HOST_BUILD=1` または `-DSWBT_ALLOW_HOST_BUILD=ON` で opt-in する。
- Windows native は WinUSB ドライバー、Bluetooth ドングル、Switch pairing、latency / report rate 実測のための実機検証環境として別扱いにする。
- host OS へ個別 toolchain を手作業で入れることを通常の前提にしない。

## BTstack 方針

- BTstack は `vendor/btstack` submodule の固定済み source として扱う。
- `vendor/btstack` を直接編集しない。
- BTstack に patch が必要な場合は、まず `swbt/btstack_bridge/` で吸収できないか検討する。
- fork や upstream patch が必要になった場合は、理由、対象コミット、代替案を `docs/upstream-btstack.md` と spec に記録する。
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
- C source の整形は `scripts/format.sh` と `scripts/check-format.sh` で行う。
- clang-tidy は `linux-clang-tidy` preset で実行する。
- CMake 側で BTstack source list を追加する場合は根拠監査を先に行う。

## テストと検証

通常の debug build/test:

```console
just debug
```

`just debug` は `format-check` と `clang-tidy` を実行しない。

sanitizer:

```console
just asan
```

Windows cross build:

```console
just windows-cross
```

全体検証:

```console
just verify
```

`just verify` は `format-check`、`clang-tidy`、debug build/test、ASan、Windows cross build を実行する。

CMake / Ninja 生成物の削除:

```console
just clean
```

`just clean` は Git の除外対象である `build/` と `cmake-build-*` だけを削除する。`tmp/`、Dev Container、submodule には触れない。

変更範囲に応じて、targeted CTest、sanitizer、cross build、実機未実行理由を報告する。

## ドキュメントルール

- work unit record は `work-units/wip/local_{nnn}/FEATURE_NAME.md` に作る。
- 完了した work unit record は `work-units/complete/local_{nnn}/FEATURE_NAME.md` へ移す。
- 安定した spec は `spec/architecture/`、`spec/protocols/`、`spec/operations/` に置く。
- 外部資料や upstream 調査の要約は `spec/references/` に置く。ここは規範ではなく根拠である。
- 置き換え済みの spec は `spec/archive/` に移す。
- 初期構想と長期方針は `spec/initial/` に置く。
- 小さい設計観測や先送り事項は `spec/dev-journal.md` に記録する。
- 実機観測は `docs/hardware-test-log.md` に記録する。
- `tmp/` は一時検討や移行中メモに限定し、恒久情報は `work-units/`、`spec/`、`docs/` のいずれかへ昇格する。

## Agent Skills

- `source-audit`: Switch HID / BTstack / 実機根拠を監査する。
- `hardware-harness`: Switch pairing や Bluetooth ドングル実機検証の安全境界を確認する。
- `work-unit-record`: work unit record を作成・更新する。
- `spec-page`: 安定した spec page を作成・更新する。
- `tdd-workflow`: CMake / CTest 前提で TDD を進める。
- `tdd-test-list`: use case から TDD Test List を作成・更新する。
- `tdd-one-cycle`: TDD Test List の 1 item だけを red / green / refactor で進める。
- `refactor-after-green`: green 後の構造変更と `refactor-skipped` 判断を扱う。
- `tidy-first`: behavior change と structure change を分離する。
- `test-desiderata-review`: test の価値と trade-off を確認する。
- `dev-journal`: `spec/dev-journal.md` に観測や先送り事項を記録する。
- `agentic-self-review`: PR 前や handoff 前に判定結果を整理する。
- `pr-merge-cleanup`: PR 作成、マージ、既定ブランチ同期、ブランチの後片付けを行う。

## Git / PR

- 変更を伴う作業では開始時にブランチと `git status --short` を確認する。
- 既定ブランチへの直接コミットは、ユーザの明示指示がある場合を除き避ける。
- Git hooks は `.githooks/` を正本とし、clone 後は `sh scripts/install-git-hooks.sh` または `scripts/install-git-hooks.ps1` で有効化する。
- `pre-commit` は staged diff の whitespace、`just` 経由の CMake presets の読み取り、staged C source がある場合の format を確認する。
- `commit-msg` は Conventional Commits の形式と subject 末尾句点なしを確認する。
- `pre-push` は `just verify` を実行する。`format-check`、`clang-tidy`、debug build/test、ASan、Windows cross build を含む。host からは `justfile` が Dev Container CLI へ委譲する。
- `pre-push` は `SWBT_FAST_PRE_PUSH=1` のとき `just debug` だけを実行する。これは build/test only で、`format-check` と `clang-tidy` は含まない。
- `SWBT_SKIP_HOOKS=1` は hook 全体を明示的にスキップする。
- PR では `.github/PULL_REQUEST_TEMPLATE.md` に従い、テスト、実機、根拠監査、BTstack / License impact を明記する。

## Commit ルール

Conventional Commits に準拠する。

```text
<type>(<scope>): <subject>
```

type は `feat` / `fix` / `docs` / `style` / `refactor` / `perf` / `test` / `build` / `ci` / `chore` / `revert` を使う。
subject は日本語で記述し、末尾句点は付けない。
