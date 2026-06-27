# Current State And Support Matrix

この文書は、README の状態要約、`apps/swbt-daemon/main.c` の起動条件、`docs/hardware-test-log.md` の実機証跡を矛盾なく読むための状態表である。実機ログ全文はここへ転記せず、証跡としてリンクする。

## 確認済み

| 項目 | 確認済みの範囲 | 根拠 |
|---|---|---|
| 既定起動 | `swbt-daemon` は既定で production backend を選ぶ。production backend は `--adapter-location` 未指定では adapter open 前に失敗する。Bluetooth アダプターを開かない test / smoke は `--backend noop` を明示する。 | `apps/swbt-daemon/main.c`, `CMakeLists.txt`, `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`, `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md` |
| noop backend opt-in | `swbt-daemon --backend noop` または `--backend=noop` は noop backend を選び、Bluetooth アダプターを開かない。 | `apps/swbt-daemon/main.c`, `swbt/daemon/launch_options.c`, `tests/daemon_launch_options_test.c` |
| adapter location selector | production backend は `--adapter-location winusb:<location-path>` または `--adapter-location libusb:<bus>:<port-path>` を受け付ける。選択中の build backend と prefix が一致しない場合、adapter open 前に失敗する。Windows WinUSB は CSR8510 A10 で selector による adapter open と HCI power on を確認済み。 | `apps/swbt-daemon/main.c`, `swbt/daemon/launch_options.c`, `swbt/btstack_bridge/usb_adapter_location.c`, `swbt/btstack_bridge/production_btstack_impl.c`, `tests/btstack_usb_adapter_location_test.c`, `tests/daemon_production_runner_test.c`, `docs/hardware-test-log.md` |
| CLI management commands | `swbt-daemon help` は command 一覧と起動オプションを表示する。`swbt-daemon adapters` は `--adapter-location` selector 候補を表示し、HCI power on、HID advertising、Switch pairing、report loop へ進まない。`swbt-daemon config [options]` は `default -> TOML config file -> environment override` の effective config と起動時 option 状態を表示し、invalid config では nonzero で終了する。 | `apps/swbt-daemon/main.c`, `swbt/daemon/cli.c`, `swbt/btstack_bridge/production_btstack_impl.c`, `tests/daemon_cli_test.c`, `work-units/complete/local_078/DAEMON_CLI_MANAGEMENT_SURFACE.md` |
| 実機承認条件 | production backend の実装上の環境変数分岐は削除済み。実機操作は引き続き人間の明示承認、専用 USB Bluetooth ドングル、adapter location、driver / permission state 記録を必要とする。 | `apps/swbt-daemon/main.c`, `spec/operations/windows-native-preflight.md`, `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`, `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md` |
| 既知の対応構成 | Windows native、CSR8510 A10、WinUSB、Switch 2 firmware `22.1.0`（実機ログ表記は Switch2）、production backend。 | `docs/hardware-test-log.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` |
| Switch pairing / HID L2CAP | 上記構成で SSP pairing status `0x00` と PSM `0x11` / `0x13` の L2CAP open status `0x0` を観測した。 | `docs/hardware-test-log.md` |
| Switch output subcommand reply | `0x02`, `0x08`, `0x10`, `0x03`, `0x04`, `0x40`, `0x48`, `0x21`, `0x30` を含む reply sequence を観測した。 | `docs/hardware-test-log.md` |
| Switch UI 入力反映 | `0x21` reply 後、NyXPy の Button A 入力が Switch2 の画面遷移に反映された。 | `docs/hardware-test-log.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` |
| `swbt-pro` default profile | 上記構成で、`SWBT_DEVICE_INFO_PROFILE` 未指定の production run が `swbt-pro` の request device info reply と Switch UI 入力反映まで進むことを観測した。 | `docs/hardware-test-log.md`, `work-units/complete/local_049/SWBT_PRO_HARDWARE_VERIFICATION.md` |
| report period comparison | `8000 / 8333 / 15000 / 16667 us` は画面遷移までの粗い受理確認を通過した。 | `docs/hardware-test-log.md`, `spec/references/btstack-periodic-input-report-core.md` |
| neutral fail-safe | owner disconnect、heartbeat timeout、shutdown で neutral report へ戻ることを観測した。architecture cutover 後の H1 でも owner disconnect neutral と shutdown trailing neutral を確認した。shutdown neutral pending 後の `CAN_SEND_NOW` 再送失敗は software fake で power-off と run-loop exit まで確認した。 | `docs/hardware-test-log.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`, `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`, `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md` |
| 環境変数依存の限定 smoke | `local_045` 完了後の `8000us` Button A + release smoke は同じ構成で pass。 | `docs/hardware-test-log.md`, `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md` |
| 設定ファイル入力境界 | `swbt_daemon_config_apply_file()` は TOML v1.0 の `report.period_us`、`ipc.host`、`ipc.port`、`ipc.backlog`、`ipc.heartbeat_timeout_ms`、`device.profile`、`active_reconnect.switch_address`、`active_reconnect.learned.switch_address` を受け付ける。未知 key と不正値は失敗し、既存 config を部分更新しない。設定ファイル適用後に `swbt_daemon_config_apply_env()` を呼ぶと環境変数が override する。learned Switch address は `swbt_daemon_config_save_active_reconnect_learned_switch_address()` で同一 TOML file に書き戻せる。 | `swbt/daemon/config.h`, `swbt/daemon/config_file.cpp`, `tests/daemon_config_file_test.c`, `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`, `work-units/complete/local_072/ACTIVE_SWITCH_RECONNECT.md` |
| daemon 起動時の config path | `swbt-daemon --config <path>` または `--config=<path>` で TOML config file を明示できる。daemon は `default -> TOML config file -> environment override` の順で runtime config を作る。production backend では同じ config path を learned Switch address の書き戻し先 target として使う。 | `apps/swbt-daemon/main.c`, `swbt/daemon/launch_options.c`, `tests/daemon_launch_options_test.c`, `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`。 |
| architecture cutover | daemon logical state は `swbt_domain_t`、lifecycle / cleanup は `swbt_daemon_process_t`、production BTstack 操作は能力別 port group を持つ `swbt_btstack_production_ports_t` を経由する。旧 session、mailbox、runtime predecessor、production backend ops、`swbt_core` は source / tests / build graph から削除済み。H1 は dedicated adapter で pass。shutdown pending failure cleanup は `local_058` で固定済み。 | `spec/architecture/daemon-architecture-cutover.md`, `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`, `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`, `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md`, `work-units/complete/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md`, `CMakeLists.txt` |
| bonded reconnect boundary | `local_065` で TLV-backed link key DB 経路を実装して実機確認したが、Switch2 `22.1.0` は bonding を要求せず、HCI dump は `Remote not bonding, dropping local flag` を記録した。daemon restart と Switch sleep / resume では L2CAP open と Button A smoke は成立したが、どちらも再接続時に `pairing complete, status 00` が出たため、既存 bond reconnect ではなく再 pairing と扱う。この経路は現行 tree から削除済みである。 | `docs/hardware-test-log.md`, `work-units/complete/local_065/BONDED_RECONNECT_PERSISTENCE.md`, `spec/archive/bond-cache-persistence.md` |
| active reconnect request boundary | `local_073` で `--config <path>` から learned Switch address を読み、power on 後に `hid_device_connect()` 経由の active reconnect request が発行されることを実機で観測した。最初の restart run は outgoing connection が `Connection_complete (status=8)` と control PSM `0x11` の `L2CAP_EVENT_CHANNEL_OPENED status 0x8` で失敗した。その後の Switch 側再接続操作は incoming connection と `pairing complete, status 00` を記録したため、active reconnect 成功ではなく incoming pairing path と扱う。後続の manual rerun では Switch 側操作前に daemon 起点の outgoing connection が `Connection_complete (status=0)`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0`、trace の `production: hid connection opened` まで到達した。ただし `have link key db: 0` と `pairing complete, status 00` も出たため、これは daemon initiated active reconnect transport の成功であり、保存済み link key による pairing-free reconnect ではない。 | `docs/hardware-test-log.md`, `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md` |
| pairing-free active reconnect | `local_073` の link key DB rerun で、保存済み address と保存済み TLV-backed Classic link key DB から daemon initiated outgoing reconnect が `Connection_complete (status=0)`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0`、HID open、Button A smoke まで到達した。通常名へ整理した後の再試験でも `--link-key-db` と trace `btstack: link key db open ok` で同じ条件を満たした。HCI dump は `responding to link key request` と `have link key db: 1` を記録し、`Connection_incoming`、`pairing started`、`pairing complete, status 00` は記録しなかった。 | `docs/hardware-test-log.md`, `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md` |

## 未確認

| 項目 | 未確認の内容 | 次に必要な根拠 |
|---|---|---|
| 初代Switch各モデル | 初代Switch、Switch Lite、Switch OLED での pairing、subcommand sequence、input 反映。 | firmware、adapter、driver、report period を記録した実機ログ。 |
| 他のUSBドングル | CSR8510 A10 以外の Bluetooth ドングルでの WinUSB / libusb 挙動。 | VID/PID、driver、BTstack backend、HCI dump を含む実機ログ。 |
| Linux実機経路 | Linux + libusb backend での adapter open、pairing、HID report loop。software selector は `libusb:<bus>:<port-path>` を受け付けるが、実機では未確認。 | Linux host、libusb bus / port path、udev / permission、HCI dump を含む実機ログ。 |
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

`apps/swbt-daemon/main.c` は、まず読み取り専用の management command を dispatch する。`help`、`adapters`、`config` は通常の production / noop 起動へ進まずに終了する。`adapters` は USB inventory を読むだけで、BTstack platform start、HCI power on、HID advertising、Switch pairing、report loop を始めない。

通常起動では、引数なしで production backend を選ぶ。production backend は `--adapter-location` 未指定では adapter open 前に失敗する。Bluetooth アダプターを開かない test / smoke は `--backend noop` または `--backend=noop` を明示する。不正な backend 値と不正な adapter location は adapter open 前の CLI validation で失敗する。

runtime config の reusable boundary は `default -> TOML config file -> environment override` の優先順位で test されている。現行 `apps/swbt-daemon/main.c` は `--config <path>` / `--config=<path>` を受け取り、同じ path を learned Switch address の書き戻し先 target として production backend へ渡す。`--link-key-db <path>` / `--link-key-db=<path>` を指定した起動では、production BTstack が TLV-backed Classic link key DB を接続し、HCI link key notification を同じ DB へ保存する。`--adapter-location <selector>` は production BTstack の USB transport selector と production backend の missing-selector guard を設定する。診断出力 path は `--trace-path`、`--hci-dump-path`、`--crash-dump-path` を指定した起動だけで有効になる。`SWBT_DAEMON_BACKEND`、`SWBT_RUN_HARDWARE`、`SWBT_HARDWARE_APPROVED`、`SWBT_DIAGNOSTIC_TRACE_PATH`、`SWBT_HCI_DUMP_TRACE_PATH`、`SWBT_CRASH_DUMP_PATH` は current implementation の起動 mode / 診断 path selector ではない。

| 区分 | 指定 | 値 | 備考 |
|---|---|---|---|
| 起動 mode | なし | production | 既定。ただし `--adapter-location` がない production 起動は adapter open 前に失敗する。 |
| 起動 mode | `--backend noop` | noop | test / smoke 用。Bluetooth アダプターを開かない。 |
| adapter selector | `--adapter-location winusb:<location-path>` | Windows WinUSB の USB location path | Windows WinUSB build 用。指定した location path に一致した USB device interface だけを open 対象にする。 |
| adapter selector | `--adapter-location libusb:<bus>:<port-path>` | Linux libusb の bus + port path | libusb build 用。BTstack の `hci_transport_usb_set_bus_and_path(...)` へ接続する。 |
| 診断 | `--trace-path <path>` | trace 出力先 | startup / cleanup trace を残す。 |
| 診断 | `--hci-dump-path <path>` | HCI dump text 出力先 | pairing、L2CAP、HID report の証跡を残す。path が開けない場合は production run loop 前に失敗する。 |
| 診断 | `--crash-dump-path <path>` | dump 出力先 | Windows crash dump 出力先。非 Windows build では flag を受け付けるが no-op。 |
| 実機実行で推奨 | `SWBT_IPC_HOST` | `127.0.0.1` | 既定値と同じ。 |
| 実機実行で推奨 | `SWBT_IPC_PORT` | `37637` | 実機ログで使った固定 port。未指定時の既定は `0`。 |
| 実機実行で推奨 | `SWBT_REPORT_PERIOD_US` | `8000` | 既定値と同じ。比較実行では `8333 / 15000 / 16667` も使用。 |
| 任意 profile selector | `SWBT_DEVICE_INFO_PROFILE` | `swbt-pro` | 未指定時も `swbt-pro`。`mizuyoukanao-pro` は削除済み。 |
| 任意 fail-safe | `SWBT_IPC_HEARTBEAT_TIMEOUT_MS` | 例: `1000` | 既定値は `0`。heartbeat timeout neutral 実行で使用。 |

実機前の安全確認:

- 専用 USB Bluetooth ドングルを使う。
- 内蔵 Bluetooth と普段使いのドングルを対象にしない。
- ユーザが Bluetooth adapter open、Switch pairing、HID advertising、report loop、IPC input、cleanup confirmation のどこまで許可するかを明示する。
- Windows native では対象ドングルの driver が WinUSB に割り当たっていることと `winusb:<location-path>` selector を記録する。
- Linux では対象ドングルの libusb permission と `libusb:<bus>:<port-path>` selector を記録する。
- Switch firmware、dongle VID/PID、driver / permission、adapter location、BTstack commit、swbt commit、report period、artifact path を `docs/hardware-test-log.md` に記録する。

## 証跡

| 証跡 | 使う場面 |
|---|---|
| `docs/hardware-test-log.md` | 実機実行の一次ログ。巨大なため、状態表では要約だけを扱う。 |
| `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | Windows native hardware bring-up の完了記録。 |
| `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md` | 環境変数依存監査と完了後 smoke の記録。 |
| `work-units/complete/local_046/DOC_IMPLEMENTATION_STATE_ALIGNMENT.md` | README / spec の直近整合更新記録。 |
| `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md` | architecture cutover 後の Hardware Gate H1 実行記録。 |
| `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md` | shutdown neutral pending 後の再送失敗でも power-off と run-loop exit へ進む software failure cleanup の完了記録。 |
| `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md` | USB location adapter selector guard の完了記録。 |
| `apps/swbt-daemon/main.c` | backend selection と起動時 config / CLI diagnostic path の実装根拠。 |
| `swbt/daemon/process.c` | domain、IPC、BTstack port、report timer、shutdown cleanup ordering の実装根拠。 |
| `swbt/daemon/production_runner.c` | production lifecycle の実装根拠。 |
| `swbt/btstack_bridge/production_btstack_impl.c` | pinned BTstack へ接続する production port implementation の実装根拠。 |
