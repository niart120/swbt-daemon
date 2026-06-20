# Production Daemon BTstack Entrypoint

## 1. 概要

`swbt-daemon.exe` が no-op backend ではなく production BTstack backend を選び、Windows hardware bring-up に進める executable entrypoint を作る work unit。

この work unit は software integration を主対象にする。Bluetooth adapter を開く code path は実装対象だが、実機 daemon run はこの record の完了条件にしない。実機実行は `local_037` で承認、adapter identity、daemon log、NyX artifact と一緒に記録する。

## 2. 起点 / ユースケース

source:

- ユーザ要求: NyXpy を使う実機検証の前に、手順を spec に載せ、必要な work unit record を全て執筆する。
- `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` の停止条件。現行 `swbt-daemon.exe` は no-op backend で、Bluetooth adapter、IPC listener、HID advertising、periodic report loop を起動しない。
- `spec/operations/windows-hardware-bringup-sequence.md` の gate 3。
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md` の先送り事項。production daemon が実 Bluetooth adapter を開く順序と失敗時 cleanup は fake backend test だけでは証明できない。
- `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md`。production send-ready path が実機前の software gate である。
- `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md`。NyXpy が接続する IPC endpoint と state handoff が前提である。

use case:

- actor: hardware bring-up operator。
- 入力または状態: Windows native `swbt-daemon.exe`、専用 USB Bluetooth dongle、WinUSB driver assignment、hardware approval environment、report period、IPC endpoint config。
- 期待する観測結果: hardware approval がない実行は adapter open 前に失敗する。approval がある実行では production backend が IPC runner、HID registration、output report callbacks、input report timer、BTstack run loop を構成し、daemon log に endpoint と configuration を残せる。
- 制約: `vendor/btstack` を直接編集しない。BTstack API は `swbt/btstack_bridge/` に閉じる。実機成功はこの software work unit で断定しない。
- 対象外: Switch pairing 成功判定、NyXpy macro 実行、report period 採用値、binary release。
- source から use case へ変換した判断: `main.c` を単に no-op から差し替えるだけでは不足する。実機に触れる前に approval gate、config、production backend composition、cleanup を fake / build test で固定する。

## 3. 対象範囲

- production backend context を追加し、daemon runtime backend table へ接続する。
- hardware approval gate を追加する。`SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` がない場合、Bluetooth adapter open 前に失敗する。
- report period と IPC endpoint config を executable から指定できるようにする。
- HID Device registration、output report callbacks、input report timer、BTstack send-ready path、IPC runner を production backend に接続する。
- BTstack run loop startup / shutdown の順序を source-audit し、bridge 内に閉じる。
- start failure と shutdown path で、起動済み resource を逆順に cleanup する。
- Linux debug build と Windows MinGW cross build で compile/link 境界を確認する。

## 4. 対象外

- Nintendo Switch との pairing 成功判定。
- HID advertising が Switch に受け入れられることの断定。
- actual report rate、jitter、latency の実測。
- NyXpy macro 実行と Project NyX artifact の生成。
- stable IPC metrics / status protocol。
- 複数 controller。
- binary release。

## 5. 関連 spec / docs

- `spec/operations/windows-hardware-bringup-sequence.md`
- `spec/operations/windows-native-preflight.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/protocols/switch-hid-core.md`
- `spec/protocols/daemon-ipc-v1.md`
- `docs/hardware-test-log.md`
- `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md`
- `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md`
- `work-units/complete/local_018/BTSTACK_PRODUCTION_ADAPTER.md`
- `work-units/complete/local_019/BTSTACK_OUTPUT_REPORT_CALLBACKS.md`
- `work-units/complete/local_023/BTSTACK_INPUT_REPORT_TIMER_ADAPTER.md`
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md`

## 6. 根拠監査

`source-audit` が必要である。

この work unit は BTstack run loop startup、WinUSB backend open、HID Device lifecycle、can-send event、cleanup order に触れる。既存 reference で足りない upstream fact を実装に入れる前に、`vendor/btstack` の固定 commit と関連 source path を監査する。

| 項目 | 状態 | 扱い |
|---|---|---|
| HID Device registration API | recorded | `work-units/complete/local_018` と `spec/references/btstack-production-adapter.md` を使う。 |
| output report callbacks | recorded | `work-units/complete/local_019` と `spec/references/btstack-output-report-callbacks.md` を使う。 |
| input report timer API | recorded | `work-units/complete/local_023` と `spec/references/btstack-periodic-input-report-core.md` を使う。 |
| BTstack run loop startup / shutdown | pending | この work unit で source-audit が必要である。 |
| Windows WinUSB adapter open behavior | pending | 実装前に source-audit、実行結果は `local_037` で hardware observation として記録する。 |

## 7. 設計メモ

- entrypoint は config 作成、approval gate、backend selection、exit code 変換に限定する。
- production backend の BTstack API 呼び出しは `swbt/btstack_bridge/` に閉じる。
- approval gate は adapter open より前に評価する。環境変数が欠ける場合は、IPC runner や BTstack run loop も開始しない。
- `local_042` の IPC runner software gate と `local_038` の send-ready software gate は完了済みである。この work unit では production backend composition と hardware approval gate を扱う。
- 実機成功は `local_037` でだけ記録する。この work unit では build / fake backend / source-audit の根拠を pass 条件にする。

## 8. 対象ファイル

- `CMakeLists.txt`
- `apps/swbt-daemon/main.c`
- `swbt/daemon/config.h`
- `swbt/daemon/config.c`
- `swbt/daemon/runtime.h`
- `swbt/daemon/runtime.c`
- `swbt/daemon/production_backend.h`
- `swbt/daemon/production_backend.c`
- `swbt/btstack_bridge/*`
- `tests/daemon_production_backend_test.c`
- `spec/references/`
- `work-units/wip/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | daemon entrypoint rejects production hardware mode before adapter open when approval environment is missing | edge | unit | no |
| todo | approved production config composes IPC runner, HID registration, output callbacks, report timer, and run loop in start order with fake backend ops | new | integration | no |
| todo | start failure cleans up only resources that were started, in reverse order | edge | integration | no |
| todo | report period and IPC endpoint config are logged or exposed before hardware run | new | unit | no |
| todo | Windows `windows-winusb` cross build links production backend boundary without editing `vendor/btstack` | regression | build | no |
| deferred | approved daemon run opens the selected WinUSB dongle and reaches Switch pairing / HID advertising | characterization | hardware | yes |

## 10. 検証

未実行。

この record は work unit の範囲と TDD Test List を作成しただけであり、code、CTest、実機コマンドは実行していない。

## 11. 実機実行条件

この work unit の software verification では実機を実行しない。

実機 daemon run は `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` で行う。実行時は専用 USB Bluetooth dongle、WinUSB driver assignment、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、承認範囲、daemon log、cleanup plan を記録する。

この work unit で実装する approval gate は、実機 run を許可するものではない。人間の承認と hardware log 記録を代替しない。

## 12. 先送り事項

- 観測: Switch pairing、HID advertising、periodic report loop が実機で成功するかは、この software work unit では証明しない。
  先送り理由: 専用 dongle、WinUSB driver、Switch firmware、daemon log、NyX artifact が必要である。
  次の置き場: `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `docs/hardware-test-log.md`。
- 観測: actual report rate、jitter、adapter identity を stable status protocol で公開するかは未決定である。
  先送り理由: production entrypoint の最低要件は hardware run に必要な config と cleanup である。
  次の置き場: `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`。

## 13. チェックリスト

- [x] `local_038` 完了後に着手した。
- [x] `local_042` 完了後に着手した。
- [ ] source-audit を実施または既存 reference で足りることを記録した。
- [ ] red test を追加した。
- [ ] production backend entrypoint を実装した。
- [ ] targeted CTest を実行した。
- [ ] `just debug` を実行した。
- [ ] `just windows-cross` を実行した。
- [ ] 実機状態または未実行理由を記録した。
