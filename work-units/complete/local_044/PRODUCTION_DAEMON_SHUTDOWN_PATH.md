# Production Daemon Shutdown Path

## 1. 概要

Windows native 実機 bring-up の前提として、production daemon が強制終了ではなく明示的な停止要求で BTstack run loop を抜け、HCI power-off と runtime cleanup に到達できる停止経路を実装する。

この work unit は実機を起動しない。Windows console control event から production backend に stop request を渡す software boundary を作り、fake backend test と Windows cross build で確認する。実機での CSR8510 A10 adapter open、Switch pairing、HID advertising、report loop は `local_037` に残す。

## 2. 起点 / ユースケース

source:

- ユーザ要求: 停止経路を実装する work unit record を執筆し、実装し、マージする。
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` の停止条件。現行 daemon は IPC shutdown command や非対話の停止入口がなく、Codex が背景プロセスとして起動すると強制終了になり得る。
- `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md` の production backend。`run_loop_trigger_exit` op は存在するが、外部停止要求から呼ぶ経路がない。
- BTstack source fact。Windows winusb port は Ctrl+C callback で `hci_power_control(HCI_POWER_OFF)` を呼ぶ。BTstack run loop は `btstack_run_loop_trigger_exit()` を公開している。

use case:

- actor: Windows hardware bring-up operator。
- 入力または状態: `SWBT_DAEMON_BACKEND=production`、hardware approval 済み daemon、BTstack run loop 実行中、operator の停止要求。
- 期待する観測結果: 停止要求で HCI power-off と run loop exit trigger が呼ばれ、`run_loop_execute` が戻った後に runtime stop が実行される。cleanup は report timer、output handler、HID registration、BTstack platform、IPC runner の順に到達する。
- 制約: 実機成功をこの work unit で主張しない。`vendor/btstack` は編集しない。停止要求は adapter を開く前の approval gate を緩めない。NyXpy 操作はこの work unit の対象外にする。
- 対象外: IPC shutdown command、daemon status protocol、CSR8510 A10 実機 daemon run、Switch pairing、HID advertising、report period comparison。
- source から use case へ変換した判断: NyXpy 入力反映の前に必要なのは client protocol 拡張ではなく、daemon process が foreground console から明示停止できる lifecycle boundary である。

## 3. 対象範囲

- production backend に stop request listener を注入できる entrypoint を追加する。
- stop request を受けたときに HCI power-off と run loop exit trigger を 1 回だけ実行する。
- stop request 後も既存の runtime cleanup order に到達することを fake backend test で固定する。
- `apps/swbt-daemon/main.c` に Windows console control event の停止入口を追加する。非 Windows build では injected listener なしで既存 production backend entrypoint を使う。
- BTstack run loop / Windows winusb port の停止根拠をこの record に残す。
- `local_037` の停止条件を更新する。
- Windows cross build で `swbt-daemon.exe` が link できることを確認する。

## 4. 対象外

- 実機 daemon run。
- NyXpy 実行。
- Project NyX 側の設定変更。
- IPC `shutdown` command の追加。
- stable status / metrics protocol。
- BTstack submodule の編集。

## 5. 関連 spec / docs

- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `spec/operations/windows-native-preflight.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`

## 6. 根拠監査

`source-audit` を使う。BTstack run loop exit と Windows winusb port の Ctrl+C shutdown behavior に触れるため、upstream source と固定 commit で根拠を分ける。

### 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| BTstack run loop exit API | `btstack_run_loop_trigger_exit()` | source fact | `vendor/btstack/src/btstack_run_loop.h`, `vendor/btstack/src/btstack_run_loop.c` | current; implementation uses existing bridge op |
| Windows run loop trigger | `run_loop_exit_requested = true` | source fact | `vendor/btstack/platform/windows/btstack_run_loop_windows.c` | may not wake an idle wait by itself |
| Windows winusb Ctrl+C path | `trigger_shutdown` calls `hci_power_control(HCI_POWER_OFF)` | source fact | `vendor/btstack/port/windows-winusb/main.c` | shutdown request should power off before run loop exit trigger |

### 未解決事項

- Windows run loop が全状態で即時に wake するかは source だけでは断定しない。実機 run では停止操作の所要時間と cleanup 結果を `docs/hardware-test-log.md` に記録する。

## 7. 設計メモ

- production backend は stop request を受けると `power_off` と `run_loop_trigger_exit` を呼ぶ。`hardware_powered` と `shutdown_requested` は C11 atomic とし、Windows console control handler と通常 cleanup が競合しても `power_off` を二重実行しない。
- listener の install / uninstall は production main の外側から注入する。test では fake listener、`swbt-daemon.exe` では platform listener を使う。
- approval gate が失敗した場合、listener は install しない。
- shutdown request は daemon lifecycle の入力であり、Switch protocol や IPC state snapshot の contract にはしない。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `swbt/daemon/production_backend.h`
- `swbt/daemon/production_backend.c`
- `tests/daemon_production_backend_test.c`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | stop request during production run powers off hardware and triggers run loop exit before cleanup | new | integration | no |
| green | shutdown listener is not installed when hardware approval is missing | edge | integration | no |
| green | repeated stop request does not call power-off twice | edge | unit | no |
| green | Windows cross build links daemon shutdown path | regression | build | no |
| deferred | CSR8510 A10 run can be stopped without forced process termination | characterization | hardware | yes |

## 10. 検証

- red: 2026-06-20 `just build-debug` は `swbt_daemon_shutdown_request_t`、`swbt_daemon_shutdown_listener_t`、`swbt_daemon_production_main_with_backend_and_shutdown` が未定義で失敗した。これは shutdown listener entrypoint をまだ実装していない期待通りの失敗である。

Green:

- `swbt/daemon/production_backend.*` に shutdown listener injection entrypoint を追加した。listener は approval gate、runtime start、HCI power-on の後にだけ install する。
- stop request は `power_off` と `run_loop_trigger_exit` を呼ぶ。`hardware_powered` と `shutdown_requested` の C11 atomic で通常 cleanup と重複 stop request の二重 power-off を防ぐ。
- `apps/swbt-daemon/main.c` は Windows console control event から production backend の stop request を呼ぶ。実機 bring-up の対象は Windows console control event である。
- `tests/daemon_production_backend_test.c` で stop request、approval failure、重複 stop request、cleanup order を fake backend で固定した。

検証:

- targeted: 2026-06-20 Dev Container の repository root で `ctest --preset linux-debug -R daemon_production_backend_test --output-on-failure` を実行し、1/1 passed。
- `just debug`: pass。27/27 tests passed。
- `just verify`: pass。format-check、clang-tidy、linux-debug、linux-asan、windows-mingw-debug を実行した。Windows cross build は `swbt-daemon.exe` link まで成功した。

Refactor:

- refactor-skipped。shutdown listener は production backend lifecycle の既存境界に収まり、green 後に分離すべき重複や新しい ownership は残っていない。

## 11. 実機実行条件

この work unit では実機を実行しない。Bluetooth adapter open、Switch pairing、HID advertising、report loop は `local_037` の承認条件に従う。

この work unit の成果は、実機 run 前に使う daemon 停止経路である。実機で使う場合は、対象 adapter、承認範囲、停止操作、cleanup 結果を `docs/hardware-test-log.md` に記録する。

## 12. 先送り事項

- 観測: CSR8510 A10 を開いた状態で Ctrl+C 停止が実際に HCI power-off と cleanup に到達するかは未観測である。
  先送り理由: この work unit は software boundary の実装と build/test が範囲であり、adapter open は実機作業である。
  次の置き場: `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `docs/hardware-test-log.md`。

## 13. チェックリスト

- [x] source と use case を記録した。
- [x] TDD red を確認した。
- [x] production daemon shutdown path を実装した。
- [x] targeted CTest を実行した。
- [x] `just debug` を実行した。
- [x] `just verify` を実行した。
- [x] `just windows-cross` 相当を `just verify` 内で実行した。
- [x] `local_037` の停止条件を更新した。
- [x] 実機未実行理由を記録した。
