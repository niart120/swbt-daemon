# Current State And Support Matrix

この文書は、README の状態要約、`apps/swbt-daemon/main.c` の起動条件、`docs/hardware-test-log.md` の実機証跡を矛盾なく読むための状態表である。実機ログ全文はここへ転記せず、証跡としてリンクする。

## 確認済み

| 項目 | 確認済みの範囲 | 根拠 |
|---|---|---|
| 既定起動 | `SWBT_DAEMON_BACKEND` 未指定では noop backend を選び、Bluetooth アダプターを開かない。 | `apps/swbt-daemon/main.c` |
| production backend opt-in | `SWBT_DAEMON_BACKEND=production` のときだけ production backend を選ぶ。 | `apps/swbt-daemon/main.c` |
| 実機承認条件 | production backend は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` がない場合、adapter open 前に失敗する。 | `swbt/daemon/production_backend.c`, `tests/daemon_production_backend_test.c` |
| 既知の対応構成 | Windows native、CSR8510 A10、WinUSB、Switch 2 firmware `22.1.0`（実機ログ表記は Switch2）、production backend。 | `docs/hardware-test-log.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` |
| Switch pairing / HID L2CAP | 上記構成で SSP pairing status `0x00` と PSM `0x11` / `0x13` の L2CAP open status `0x0` を観測した。 | `docs/hardware-test-log.md` |
| Switch output subcommand reply | `0x02`, `0x08`, `0x10`, `0x03`, `0x04`, `0x40`, `0x48`, `0x21`, `0x30` を含む reply sequence を観測した。 | `docs/hardware-test-log.md` |
| Switch UI 入力反映 | `0x21` reply 後、NyXPy の Button A 入力が Switch2 の画面遷移に反映された。 | `docs/hardware-test-log.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` |
| `swbt-pro` default profile | 上記構成で、`SWBT_DEVICE_INFO_PROFILE` 未指定の production run が `swbt-pro` の request device info reply と Switch UI 入力反映まで進むことを観測した。 | `docs/hardware-test-log.md`, `work-units/complete/local_049/SWBT_PRO_HARDWARE_VERIFICATION.md` |
| report period comparison | `8000 / 8333 / 15000 / 16667 us` は画面遷移までの粗い受理確認を通過した。 | `docs/hardware-test-log.md`, `spec/references/btstack-periodic-input-report-core.md` |
| neutral fail-safe | owner disconnect、heartbeat timeout、shutdown で neutral report へ戻ることを観測した。architecture cutover 後の H1 でも owner disconnect neutral と shutdown trailing neutral を確認した。shutdown neutral pending 後の `CAN_SEND_NOW` 再送失敗は software fake で power-off と run-loop exit まで確認した。 | `docs/hardware-test-log.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`, `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`, `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md` |
| 環境変数依存の限定 smoke | `local_045` 完了後の `8000us` Button A + release smoke は同じ構成で pass。 | `docs/hardware-test-log.md`, `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md` |
| 設定ファイル入力境界 | `swbt_daemon_config_apply_file()` は TOML v1.0 の `report.period_us`、`ipc.host`、`ipc.port`、`ipc.backlog`、`ipc.heartbeat_timeout_ms`、`device.profile`、`active_reconnect.switch_address`、`active_reconnect.learned.switch_address` を受け付ける。未知 key と不正値は失敗し、既存 config を部分更新しない。設定ファイル適用後に `swbt_daemon_config_apply_env()` を呼ぶと環境変数が override する。learned Switch address は `swbt_daemon_config_save_active_reconnect_learned_switch_address()` で同一 TOML file に書き戻せる。 | `swbt/daemon/config.h`, `swbt/daemon/config_file.cpp`, `tests/daemon_config_file_test.c`, `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`, `work-units/complete/local_072/ACTIVE_SWITCH_RECONNECT.md` |
| architecture cutover | daemon logical state は `swbt_app_t`、lifecycle / cleanup は `swbt_daemon_host_t`、production BTstack 操作は能力別 port group を持つ `swbt_btstack_production_adapter_t` を経由する。旧 session、mailbox、runtime、production backend ops、`swbt_core` は source / tests / build graph から削除済み。H1 は dedicated adapter で pass。shutdown pending failure cleanup は `local_058` で固定済み。 | `spec/architecture/daemon-architecture-cutover.md`, `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`, `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`, `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md`, `work-units/complete/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md`, `CMakeLists.txt` |
| bonded reconnect boundary | `local_065` で TLV-backed link key DB 経路を実装して実機確認したが、Switch2 `22.1.0` は bonding を要求せず、HCI dump は `Remote not bonding, dropping local flag` を記録した。daemon restart と Switch sleep / resume では L2CAP open と Button A smoke は成立したが、どちらも再接続時に `pairing complete, status 00` が出たため、既存 bond reconnect ではなく再 pairing と扱う。この経路は現行 tree から削除済みである。 | `docs/hardware-test-log.md`, `work-units/complete/local_065/BONDED_RECONNECT_PERSISTENCE.md`, `spec/archive/bond-cache-persistence.md` |
| active reconnect request boundary | `local_073` で `--config <path>` から learned Switch address を読み、power on 後に `hid_device_connect()` 経由の active reconnect request が発行されることを実機で観測した。最初の restart run は outgoing connection が `Connection_complete (status=8)` と control PSM `0x11` の `L2CAP_EVENT_CHANNEL_OPENED status 0x8` で失敗した。その後の Switch 側再接続操作は incoming connection と `pairing complete, status 00` を記録したため、active reconnect 成功ではなく incoming pairing path と扱う。後続の manual rerun では Switch 側操作前に daemon 起点の outgoing connection が `Connection_complete (status=0)`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0`、trace の `production: hid connection opened` まで到達した。ただし `have link key db: 0` と `pairing complete, status 00` も出たため、これは daemon initiated active reconnect transport の成功であり、保存済み link key による pairing-free reconnect ではない。 | `docs/hardware-test-log.md`, `work-units/wip/local_073/DAEMON_CLI_LAUNCH_MODE.md` |

## 未確認

| 項目 | 未確認の内容 | 次に必要な根拠 |
|---|---|---|
| 初代Switch各モデル | 初代Switch、Switch Lite、Switch OLED での pairing、subcommand sequence、input 反映。 | firmware、adapter、driver、report period を記録した実機ログ。 |
| 他のUSBドングル | CSR8510 A10 以外の Bluetooth ドングルでの WinUSB / libusb 挙動。 | VID/PID、driver、BTstack backend、HCI dump を含む実機ログ。 |
| Linux実機経路 | Linux + libusb backend での adapter open、pairing、HID report loop。 | Linux host、libusb device、udev / permission、HCI dump を含む実機ログ。 |
| pairing-free active reconnect success | 保存済み address から daemon initiated reconnect が、新しい `pairing complete, status 00` なしに HID control / interrupt channel open と Button A smoke まで到達すること。`local_073` の manual rerun では daemon 起点の outgoing HID connection と report exchange は成立したが、`have link key db: 0` と `pairing complete, status 00` も出たため、pairing-free reconnect としては未確認。 | Switch 側操作の前提を分けた再試験、link key DB 有無の source audit / 実測、Button A smoke 付き manual rerun。 |
| daemon 起動時の config path | `swbt-daemon --config <path>` または `--config=<path>` で TOML config file を明示できる。daemon は `default -> TOML config file -> environment override` の順で runtime config を作る。production backend では同じ config path を learned Switch address の書き戻し先 target として使う。 | `apps/swbt-daemon/main.c`, `swbt/daemon/launch_options.c`, `tests/daemon_launch_options_test.c`, `work-units/wip/local_073/DAEMON_CLI_LAUNCH_MODE.md`。 |
| 厳密な遅延・jitter・取りこぼし率 | input report の実送信周期、Switch 側入力遅延、取りこぼし率。 | timestamp 付き計測、サンプル数、解析方法、測定誤差の記録。 |

## 未実装

| 項目 | 現在の扱い | 根拠 |
|---|---|---|
| daemon protocol の時間指定 macro | `tap`、`duration_ms`、`sequence`、`at_ms` は daemon protocol に実装しない。client-side helper の責務。 | `README.md`, project policy |
| 複数 controller 同時接続 | 初期範囲外。daemon は single active owner の state snapshot を扱う。 | project policy |
| NFC/IR MCU / amiibo の意味処理 | bring-up sequence を進める `0x21` reply はあるが、NFC/IR の意味状態は持たない。 | `docs/hardware-test-log.md`, `spec/protocols/switch-hid-core.md` |
| rumble 周波数 / 振幅の意味変換 | raw payload の保持と露出に留まり、actuator-safe conversion は未実装。 | `spec/references/switch-rumble-core.md`, `spec/references/switch-rumble-status-exposure.md` |
| report mode / IMU / vibration の session state persistence | reply と初期化列の一部は扱うが、session state の保存は未実装。 | `spec/protocols/switch-hid-core.md` |
| manual pairing / regulated voltage reply data | stable implementation としては未実装。 | `spec/protocols/switch-hid-core.md` |

## production 起動条件

`apps/swbt-daemon/main.c` は `SWBT_DAEMON_BACKEND=production` のときだけ production backend を選ぶ。production backend は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` がそろわない場合、実機経路を開始しない。

runtime config の reusable boundary は `default -> TOML config file -> environment override` の優先順位で test されている。現行 `apps/swbt-daemon/main.c` は `--config <path>` / `--config=<path>` を受け取り、同じ path を learned Switch address の書き戻し先 target として production backend へ渡す。production / noop 起動モードの反転、実機承認 env の整理、diagnostic path の CLI flag 化は `local_073` の後続 item として残る。

| 区分 | 環境変数 | 値 | 備考 |
|---|---|---|---|
| 必須 | `SWBT_DAEMON_BACKEND` | `production` | 未指定時は noop backend。 |
| 必須 | `SWBT_RUN_HARDWARE` | `1` | 実機実行の意図を明示する。 |
| 必須 | `SWBT_HARDWARE_APPROVED` | `1` | 人間が対象 adapter と実行範囲を承認済みであることを明示する。 |
| 実機実行で推奨 | `SWBT_IPC_HOST` | `127.0.0.1` | 既定値と同じ。 |
| 実機実行で推奨 | `SWBT_IPC_PORT` | `37637` | 実機ログで使った固定 port。未指定時の既定は `0`。 |
| 実機実行で推奨 | `SWBT_REPORT_PERIOD_US` | `8000` | 既定値と同じ。比較実行では `8333 / 15000 / 16667` も使用。 |
| 任意 profile selector | `SWBT_DEVICE_INFO_PROFILE` | `swbt-pro` | 未指定時も `swbt-pro`。`mizuyoukanao-pro` は削除済み。 |
| 任意診断 | `SWBT_DIAGNOSTIC_TRACE_PATH` | trace 出力先 | startup / cleanup trace を残す。 |
| 任意診断 | `SWBT_HCI_DUMP_TRACE_PATH` | HCI dump text 出力先 | pairing、L2CAP、HID report の証跡を残す。 |
| 任意診断 | `SWBT_CRASH_DUMP_PATH` | dump 出力先 | Windows crash dump 出力先。 |
| 任意 fail-safe | `SWBT_IPC_HEARTBEAT_TIMEOUT_MS` | 例: `1000` | 既定値は `0`。heartbeat timeout neutral 実行で使用。 |

実機前の安全確認:

- 専用 USB Bluetooth ドングルを使う。
- 内蔵 Bluetooth と普段使いのドングルを対象にしない。
- Windows native では対象ドングルの driver が WinUSB に割り当たっていることを記録する。
- Switch firmware、dongle VID/PID、driver、BTstack commit、swbt commit、report period、artifact path を `docs/hardware-test-log.md` に記録する。

## 証跡

| 証跡 | 使う場面 |
|---|---|
| `docs/hardware-test-log.md` | 実機実行の一次ログ。巨大なため、状態表では要約だけを扱う。 |
| `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | Windows native hardware bring-up の完了記録。 |
| `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md` | 環境変数依存監査と完了後 smoke の記録。 |
| `work-units/complete/local_046/DOC_IMPLEMENTATION_STATE_ALIGNMENT.md` | README / spec の直近整合更新記録。 |
| `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md` | architecture cutover 後の Hardware Gate H1 実行記録。 |
| `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md` | shutdown neutral pending 後の再送失敗でも power-off と run-loop exit へ進む software failure cleanup の完了記録。 |
| `apps/swbt-daemon/main.c` | backend selection と起動時 environment config の実装根拠。 |
| `swbt/daemon/host.c` | application、IPC、BTstack adapter、report timer、shutdown cleanup ordering の実装根拠。 |
| `swbt/daemon/production_backend.c` | 実機承認条件と production lifecycle の実装根拠。 |
| `swbt/btstack_bridge/production_btstack.c` | pinned BTstack へ接続する production adapter の実装根拠。 |
