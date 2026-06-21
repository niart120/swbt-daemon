# Codebase Environment Dependency Audit

## 1. 概要

コードベース全体に残っている環境変数依存、実験用 code path、観測用 trace / dump 経路を分類し、未設定時に通常の build / test / daemon 起動が壊れないことを確認する work unit。

この work unit は、環境変数を消すことを目的にしない。目的は、未設定でも既定値で動くべき設定、未設定なら止めるべき実機 gate、設定された場合だけ働く診断経路を分け、必要なら小さく refactor することである。

## 2. 起点 / ユースケース

source:

- 2026-06-21 ユーザ要求。デッドコード、実験用コード、観測用だけに生やした環境変数依存が残置していないか確認し、環境変数が無いとコードが実行できない状態を避けたい。
- `spec/architecture/daemon-runtime-boundaries.md`。現行 entrypoint は既定で `swbt_daemon_runtime_noop_backend()` を選び、`SWBT_DAEMON_BACKEND=production` のときだけ production backend を選ぶ。
- `spec/operations/development-tooling.md`。host build は Dev Container 経路を標準とし、direct host build は `SWBT_ALLOW_HOST_BUILD` または CMake option の明示 opt-in にする。
- `spec/operations/windows-native-preflight.md`。実機実行は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を必要条件にする。
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`。`SWBT_DEVICE_INFO_PROFILE`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`, `SWBT_IPC_HEARTBEAT_TIMEOUT_MS` を使った実機観測が残っている。

use case:

- actor: maintainer。
- 入力または状態: clean checkout、環境変数未設定の通常 build / test、環境変数未設定の `swbt-daemon` 起動、production hardware mode の明示 opt-in、診断 trace / dump の任意設定。
- 期待する観測結果: optional runtime 設定が未設定でも既定値で進む。実機 gate は未設定なら adapter open 前に止まる。診断系環境変数は未設定なら no-op になる。invalid override は早期に失敗し、黙って危険な既定値へ戻らない。
- 制約: 実機安全 gate を緩めない。BTstack source selection、Switch protocol bytes、report timing 採用値はこの work unit で変更しない。`vendor/btstack` は編集しない。
- 対象外: production を既定 backend にするかどうかの方針変更、CLI option / config file への全面移行、stable metrics protocol、実機 rerun。
- source から use case へ変換した判断: まず環境変数を分類し、未設定時 regression test を追加する。behavior change が必要な項目は refactor と混ぜず、後続 work unit へ送る。

## 3. 対象範囲

- `getenv` / CMake `ENV{...}` / `justfile` / hook の環境変数参照を棚卸しする。
- 環境変数を次に分類する。
  - optional runtime override。
  - required hardware safety gate。
  - optional diagnostic sink。
  - development tooling gate。
  - experimental compatibility selector。
- `apps/swbt-daemon/main.c` の環境変数 parsing を、可能なら testable な `swbt/daemon/` 側 helper へ移す。
- optional runtime override が未設定の場合、`swbt_daemon_config_default()` の既定値で起動設定が成立することを unit test で固定する。
- diagnostic trace / dump / HCI dump path が未設定なら no-op であることを regression test または既存 test で固定する。
- direct host build を止める Dev Container gate と、実機 adapter open を止める hardware gate を混同しないよう、record と必要な docs を更新する。
- デッドコード候補は、削除前に呼び出し元、test coverage、spec / work unit 上の役割を確認する。

## 4. 対象外

- `SWBT_RUN_HARDWARE` / `SWBT_HARDWARE_APPROVED` の削除または緩和。
- `SWBT_DAEMON_BACKEND=production` を既定化する挙動変更。
- `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` を正規 identity として固定すること。
- `SWBT_REPORT_PERIOD_US` の default 採用値変更。
- `tap`, `duration_ms`, `sequence`, `at_ms` の daemon protocol 追加。
- 実 Bluetooth adapter を開くコマンド、Switch pairing、HID advertising、report loop。

## 5. 関連 spec / docs

- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/operations/development-tooling.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `spec/protocols/switch-hid-core.md`
- `spec/references/switch-subcommand-dispatcher-core.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`
- `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md`
- `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`

## 6. 根拠監査

not applicable。

この work unit は環境変数依存、entrypoint 構造、開発 tooling gate、診断 sink の整理を扱う。Switch HID report bytes、BTstack source selection、report period 採用値、WinUSB/libusb facts は追加または変更しない。

`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` の profile 自体を変更する場合は、Switch device info bytes に触れるため `source-audit` を使う。この work unit では、その profile が実験条件であることの分類だけを扱う。

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: 現在の懸念は「環境変数がないと通常実行が壊れるか」の境界であり、まず parsing と分類を testable にすると、後続の behavior change を混ぜずに確認できる。
- verification: refactor 前後で `just build-debug`、targeted CTest、`just test-debug`、必要に応じて `just windows-cross` を同じ条件で実行する。

初期監査の分類:

| 分類 | 環境変数 | 現在の挙動 | 初期判断 |
|---|---|---|---|
| optional runtime override | `SWBT_REPORT_PERIOD_US` | 未設定なら `8000`。invalid は startup failure。 | 既定値 test を追加する。 |
| optional runtime override | `SWBT_IPC_HOST` | 未設定なら `127.0.0.1`。空文字は既定値維持。 | 既定値 test を追加する。 |
| optional runtime override | `SWBT_IPC_PORT` | 未設定なら `0`。ephemeral port を使う。 | 実行はできるが外部 client discoverability は別論点。 |
| optional runtime override | `SWBT_IPC_BACKLOG` | 未設定なら `1`。invalid または `0` 以下は failure。 | 既定値 test を追加する。 |
| optional runtime override | `SWBT_IPC_HEARTBEAT_TIMEOUT_MS` | 未設定なら `0`、timeout disabled。 | default を変えるなら behavior change として分ける。 |
| experimental compatibility selector | `SWBT_DEVICE_INFO_PROFILE` | 未設定なら default profile。`mizuyoukanao-pro` は実機切り分け条件。unknown は failure。 | 実験条件として残し、default とは分ける。 |
| required hardware safety gate | `SWBT_DAEMON_BACKEND` | `production` のときだけ production backend。未設定では no-op backend。 | 安全境界として現状維持。既定化は対象外。 |
| required hardware safety gate | `SWBT_RUN_HARDWARE`, `SWBT_HARDWARE_APPROVED` | 両方 `1` でなければ adapter open 前に failure。 | 緩めない。regression test を維持する。 |
| optional diagnostic sink | `SWBT_DIAGNOSTIC_TRACE_PATH` | 未設定なら no-op。設定時だけ trace file へ追記。 | 残す。失敗しても通常動作を止めない。 |
| optional diagnostic sink | `SWBT_CRASH_DUMP_PATH` | Windows のみ。未設定なら handler 未登録。 | 残す。診断用として docs 上も任意にする。 |
| optional diagnostic sink | `SWBT_HCI_DUMP_TRACE_PATH` | 未設定なら no-op。設定時に HCI dump text sink を開く。 | open failure は診断要求の失敗として production startup を止めている。妥当性を test で確認する。 |
| development tooling gate | `SWBT_DEVCONTAINER` | container 内 recipe / CMake を識別する。未設定の `just` は Dev Container へ委譲する。 | 開発入口の gate として現状維持。 |
| development tooling gate | `SWBT_ALLOW_HOST_BUILD` | direct host build の明示 opt-in。 | 通常実行を壊すものではなく、unsupported path の gate。 |
| development tooling option | `DEVCONTAINER_CLI`, `DEVCONTAINER_WORKSPACE`, `DEVCONTAINER_UP_FLAGS`, `CTEST_ARGS`, `SWBT_FULL_PRE_PUSH` | tooling override または test 絞り込み。 | product runtime 依存と分ける。 |

初期検索では、`apps/`, `swbt/`, `api/`, `tests/`, `cmake/`, `scripts/`, `justfile` に `TODO` / `FIXME` / `HACK` / `WIP` は見つからなかった。デッドコード候補はコメント marker ではなく、no-op backend、debug client、diagnostic path、experimental profile の役割を個別に確認する。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `swbt/daemon/config.h`
- `swbt/daemon/config.c`
- `swbt/daemon/runtime.h`
- `swbt/daemon/runtime.c`
- `swbt/daemon/production_backend.h`
- `swbt/daemon/production_backend.c`
- `swbt/btstack_bridge/production_btstack.c`
- `swbt/core/diagnostics.c`
- `apps/swbt-debug-client/*`
- `tests/*`
- `CMakeLists.txt`
- `justfile`
- `scripts/require-dev-environment.sh`
- `.githooks/pre-push`
- 関連 spec / work unit record

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | daemon config uses defaults when optional runtime environment variables are absent | regression | unit | no |
| todo | invalid numeric runtime environment override fails startup config instead of silently falling back | edge | unit | no |
| todo | production hardware mode rejects before IPC runner, BTstack platform, or HCI power-on when approval environment is absent | regression | integration | no |
| todo | diagnostic trace and crash dump paths are optional and missing paths do not affect normal startup | regression | unit | no |
| todo | HCI dump path is optional, but explicit dump path open failure is reported as startup failure before hardware observation is trusted | edge | unit/integration | no |
| todo | `SWBT_DEVICE_INFO_PROFILE` missing keeps default device info and unknown profile is rejected | regression | unit | no |
| todo | direct `just debug` from host does not require users to set `SWBT_DEVCONTAINER` manually | regression | tooling | no |
| deferred | decide whether no-op backend should remain the default daemon mode or become an explicit dry-run mode | behavior | design | no |
| deferred | decide whether heartbeat timeout default should remain disabled or become a nonzero fail-safe default | behavior | design/hardware | yes |

## 10. 検証

初期監査:

- `git branch --show-current`: `main`。
- `git status --short`: clean。
- `git switch -c refactor/env-dependency-audit`: branch 作成。
- `rg -n "getenv\(" apps swbt tests --glob '!vendor/**'`: runtime `getenv` は `apps/swbt-daemon/main.c`, `swbt/core/diagnostics.c`, `swbt/btstack_bridge/production_btstack.c` に限定されている。
- `rg -n "ENV\{|SWBT_DEVCONTAINER|SWBT_ALLOW_HOST_BUILD|DEVCONTAINER_|CTEST_ARGS|SWBT_FULL_PRE_PUSH" CMakeLists.txt justfile scripts .githooks .github --glob '!vendor/**'`: tooling gate と override を確認した。
- `rg -n "TODO|FIXME|HACK|WIP" apps swbt api tests cmake CMakeLists.txt CMakePresets.json justfile scripts --glob '!vendor/**'`: no matches。
- red: `just build-debug` failed as expected。`tests/daemon_runtime_test.c` が `swbt_daemon_config_apply_env` を参照し、`undefined reference to swbt_daemon_config_apply_env` で link failure。
- green: `apps/swbt-daemon/main.c` の runtime env parsing を `swbt_daemon_config_apply_env` と `swbt_daemon_config_env_t` として `swbt/daemon/config.*` へ移した。`main.c` は `getenv` で値を集め、解釈は config layer へ委譲する。
- green build: `just build-debug` pass。
- targeted: `CTEST_ARGS='-R daemon_runtime_test --output-on-failure' just test-debug` pass。最初の sandboxed run は Dev Container の Docker setup で失敗し、同じ command を Docker access 付きで再実行して pass。
- refactor: `scripts/format.sh` pass。追加の責務移動は行わず、`main.c` の entrypoint 責務を小さくした状態を維持した。
- refactor verification: `git diff --check` pass、`scripts/check-format.sh` pass、`just test-debug` pass（31/31）、`just windows-cross` pass。

この時点では、optional runtime env 未設定時の default config だけを TDD cycle として完了した。invalid override、diagnostic sink、HCI dump path、tooling gate の regression test は未完了である。

## 11. 実機実行条件

この work unit の初期監査と software refactor では実機は不要である。

理由: Bluetooth adapter open、Switch pairing、HID advertising、report loop を実行しない。hardware safety gate の regression は fake backend または software test で確認する。

実機を必要とするのは、heartbeat timeout default の変更、report period default の変更、device info profile の正規化、または production backend 既定化のように Switch-facing behavior が変わる後続 work unit である。その場合は `hardware-harness` を使い、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、専用 USB Bluetooth dongle、WinUSB driver assignment、`docs/hardware-test-log.md` 記録を必要条件にする。

## 12. 先送り事項

- 観測: no-op backend が既定であるため、環境変数未設定の `swbt-daemon` は production daemon としては動かない。
  先送り理由: これは現在の安全境界として spec に記録済みであり、変更すると behavior change になる。
  次の置き場: 後続 work unit。候補名は `PRODUCTION_BACKEND_DEFAULT_POLICY`。
- 観測: `SWBT_IPC_HEARTBEAT_TIMEOUT_MS` の既定値は `0` で timeout disabled である。
  先送り理由: fail-safe default を変えると owner disconnect / heartbeat behavior と実機観測に影響する。
  次の置き場: 後続 work unit または `local_039` の observability protocol 判断。
- 観測: `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` は Switch2 bring-up を進めるための実験条件として有効だったが、正規 identity として source-audited default にはなっていない。
  先送り理由: device info bytes は Switch protocol 値であり、単なる cleanup として変えない。
  次の置き場: Switch device info profile の source-audit work unit。

## 13. チェックリスト

- [x] branch と worktree status を確認した。
- [x] 環境変数参照の初期棚卸しを行った。
- [x] tooling gate と product runtime dependency を分けた。
- [x] hardware safety gate を削除対象にしない判断を記録した。
- [x] env config parsing の testable boundary を作った。
- [x] optional runtime override の未設定時 regression test を追加した。
- [ ] diagnostic sink の未設定時 no-op を確認した。
- [x] code refactor 前後で同じ検証を実行した。
- [x] 実機未実行理由を更新した。
