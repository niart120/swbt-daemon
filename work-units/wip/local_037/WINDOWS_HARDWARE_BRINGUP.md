# Windows Hardware Bring-Up

## 1. 概要

Windows native + 専用 USB Bluetooth dongle + WinUSB で、production daemon が Switch と実機接続できるか確認する work unit。

この work unit は `spec/operations/windows-native-preflight.md` の gate を満たした後に実行する。Switch pairing、HID advertising、periodic report loop、debug IPC client からの入力反映、report period comparison、neutral fail-safe を `docs/hardware-test-log.md` に記録する。

NyX handoff は、Project NyX の `swbt_hardware_bringup` マクロを一時的な debug IPC client として使う実行手順である。swbt-daemon 側は daemon 起動、adapter / driver / pairing / log 記録を担当し、NyX の導入、設定更新、macro 実行は Project NyX 側で行う。

2026-06-20 の準備では、入力経路として NyXpy handoff を使う方針を選んだ。準備開始時点の `swbt-daemon.exe` は no-op backend だけを使っていたため、Bluetooth adapter、IPC listener、HID advertising、periodic report loop を起動しなかった。この entrypoint 条件は `local_043` で解消済みである。実機 adapter open 前に必要だった Windows console control event からの停止経路は `local_044` で追加済みである。現行 artifact は `SWBT_DAEMON_BACKEND=production` で production backend を選べるが、実機 daemon run、Switch pairing、HID advertising、report loop、実機停止時 cleanup はまだ観測していない。

## 2. 起点 / ユースケース

source:

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md` の Phase 5: Windows 実機確認。
- `spec/operations/windows-native-preflight.md` の未解決事項。Windows native execution、WinUSB driver assignment、Bluetooth dongle recognition、Switch pairing、report period comparison、neutral fail-safe は未検証である。
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md` の先送り事項。実 Bluetooth adapter を開く順序と失敗時 cleanup は fake backend test だけでは証明できない。
- `work-units/complete/local_026/REPORT_METRICS_AND_LOGGING.md` の先送り事項。actual report rate、jitter、adapter identity、driver state は fake timestamp からは証明できない。
- `spec/protocols/switch-hid-core.md` の未解決事項。current HID descriptor、subcommand replies、player lights replies、virtual SPI seed、rumble output handling の実機 acceptability は未検証である。
- NyX handoff。NyX macro を起動済み daemon の local TCP JSON Lines IPC client として使い、Switch capture と daemon log を突き合わせる一時手順。

use case:

- actor: hardware bring-up operator。
- 入力または状態: 専用 USB Bluetooth dongle、WinUSB driver assignment、Windows native daemon build、Switch pairing 画面、debug IPC client または NyX `swbt_hardware_bringup` macro。
- 期待する観測結果: daemon が対象 dongle を開き、Switch と pairing し、periodic input report を送り、IPC client からの state snapshot が button / stick 入力として観測できる。NyX macro を使う場合は、IPC request、Switch capture、daemon log が同じ実行条件へ対応付けられる。disconnect、owner release、timeout、process exit では neutral state へ戻る。
- 制約: 対象 adapter を曖昧にしない。内蔵または普段使いの Bluetooth adapter を使わない。実機コマンドは人間の明示承認なしに実行しない。
- 対象外: 自動 hardware test framework、複数 controller、binary release、NFC/IR、rumble semantic decode。
- source から use case へ変換した判断: 複数の record に散らばる「実機未検証」は同じ安全境界に属するため、個別に完了扱いせず、Windows hardware bring-up の record に集約する。

## 3. 対象範囲

- `spec/operations/windows-native-preflight.md` に従って、実行前確認項目を記録する。
- 専用 USB Bluetooth dongle の VID/PID と driver state を確認する。
- Windows native daemon build を起動する。
- Switch pairing を実行する。
- HID advertising と connection state を daemon log で確認する。
- report period `8000 / 8333 / 15000 / 16667 us` を比較する。
- debug IPC client から neutral、button、stick state を送る。
- NyX handoff を使う場合は、swbt commit、BTstack ref、dongle VID/PID、driver state、Switch firmware、IPC endpoint、daemon log path を Project NyX 側へ渡す。
- NyX artifact root、`run_context.json`、`ipc_session.json`、`hardware_log_draft.md`、daemon log を突き合わせる。
- owner disconnect、heartbeat timeout、daemon shutdown の neutral fail-safe を確認する。
- `docs/hardware-test-log.md` に OS、dongle、driver、backend、BTstack commit、swbt commit、Switch firmware、report period、result、cleanup を記録する。

## 4. 対象外

- 普段使いの Bluetooth adapter を使う検証。
- 内蔵 Bluetooth を使う検証。
- Linux/libusb 実機 bring-up。
- 自動 hardware test label の追加。
- swbt-daemon repo への NyX 用仮想環境作成。
- マシン全体への `nyxpy` 導入。
- Project NyX repo の `resources/swbt_hardware_bringup/settings.toml` 更新と NyX macro 実行。
- firmware / driver の一般互換性保証。
- NFC/IR、amiibo、複数 controller、rumble semantic decode。
- binary release。

## 5. 関連 spec / docs

- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/protocols/daemon-ipc-v1.md`
- `spec/protocols/switch-hid-core.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `spec/operations/windows-native-preflight.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`
- `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md`
- `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md`
- `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`
- `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md`
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md`
- `work-units/complete/local_026/REPORT_METRICS_AND_LOGGING.md`
- `work-units/complete/local_027/WINDOWS_NATIVE_PREFLIGHT.md`

## 6. 根拠監査

`source-audit` と `hardware-harness` を使う。

この work unit では hardware observation を追加する。OS、driver、dongle、Switch firmware、BTstack commit、swbt commit、backend、report period を分けて記録し、別環境の一般的事実として扱わない。

report period の採用判断は実測後に行う。`8000us` は current configurable default であり、実機で最適値として確認済みの値ではない。

### 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| BTstack HID SDP record builder | caller-provided `uint8_t *service` に書き込む | source fact | `vendor/btstack/src/classic/hid_device.h`, `vendor/btstack/src/classic/hid_device.c` の `hid_create_sdp_record` | fixed BTstack source |
| BTstack SDP record length reader | `de_get_len` で生成済み record length を読む | source fact | `vendor/btstack/src/classic/sdp_util.h`, `vendor/btstack/src/classic/sdp_util.c` | fixed BTstack source |
| detailed diagnostic crash marker | `hid_registration: sdp record too large` | hardware observation | `tmp/hardware/local_037/20260620-8000us-detailed-diagnostic-rerun/startup-trace.txt` | CSR8510 A10 / Windows local observation |
| generated Switch HID SDP record length | `404` bytes | implementation fact | `tests/daemon_production_hid_sdp_record_test.c` red output: `required=404 capacity=300` | covered by regression test |
| production HID service buffer capacity | `512` bytes | implementation fact | `swbt/daemon/production_backend.h` | configurable implementation capacity, not protocol fact |
| fixed rerun startup marker | `hid_registration: ok` and `btstack: hci power on ok` | hardware observation | `tmp/hardware/local_037/20260620-234720-8000us-fixed-rerun/startup-trace.txt` | CSR8510 A10 / Windows local observation |
| direct cleanup marker | `runtime: stop done` and `production: runtime stop done` | hardware observation | `tmp/hardware/local_037/20260620-235943-8000us-cleanup-direct-rerun/startup-trace.txt` | CSR8510 A10 / Windows local observation |
| Switch2 pairing observation | pairing screen showed no device | hardware observation | `tmp/hardware/local_037/20260621-000629-8000us-pairing/startup-trace.txt` and user observation | Switch2 firmware `22.1.0`; not generalized |
| BTstack HID keyboard discoverability setup | `gap_discoverable_control(1)`, class of device, local name, link policy, role switch are configured before power-on | source fact | `vendor/btstack/example/hid_keyboard_demo.c:452-460` | fixed BTstack source |
| BTstack Classic GAP discoverable default | discoverable is OFF by default | source fact | `vendor/btstack/src/gap.h:1222-1234` | fixed BTstack source |
| BTstack Classic GAP configuration APIs | `gap_set_local_name`, `gap_set_class_of_device`, `gap_set_default_link_policy_settings`, `gap_set_allow_role_switch` | source fact | `vendor/btstack/src/gap.h:417-443` | fixed BTstack source |
| production Classic GAP discovery config | discoverable `true`, class of device from production HID subclass `0x2508`, local name `Pro Controller`, link policy role switch + sniff, allow role switch `true` | implementation fact | `swbt/btstack_bridge/production_btstack.c`, `swbt/btstack_bridge/classic_discovery.c` | implemented for next hardware rerun; Switch2 acceptability unverified |
| GAP discovery rerun marker | `btstack: classic discovery configure ok` before `btstack: hci power on`; Switch2 pairing screen did not move | hardware observation | `tmp/hardware/local_037/20260621-004439-8000us-gap-discovery-pairing/startup-trace.txt` and user observation | CSR8510 A10 / Switch2 firmware `22.1.0`; red continues |
| HCI scan enable after GAP config | `Write Scan Enable` value `0x03` returned status `0x00` | hardware observation | `tmp/hardware/local_037/20260621-005526-8000us-hci-dump-pairing/hci-dump.txt` lines 163-164 | CSR8510 A10 / Windows local observation |
| HCI incoming connection after scan enable | `Connection_incoming` and `Connection_complete (status=0)` from `C8:48:05:F7:B5:21` repeated | hardware observation | `tmp/hardware/local_037/20260621-005526-8000us-hci-dump-pairing/hci-dump.txt` | Switch-side address inference; pairing screen did not visibly move |
| SSP confirmation handling expected by BTstack HID examples | `HCI_EVENT_USER_CONFIRMATION_REQUEST` is handled by calling `gap_ssp_confirmation_response` | source fact | `vendor/btstack/example/hid_keyboard_demo.c:366-371`, `vendor/btstack/example/hid_mouse_demo.c:237-242` | fixed BTstack source |
| BTstack SSP auto accept default | `ssp_auto_accept = 0` | source fact | `vendor/btstack/src/hci.c:5575-5579` | fixed BTstack source |
| HCI dump SSP failure marker | user confirmation request is followed by `Simple Pairing Complete` status `0x13`; no `User Confirmation Request Reply` opcode `0x042c` is visible | hardware observation / inference | `tmp/hardware/local_037/20260621-005526-8000us-hci-dump-pairing/hci-dump.txt` | next software fix target; post-fix hardware rerun required |
| production SSP confirmation handler | reverses the HCI event address and calls backend `ssp_confirm_user_confirmation` | implementation fact | `swbt/daemon/production_backend.c`, `tests/daemon_production_backend_test.c` | covered by unit regression; hardware acceptability unverified |
| SSP confirmation post-fix rerun marker | user confirmation request is still followed by `Simple Pairing Complete` status `0x13`; no `User Confirmation Request Reply` opcode `0x042c` is visible | hardware observation / inference | `tmp/hardware/local_037/20260621-010951-8000us-ssp-confirm-hci-dump-pairing/hci-dump.txt` | production handler was not subscribed to HCI events |
| BTstack HID examples event registration | same `packet_handler` is registered with both `hci_add_event_handler` and `hid_device_register_packet_handler` | source fact | `vendor/btstack/example/hid_keyboard_demo.c:507-510`, `vendor/btstack/example/hid_mouse_demo.c:345-348` | fixed BTstack source |
| BTstack HCI event handler registration | `hci_add_event_handler` appends callback registration to `hci_stack->event_handlers` | source fact | `vendor/btstack/src/hci.c:5383-5385`, `vendor/btstack/src/hci.h:1561` | fixed BTstack source |
| BTstack HID device packet handler registration | `hid_device_register_packet_handler` only stores the HID device callback | source fact | `vendor/btstack/src/classic/hid_device.c:882-884` | fixed BTstack source |
| production HID BTstack adapter HCI registration | production packet handler is registered for both HCI events and HID device events | implementation fact | `swbt/btstack_bridge/hid_device_btstack_adapter.c`, `tests/btstack_hid_device_btstack_adapter_test.c` | covered by unit regression; post-fix hardware rerun required |
| HCI event handler rerun SSP confirmation | `User Confirmation Request Reply` opcode `0x042c` is sent; `Simple Pairing Complete` status is `0x00` | hardware observation | `tmp/hardware/local_037/20260621-012253-8000us-hci-event-handler-hci-dump-pairing/hci-dump.txt` | CSR8510 A10 / Switch2 firmware `22.1.0` |
| HCI event handler rerun HID L2CAP channels | PSM `0x11` and PSM `0x13` opened with status `0x0` | hardware observation | `tmp/hardware/local_037/20260621-012253-8000us-hci-event-handler-hci-dump-pairing/hci-dump.txt` | Bluetooth HID control / interrupt channels open |
| 8000us input report loop after HID channel open | 57 byte ACL packets from swbt to Switch-side interrupt channel were sent 548 times | hardware observation | `tmp/hardware/local_037/20260621-012253-8000us-hci-event-handler-hci-dump-pairing/hci-dump.txt` | neutral report loop observed; report interval not analyzed here |
| Switch HID output after HID channel open | no `packet type=0x02 in=1` payload beyond L2CAP setup was observed | hardware observation / inference | `tmp/hardware/local_037/20260621-012253-8000us-hci-event-handler-hci-dump-pairing/hci-dump.txt` | next diagnosis target; UI did not change |
| single-shot IPC input observation | `1877` outgoing 57 byte ACL packets all had the same timer-excluded neutral state; incoming ACL payloads were L2CAP setup only | hardware observation / inference | `tmp/hardware/local_037/20260621-013528-8000us-hci-event-handler-hci-dump-pairing/hci-dump.txt` | exact client command / stdout not captured; held-state input still required |
| NyXPy held-input IPC timeout | macro initialized `scenario=held_input_probe` then timed out waiting for daemon IPC response | software observation / inference | user-provided NyXPy stdout at 2026-06-21 01:56 and swbt production IPC code inspection | production daemon listened but did not pump accept / serve during BTstack run loop |
| production IPC pump | `swbt_daemon_ipc_runner_poll_once` checks readable listener / connection sockets and accepts or serves one ready request; BTstack production schedules it every 1 ms | implementation fact | `swbt/ipc/ipc_server.c`, `swbt/daemon/ipc_runner.c`, `swbt/btstack_bridge/production_btstack.c`, `tests/daemon_ipc_runner_test.c` | covered by non-hardware tests; hardware rerun required |
| NyXPy held-input IPC success | `hello_ok`, `acquired`, Button A `state_accepted`, neutral `state_accepted`, and cleanup `release_sent=true` were recorded | hardware observation | Project NyX artifact `resources/swbt_hardware_bringup/artifacts/20260621T022219_74b5/swbt_hardware_bringup/handoff/ipc_session.json` | IPC client path pass on this run |
| NyXPy held Button A HCI report | 57 byte input reports were sent `1093` times; timer-excluded states were neutral `1034` and Button A `0x000008` `59` | hardware observation | `tmp/hardware/local_037/20260621-022214-8000us-held-input-nyxpy/hci-dump.txt` | daemon state reached HCI; Switch UI reaction still red |
| NyXPy held Button A Switch capture | baseline, Button A, and neutral captures all showed the same `L + R` prompt | hardware observation | Project NyX artifact `resources/swbt_hardware_bringup/artifacts/20260621T022219_74b5/swbt_hardware_bringup/observations/*.png` | Button A alone is not evidence of UI adoption |

## 7. 設計メモ

- 実機実行前に adapter identity を固定する。
- daemon cleanup behavior が不明な code path は実行しない。
- repo-local debug IPC client は software integration 済みである。NyX handoff の macro は、Project NyX 側の capture と request artifact を同じ実機 run に対応付けたい場合の外部 client 経路として扱う。
- swbt-daemon 側 agent は NyX を導入しない。swbt 側は daemon 起動、IPC endpoint、daemon log、adapter / driver / firmware 情報を渡し、NyX 側 agent が Project NyX repo で `uv run nyxpy run swbt_hardware_bringup` を実行する。
- NyX artifact と daemon log の対応は、NyX `request.id`、時刻、report period、daemon log excerpt で確認する。
- report period comparison は period ごとに別記録にする。
- 実機結果は `docs/hardware-test-log.md` を正本にし、work unit record には要約と実行条件を残す。
- 2026-06-20 時点で `swbt-daemon.exe` は `SWBT_DAEMON_BACKEND=production` により production backend を選べる。実機 daemon run、pairing、advertising、report loop はこの record の承認範囲と hardware log 記録に従う。
- 実機 bring-up の開始順は `spec/operations/windows-hardware-bringup-sequence.md` に従う。`local_038`、`local_042`、`local_043` は完了済み software gate であり、この record は実機観測と hardware log を扱う。
- 現行 daemon は IPC endpoint を stdout / stderr へ出力しない。最初の NyXpy run では `SWBT_IPC_PORT=37637` を明示し、Project NyX 側の `swbt_hardware_bringup` macro へ同じ `ipc_port` を渡す。`SWBT_IPC_PORT=0` の自動 port は、endpoint log または実行 metadata で port を確認できるまで使わない。
- 2026-06-20 にユーザは `CSR8510 A10` を対象に実験を進めることを承認した。NyXpy 操作はユーザが行う。swbt-daemon 側は CSR8510 A10 に限定し、内蔵 `MediaTek Bluetooth Adapter` と常用 Bluetooth device は対象外にする。
- `local_044` で production daemon の Ctrl+C / Windows console control event 経路を追加した。停止要求は HCI power-off と BTstack run loop exit trigger を 1 回だけ呼ぶ。実機では foreground console から停止し、所要時間と cleanup 到達を `docs/hardware-test-log.md` に記録する。
- 2026-06-20 の `8000us` 実機 run は process start 直後に `0xC0000005` で APPCRASH し、stdout / stderr log と WER LocalDumps は残らなかった。次回の切り分けでは `SWBT_DIAGNOSTIC_TRACE_PATH` による起動トレースと、Windows の `SWBT_CRASH_DUMP_PATH` による daemon 自前 minidump を使う。
- 2026-06-20 の SDP record buffer fix 後の `8000us` 再実行では、`hid_registration: ok`、`btstack: hci power on ok`、`production: run loop execute` まで到達した。前回の `hid_registration: sdp record too large` は再発していない。手動 `Ctrl+C` 後に `btstack: hci power off` と `production: run loop returned` へ到達したが、cleanup trace は `runtime: stop enter` で終わっており、停止完了は未確認である。
- 2026-06-21 の `8000us` cleanup 直接再実行では、`Tee-Object` を使わず foreground daemon を直接起動した。手動 `Ctrl+C` 後に `runtime: report timer stop`、`runtime: output handler stop`、`runtime: hid stop`、`production: platform stop`、`btstack: hci close done`、`btstack: run loop deinit done`、`runtime: ipc stop`、`runtime: stop done`、`production: runtime stop done` まで到達し、exit marker は `exit=0` だった。これにより Ctrl+C cleanup の実機観測は pass とする。
- 2026-06-21 の `8000us` pairing attempt では、Switch2 firmware `22.1.0` の pairing 画面に controller が表示されなかった。daemon は `btstack: hci power on ok`、`production: run loop execute` まで到達し、cleanup も `production: runtime stop done` まで到達したため、次の切り分け対象は HID registration 後の discoverable / connectable / class of device / local name / advertising 相当の状態である。
- BTstack の `hid_keyboard_demo.c` は `hci_power_control(HCI_POWER_ON)` の前に `gap_discoverable_control(1)`、class of device、local name、link policy、role switch を設定している。`gap.h` は Classic discoverable が既定で OFF であることを示す。2026-06-21 の修正では、production BTstack platform start で `hci_init` 後、`l2cap_init` と power-on の前に Classic GAP discovery config を適用する。これは discoverable 設定不足への実装修正であり、Switch2 22.1.0 での pairing 成功はまだ未検証である。
- 2026-06-21 の Classic GAP discovery config 修正後 rerun では、trace に `btstack: classic discovery configure ok` が記録されたが、Switch2 pairing 画面は動かなかった。daemon startup trace だけでは、BTstack がどの HCI command を controller へ送ったか、controller が status を返したかを確認できない。次の診断では `SWBT_HCI_DUMP_TRACE_PATH` を追加し、HCI command / event の text dump を artifact に残す。
- 2026-06-21 の HCI dump 付き pairing rerun では、`Write Scan Enable` value `0x03` が status `0x00` で返り、`C8:48:05:F7:B5:21` から incoming connection が複数回来た。発見可能化の不足ではなく、SSP pairing 中の `HCI_EVENT_USER_CONFIRMATION_REQUEST` に応答していないことが次の失敗点である。BTstack HID examples はこの event に対して `gap_ssp_confirmation_response` を呼ぶ。production handler は HID meta event だけを処理していたため、SSP confirmation を ops 境界へ追加した。post-fix の Switch2 pairing 成功は未検証である。
- 2026-06-21 の SSP confirmation fix 後 rerun では、Switch2 から incoming connection と `HCI_EVENT_USER_CONFIRMATION_REQUEST` は来たが、HCI dump には `User Confirmation Request Reply` opcode `0x042c` が出なかった。BTstack の `hid_device_register_packet_handler` は HID device callback を保存するだけで、HCI event list には登録しない。BTstack HID examples と同じく production packet handler を `hci_add_event_handler` にも登録する。
- 2026-06-21 の HCI event handler registration fix 後 rerun では、`User Confirmation Request Reply` opcode `0x042c` が送信され、`Simple Pairing Complete` は status `0x00` になった。PSM `0x11` と `0x13` の L2CAP channel も status `0x0` で開いたため、Bluetooth SSP pairing と HID control / interrupt channel open は pass とする。Switch2 側の画面は変化せず、Switch からの HID output report / subcommand は観測されていないため、次の切り分け対象は Switch が controller として採用しない理由である。
- 2026-06-21 の単発 IPC input rerun では、HCI dump 上の 57 byte report が `1877` 個に増えたが、timer byte を除いた input report state は全件 neutral だった。これは Switch が非 neutral input を無視した根拠ではなく、`swbt-debug-client` の単発 `set_state` / `release` が 8000 us report tick に捕捉されなかった可能性を残す観測である。次の入力反映確認は NyXpy または held-state capable client で、非 neutral state を 1 秒以上保持する。
- 2026-06-21 の NyXPy `held_input_probe` 初回実行は、macro 初期化後に daemon IPC 応答待ちで `timed out` した。これは Switch 側の入力反映結果ではなく、production daemon が IPC listener を開始しても BTstack run loop 中に accept / serve を回していなかった software gate red と扱う。修正後は production BTstack platform start で 1 ms IPC pump timer を登録し、readable な listener / connection socket がある場合だけ accept または 1 request serve を行う。この時点では、修正後の NyXPy held-state input 実機再実行は未実行だった。
- 2026-06-21 の production IPC pump fix 後 NyXPy rerun では、NyXPy が `hello_ok`、`acquired`、Button A `state_accepted`、neutral `state_accepted` を受け取り、cleanup で `release_sent=true` を記録した。HCI dump では timer byte を除外した state が neutral `1034` 個、Button A `0x000008` `59` 個に分かれたため、NyXPy IPC から daemon state を経由して HCI input report へ到達する経路は pass とする。一方、Switch2 capture は baseline / Button A / neutral で同じ `L + R` prompt のままだったため、Switch UI 上の入力反映は未達である。次の held-state 入力確認は、この画面が要求する L+R 同時押しで行う。

## 8. 対象ファイル

- `docs/hardware-test-log.md`
- `spec/operations/windows-native-preflight.md`
- `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `CMakeLists.txt`
- `swbt/core/diagnostics.h`
- `swbt/core/diagnostics.c`
- `apps/swbt-daemon/main.c`
- `swbt/btstack_bridge/production_btstack.c`
- `swbt/btstack_bridge/classic_discovery.h`
- `swbt/btstack_bridge/classic_discovery.c`
- `swbt/btstack_bridge/classic_discovery_btstack_adapter.h`
- `swbt/btstack_bridge/classic_discovery_btstack_adapter.c`
- `swbt/btstack_bridge/hci_dump_text.h`
- `swbt/btstack_bridge/hci_dump_text.c`
- `swbt/btstack_bridge/hid_device_registration.c`
- `swbt/btstack_bridge/hid_device_btstack_adapter.c`
- `swbt/daemon/production_backend.h`
- `swbt/daemon/production_backend.c`
- `swbt/daemon/ipc_runner.h`
- `swbt/daemon/ipc_runner.c`
- `swbt/ipc/ipc_server.h`
- `swbt/ipc/ipc_server.c`
- `tests/diagnostics_test.c`
- `tests/daemon_production_backend_test.c`
- `tests/daemon_production_hid_sdp_record_test.c`
- `tests/daemon_ipc_runner_test.c`
- `tests/btstack_hid_device_btstack_adapter_test.c`
- `tests/btstack_classic_discovery_test.c`
- `tests/btstack_hci_dump_text_test.c`
- 実行時に必要な daemon / debug client files。

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | preflight records dedicated dongle identity and WinUSB driver state before daemon run | characterization | docs | yes |
| green | diagnostic trace writes startup markers when `SWBT_DIAGNOSTIC_TRACE_PATH` is set | new | unit | no |
| green | diagnostic rerun captures startup trace or minidump for the CSR8510 A10 crash | characterization | hardware | yes |
| green | production HID service buffer fits the BTstack SDP record generated from the Switch HID descriptor | regression | unit | no |
| green | Windows native daemon reaches HCI power-on on the CSR8510 A10 WinUSB path without using an ambiguous adapter | new | hardware | yes |
| green | direct Ctrl+C shutdown reaches HCI power-off, BTstack close, IPC stop, and runtime stop done | characterization | hardware | yes |
| green | Switch pairing reaches HID L2CAP connection state and is recorded in hardware log | new | hardware | yes |
| refactor-skipped | Classic GAP discovery configuration sets discoverable, class of device, local name, link policy, and role switch before production power-on | regression | unit | no |
| green | HCI command and event packets can be written to a text artifact when `SWBT_HCI_DUMP_TRACE_PATH` is set | characterization | unit | no |
| green | production packet handler confirms SSP user confirmation request with the reversed event address | regression | unit | no |
| green | BTstack HID adapter registers the production packet handler for both HCI events and HID device events | regression | unit | no |
| green | HCI event handler rerun sends SSP confirmation reply and opens HID control / interrupt L2CAP channels | characterization | hardware | yes |
| green | 8000us report loop sends neutral input reports after HID interrupt channel opens | characterization | hardware | yes |
| green | single-shot IPC input attempt is not observed as a non-neutral HID report in HCI dump | characterization | hardware | yes |
| green | production daemon pumps pending IPC connections and requests during the BTstack run loop | regression | unit / integration | no |
| green | NyXPy held Button A request is accepted by daemon IPC and appears as non-neutral HCI input reports | characterization | hardware | yes |
| todo | Switch sends HID output report or subcommand after HID channels open | characterization | hardware | yes |
| todo | periodic input report loop runs at each selected report period and records result | characterization | hardware | yes |
| todo | held IPC client or NyX macro state updates are observed as Switch UI button and stick changes | new | hardware | yes |
| todo | owner disconnect and heartbeat timeout leave neutral state | edge | hardware | yes |

## 10. 検証

準備として実行済み:

- 2026-06-20 `just windows-cross`: pass。`windows-mingw-debug` preset、`SWBT_BACKEND=windows-winusb`、`swbt-daemon.exe` と `swbt-debug-client.exe` の link 成功。成果物は `build/windows-mingw-debug/swbt-daemon.exe` と `build/windows-mingw-debug/swbt-debug-client.exe`。
- 2026-06-20 `git rev-parse HEAD`: `5a85016b2284a448ea7227a352b9e4928436f690`。
- 2026-06-20 `git ls-tree HEAD vendor/btstack`: `075a0780f0fad7ff67d58ac19f46e8953656a752`。
- 2026-06-20 Windows PnP preflight: `CSR8510 A10` を `USB\VID_0A12&PID_0001\9&12127A34&0&1` として検出。Service は `WinUSB`、Class は `USBDevice`、Driver provider は `libwdi`、INF は `oem75.inf`。詳細は `docs/hardware-test-log.md` の `2026-06-20: Windows native CSR8510 A10 preflight` に記録した。
- 2026-06-20 IPC port preflight: `127.0.0.1:37637` は待受なし。現行 daemon は endpoint を出力しないため、NyXpy handoff の最初の実行ではこの固定 port を使う。
- 2026-06-20 approval guard preflight: `SWBT_DAEMON_BACKEND=production`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000` を設定し、`SWBT_RUN_HARDWARE` と `SWBT_HARDWARE_APPROVED` を設定せずに `build/windows-mingw-debug/swbt-daemon.exe` を実行した。exit code は `1`。Bluetooth adapter open は未承認のため実行していない。
- 2026-06-20 Project NyX preflight: `E:\documents\VSCodeWorkspace\Project_NyX` はブランチ `feat/swbt-hardware-bringup-macro`。`macros/swbt_hardware_bringup` と `resources/swbt_hardware_bringup/settings.toml` は存在する。`settings.toml` の既定は `hardware_approved = false`、`ipc_port = 0`。NyXpy 実行時は `hardware_approved = true` と固定 `ipc_port` を Project NyX 側で渡す必要がある。Project NyX 側には `spec/agent/wip/local_021/SWBT_HARDWARE_BRINGUP_HANDOFF.md` と `spec/macro/swbt_hardware_bringup/spec.md` の削除差分があるため、swbt 側からは変更しない。
- 2026-06-20 shutdown path review: `run_loop_trigger_exit` は production backend ops に存在するが、当時の `swbt-daemon.exe` には外部停止要求から呼ぶ経路がなかった。背景プロセスとして起動した場合、Codex 側の停止手段は強制終了になり得るため、実機 adapter open は実行しなかった。
- 2026-06-20 shutdown path software gate: `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md` で Ctrl+C / Windows console control event から HCI power-off と BTstack run loop exit trigger へ到達する経路を追加した。実機での停止所要時間と cleanup は未観測である。
- `apps/swbt-daemon/main.c` と `spec/architecture/daemon-runtime-boundaries.md` の確認: `SWBT_DAEMON_BACKEND=production` のときだけ production backend を選び、approval 環境変数が欠ける場合は実 Bluetooth adapter を開く前に失敗する。
- 2026-06-20 resumed preflight after `local_044` merge: ブランチ `local-037-hardware-verification`、`git rev-parse HEAD` は `c090ab1cc463066a0f1bfa047b583f2ff0589b4a`、`git ls-tree HEAD vendor/btstack` は `075a0780f0fad7ff67d58ac19f46e8953656a752`。
- 2026-06-20 current Windows cross build: `just windows-cross` は pass。成果物は `build/windows-mingw-debug/swbt-daemon.exe` と `build/windows-mingw-debug/swbt-debug-client.exe`。
- 2026-06-20 current CSR8510 A10 preflight: `Get-PnpDevice` で Status `OK`、Class `USBDevice`、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1` を確認。`Get-PnpDeviceProperty` で Service `WinUSB`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385` を確認。
- 2026-06-20 current IPC port preflight: `Test-NetConnection 127.0.0.1:37637` は `TcpTestSucceeded=False`。
- 2026-06-20 current approval guard preflight: `SWBT_DAEMON_BACKEND=production`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000` を設定し、`SWBT_RUN_HARDWARE` と `SWBT_HARDWARE_APPROVED` を設定せずに current `build/windows-mingw-debug/swbt-daemon.exe` を実行した。exit code は `1`。Bluetooth adapter open は未実行。
- 2026-06-20 `8000us` initial run: ユーザ承認後に foreground PowerShell で production daemon を起動した。PowerShell exit marker は `exit=-1073741819`。Windows exception としては `0xC0000005` access violation である。`tmp/hardware/local_037/20260620-8000us-initial/daemon-8000us.log` は作成されず、`daemon-8000us-exit.txt` だけが作成された。Windows Event Log / WER は `APPCRASH`、faulting module `ntdll.dll`、exception code `0xc0000005`、fault offset `0x000000000002048a`、loaded module に `WINUSB.DLL` を記録した。Switch pairing、HID advertising、report loop、NyXpy IPC input、Ctrl+C cleanup は未観測。
- 2026-06-20 `8000us` dump rerun: ユーザ承認後に同じ条件で daemon を再実行した。PowerShell exit marker は再び `exit=-1073741819`。`tmp/hardware/local_037/20260620-8000us-dump-rerun` には `daemon-8000us-exit.txt` だけが残った。HKCU LocalDumps key は確認できたが `.dmp` は保持されず、WER temporary `.tmp.mdmp` は copy 前に削除されていた。HKLM LocalDumps の設定は access denied で未設定。
- 2026-06-20 TDD red: `tests/diagnostics_test.c` を追加し、`just build-debug` を実行。`core/diagnostics.h` が存在しないため fail した。
- 2026-06-20 diagnostic trace implementation: `swbt/core/diagnostics.c` と `SWBT_DIAGNOSTIC_TRACE_PATH` を追加し、production daemon startup / BTstack startup の marker を挿入した。Windows では `SWBT_CRASH_DUMP_PATH` が設定されている場合だけ `SetUnhandledExceptionFilter` と `MiniDumpWriteDump` を使う。`SWBT_DIAGNOSTIC_TRACE_PATH` と `SWBT_CRASH_DUMP_PATH` が未設定なら通常実行への影響は no-op である。
- 2026-06-20 `just test-debug`: pass。28/28 tests passed。`diagnostics_test` は `SWBT_DIAGNOSTIC_TRACE_PATH` 設定時に marker が file へ書かれることを確認した。
- 2026-06-20 `just windows-cross`: pass。`swbt-daemon.exe` は `dbghelp` link を含めて再生成済み。途中で `dbghelp.h` include order と `setenv` の MinGW 非対応を修正した。
- 2026-06-20 `just format`: pass。
- 2026-06-20 `8000us` diagnostic rerun: ユーザ承認後に `SWBT_DIAGNOSTIC_TRACE_PATH` と `SWBT_CRASH_DUMP_PATH` を設定して daemon を再実行した。PowerShell exit marker は `exit=-1073741819`。`tmp/hardware/local_037/20260620-8000us-diagnostic-rerun` には `daemon-8000us-exit.txt` と `startup-trace.txt` だけが残り、`swbt-daemon-crash.dmp` は作成されなかった。trace は `production: enter main`、`btstack: l2cap init`、`btstack: hci close`、`btstack: run loop deinit` まで到達し、`btstack: hci power on` には到達していない。これにより crash は USB power-on 前、HID registration 失敗時 cleanup またはその直後に絞られた。
- 2026-06-20 detailed diagnostic build: `sdp_register_service`、`hid_device_init`、runtime start / stop、production HID registration cleanup、platform stop の前後 marker を追加した。`SWBT_CRASH_DUMP_PATH` では top-level unhandled exception filter に加え、access violation 用の vectored exception handler も使う。
- 2026-06-20 detailed diagnostic validation: `just format` pass、`just test-debug` pass（28/28）、`just windows-cross` pass、`just format-check` pass、`git diff --check` exit 0（CRLF warning のみ）。
- 2026-06-20 `8000us` detailed diagnostic rerun: ユーザ承認後に詳細診断 build で daemon を再実行した。PowerShell exit marker は `exit=-1073741819`。`tmp/hardware/local_037/20260620-8000us-detailed-diagnostic-rerun` には `daemon-8000us-exit.txt`、`startup-trace.txt`、`swbt-daemon-crash.dmp` が残った。trace は `hid_registration: sdp record too large` を記録した。root cause は production HID service buffer が BTstack の生成する Switch SDP record を収容できないこと。
- 2026-06-20 SDP record regression red: `tests/daemon_production_hid_sdp_record_test.c` を追加し、`just debug` を実行。`daemon_production_hid_sdp_record_test` が `production HID service buffer too small: required=404 capacity=300` で fail した。
- 2026-06-20 SDP record regression green: `SWBT_DAEMON_PRODUCTION_HID_SERVICE_BUFFER_SIZE` を `512u` に変更した。BTstack `hid_create_sdp_record` は出力先 size を受け取らないため、`swbt_btstack_hid_device_register` は 1024 byte scratch に SDP record を生成し、`de_get_len` 確認後に caller の永続 service buffer へコピーする。
- 2026-06-20 fixed build validation: `just format` pass、`just debug` pass（29/29）、`just windows-cross` pass、`just format-check` pass、`git diff --check` exit 0（CRLF warning のみ）。
- 2026-06-20 `8000us` fixed rerun: ユーザ承認後に SDP record buffer fix build で daemon を再実行し、手動 `Ctrl+C` で停止した。PowerShell exit marker は `exit=-1`。`tmp/hardware/local_037/20260620-234720-8000us-fixed-rerun` には `daemon-8000us-exit.txt` と `startup-trace.txt` が残り、`swbt-daemon-crash.dmp` と `daemon-8000us.log` は作成されなかった。trace は `hid_registration: sdp register service`、`hid_registration: hid device init`、`hid_registration: ok`、`btstack: hci power on ok`、`production: run loop execute` まで到達した。前回の `hid_registration: sdp record too large` は再発していない。手動停止後の cleanup は `btstack: hci power off`、`production: run loop returned`、`runtime: stop enter` までで、`runtime: stop done` は未確認である。
- 2026-06-21 `8000us` cleanup direct rerun: ユーザ承認後に `Tee-Object` なしで daemon を foreground 直接起動し、手動 `Ctrl+C` で停止した。PowerShell exit marker は `exit=0`。`tmp/hardware/local_037/20260620-235943-8000us-cleanup-direct-rerun` には `daemon-8000us-exit.txt` と `startup-trace.txt` が残り、`swbt-daemon-crash.dmp` と daemon stdout / stderr log は作成されなかった。trace は `btstack: hci power off`、`production: run loop returned`、`runtime: report timer stop`、`runtime: output handler stop`、`runtime: hid stop`、`production: hid stop`、`production: platform stop`、`btstack: hci close done`、`btstack: run loop deinit done`、`runtime: ipc stop`、`runtime: stop done`、`production: runtime stop done` まで到達した。cleanup 直接確認は pass。
- 2026-06-21 `8000us` pairing attempt: ユーザ承認後に daemon を foreground 直接起動し、Switch2 firmware `22.1.0` の pairing 画面を観測した。Switch2 側には何も表示されなかった。PowerShell exit marker は `exit=0`。`tmp/hardware/local_037/20260621-000629-8000us-pairing` には `daemon-8000us-exit.txt` と `startup-trace.txt` が残り、`swbt-daemon-crash.dmp` と daemon stdout / stderr log は作成されなかった。trace は `btstack: hci power on ok`、`production: run loop execute`、手動停止後の `runtime: stop done`、`production: runtime stop done` まで到達した。
- 2026-06-21 Classic GAP discovery TDD red: `tests/btstack_classic_discovery_test.c` と test target を追加し、`just build-debug` を実行した。`swbt_btstack_classic_discovery_configure` の undefined reference で fail したため、期待通りの red と判断した。
- 2026-06-21 Classic GAP discovery green: `swbt/btstack_bridge/classic_discovery.c` と BTstack adapter を追加し、production startup で `hci_init` 後、`l2cap_init` と power-on の前に Classic GAP discovery config を適用するようにした。`just build-debug` は pass。`CTEST_ARGS='-R btstack_classic_discovery_test' just test-debug` は pass。`just format` は pass。
- 2026-06-21 Classic GAP discovery validation: `just test-debug` は pass（30/30）。`just format-check` は pass。`just windows-cross` は pass。`git diff --check` は exit 0（CRLF warning のみ）。
- Test desiderata: 追加 test は regression / unit。BTstack 実機状態を固定せず、fake backend に渡る設定値と call order だけを確認するため fast / deterministic / isolated に寄せた。Switch2 22.1.0 で発見されるかは predictive な hardware-gated item として残す。
- 2026-06-21 GAP discovery pairing rerun: ユーザ承認後に daemon を foreground 直接起動し、Switch2 firmware `22.1.0` の pairing 画面を観測した。Switch2 側の pairing 画面は動かなかった。PowerShell exit marker は `exit=0`。`tmp/hardware/local_037/20260621-004439-8000us-gap-discovery-pairing` には `daemon-8000us-exit.txt` と `startup-trace.txt` が残った。trace は `btstack: classic discovery configure ok`、`btstack: hci power on ok`、`production: run loop execute`、手動停止後の `runtime: stop done`、`production: runtime stop done` まで到達した。
- 2026-06-21 HCI dump text TDD red: `tests/btstack_hci_dump_text_test.c` と test target を追加し、`just build-debug` を実行した。`swbt/btstack_bridge/hci_dump_text.c` が存在しないため CMake generate が fail した。期待通りの red と判断した。
- 2026-06-21 HCI dump text green: `swbt/btstack_bridge/hci_dump_text.c` を追加し、`SWBT_HCI_DUMP_TRACE_PATH` が設定された場合だけ production startup で `hci_dump_init()` に text sink を接続するようにした。`just build-debug` は pass。`CTEST_ARGS='-R btstack_hci_dump_text_test' just test-debug` は pass。
- 2026-06-21 HCI dump text validation: `just test-debug` は pass（31/31）。`just format-check` は pass。`just windows-cross` は pass。`git diff --check` は exit 0（CRLF warning のみ）。
- 2026-06-21 HCI dump pairing rerun: ユーザ承認後に `SWBT_HCI_DUMP_TRACE_PATH` を設定して daemon を foreground 直接起動し、Switch2 firmware `22.1.0` の pairing 画面を観測した。Switch2 側の pairing 画面は動かなかった。PowerShell exit marker は `exit=0`。`tmp/hardware/local_037/20260621-005526-8000us-hci-dump-pairing` には `daemon-8000us-exit.txt`、`startup-trace.txt`、`hci-dump.txt` が残った。HCI dump は `Write Scan Enable` value `0x03` status `0x00`、incoming connection、connection complete status `0`、SSP pairing start、`HCI_EVENT_USER_CONFIRMATION_REQUEST`、`Simple Pairing Complete` status `0x13` を記録した。`User Confirmation Request Reply` opcode `0x042c` は見えていない。
- 2026-06-21 SSP confirmation fix: production packet handler で `HCI_EVENT_USER_CONFIRMATION_REQUEST` を検出し、HCI event の BD_ADDR を BTstack accessor と同じ向きへ反転して backend ops の `ssp_confirm_user_confirmation` へ渡すようにした。BTstack production ops は `gap_ssp_confirmation_response` を呼ぶ。
- 2026-06-21 SSP confirmation fix validation: `just build-debug` は pass。`just test-debug` は pass（31/31）。`just format-check` は pass。`just windows-cross` は pass。`git diff --check` は exit 0（CRLF warning のみ）。host PowerShell から直接 `ctest --preset linux-debug -R daemon_production_backend_test --output-on-failure` を実行すると、CTest が container 内 path `/workspaces/swbt-daemon/...` を Windows host から解決できず `Unable to find executable` で not run になったため、標準入口の `just test-debug` を正本とする。
- 2026-06-21 SSP confirmation post-fix HCI dump pairing rerun: ユーザ承認後に `SWBT_HCI_DUMP_TRACE_PATH` を設定して daemon を foreground 直接起動し、Switch2 firmware `22.1.0` の pairing 画面を観測した。Switch2 側の pairing 画面は変化しなかった。PowerShell exit marker は `exit=0`。`tmp/hardware/local_037/20260621-010951-8000us-ssp-confirm-hci-dump-pairing` には `daemon-8000us-exit.txt`、`startup-trace.txt`、`hci-dump.txt` が残った。HCI dump は incoming connection、connection complete status `0`、SSP pairing start、`HCI_EVENT_USER_CONFIRMATION_REQUEST`、`Simple Pairing Complete` status `0x13` を記録した。`User Confirmation Request Reply` opcode `0x042c` はまだ見えていない。
- 2026-06-21 HCI event handler registration fix: BTstack HID examples は同じ packet handler を `hci_add_event_handler` と `hid_device_register_packet_handler` の両方へ登録している。`hid_device_register_packet_handler` は HCI event list へ登録しないため、production HID BTstack adapter で production packet handler を HCI events にも登録するようにした。
- 2026-06-21 HCI event handler registration fix validation: `just build-debug` は pass。`just test-debug` は pass（31/31）。`just format-check` は pass。`just windows-cross` は pass。`git diff --check` は exit 0。
- 2026-06-21 HCI event handler registration fix HCI dump pairing rerun: ユーザ承認後に `SWBT_HCI_DUMP_TRACE_PATH` を設定して daemon を foreground 直接起動し、Switch2 firmware `22.1.0` の pairing 画面を観測した。Switch2 側の画面は変化しなかった。PowerShell exit marker は `exit=0`。`tmp/hardware/local_037/20260621-012253-8000us-hci-event-handler-hci-dump-pairing` には `daemon-8000us-exit.txt`、`startup-trace.txt`、`hci-dump.txt` が残った。HCI dump は `User Confirmation Request Reply` opcode `0x042c`、`Simple Pairing Complete` status `0x00`、security level `2`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0`、57 byte ACL packet 548 個の送信を記録した。Switch 側からの `packet type=0x02 in=1` は L2CAP setup までで、HID output report / subcommand は観測されなかった。
- 2026-06-21 single-shot IPC input HCI dump pairing rerun: ユーザ承認後に `SWBT_HCI_DUMP_TRACE_PATH` を設定した daemon run で単発 IPC input を試した。Switch2 側の画面は変化しなかった。PowerShell exit marker は `exit=0`。`tmp/hardware/local_037/20260621-013528-8000us-hci-event-handler-hci-dump-pairing` には `daemon-8000us-exit.txt`、`startup-trace.txt`、`hci-dump.txt` が残った。HCI dump は SSP pairing status `0x00` と PSM `0x11` / `0x13` の channel open を再確認した。57 byte ACL packet は `1877` 個送信されたが、timer byte を除いた input report state は 1 種類の neutral state だけだった。Switch 側からの `packet type=0x02 in=1` は `8` 件で、L2CAP setup payload だけだった。
- 2026-06-21 NyXPy held_input_probe timeout: ユーザが Project NyX 側で macro を実行し、`swbt hardware bring-up IPC probe initialized: scenario=held_input_probe, steps=5` の後に `timed out` した。daemon 側 code inspection では、production IPC runner が `listen()` だけを開始し、BTstack run loop 中に `accept` / JSON Lines `serve` を呼んでいなかった。
- 2026-06-21 production IPC pump fix: `swbt_daemon_ipc_runner_poll_once` を追加し、listener / connection の readable 状態を non-blocking に確認して、ready な場合だけ accept または 1 request serve を行うようにした。production BTstack platform start では 1 ms timer で IPC pump を回す。停止時は platform stop / IPC stop で timer を外す。
- 2026-06-21 production IPC pump validation: `just format` は pass。`just format-check` は pass。`just windows-cross` は pass。`just debug` は pass（31/31）。`git diff --check` は exit 0（CRLF warning のみ）。host PowerShell から直接 `cmake --build --preset windows-mingw-debug` を実行すると、Dev Container 内の CMake cache path `/workspaces/swbt-daemon/...` を Windows host から解決できず失敗したため、標準入口の `just windows-cross` を正本とする。
- 2026-06-21 NyXPy held Button A rerun: ユーザ承認後、production IPC pump fix 後の daemon と Project NyX `held_input_probe` を組み合わせて実行した。daemon artifact は `tmp/hardware/local_037/20260621-022214-8000us-held-input-nyxpy`、NyXPy artifact は `E:\documents\VSCodeWorkspace\Project_NyX\resources\swbt_hardware_bringup\artifacts\20260621T022219_74b5`。NyXPy `ipc_session.json` は `hello_ok`、`acquired`、Button A `state_accepted`、neutral `state_accepted`、cleanup `release_sent=true` を記録した。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達し、exit marker は `exit=0`。HCI dump の 57 byte input report は `1093` 個で、timer byte を除外した state は neutral `1034` 個、Button A `0x000008` `59` 個だった。NyXPy capture は baseline / Button A / neutral ともに Switch2 の `L + R` prompt のままで、Switch UI 入力反映は未達である。

未実行:

- Switch UI 上の controller 採用、HID output report / subcommand 受信、held-state IPC input の Switch UI 反映。理由は、NyXPy held Button A は daemon IPC と HCI input report までは到達したが、Switch2 22.1.0 の画面は `L + R` prompt のまま変化せず、Switch からの HID output report / subcommand も観測されていないためである。
- owner disconnect と heartbeat timeout の実機 neutral 確認。理由は、cleanup 直接再実行では IPC owner を取得していないためである。

## 11. 実機実行条件

実機承認が必要である。

実行前にユーザから、対象 dongle、pairing、HID advertising、report loop、IPC input、cleanup 確認の範囲について明示承認を得る。

2026-06-20 の承認範囲:

- 対象 adapter: `CSR8510 A10`、`USB\VID_0A12&PID_0001\9&12127A34&0&1`。
- 許可された方向: CSR8510 A10 を対象にした実験を進める。
- 分離する操作: NyXpy の実行と Project NyX 側の設定操作はユーザが行う。
- Codex 側の停止条件: daemon の停止経路が強制終了だけになる形では、Switch pairing、HID advertising、report loop に進まない。

NyX handoff を使う場合も承認範囲は swbt-daemon 側で記録する。NyX macro の `hardware_approved = true` は Project NyX 側の実行条件であり、swbt 側の daemon 起動、pairing、advertising、report loop、cleanup 承認を代替しない。

現行 artifact での停止条件:

- `SWBT_DAEMON_BACKEND=production` を指定していない場合は実機 daemon run に進まない。
- `SWBT_IPC_PORT=0` のまま endpoint を daemon log または実行 metadata から確認できない場合は NyXpy macro を実行しない。最初の run では `SWBT_IPC_PORT=37637` を明示し、NyXpy 側にも `ipc_port=37637` を渡す。
- Codex が daemon を非対話背景プロセスとして起動し、停止が `Stop-Process` などの強制終了だけになる場合は、adapter open に進まない。最初の実機 run は Ctrl+C を送れる foreground console で起動する。

必要条件:

- 専用 USB Bluetooth dongle を使う。
- 内蔵または普段使いの Bluetooth adapter を使わない。
- Windows native host で WinUSB driver assignment を確認する。
- `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。
- `8000us` diagnostic rerun では、追加で `SWBT_DIAGNOSTIC_TRACE_PATH` と `SWBT_CRASH_DUMP_PATH` を artifact directory 配下へ設定する。
- `docs/hardware-test-log.md` に実行記録を残す。
- cleanup と neutral fail-safe の確認手順を実行前に決める。

## 12. 先送り事項

- 観測: production backend entrypoint の software gate は完了したが、Windows native + WinUSB + Switch pairing で受け入れられるかは未観測である。
  先送り理由: この work unit は実機 bring-up の記録単位であり、専用 dongle、driver state、Switch firmware、daemon log が必要である。
  次の置き場: `docs/hardware-test-log.md` とこの work unit record。

## 13. チェックリスト

- [x] 実行前承認範囲を記録した。
- [x] NyXpy handoff を今回の入力経路として選んだ。
- [x] Windows cross build を現在の `HEAD` で実行した。
- [x] 現行 daemon entrypoint が実機 run に進める状態か確認した。
- [x] 専用 dongle identity と driver state を記録した。
- [x] Windows native daemon run を実行した。2026-06-20 の初回 `8000us` run は APPCRASH したが、SDP record buffer fix 後の再実行は `btstack: hci power on ok` と `production: run loop execute` まで到達した。
- [x] `8000us` APPCRASH の dump 取得再実行を記録した。
- [x] stdout / stderr と WER dump が残らない crash に備え、daemon 自前の診断 trace / minidump 経路を追加した。
- [x] 診断ビルドで `8000us` rerun を実行し、trace / dump artifact を記録した。dump は作成されなかった。
- [x] 詳細診断ビルドで `8000us` rerun を実行し、HID registration / cleanup の trace と dump artifact を記録した。
- [x] `hid_registration: sdp record too large` の regression test を red / green で追加した。
- [x] SDP record buffer fix 後に `8000us` run を再実行した。
- [x] `8000us` direct cleanup rerun で `Ctrl+C` から `runtime: stop done` と `production: runtime stop done` まで到達することを記録した。
- [x] Switch pairing 結果を記録した。Switch2 22.1.0 では何も表示されなかったため、pairing success は未達である。
- [x] BTstack Classic GAP discoverable 設定不足の切り分けを TDD で修正した。
- [x] Classic GAP discovery config 修正後の `8000us` pairing rerun を記録した。`btstack: classic discovery configure ok` は出たが、Switch2 pairing 画面は動かなかった。
- [x] HCI command / event を text artifact に残す診断機能を追加した。
- [x] HCI dump text 診断付きの `8000us` pairing rerun を記録した。scan enable と incoming connection は確認できたが、SSP user confirmation に応答できず pairing は完了しなかった。
- [x] SSP user confirmation に応答する production handler を追加した。
- [x] SSP confirmation 修正後の HCI dump text 診断付き `8000us` pairing rerun を記録した。`User Confirmation Request Reply` opcode `0x042c` は出ず、HCI event handler 登録漏れを確認した。
- [x] production packet handler を HCI events と HID device events の両方に登録する修正を追加した。
- [x] HCI event handler 登録修正後の HCI dump text 診断付き `8000us` pairing rerun を記録した。SSP pairing は status `0x00`、HID L2CAP channel open は status `0x0` まで到達した。
- [x] `8000us` の neutral input report 送信を HCI dump で記録した。
- [x] 単発 IPC input rerun を HCI dump で記録した。非 neutral report は観測されなかったため、held-state 入力反映は未達である。
- [x] NyXPy held_input_probe timeout の daemon 側原因を切り分け、production IPC pump を追加した。
- [x] production IPC pump fix 後の NyXPy held Button A rerun を記録した。daemon IPC と HCI input report までは pass、Switch UI 入力反映は未達である。
- [ ] Switch UI 上の controller 採用、HID output report / subcommand 受信、または採用されない理由を記録した。
- [ ] report period comparison を記録した。
- [ ] Switch UI での IPC input 反映を記録した。
- [x] NyX handoff を使った場合は artifact root と daemon log path を記録した。
- [ ] neutral fail-safe を記録した。
- [x] `docs/hardware-test-log.md` を preflight 結果で更新した。
