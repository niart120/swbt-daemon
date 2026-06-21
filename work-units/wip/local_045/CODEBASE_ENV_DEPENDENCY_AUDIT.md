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
| refactor-done | invalid numeric runtime environment override fails startup config without partially mutating the existing config | edge | unit | no |
| refactor-done | production hardware mode rejects before IPC runner, BTstack platform, or HCI power-on when approval environment is absent | regression | integration | no |
| refactor-done | diagnostic trace path is optional and missing path does not create output or affect normal execution | regression | unit | no |
| refactor-done | HCI dump path is optional, but explicit dump path open failure is reported as startup failure before hardware observation is trusted | edge | unit/integration | no |
| refactor-done | `SWBT_DEVICE_INFO_PROFILE` missing keeps default device info and unknown profile is rejected | regression | unit | no |
| refactor-done | direct `just debug` from host does not require users to set `SWBT_DEVCONTAINER` manually | regression | tooling | no |
| refactor-done | crash dump path is optional and missing path uses the same no-op diagnostic path predicate | regression | unit/cross-build | no |
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
- red: invalid numeric runtime env item。`just build-debug` pass。`CTEST_ARGS='-R daemon_runtime_test --output-on-failure' just test-debug` failed as expected。`SWBT_REPORT_PERIOD_US=0` に対する `swbt_daemon_config_apply_env` が false を返す前に `config.report_period_us` を `0` へ部分更新するため、追加 test が失敗した。
- green: `swbt_daemon_config_apply_env` は一時 copy に env override を適用し、全検証に通った場合だけ元 config へ反映する。invalid override では false を返し、既存 config を維持する。
- green build: `just build-debug` pass。
- targeted: `CTEST_ARGS='-R daemon_runtime_test --output-on-failure' just test-debug` pass。build と test を並列にした最初の green 確認は古い red binary を拾って fail したため、build 完了後に同じ targeted test を再実行して pass。
- refactor: `scripts/format.sh` pass。追加の構造変更は行わず、config helper の atomic apply に閉じた。
- refactor verification: `scripts/check-format.sh` pass、`git diff --check` pass、`just build-debug` pass（no work to do）、`just test-debug` pass（31/31）、`just windows-cross` pass。
- red: diagnostic trace optional path item。`just build-debug` failed as expected。`tests/diagnostics_test.c` が `swbt_diagnostic_trace_to_path` を参照し、`undefined reference to swbt_diagnostic_trace_to_path` で link failure。
- green: `swbt_diagnostic_trace_to_path` を追加し、`swbt_diagnostic_trace` は `SWBT_DIAGNOSTIC_TRACE_PATH` を読んで helper へ委譲する構造にした。path 未指定、空文字、message 未指定は no-op として test で固定した。
- green build: `just build-debug` pass。
- targeted: `CTEST_ARGS='-R diagnostics_test --output-on-failure' just test-debug` pass。
- refactor: `scripts/format.sh` pass。追加の構造変更は行わず、diagnostic trace helper の切り出しに閉じた。
- refactor verification: `scripts/check-format.sh` pass、`git diff --check` pass、`just build-debug` pass（no work to do）、`just test-debug` pass（31/31）、`just windows-cross` pass。
- red: HCI dump path item。最初の `just build-debug` は test source の POSIX feature macro 位置が原因で `setenv` / `unsetenv` の implicit declaration となったため、test bug として修正した。再実行した `just build-debug` は expected red。`tests/btstack_production_hci_dump_test.c` が `swbt_btstack_production_hci_dump_start` を参照し、`undefined reference to swbt_btstack_production_hci_dump_start` で link failure。
- green: `swbt_btstack_production_hci_dump_start` を追加し、`SWBT_HCI_DUMP_TRACE_PATH` から読む private 関数は path 引数の helper へ委譲する構造にした。path 未指定または空文字は no-op、明示された path の open failure は `platform_start` failure として test で固定した。
- green build: `just build-debug` pass。
- targeted: `CTEST_ARGS='-R btstack_production_hci_dump_test --output-on-failure' just test-debug` pass。
- refactor: `scripts/format.sh` pass。追加の構造変更は行わず、HCI dump path helper と production test target の追加に閉じた。
- refactor verification: `scripts/check-format.sh` pass、`git diff --check` pass、`just build-debug` pass（no work to do）、`just test-debug` pass（32/32）、`just windows-cross` pass。
- red: hardware approval env parser item。`just build-debug` failed as expected。`tests/daemon_production_backend_test.c` が `swbt_daemon_hardware_approval_from_env` を参照し、`undefined reference to swbt_daemon_hardware_approval_from_env` で link failure。
- green: `swbt_daemon_hardware_approval_from_env` を追加し、`SWBT_RUN_HARDWARE` と `SWBT_HARDWARE_APPROVED` の文字列解釈を production backend layer で testable にした。`main.c` は process env を集めて helper へ渡す構造にした。両方が `"1"` のときだけ承認され、それ以外は承認されないことを test で固定した。
- green build: `just build-debug` pass。
- targeted: `CTEST_ARGS='-R daemon_production_backend_test --output-on-failure' just test-debug` pass。
- refactor: `scripts/format.sh` pass。追加の構造変更は行わず、hardware approval env parser の移動に閉じた。
- refactor verification: `scripts/check-format.sh` pass、`git diff --check` pass、`just build-debug` pass（no work to do）、`just test-debug` pass（32/32）、`just windows-cross` pass。
- characterization: device info profile env item。`config_env_absent_uses_defaults` と `config_rejects_unknown_device_info_profile` で既に一部固定されていたため、red は発生しない既存挙動として扱った。`swbt_daemon_config_apply_env` 経由の unknown profile が false を返し、既存 config を維持する test を追加した。
- characterization build: `just build-debug` pass。
- targeted: `CTEST_ARGS='-R daemon_runtime_test --output-on-failure' just test-debug` pass。
- refactor: `scripts/format.sh` pass。追加の実装変更は行わず、test coverage の補強に閉じた。
- refactor verification: `scripts/check-format.sh` pass、`git diff --check` pass、`just build-debug` pass（no work to do）、`just test-debug` pass（32/32）、`just windows-cross` pass。
- tooling verification: `SWBT_DEVCONTAINER` と `SWBT_ALLOW_HOST_BUILD` が未設定であることを確認したうえで、host PowerShell 入口から `just debug` を実行して pass。Dev Container CLI へ委譲され、`cmake --fresh --preset linux-debug`、`cmake --build --preset linux-debug`、`ctest --preset linux-debug --output-on-failure` が通った。CTest は 32/32 pass。
- red: crash dump path item。`just build-debug` failed as expected。`tests/diagnostics_test.c` が `swbt_diagnostic_path_is_enabled` を参照し、`undefined reference to swbt_diagnostic_path_is_enabled` で link failure。
- green: `swbt_diagnostic_path_is_enabled` を追加し、diagnostic trace、HCI dump、Windows crash dump handler install / write path の未設定判定を共通 predicate へ寄せた。predicate 自体は `diagnostics_test` で固定した。Windows crash dump の実書き込みは実行せず、Windows 側の使用箇所は `just windows-cross` で compile gate として確認した。
- green build: `just build-debug` pass。
- targeted: `CTEST_ARGS='-R diagnostics_test --output-on-failure' just test-debug` pass。
- refactor: `scripts/format.sh` pass。追加の構造変更は行わず、diagnostic path predicate の導入と既存 path 判定の置き換えに閉じた。
- refactor verification: `scripts/check-format.sh` pass、`git diff --check` pass、`just build-debug` pass（no work to do）、`just test-debug` pass（32/32）、`just windows-cross` pass。

この時点では、optional runtime env 未設定時の default config、invalid numeric runtime env override、hardware approval env parser、diagnostic trace path optionality、HCI dump path optionality / explicit failure、device info profile env characterization、direct `just debug` host delegation、crash dump path optionality の 8 cycle を完了した。残る判断は deferred の no-op backend default policy と heartbeat timeout default policy だけであり、どちらも behavior / hardware 影響を伴うため、この work unit では実装しない。

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
- [x] diagnostic trace sink の未設定時 no-op を確認した。
- [x] HCI dump sink の未設定時 no-op と explicit path failure を確認した。
- [x] crash dump sink の未設定時 no-op 判定を確認した。
- [x] code refactor 前後で同じ検証を実行した。
- [x] 実機未実行理由を更新した。
