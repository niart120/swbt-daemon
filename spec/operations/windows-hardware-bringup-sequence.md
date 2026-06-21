# Windows Hardware Bring-Up Sequence

## 1. 状態

current。

## 2. 目的

Windows native + WinUSB + 専用 USB Bluetooth dongle で NyXpy handoff を使う実機 bring-up へ進む前に、software 側で満たす順序を固定する。

この spec は、既定では no-op backend の `swbt-daemon.exe` から、NyXpy が接続できる production daemon、BTstack-backed HID session、最後の実機記録までの work unit 順序を定める。実機の成功結果はこの spec ではなく `docs/hardware-test-log.md` に記録する。

## 3. 適用範囲

- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` を実行する前の software gate。
- NyXpy handoff で必要になる loopback IPC endpoint、daemon log、report period、artifact root の準備順。
- production daemon entrypoint が no-op backend から実 BTstack backend へ移るまでの work unit 分割。
- 実機実行条件を伴う work unit と、実機不要の software integration work unit の分離。

次は対象外である。

- Switch pairing、HID advertising、periodic report loop の実行。
- report period の採用値決定。
- Project NyX 側の `resources/swbt_hardware_bringup/settings.toml` 更新や NyXpy macro 実行。
- binary release。

## 4. 決定事項

`local_037` の実機 bring-up は、次の software gate が満たされるまで開始しない。

1. `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md`
   - output report から作った `0x21` subcommand reply を queue に入り、BTstack can-send event で periodic `0x30` より先に送れる。
   - send failure で queued reply を保持できる。
   - 実機 acceptability はこの gate では判定しない。
2. `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md`
   - production daemon が `127.0.0.1` の JSON Lines IPC endpoint を起動し、NyXpy または debug IPC client が接続できる。
   - IPC side と BTstack-owning side の state handoff が data race にならない。
   - endpoint は daemon log または実行メタデータから記録できる。
3. `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`
   - `swbt-daemon.exe` が no-op backend ではなく production backend を選べる。
   - `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` がない実行では、Bluetooth adapter を開く前に失敗する。
   - HID registration、output report callbacks、input report timer、IPC runner、BTstack run loop の start / cleanup order が software test で固定されている。
4. `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md`
   - production daemon が Windows console control event で HCI power-off と BTstack run loop exit trigger を実行できる。
   - 停止要求の重複で HCI power-off を二重実行しない。
   - 実機停止の所要時間と cleanup 観測は `local_037` に残す。
5. `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
   - Windows native + 専用 USB Bluetooth dongle + WinUSB で daemon を起動し、Switch pairing、HID advertising、report loop、NyXpy input observation、neutral fail-safe を記録する。

`work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md` は first bring-up の hard gate ではない。実装済みであれば診断に使ってよいが、stable status protocol が未完成であることだけを理由に `local_037` を止めない。adapter identity、driver state、actual report rate、jitter を measured value として公開 contract にする場合は `local_039` の範囲で扱う。

NyXpy handoff で `local_037` へ渡す値は次の通りである。

- swbt commit。
- BTstack submodule commit。
- daemon executable path。
- backend。Windows 実機では `windows-winusb`。
- IPC endpoint。
- daemon log path。
- report period。
- dongle VID/PID と driver state。
- Switch firmware。判明している場合だけでよい。
- NyX artifact root。`run_context.json`、`ipc_session.json`、`hardware_log_draft.md` を辿れる場所。

実機操作は `hardware-harness` の承認境界に従う。承認範囲に Bluetooth adapter open、pairing、advertising、report loop、IPC input、cleanup confirmation のどれを含めるかを `local_037` と `docs/hardware-test-log.md` に分けて記録する。

## 5. 根拠

この spec は新しい Switch protocol byte、BTstack source selection、WinUSB 実測値、report period 採用値を追加しない。根拠は既存の implementation fact と work unit record の未解決事項である。

| 項目 | 根拠 | source | status |
|---|---|---|---|
| daemon entrypoint defaults to no-op and can select production backend | implementation fact | `apps/swbt-daemon/main.c`, `spec/architecture/daemon-runtime-boundaries.md` | current software gate |
| BTstack send-ready integration gate is complete | work unit source | `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md` | complete; hardware acceptability observed in `local_037` |
| production IPC runner is available and production entrypoint is wired | implementation fact | `spec/architecture/daemon-runtime-boundaries.md`, `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md`, `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md` | software gate complete; hardware run observed in `local_037` |
| production daemon shutdown path is available | implementation fact / source audit | `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md` | software gate complete; hardware stop observation recorded in `local_037` |
| state mailbox is the IPC/report boundary | implementation fact | `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md` | software synchronized boundary complete; IPC input observed in `local_037` |
| Windows native hardware gate | operations spec | `spec/operations/windows-native-preflight.md` | current |
| NyXpy handoff artifact logging | docs template | `docs/hardware-test-log.md` | current |

BTstack run loop startup、WinUSB adapter open、HID advertising の concrete API order を新しく実装する場合は、該当 work unit で `source-audit` を使う。

## 6. 関連 work units

- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md`
- `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`
- `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md`
- `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`
- `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md`
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md`
- `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`

## 7. 未解決事項

- なし。first bring-up sequence は `local_037` で完了した。report period の厳密な jitter / latency / drop-rate 測定や公開 diagnostics contract は、この sequence spec ではなく `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md` または後続 work unit で扱う。
