# Daemon Runtime Boundaries

## 1. 状態

current。

## 2. 目的

この spec は、daemon runtime、IPC、Switch protocol core、BTstack bridge、実機実行条件の責務境界を定める。

`spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` には目標構成と将来案が混在している。現在有効な実装境界は、この spec と関連 protocol / operations spec を正本にする。

## 3. 適用範囲

- `apps/swbt-daemon/` の entrypoint。
- `swbt/daemon/` の runtime lifecycle。
- `swbt/ipc/` の local IPC と owner/session 管理。
- `swbt/core/state_mailbox.*` の IPC/report loop 境界。
- `swbt/switch/` の BTstack 非依存 protocol core。
- `swbt/btstack_bridge/` の BTstack integration boundary。
- unit test、fake backend、実機 bring-up の境界。

次は対象外である。

- Switch HID packet layout の個別値。これは `spec/protocols/switch-hid-core.md` が扱う。
- daemon IPC message contract の詳細。これは `spec/protocols/daemon-ipc-v1.md` が扱う。
- Windows native 実機 preflight 手順。これは `spec/operations/windows-native-preflight.md` が扱う。

## 4. 決定事項

daemon runtime の production boundary は Bluetooth adapter、BTstack run loop、HID Device registration、output report handling、periodic input report scheduler、local IPC server を所有する。client は daemon IPC に controller state snapshot を送る。

`apps/swbt-daemon/main.c` は thin entrypoint とし、設定作成、runtime 起動、exit code 変換だけを担当する。runtime lifecycle は `swbt/daemon/runtime.*` に置く。
現行の entrypoint は既定では `swbt_daemon_runtime_noop_backend()` を渡すため、実 Bluetooth adapter、実 IPC listener、BTstack run loop、HID advertising、periodic report loop は起動しない。
`SWBT_DAEMON_BACKEND=production` のときだけ production backend を選ぶ。production mode は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` が揃わない限り、runtime、IPC runner、BTstack platform、HCI power-on を開始しない。
Windows production mode は console control event listener を HCI power-on 後に登録する。停止要求を受けた場合、production backend は HCI power-off と BTstack run loop exit trigger を 1 回だけ実行し、`run_loop_execute` が戻った後に runtime cleanup へ進む。現時点で IPC shutdown command は定義しない。

IPC path は `hello`、`acquire`、`release`、`set_state`、`get_status`、owner disconnect、heartbeat timeout を扱う。IPC path は BTstack API を直接呼ばない。

IPC thread と report scheduler の間は latest-state mailbox で分離する。複数の state update が report tick 間に届いた場合、daemon は最新 snapshot を使う。過去の state update を全件 replay しない。

Switch protocol core は `swbt/switch/` に置き、BTstack header に依存しない。input report、output report parser、subcommand reply、dispatcher、virtual SPI、rumble、player lights はこの層で扱う。

BTstack integration は `swbt/btstack_bridge/` に閉じる。BTstack API、timer source、HID registration、DATA / SET_REPORT callback、can-send event、interrupt send はこの境界で扱う。

rearchitecture cutover 後に残す中継点は、次の current responsibility を持つ場合だけ認める。

| item | current responsibility |
|---|---|
| `production_btstack` IPC pump | BTstack run loop 上で generic IPC pump callback を schedule する BTstack bridge port adapter である。daemon IPC runner 型と production backend 内部型は参照しない。 |
| production backend ops table | production backend が hardware-facing 能力を composition root へ渡す current port table である。platform、HID、timer、SSP、clock、power、run loop、IPC pump を束ねる。 |
| `swbt_ipc_session_t` | IPC wire 互換 status、rumble status、mailbox publish、application result mapping を束ねる current IPC/application facade である。owner、sequence、neutral 化の authoritative logic は `swbt_app_t` が持つ。 |
| `state_mailbox` | IPC/application update と report scheduler の間で latest state を渡す current concurrency boundary である。report scheduler は過去 update を replay せず、最新 snapshot を読む。 |
| `swbt_core` aggregate target | daemon executable、public C ABI、IPC、BTstack bridge を結合する current integration target である。protocol と application の単体 target は分離済みであり、禁止 include は CMake 検査で補う。 |

`vendor/btstack` は固定 submodule として扱い、直接編集しない。BTstack に patch が必要な場合は、まず `swbt/btstack_bridge/` で吸収できるか判断し、fork や upstream patch が必要な場合は `docs/upstream-btstack.md` と関連 spec / work unit record に理由を残す。

unit test と integration test は fake backend を使う。fake backend は runtime lifecycle、cleanup ordering、mailbox connection、BTstack adapter API の呼び出し順を固定するためのものであり、Switch pairing、HID advertising、report loop、WinUSB driver assignment を証明しない。

実 Bluetooth adapter を開く daemon run、Switch pairing、HID advertising、periodic report loop は実機作業である。人間の明示承認、専用 USB Bluetooth dongle、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、`docs/hardware-test-log.md` への記録を必要条件にする。

metrics と logging は現時点では in-process API と log sink である。stable IPC metrics protocol は未定義であり、`get_status` の公開 schema として扱わない。

BTstack を含む build artifact は MIT-only artifact と表現しない。ライセンスや notice に触れる変更では `THIRD_PARTY_NOTICES.md` を確認する。

## 5. 根拠

この spec は新しい Switch protocol 値、BTstack source selection、report period、WinUSB/libusb 実測値を追加しない。既存実装と完了済み work unit record の設計境界を current spec へ昇格する。

| 項目 | 根拠 | source | status |
|---|---|---|---|
| daemon runtime entrypoint | implementation fact | `apps/swbt-daemon/main.c`, `swbt/daemon/runtime.*`, `swbt/daemon/production_backend.*` | default no-op; production opt-in |
| local IPC owner/session model | implementation fact | `swbt/ipc/*`, `work-units/complete/local_008` から `local_011` | current |
| latest-state mailbox boundary | implementation fact | `swbt/core/state_mailbox.*`, `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md` | current |
| fake backend runtime integration | implementation fact | `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md` | current |
| in-process metrics/logging only | implementation fact / design policy | `work-units/complete/local_026/REPORT_METRICS_AND_LOGGING.md` | current |
| BTstack bridge owns BTstack API calls | design policy / implementation fact | `swbt/btstack_bridge/*`, `spec/references/btstack-*.md` | current |
| production daemon entrypoint | implementation fact / source audit | `swbt/daemon/production_backend.*`, `swbt/btstack_bridge/production_btstack.*`, `spec/references/btstack-daemon-entrypoint.md` | current software gate |
| production daemon shutdown path | implementation fact / source audit | `swbt/daemon/production_backend.*`, `apps/swbt-daemon/main.c`, `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | current; hardware stop observation recorded |
| real adapter open and pairing | hardware observation | `docs/hardware-test-log.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | recorded for CSR8510 A10 / Switch2 22.1.0 |
| rearchitecture cutover inventory | implementation fact / verification | `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`, `tests/cmake/cutover_acceptance_test.cmake`, `tests/daemon_cutover_journey_test.c` | current software gate |

## 6. 関連 work units

- `work-units/complete/local_008/IPC_SESSION_CORE.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`
- `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md`
- `work-units/complete/local_012/BTSTACK_HID_DEVICE_REGISTRATION.md`
- `work-units/complete/local_018/BTSTACK_PRODUCTION_ADAPTER.md`
- `work-units/complete/local_019/BTSTACK_OUTPUT_REPORT_CALLBACKS.md`
- `work-units/complete/local_023/BTSTACK_INPUT_REPORT_TIMER_ADAPTER.md`
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md`
- `work-units/complete/local_026/REPORT_METRICS_AND_LOGGING.md`
- `work-units/complete/local_036/SPEC_WORK_UNIT_INVENTORY.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md`
- `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`
- `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md`
- `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`
- `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md`
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`

## 7. 未解決事項

- production daemon が実 Bluetooth adapter を開く順序、Switch pairing、HID L2CAP open、report loop、IPC input 反映、owner disconnect / heartbeat timeout / shutdown neutral cleanup は `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` で観測済みである。これは CSR8510 A10、WinUSB、Switch2 firmware `22.1.0` の hardware observation であり、他 adapter / firmware の一般互換性は別途確認が必要である。
- production daemon が実 BTstack backend を runtime backend として接続する software gate は `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md` で完了した。実 IPC runner と synchronized state boundary は `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md` で完了済みである。BTstack can-send event と subcommand reply queue / periodic scheduler の software integration は `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md` で完了している。
- stable IPC metrics / status protocol は未定義である。`work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md` で扱う。
