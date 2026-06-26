# Daemon CLI Launch Mode

## 1. 概要

daemon の起動モードと一時診断出力を、環境変数中心の指定から CLI flag へ移す work unit。

現行の `SWBT_DAEMON_BACKEND=production` は、daemon host の noop 経路と production BTstack 経路を切り替える。noop は Bluetooth adapter を開かない test / smoke 用の退避経路であり、production が実際の daemon 経路である。この work unit では、その意味に合わせて既定を production に寄せ、test / smoke 時だけ `--backend noop` のように明示する設計へ移行する。

診断出力 path は永続設定ではなく、その起動だけの観測指定として扱う。`SWBT_DIAGNOSTIC_TRACE_PATH`、`SWBT_HCI_DUMP_TRACE_PATH`、`SWBT_CRASH_DUMP_PATH` は CLI flag への移行対象にする。

## 2. 起点 / ユースケース

source:

- user discussion, 2026-06-26: 実機承認 env はそろそろ外せるのではないか。診断出力 path は環境変数で有効化すると意図しない場所へファイルが作られ得るため、実行時引数の flag に寄せる案が出た。
- user discussion, 2026-06-26: backend 指定は何かを確認した結果、現行の noop は test / smoke 用であり、daemon の通常起動は production を既定にして、test 時だけ明示的に noop を与える方針が妥当と判断した。
- user discussion, 2026-06-26: CLI parser を入れて backend / 実機承認 / 診断出力を反転させる話は、設定ファイル work unit に混ぜず、別 work unit として record 執筆に留める。実装は設定ファイル方針の反映を優先した後で再開する。
- user discussion, 2026-06-26: 実機 reconnect 検証へ進む。現行 `main` は設定ファイル path と learned address target を production backend へ渡さないため、実機を開く前に `--config` 相当の起動契約を最小実装する。
- user discussion, 2026-06-26: active reconnect manual rerun で daemon initiated HID transport は成功したが、`have link key db: 0` と `pairing complete, status 00` が出た。HCI event `0x18` により link key notification は観測できたため、実験用 link key DB を指定起動だけで接続し、active outgoing path で保存と pairing-free reconnect が成立するか検証する。
- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`: 設定ファイル schema は `ipc`、`report`、`device.profile` に絞り、backend 起動モード、実機承認、診断出力 path はこの work unit へ切り出す。
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`: 環境変数を backend selection、hardware safety gate、runtime override、diagnostic sink に分類した。
- `docs/status.md`: 現行状態として、未指定では noop backend、`SWBT_DAEMON_BACKEND=production` で production backend、production には `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` が必要と記録している。

use case:

- actor: hardware operator、maintainer、unit / integration test。
- 入力または状態: daemon を引数なしで起動する通常運用、Bluetooth adapter を開かず host lifecycle だけを通したい test / smoke、実機検証時だけ trace / HCI dump / crash dump を保存したい起動。
- 期待する観測結果:
  - `swbt-daemon` は production backend を選ぶ。
  - `swbt-daemon --backend noop` は Bluetooth adapter を開かず、現行 noop host 経路を選ぶ。
  - 不正な `--backend` 値は adapter open 前に失敗する。
  - 診断出力 path は CLI flag を指定した起動だけで有効になる。
  - 診断出力 path が不正な場合は、可能な限り adapter open 前に失敗し、意図しない場所へ黙って書き込まない。
  - `--config <path>` を指定した production daemon は、default config に TOML file を適用し、その後に環境変数 override を適用する。
  - `--config <path>` で読み込んだ同じ TOML file を learned Switch address の書き戻し先として production backend に渡す。
  - test / CI / smoke は必要に応じて `--backend noop` を明示する。
- 制約: エージェント運用上の実機コマンド承認は残す。Bluetooth adapter open、Switch pairing、HID advertising、report loop は `hardware-harness` の承認境界に従う。

source から use case への変換:

`local_071` は永続設定の対象を runtime 値へ絞る。backend selection と診断出力 path は永続設定よりも起動コマンドに現れている方が事故を減らせるため、CLI parser の導入と同時に扱う。

## 3. 対象範囲

- `swbt-daemon` 用の testable CLI parser を追加する。
- `--config <path>` を設計し、daemon config file 読み込みと learned address target を production backend へ接続する。
- `--backend production|noop` を設計し、既定を production にする。
- `SWBT_DAEMON_BACKEND` の削除または互換期間を決める。
- `SWBT_RUN_HARDWARE` / `SWBT_HARDWARE_APPROVED` を production backend の code gate から削除するか、互換期間を決める。
- `--trace-path`、`--hci-dump-path`、`--crash-dump-path` を設計する。
- `SWBT_DIAGNOSTIC_TRACE_PATH`、`SWBT_HCI_DUMP_TRACE_PATH`、`SWBT_CRASH_DUMP_PATH` の削除または互換期間を決める。
- 診断出力 path の open failure policy を決める。
- test / CI / smoke entrypoint が Bluetooth adapter を開かない場合は `--backend noop` を明示するよう更新する。
- `docs/status.md` と関連 operations docs に、新しい起動モードと実機承認境界を記録する。

## 4. 対象外

- `local_071` の設定ファイル schema、TOML parser / serializer、環境変数 runtime override precedence。
- active reconnect、Switch address の取得 / 保存 / 削除。
- adapter selector、VID/PID selector、複数 Bluetooth adapter の選択 policy。
- Switch protocol byte、device info payload、report period の既定値変更。
- service manager、installer、Windows registry、binary release。

## 5. 関連 spec / docs

- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `spec/architecture/daemon-architecture-cutover.md`

## 6. 根拠監査

source-audit required for BTstack source selection。

この work unit は CLI 起動指定、環境変数削除、診断出力 path の扱いを主対象にする。Switch HID report bytes、subcommand、SPI、rumble、report period 採用値は追加または変更しない。

ただし実験用 link key DB を `production_btstack` に接続するため、BTstack platform TLV source selection を変更した。根拠は BTstack port 実装であり、`vendor/btstack/port/libusb/main.c`、`vendor/btstack/port/windows-winusb/main.c`、`vendor/btstack/port/windows-h4/main.c` は `btstack_tlv_*_init_instance()`、`btstack_tlv_set_instance()`、`btstack_link_key_db_tlv_get_instance()`、`hci_set_link_key_db()` の順に Classic link key DB を接続している。

swbt 側では `vendor/btstack` を編集せず、`CMakeLists.txt` の backend 別 link source に `platform/posix/btstack_tlv_posix.c` と `platform/windows/btstack_tlv_windows.c` を追加した。`tests/cmake/btstack_sources_test.cmake` で source selection に TLV source が含まれることを固定し、`just windows-cross` で Windows MinGW executable link まで確認した。

production 既定化は Bluetooth adapter open に直結するため、実機実行や docs の表現では `hardware-harness` を使う。

## 7. 設計メモ

- `backend` は adapter 種別ではなく daemon host backend の選択である。`production` は BTstack USB transport、HID registration、discoverable 化、HCI power on、run loop を使う。`noop` は Bluetooth adapter を開かない test / smoke 用である。
- `--config` は reconnect 実機検証の前提である。backend 既定化、実機承認 env の削除、診断 flag 化より先に実装してよい。
- `--config` で指定した TOML file は読み込み元であり、learned Switch address の書き戻し先でもある。削除境界は設定ファイル上の値の除去とする。
- CLI flag は永続設定より優先する。ただし `backend` と診断出力 path は `local_071` の設定ファイル schema には入れない。
- `--config` による file 値は、`default -> TOML config file -> environment override` の優先順位を保つ。環境変数による runtime override は現行互換として残す。
- `--experimental-link-key-db <path>` は恒久設定ではなく、active reconnect の link key 保存可否を切り分ける実験入口である。設定ファイル schema には入れず、指定された起動だけ BTstack TLV-backed Classic link key DB を接続する。
- `swbt-daemon` 引数なしを production にする場合、unit / smoke / CI で daemon binary を直接起動する箇所は `--backend noop` へ移す。
- `SWBT_RUN_HARDWARE` / `SWBT_HARDWARE_APPROVED` は code-level double gate としては削除候補である。エージェント運用上の実機承認は別物として残す。
- この record の起票時点では CLI parser 実装を開始しない。未マージの試作実装は採用済み状態として扱わず、再開時は `main` 上の現行挙動から TDD Test List の先頭 item を選ぶ。
- 診断出力 path は、明示 flag がある起動だけで有効化する。設定ファイルや残留環境変数から暗黙にファイルを書き始める設計にはしない。
- 診断出力 path の open failure は失敗として扱う方向で検討する。operator が明示した観測対象が作れないまま実機へ進むと、失敗時に根拠が残らないためである。
- 親ディレクトリの自動作成は初期案では行わない。意図しない path への作成範囲を広げないためである。
- crash dump は Windows 専用挙動を含むため、非 Windows では flag の扱いを no-op にするか、unsupported として失敗させるかを test で固定する。
- adapter selector はこの work unit の対象外だが、production 既定化後に残る安全上の違和感である。専用 WinUSB ドングル運用は docs で維持し、selector が必要なら後続 work unit に切り出す。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `swbt/daemon/launch_options.*`
- `swbt/daemon/production_backend.*`
- `swbt/daemon/host.*`
- `swbt/core/diagnostics.*`
- `swbt/btstack_bridge/production_btstack.*`
- `tests/daemon_*`
- `tests/diagnostics_test.c`
- `tests/btstack_production_hci_dump_test.c`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `work-units/wip/local_073/DAEMON_CLI_LAUNCH_MODE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | CLI parser accepts `--config <path>` / `--config=<path>` and rejects missing or unknown options before adapter open | new | unit | no |
| refactor-skipped | daemon main applies config file before environment override and passes the same config path as learned address target to production backend | new | integration | no |
| refactor-skipped | CLI parser accepts `--experimental-link-key-db <path>` / `--experimental-link-key-db=<path>` and stores it outside persistent config | characterization | unit | no |
| refactor-skipped | production BTstack platform start connects an explicitly configured experimental TLV-backed Classic link key DB and closes it on platform stop | characterization | integration | no |
| todo | active outgoing reconnect with experimental link key DB records whether link key notification is persisted to a non-empty TLV file | characterization | hardware | yes |
| todo | daemon restart after experimental link key DB persistence reaches HID channel open without a new `pairing complete, status 00` | characterization | hardware | yes |
| todo | CLI parser defaults to production backend when no backend flag is supplied | new | unit | no |
| todo | `--backend noop` selects noop backend and does not require production hardware approval state | new | unit/integration | no |
| todo | invalid backend value fails before adapter open | edge | unit/integration | no |
| todo | production backend no longer requires `SWBT_RUN_HARDWARE` and `SWBT_HARDWARE_APPROVED` as code-level gates, while hardware execution remains documented as human-approved | behavior | unit/integration | no |
| todo | diagnostic trace path is enabled only by CLI flag and not by persistent config file | regression | unit/integration | no |
| todo | HCI dump path is enabled only by CLI flag and fails before production run loop when the path cannot be opened | edge | unit/integration | no |
| todo | crash dump path CLI behavior is fixed for Windows and non-Windows builds | edge | unit/build | no |
| todo | test / smoke entrypoints that need no hardware pass `--backend noop` explicitly | regression | integration | no |
| deferred | adapter selector or dedicated dongle identity guard is designed if production default leaves adapter selection ambiguous | behavior | design/hardware | yes |

## 10. 検証

初期起票時点では、実装、software test、実機検証はまだ実行していなかった。

2026-06-26 の方針確認では、CLI parser 実装は設定ファイル work unit に混ぜず、この record を後続 work unit の source として残す判断にした。この時点では `CMakeLists.txt`、`apps/swbt-daemon/main.c`、`swbt/daemon/*`、`tests/daemon_*` の挙動は変更しない。元々の設定ファイル方針の反映を優先し、CLI parser、production 既定化、diagnostic flag 化は後続で再開する。

起票時確認:

- `apps/swbt-daemon/main.c` は `main(void)` で CLI 引数を受け取らず、`SWBT_DAEMON_BACKEND=production` のときだけ production backend を選ぶ。
- `swbt/daemon/host.c` の noop backend は Bluetooth adapter を開かない dummy port set である。
- `swbt/daemon/production_backend.c` は `SWBT_RUN_HARDWARE` と `SWBT_HARDWARE_APPROVED` 由来の approval がない場合に production main を拒否する。
- `swbt/core/diagnostics.c`、`swbt/btstack_bridge/production_btstack.c`、`apps/swbt-daemon/main.c` は診断出力 path を環境変数から読む。

TDD status:
- source: user discussion, 2026-06-26: 実機 reconnect 検証へ進む前に `--config` 起動契約が必要である。
- use case: hardware operator は daemon binary に config file path を明示し、adapter open 前に CLI 引数 validation の失敗を観測できる。
- item: CLI parser accepts `--config <path>` / `--config=<path>` and rejects missing or unknown options before adapter open。
- state: refactor-skipped
- commands:
  - red: `just build-debug`
  - green: `just build-debug`
  - green: `$env:CTEST_ARGS='-R daemon_launch_options_test'; just test-debug`
- notes: red は `swbt/daemon/launch_options.c` と `daemon/launch_options.h` が未実装のため CMake regenerate が失敗した。green では `swbt_daemon_launch_options_parse()` を追加し、`--config <path>`、`--config=<path>`、missing value、unknown option、no option を unit test で固定した。refactor-after-green は見直したが、parser は独立 module と test に閉じており追加構造変更は不要と判断した。

Refactor status:
- decision: refactor-skipped
- change: none
- unchanged behavior: daemon main の backend selection、hardware approval、config file 読み込み、diagnostic env はまだ変更しない。
- verification: `just build-debug`、`$env:CTEST_ARGS='-R daemon_launch_options_test'; just test-debug`
- notes: 次 cycle で parser result を `apps/swbt-daemon/main.c` に接続し、config file apply と learned address target を production backend へ渡す。

TDD status:
- source: user discussion, 2026-06-26: 実機 reconnect 検証へ進む前に `--config` の読み込み元と learned address target を production backend に接続する必要がある。
- use case: hardware operator は `--config <path>` で TOML file を指定し、file 値を読んだ後に環境変数 override を適用し、同じ file へ learned Switch address を書き戻せる。
- item: daemon main applies config file before environment override and passes the same config path as learned address target to production backend。
- state: refactor-skipped
- commands:
  - red: `just build-debug`
  - green: `just build-debug`
  - green: `$env:CTEST_ARGS='-R daemon_launch_options_test'; just test-debug`
  - green: `just test-debug`
- notes: red は `swbt_daemon_launch_config_t`、`swbt_daemon_launch_config_prepare()`、`swbt_daemon_config_env_t` の include 境界が未実装で、`daemon_launch_options_test` の compile が失敗した。green では launch config helper が default config に required TOML file を適用し、その後に env override を適用する。`apps/swbt-daemon/main.c` は `--config` を parse し、config path がある場合だけ同じ path を learned address target として production backend へ渡す。backend selection、hardware approval env、diagnostic env は現行互換のまま残した。refactor-after-green は見直したが、reconnect 実機前提の最小接続に閉じるため追加構造変更は行わない。

Refactor status:
- decision: refactor-skipped
- change: none
- unchanged behavior: `SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、diagnostic env は現行互換のまま残す。
- verification: `just build-debug`、`$env:CTEST_ARGS='-R daemon_launch_options_test'; just test-debug`、`just test-debug`
- notes: backend 既定化、hardware approval env 削除、diagnostic CLI flag 化は後続 item のままにする。

TDD status:
- source: user discussion, 2026-06-26: active reconnect manual rerun は daemon initiated HID transport に成功したが、`have link key db: 0` と `pairing complete, status 00` が残った。link key notification は観測できるため、実験用 link key DB で保存可否を切り分ける。
- use case: hardware operator は `--experimental-link-key-db <path>` を指定した起動だけで BTstack TLV-backed Classic link key DB を接続し、通常の設定ファイル schema には恒久保存しない。
- item: CLI parser accepts `--experimental-link-key-db <path>` / `--experimental-link-key-db=<path>` and stores it outside persistent config。
- state: refactor-skipped
- commands:
  - red: `just build-debug`
  - green: `$env:CTEST_ARGS='-R daemon_launch_options_test'; just test-debug`
  - green: `$env:CTEST_ARGS='-R "btstack_sources_cmake_test|daemon_launch_options_test|btstack_production_hci_dump_test"'; just test-debug`
- notes: red は launch option / launch config に experimental path field が未実装で compile が失敗した。green では `--experimental-link-key-db <path>` と `--experimental-link-key-db=<path>` を受け付け、missing value を拒否する。prepared launch config には path と configured flag だけを保持し、TOML config schema へは書かない。

TDD status:
- source: user discussion, 2026-06-26: link key notification が観測できるため、BTstack の Classic link key DB を production 経路に接続して保存と pairing-free reconnect を検証する。
- use case: hardware operator は事前に用意した artifact directory 配下の TLV path を指定し、その起動だけ BTstack の link key DB を有効化できる。親 directory の自動作成は行わない。
- item: production BTstack platform start connects an explicitly configured experimental TLV-backed Classic link key DB and closes it on platform stop。
- state: refactor-skipped
- commands:
  - red: `just windows-cross`
  - green: `just build-debug`
  - green: `$env:CTEST_ARGS='-R "btstack_sources_cmake_test|daemon_launch_options_test|btstack_production_hci_dump_test"'; just test-debug`
  - green: `just test-debug`
  - green: `just windows-cross`
  - green: `just format-check`
- notes: `swbt_btstack_production_experimental_link_key_db_configure()` を追加し、production platform start で `btstack_tlv_*_init_instance()`、`btstack_tlv_set_instance()`、`btstack_link_key_db_tlv_get_instance()`、`hci_set_link_key_db()` を呼ぶ。最初の Windows cross build は `btstack_tlv_windows_init_instance` / `btstack_tlv_windows_deinit` の unresolved symbol で失敗したため、backend 別 link source に `btstack_tlv_posix.c` / `btstack_tlv_windows.c` を明示追加した。`hci_close()` 後に `hci_set_link_key_db(NULL)` を呼ぶと `hci_stack` 解放後参照で segfault するため、stop 側は BTstack TLV singleton と TLV context の後始末に限定した。

Refactor status:
- decision: refactor-skipped
- change: none
- unchanged behavior: `--experimental-link-key-db` 未指定時は link key DB を接続しない。`SWBT_DAEMON_BACKEND=production`、hardware approval env、diagnostic env は現行互換のまま残す。
- verification: `just build-debug`、`$env:CTEST_ARGS='-R "btstack_sources_cmake_test|daemon_launch_options_test|btstack_production_hci_dump_test"'; just test-debug`、`just test-debug`、`just windows-cross`、`just format-check`
- notes: 保存された link key によって pairing-free reconnect が成立するかは実機未検証であり、hardware item として残す。

実機前 software verification:

- `just windows-cross`: pass。Windows MinGW debug build で `swbt-daemon.exe` と関連 tests の link まで確認した。

実機前追加 build fix:

- 観測: C++ / TOML code path が入った後の Windows MinGW executable は host 側起動時に `libgcc_s_seh-1.dll` を要求し、実機 daemon 起動前に失敗した。
- 対応: `CMakeLists.txt` の `MINGW` 条件で `add_link_options(-static)` を追加し、cross-built Windows executable が host 側 MinGW runtime DLL 配置に依存しないようにした。
- verification: `just windows-cross`: pass。`build/windows-mingw-debug/swbt-daemon.exe --definitely-invalid-option`: exit code `1`。起動前 DLL error は解消した。

実機 reconnect verification:

- date: 2026-06-26
- approval: user approved CSR8510 A10 / WinUSB adapter open、initial pairing、learned address config writeback、daemon restart、active reconnect request、Switch 側再接続操作、Button A smoke、HCI dump / diagnostic trace 保存、cleanup 確認。
- artifact: `tmp/hardware/local_073/20260626-171348-active-reconnect`
- environment: Windows native PowerShell、CSR8510 A10 `USB\VID_0A12&PID_0001\9&12127A34&0&1`、Service `WinUSB`、backend `windows-winusb`、BTstack `075a0780f0fad7ff67d58ac19f46e8953656a752`、Switch2 firmware `22.1.0`。
- swbt: git HEAD `d3f2b9cc6b4adf5de2acf252f94f0ca743707bed` plus uncommitted `CMakeLists.txt` MinGW static link fix at experiment time。
- procedure: empty TOML config を `--config` で指定して initial pairing を実行し、learned Switch address を同じ file へ保存した。Button A smoke は IPC status `state.buttons=8` と exit code `0` を返した。その後 daemon を停止し、同じ config で restart run を実行した。
- result: learned address config writeback は pass。restart run は `production: active reconnect request ok` を `1` 件記録したが、HCI は outgoing connection `Connection_complete (status=8)` `1` 件と control PSM `0x11` の `L2CAP_EVENT_CHANNEL_OPENED status 0x8` `1` 件を記録した。active reconnect 成功は未達。
- Switch-side reconnect operation: ユーザ操作後、HCI は incoming connection、`pairing complete, status 00` `2` 件、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `4` 件を記録した。これは active reconnect 成功ではなく incoming pairing path と扱う。
- cleanup: stop 前に neutral state を accepted させた。`GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT)` は `sent=True`、process は終了した。trace は HCI power off、HCI close、run loop deinit、HCI dump close、host stop done まで到達した。connection closed 後のため shutdown neutral send は failed。
- log: `docs/hardware-test-log.md` に raw Switch address を転記せず記録した。

実機 reconnect manual rerun:

- date: 2026-06-26
- artifact: `tmp/hardware/local_073/manual-20260626-174907-active-reconnect`
- procedure: 保存済み `--config` を使い、Switch 側操作前に daemon 起点の active reconnect request を手動 run で再確認した。
- result: trace は `production: active reconnect request ok` と `production: hid connection opened` を記録した。HCI は `Create outgoing HID Control`、`Connection_complete (status=0)`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` を記録し、`Connection_incoming`、`Connection_complete (status=8)`、`L2CAP_EVENT_CHANNEL_OPENED status 0x8` は記録しなかった。`a1 30` input report は `9532` 件、Switch 側 `a2 01` output report は `17` 件、`a1 21` reply は `17` 件だった。
- boundary: 同じ run で `have link key db: 0`、`pairing started, ssp 1, initiator 1`、`pairing complete, status 00` `1` 件も記録した。したがって daemon initiated active reconnect transport は成功したが、保存済み link key による pairing-free reconnect は未確認のまま残す。
- cleanup: trace は `production: shutdown neutral send ok`、HCI power off、HCI close、run loop deinit、HCI dump close、host stop done まで到達した。`swbt-daemon.exe` process は残っていない。

## 11. 実機実行条件

この work unit は production 既定化と hardware approval env の削除候補を含むため、最終確認では実機が必要になる可能性が高い。

実機実行前条件:

- `hardware-harness` を読む。
- 人間の明示承認を得る。
- 専用 USB Bluetooth dongle と WinUSB driver assignment を確認する。
- 実行範囲を adapter open、HID advertising、pairing、report loop、diagnostic output のどこまで含むか明示する。
- `docs/hardware-test-log.md` へ OS、dongle VID/PID、driver、BTstack commit、swbt commit、Switch firmware、結果、cleanup を記録する。

software-only で確認できる範囲:

- CLI parser の既定値と validation。
- noop 明示指定が adapter port を呼ばないこと。
- invalid backend / invalid diagnostic path が production run loop 前に失敗すること。
- 環境変数依存削除または互換期間の挙動。

## 12. 先送り事項

- 観測: production 既定化後も、BTstack USB transport がどの adapter を開くかは swbt 側で明示 selector を持たない。
  先送り理由: backend 起動モードと CLI parser の整理とは別の問題であり、VID/PID selector や driver state 確認を含めると scope が広がる。
  次の置き場: 後続 work unit。必要なら `spec/operations/windows-native-preflight.md` と接続する。
- 観測: `show-config` / `validate-config` / reconnect state cleanup などの CLI subcommand は有用である。
  先送り理由: この work unit は daemon 起動引数に絞る。管理 command surface は active reconnect と設定ファイル運用が固まった後に扱う。
  次の置き場: 後続 work unit。

## 13. チェックリスト

- [x] source を user discussion、`local_071`、`local_045`、`docs/status.md` から特定した。
- [x] use case を production default、explicit noop、CLI diagnostic flag として定義した。
- [x] 設定ファイル schema とは別 work unit に分離した。
- [x] CLI parser の test list を実装前に再確認した。
- [x] red test または characterization test を追加した。
- [ ] production default / noop explicit behavior を実装した。
- [ ] diagnostic CLI flag を実装した。
- [ ] hardware approval env の削除または互換期間を決めた。
- [x] docs / status を更新した。
- [x] software verification と実機未実行理由または実機結果を記録した。
