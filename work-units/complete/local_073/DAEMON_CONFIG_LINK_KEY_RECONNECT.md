# Daemon Config Path And Link Key Reconnect

## 1. 概要

daemon の実機 reconnect 検証に必要な `--config` と `--link-key-db` の起動契約を追加する work unit。

`--config <path>` は TOML config file の読み込み元であり、production backend では learned Switch address の書き戻し先でもある。`--link-key-db <path>` は起動単位で TLV-backed Classic link key DB を接続し、HCI link key notification を raw key を trace に出さず保存する。

保存済み config と link key DB により、Switch 側の再登録操作なしで daemon initiated outgoing reconnect が HID open と Button A smoke まで到達することを実機で確認する。production / noop 起動モード、実機承認 env、diagnostic path の CLI flag 化は `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md` に分離する。

## 2. 起点 / ユースケース

source:

- user discussion, 2026-06-26: 実機承認 env はそろそろ外せるのではないか。診断出力 path は環境変数で有効化すると意図しない場所へファイルが作られ得るため、実行時引数の flag に寄せる案が出た。
- user discussion, 2026-06-26: backend 指定は何かを確認した結果、現行の noop は test / smoke 用であり、daemon の通常起動は production を既定にして、test 時だけ明示的に noop を与える方針が妥当と判断した。
- user discussion, 2026-06-26: CLI parser を入れて backend / 実機承認 / 診断出力を反転させる話は、設定ファイル work unit に混ぜず、別 work unit として record 執筆に留める。実装は設定ファイル方針の反映を優先した後で再開する。
- user discussion, 2026-06-26: 実機 reconnect 検証へ進む。現行 `main` は設定ファイル path と learned address target を production backend へ渡さないため、実機を開く前に `--config` 相当の起動契約を最小実装する。
- user discussion, 2026-06-26: active reconnect manual rerun で daemon initiated HID transport は成功したが、`have link key db: 0` と `pairing complete, status 00` が出た。HCI event `0x18` により link key notification は観測できたため、link key DB を指定起動だけで接続し、active outgoing path で保存と pairing-free reconnect が成立するか検証する。
- user discussion, 2026-06-26: link key DB 起動では DB open 自体は成功したが、TLV file は `8` bytes の header だけに留まり、BTstack log は `Remote not bonding, dropping local flag` を記録した。BTstack の bonding policy と DB persistence を切り分けるため、HCI link key notification を swbt 側で明示的に DB へ保存する経路を作る。
- user discussion, 2026-06-26: 保存済み link key DB を使った outgoing reconnect が、新規 `pairing complete, status 00` なしで HID open と Button A smoke まで到達した。実験用と銘打っていた実装名を撤去し、余計な接頭辞・接尾辞を避けて通常の link key DB 経路として整理する。
- user discussion, 2026-06-26: 通常名へ整理した後に再試験し、その後 local_073 を `--config` / `--link-key-db` / reconnect の範囲に分離する。production / noop 起動モード、実機承認 env 整理、diagnostic CLI flag 化は後続 work unit に移す。
- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`: 設定ファイル schema は `ipc`、`report`、`device.profile` に絞り、backend 起動モード、実機承認、診断出力 path はこの work unit へ切り出す。
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`: 環境変数を backend selection、hardware safety gate、runtime override、diagnostic sink に分類した。
- `docs/status.md`: 現行状態として、未指定では noop backend、`SWBT_DAEMON_BACKEND=production` で production backend、production には `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` が必要と記録している。

use case:

- actor: hardware operator、maintainer、unit / integration test。
- 入力または状態: daemon に config file path と link key DB path を明示し、保存済み learned Switch address と保存済み Classic link key を再利用する起動。
- 期待する観測結果:
  - `--config <path>` を指定した production daemon は、default config に TOML file を適用し、その後に環境変数 override を適用する。
  - `--config <path>` で読み込んだ同じ TOML file を learned Switch address の書き戻し先として production backend に渡す。
  - `--link-key-db <path>` を指定した起動では、HCI link key notification から raw key を trace に出さず、TLV DB に保存する。
  - 保存済み config と link key DB により、Switch 側の再登録操作なしで active reconnect request、saved link key DB response、HID open、Button A smoke へ到達する。
- 制約: エージェント運用上の実機コマンド承認は残す。Bluetooth adapter open、Switch pairing、HID advertising、report loop は `hardware-harness` の承認境界に従う。

source から use case への変換:

`local_071` は永続設定の対象を runtime 値へ絞る。実機 reconnect 検証では config path と link key DB path が先に必要になったため、この work unit ではその起動契約だけを先に実装し、daemon 起動モード全体の反転は `local_074` に送る。

## 3. 対象範囲

- `swbt-daemon` 用の testable CLI parser を追加する。
- `--config <path>` を設計し、daemon config file 読み込みと learned address target を production backend へ接続する。
- link key DB 指定時だけ、HCI link key notification を BTstack TLV-backed Classic link key DB へ明示保存する経路を追加する。
- `--link-key-db <path>` を通常名の起動契約として整える。
- 保存済み config / link key DB による pairing-free active reconnect を実機で確認する。
- `docs/status.md`、`docs/hardware-test-log.md`、この record に検証結果を記録する。

## 4. 対象外

- `local_071` の設定ファイル schema、TOML parser / serializer、環境変数 runtime override precedence。
- production / noop 起動モードの反転、`--backend noop`、実機承認 env 整理、diagnostic path の CLI flag 化。`work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md` で扱う。
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
- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`

## 6. 根拠監査

source-audit required for BTstack source selection。

この work unit は `--config`、`--link-key-db`、BTstack TLV-backed Classic link key DB 接続、保存済み link key DB による reconnect 観測を主対象にする。Switch HID report bytes、subcommand、SPI、rumble、report period 採用値は追加または変更しない。

ただし link key DB を `production_btstack` に接続するため、BTstack platform TLV source selection を変更した。根拠は BTstack port 実装であり、`vendor/btstack/port/libusb/main.c`、`vendor/btstack/port/windows-winusb/main.c`、`vendor/btstack/port/windows-h4/main.c` は `btstack_tlv_*_init_instance()`、`btstack_tlv_set_instance()`、`btstack_link_key_db_tlv_get_instance()`、`hci_set_link_key_db()` の順に Classic link key DB を接続している。

swbt 側では `vendor/btstack` を編集せず、`CMakeLists.txt` の backend 別 link source に `platform/posix/btstack_tlv_posix.c` と `platform/windows/btstack_tlv_windows.c` を追加した。`tests/cmake/btstack_sources_test.cmake` で source selection に TLV source が含まれることを固定し、`just windows-cross` で Windows MinGW executable link まで確認した。

link key notification 保存経路の根拠:

- source fact: `vendor/btstack/src/hci.c:4779-4812` は `HCI_EVENT_LINK_KEY_NOTIFICATION` を処理し、`gap_store_link_key_for_bd_addr()` を呼び得る。ただし保存は BTstack の bonding 条件に依存する。
- source fact: `vendor/btstack/src/hci.c:7993` は remote が bonding しない場合に local bonding flag を落とす。この条件は 2026-06-26 の link key DB 実機ログでも観測した。
- source fact: `vendor/btstack/src/hci.c:571-574` の `gap_store_link_key_for_bd_addr()` は現在の `hci_stack->link_key_db` に保存を委譲する。
- source fact: `vendor/btstack/src/hci.c:5383-5396` と `vendor/btstack/src/hci.c:8709-8713` は HCI event handler の登録と dispatch 経路である。swbt 側は link key DB open 中だけ handler を登録する。
- source fact: `vendor/btstack/src/classic/btstack_link_key_db.h:88-96` と `vendor/btstack/src/classic/btstack_link_key_db_tlv.c:88-180` は `get_link_key` / `put_link_key` と TLV 永続化の実装である。
- source fact: `vendor/btstack/src/btstack_defines.h:378` は `HCI_EVENT_LINK_KEY_NOTIFICATION` を `0x18` と定義する。event の address bytes は `vendor/btstack/src/btstack_event.h:715-716` の generated accessor と同じ向きへ swbt 側で変換する。

production backend の実機実行は Bluetooth adapter open に直結するため、実機実行や docs の表現では `hardware-harness` を使う。

## 7. 設計メモ

- `--config` は reconnect 実機検証の前提である。backend 既定化、実機承認 env の削除、診断 flag 化より先に実装してよい。
- `--config` で指定した TOML file は読み込み元であり、learned Switch address の書き戻し先でもある。削除境界は設定ファイル上の値の除去とする。
- CLI flag は永続設定より優先する。ただし `backend` と診断出力 path は `local_071` の設定ファイル schema には入れない。
- `--config` による file 値は、`default -> TOML config file -> environment override` の優先順位を保つ。環境変数による runtime override は現行互換として残す。
- `--link-key-db <path>` は恒久設定ではなく、active reconnect の link key 保存可否を起動単位で切り分ける入口である。設定ファイル schema には入れず、指定された起動だけ BTstack TLV-backed Classic link key DB を接続する。
- BTstack TLV DB を接続するだけでは、Switch 側が bonding しない条件で link key が保存されない。`--link-key-db` 指定時は HCI event handler を登録し、`HCI_EVENT_LINK_KEY_NOTIFICATION` の非 null key を `gap_store_link_key_for_bd_addr()` で明示保存する。raw address と raw key は trace に出さない。
- production / noop 起動モードの反転、実機承認 env 整理、diagnostic path の CLI flag 化は `local_074` に分離した。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `swbt/daemon/launch_options.*`
- `swbt/btstack_bridge/production_btstack.*`
- `tests/daemon_launch_options_test.c`
- `tests/btstack_production_hci_dump_test.c`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | CLI parser accepts `--config <path>` / `--config=<path>` and rejects missing or unknown options before adapter open | new | unit | no |
| refactor-skipped | daemon main applies config file before environment override and passes the same config path as learned address target to production backend | new | integration | no |
| refactor-skipped | CLI parser accepts `--link-key-db <path>` / `--link-key-db=<path>` and stores it outside persistent config | characterization | unit | no |
| refactor-skipped | production BTstack platform start connects an explicitly configured TLV-backed Classic link key DB and closes it on platform stop | characterization | integration | no |
| refactor-skipped | link key DB explicitly stores a non-null HCI link key notification into the TLV DB during platform lifetime | characterization | integration | no |
| refactor-skipped | active outgoing reconnect with link key DB records whether link key notification is persisted to a non-empty TLV file | characterization | hardware | yes |
| refactor-skipped | daemon restart after link key DB persistence reaches HID channel open but still records incoming SSP pairing | characterization | hardware | yes |
| refactor-skipped | sleep/resume after link key DB persistence issues active reconnect but does not reach controller connection complete | characterization | hardware | yes |
| refactor-skipped | sleep/resume rerun after link key DB persistence reaches HID open with saved link key DB and no incoming pairing | characterization | hardware | yes |
| refactor-skipped | link key DB implementation and CLI names drop the experimental prefix after pairing-free reconnect is observed | refactor | unit/integration | no |
| refactor-skipped | post-refactor sleep/resume rerun reaches HID open with `--link-key-db`, saved link key DB, and no incoming pairing | regression | hardware | yes |

## 10. 検証

初期起票時点では、実装、software test、実機検証はまだ実行していなかった。

2026-06-26 の方針確認では、CLI parser 実装は設定ファイル work unit に混ぜず、この record を後続 work unit の source として残す判断にした。その後、実機 reconnect 検証に必要な `--config` と `--link-key-db` だけをこの record の範囲として実装し、production 既定化、diagnostic flag 化、実機承認 env 整理は `local_074` へ分離した。

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
- source: user discussion, 2026-06-26: active reconnect manual rerun は daemon initiated HID transport に成功したが、`have link key db: 0` と `pairing complete, status 00` が残った。link key notification は観測できるため、link key DB で保存可否を切り分ける。
- use case: hardware operator は `--link-key-db <path>` を指定した起動だけで BTstack TLV-backed Classic link key DB を接続し、通常の設定ファイル schema には恒久保存しない。
- item: CLI parser accepts `--link-key-db <path>` / `--link-key-db=<path>` and stores it outside persistent config。
- state: refactor-skipped
- commands:
  - red: `just build-debug`
  - green: `$env:CTEST_ARGS='-R daemon_launch_options_test'; just test-debug`
  - green: `$env:CTEST_ARGS='-R "btstack_sources_cmake_test|daemon_launch_options_test|btstack_production_hci_dump_test"'; just test-debug`
- notes: red は launch option / launch config に link key DB path field が未実装で compile が失敗した。green では `--link-key-db <path>` と `--link-key-db=<path>` を受け付け、missing value を拒否する。prepared launch config には path と configured flag だけを保持し、TOML config schema へは書かない。

TDD status:
- source: user discussion, 2026-06-26: link key notification が観測できるため、BTstack の Classic link key DB を production 経路に接続して保存と pairing-free reconnect を検証する。
- use case: hardware operator は事前に用意した artifact directory 配下の TLV path を指定し、その起動だけ BTstack の link key DB を有効化できる。親 directory の自動作成は行わない。
- item: production BTstack platform start connects an explicitly configured TLV-backed Classic link key DB and closes it on platform stop。
- state: refactor-skipped
- commands:
  - red: `just windows-cross`
  - green: `just build-debug`
  - green: `$env:CTEST_ARGS='-R "btstack_sources_cmake_test|daemon_launch_options_test|btstack_production_hci_dump_test"'; just test-debug`
  - green: `just test-debug`
  - green: `just windows-cross`
  - green: `just format-check`
- notes: `swbt_btstack_production_link_key_db_configure()` を追加し、production platform start で `btstack_tlv_*_init_instance()`、`btstack_tlv_set_instance()`、`btstack_link_key_db_tlv_get_instance()`、`hci_set_link_key_db()` を呼ぶ。最初の Windows cross build は `btstack_tlv_windows_init_instance` / `btstack_tlv_windows_deinit` の unresolved symbol で失敗したため、backend 別 link source に `btstack_tlv_posix.c` / `btstack_tlv_windows.c` を明示追加した。`hci_close()` 後に `hci_set_link_key_db(NULL)` を呼ぶと `hci_stack` 解放後参照で segfault するため、stop 側は BTstack TLV singleton と TLV context の後始末に限定した。

Refactor status:
- decision: refactor-skipped
- change: none
- unchanged behavior: `--link-key-db` 未指定時は link key DB を接続しない。`SWBT_DAEMON_BACKEND=production`、hardware approval env、diagnostic env は現行互換のまま残す。
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

実機 experimental link key DB reconnect verification:

- date: 2026-06-26
- approval: user approved CSR8510 A10 / WinUSB adapter open、initial pairing、experimental TLV-backed Classic link key DB、daemon restart、active reconnect request、HCI dump / diagnostic trace 保存、cleanup 確認。
- artifact: `tmp/hardware/local_073/20260626-191642-link-key-db-reconnect`
- environment: Windows native PowerShell、CSR8510 A10 `USB\VID_0A12&PID_0001\9&12127A34&0&1`、Service `WinUSB`、backend `windows-winusb`、BTstack `075a0780f0fad7ff67d58ac19f46e8953656a752`、Switch2 firmware `22.1.0`。
- swbt: git HEAD `384e5d4`。
- procedure: empty TOML config と `--experimental-link-key-db <artifact>/swbt-link-key.tlv` を指定して initial pairing を実行した。その後、同じ config と TLV path で restart run を実行し、Switch 側操作なしで active reconnect request を観測した。
- result: experimental DB open は initial / restart の両方で pass。initial run は `responding to link key request, have link key db: 1`、`pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0`、trace の `production: hid connection opened` を記録した。一方、initial HCI dump は `Remote not bonding, dropping local flag` を記録し、`swbt-link-key.tlv` は `8` bytes のままだった。したがって BTstack TLV DB は接続できたが、link key material は保存されなかった。
- restart result: trace は `btstack: experimental link key db open ok` と `production: active reconnect request ok` を記録したが、`production: hid connection opened` は記録しなかった。HCI dump は `Create outgoing HID Control` の後、`Connection_complete (status=22)` と control PSM `0x11` の `L2CAP_EVENT_CHANNEL_OPENED status 0x16` を記録した。restart run に `responding to link key request` と `pairing complete, status 00` は出ていない。link key DB persistence の precondition が満たせていないため、pairing-free reconnect 成否判定には進めていない。
- cleanup: initial inspection 時点では `swbt-daemon.exe` process が残り、TLV file は process lock されていた。その後 `Get-Process swbt-daemon -ErrorAction SilentlyContinue` が空であることを確認した。TLV file は `BTstack` header の 8 bytes のみで、raw link key value は含まれていない。
- log: raw Switch address と raw link key value は転記しない。`swbt-daemon.toml` と HCI dump は scrub 対象である。

TDD status:
- source: user discussion, 2026-06-26: link key DB open は成功したが、BTstack の bonding policy により TLV file へ link key が保存されなかった。
- use case: hardware operator は `--link-key-db <path>` を指定した起動で、BTstack の bonding 条件に依存せず、観測済み HCI link key notification を TLV DB へ保存できるか切り分ける。
- item: link key DB explicitly stores a non-null HCI link key notification into the TLV DB during platform lifetime。
- state: refactor-skipped
- commands:
  - red: `just build-debug`
  - red: `$env:CTEST_ARGS='-R btstack_production_hci_dump_test'; just test-debug`
  - green: `just build-debug`
  - green: `$env:CTEST_ARGS='-R btstack_production_hci_dump_test'; just test-debug`
  - green: `just test-debug`
  - green: `just windows-cross`
  - green: `just format-check`
  - green: `just asan`
  - green: `git diff --check`
- notes: red は再ビルド後の targeted CTest で、synthetic `HCI_EVENT_LINK_KEY_NOTIFICATION` を emit しても TLV file が header `8` bytes のままになる失敗として確認した。再ビルドなしの targeted run は旧 binary を実行していたため red として扱わない。green では link key DB open 中だけ HCI event handler を登録し、非 null key を `gap_store_link_key_for_bd_addr()` へ渡す。handler は platform stop と discovery configure failure で登録解除する。`hci_close()` 後に `hci_set_link_key_db(NULL)` を呼ぶと BTstack 内部の解放済み stack に触れるため、この経路では呼ばない。

Refactor status:
- decision: refactor-skipped
- change: none
- unchanged behavior: `--link-key-db` 未指定時は HCI link key notification を swbt 側で保存しない。raw address と raw key は trace に出さない。
- verification: `just build-debug`、`$env:CTEST_ARGS='-R btstack_production_hci_dump_test'; just test-debug`、`just test-debug`、`just windows-cross`、`just format-check`、`just asan`、`git diff --check`
- notes: link key DB 保存経路の software characterization は完了した。実機で pairing-free reconnect に進むかは未検証であり、同じ artifact 形で再試験する。

実機 experimental link key notification persistence retest:

- date: 2026-06-26
- approval: user approved CSR8510 A10 / WinUSB adapter open、initial pairing、experimental TLV-backed Classic link key DB、HCI link key notification 明示保存、daemon restart、active reconnect request、Button A smoke、HCI dump / diagnostic trace 保存、cleanup 確認。
- artifact: `tmp/hardware/local_073/20260626-195716-link-key-db-reconnect-retest`
- environment: Windows native PowerShell、CSR8510 A10 `USB\VID_0A12&PID_0001\9&12127A34&0&1`、Service `WinUSB`、backend `windows-winusb`、BTstack `075a0780f0fad7ff67d58ac19f46e8953656a752`、Switch2 firmware `22.1.0`。
- swbt: git HEAD `47ea721`。
- procedure: empty TOML config と `--experimental-link-key-db <artifact>/swbt-link-key.tlv` を指定して initial pairing を実行した。その後、同じ config と TLV path で restart run を実行し、Switch 側操作なしで active reconnect request を観測した。両 run で Button A smoke と shutdown neutral send を実行した。
- result: initial run は trace の `btstack: experimental link key db stored notification`、`production: hid connection opened`、`production: learned switch address save ok` を記録し、TLV file は `48` bytes へ増えた。restart run は `production: active reconnect request ok`、`btstack: experimental link key db stored notification`、`production: hid connection opened`、`production: learned switch address save ok` を記録し、TLV file は `88` bytes へ増えた。Button A smoke は initial / restart とも exit code `0`。
- reconnect boundary: restart run は `hid connection opened` まで到達したが、HCI dump 上は outgoing connection が `Connection_complete (status=8)` と control PSM `0x11` の `L2CAP_EVENT_CHANNEL_OPENED status 0x8` で一度失敗した後、incoming connection で `pairing started, ssp 1, initiator 0`、`Remote not bonding, dropping local flag`、`pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` を記録した。保存済み link key だけによる pairing-free reconnect 成功ではない。
- cleanup: pass。initial / restart とも neutral state を accepted させた後、`CTRL_BREAK_EVENT` により daemon は exit code `0` で終了した。`Get-Process swbt-daemon -ErrorAction SilentlyContinue` は空で、crash dump は作成されなかった。
- log: raw Switch address と raw link key value は転記しない。`swbt-daemon.toml`、TLV file、HCI dump は scrub 対象である。

TDD status:
- source: user discussion, 2026-06-26: HCI link key notification を link key DB に明示保存する実装後に、実機で TLV persistence と restart reconnect の境界を再確認する。
- use case: hardware operator は initial pairing で保存した TLV DB を restart run に持ち越し、Switch 側操作なしで active reconnect と pairing-free 条件を観測する。
- item: daemon restart after link key DB persistence reaches HID channel open but still records incoming SSP pairing。
- state: refactor-skipped
- commands:
  - hardware: `powershell -NoProfile -ExecutionPolicy Bypass -File .\tmp\hardware\local_073\run-link-key-db-reconnect-retest.ps1`
- notes: TLV persistence は実機で確認できた。restart run は `hid connection opened` と Button A smoke まで到達したが、outgoing active reconnect の失敗後に incoming SSP pairing が走り、`pairing complete, status 00` を 1 件記録した。pairing-free reconnect 条件は未達のため、別の設計または追加調査が必要である。

実機 sleep/resume existing TLV reconnect experiment:

- date: 2026-06-26
- approval: user approved CSR8510 A10 / WinUSB adapter open、保存済み TOML config / experimental TLV-backed Classic link key DB の再利用、Switch HOME から sleep / resume 済み状態での active reconnect request、Button A smoke、HCI dump / diagnostic trace 保存、cleanup 確認。Switch 側の Change Grip/Order 操作は含めない。
- artifact: `tmp/hardware/local_073/20260626-201135-link-key-db-sleep-resume-existing`
- source artifact: `tmp/hardware/local_073/20260626-195716-link-key-db-reconnect-retest`
- environment: Windows native PowerShell、CSR8510 A10 `USB\VID_0A12&PID_0001\9&12127A34&0&1`、Service `WinUSB`、backend `windows-winusb`、BTstack `075a0780f0fad7ff67d58ac19f46e8953656a752`、Switch2 firmware `22.1.0`。
- swbt: git HEAD `aec3537`。
- procedure: source artifact の `swbt-daemon.toml` と `swbt-link-key.tlv` を新 artifact へコピーした。operator は事前に Switch を HOME へ戻し、sleep / resume まで完了した。`tmp/hardware/local_073/run-link-key-db-sleep-resume-existing.ps1` で daemon を起動し、Switch 側を操作せず `production: hid connection opened` を 120 秒待った。
- result: trace は `btstack: experimental link key db open ok` と `production: active reconnect request ok` を各 1 件記録した。HCI dump は `Create_connection` を記録したが、120 秒内に `Connection_complete`、`Connection_incoming`、`pairing started`、`pairing complete, status 00`、`L2CAP_EVENT_CHANNEL_OPENED status 0x0` / `0x8` は出なかった。`production: hid connection opened` は 0 件だった。TLV file は before / after とも 88 bytes で、link key notification の追加保存も 0 件だった。Button A smoke は HID open がなかったため実行していない。
- boundary: 今回の失敗は link key request / response や pairing の段階ではなく、active reconnect request 後に controller connection complete へ進まない段階で起きている。保存済み link key DB の正否だけでは説明できないため、再試行 policy、起動タイミング、Switch 側 sleep/resume 後の connectability 状態を後続で切り分ける。
- cleanup: pass。IPC neutral client は exit code 0。HID connection が開いていないため trace 上の shutdown neutral send は failed だが、daemon は `CTRL_BREAK_EVENT` で exit code 0、forced stop false、crash dump なしで終了した。
- log: raw Switch address と raw link key value は転記しない。`swbt-daemon.toml`、TLV file、HCI dump は scrub 対象である。

TDD status:
- source: user discussion, 2026-06-26: restart run が incoming SSP pairing を含んだため、Change Grip/Order の影響を外し、HOME -> sleep -> resume 後に保存済み config / TLV だけで active reconnect が成立するかを確認する。
- use case: hardware operator は initial pairing 済みの config / TLV を再利用し、Switch 側再登録画面を開かずに sleep/resume 後の active reconnect request と connection outcome を観測する。
- item: sleep/resume after link key DB persistence issues active reconnect but does not reach controller connection complete。
- state: refactor-skipped
- commands:
  - hardware: `powershell -NoProfile -ExecutionPolicy Bypass -File .\tmp\hardware\local_073\run-link-key-db-sleep-resume-existing.ps1`
- notes: `active reconnect request ok` は出たが、HCI は `Create_connection` 後に `Connection_complete` へ進まなかった。pairing-free reconnect 成功条件は未達であり、今回の観測は connection establishment failure として扱う。

実機 sleep/resume existing TLV reconnect rerun:

- date: 2026-06-26
- approval: user approved rerun of the same CSR8510 A10 / WinUSB adapter open、保存済み TOML config / experimental TLV-backed Classic link key DB の再利用、Switch HOME から sleep / resume 済み状態での active reconnect request、Button A smoke、HCI dump / diagnostic trace 保存、cleanup 確認。
- artifact: `tmp/hardware/local_073/20260626-202005-link-key-db-sleep-resume-existing`
- source artifact: `tmp/hardware/local_073/20260626-195716-link-key-db-reconnect-retest`
- environment: Windows native PowerShell、CSR8510 A10 `USB\VID_0A12&PID_0001\9&12127A34&0&1`、Service `WinUSB`、backend `windows-winusb`、BTstack `075a0780f0fad7ff67d58ac19f46e8953656a752`、Switch2 firmware `22.1.0`。
- swbt: git HEAD `8bc2218`。
- procedure: source artifact の `swbt-daemon.toml` と `swbt-link-key.tlv` を新 artifact へコピーした。`tmp/hardware/local_073/run-link-key-db-sleep-resume-existing.ps1` で daemon を起動し、Switch 側を操作せず `production: hid connection opened` を待った。HID open 後に Button A smoke と neutral cleanup を実行した。
- result: trace は `btstack: experimental link key db open ok`、`production: active reconnect request ok`、`production: hid connection opened`、`production: shutdown neutral send ok` を各 1 件記録した。HCI dump は outgoing connection の `Connection_complete (status=0)` 1 件、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` 2 件、`responding to link key request` 1 件、`have link key db: 1` 1 件を記録した。`Connection_incoming`、`Connection_complete (status=8)`、`pairing started`、`pairing complete, status 00`、`L2CAP_EVENT_CHANNEL_OPENED status 0x8` は 0 件だった。Button A smoke は exit code 0。TLV file は before / after とも 88 bytes で、追加の link key notification 保存はなかった。
- boundary: 保存済み link key DB を使った outgoing reconnect が、新規 pairing なしで HID open まで到達した。直前の failed run は `Create_connection` 後に `Connection_complete` がなく、本体が sleep または Bluetooth 接続受付前だった可能性と整合する。ただし artifact は本体の電源状態を直接記録していないため、原因は未確定である。
- cleanup: pass。Button A smoke 後に neutral state を accepted させ、daemon は `CTRL_BREAK_EVENT` で exit code 0、forced stop false、crash dump なしで終了した。
- log: raw Switch address と raw link key value は転記しない。`swbt-daemon.toml`、TLV file、HCI dump は scrub 対象である。

TDD status:
- source: user discussion, 2026-06-26: 直前の failed run は本体が sleep していた可能性があるため、同じ条件でもう一度試す。
- use case: hardware operator は initial pairing 済みの config / TLV を再利用し、Switch 側再登録画面を開かずに active reconnect request、saved link key DB response、HID open、Button A smoke を観測する。
- item: sleep/resume rerun after link key DB persistence reaches HID open with saved link key DB and no incoming pairing。
- state: refactor-skipped
- commands:
  - hardware: `powershell -NoProfile -ExecutionPolicy Bypass -File .\tmp\hardware\local_073\run-link-key-db-sleep-resume-existing.ps1`
- notes: `responding to link key request` と `have link key db: 1` が出て、`pairing complete, status 00` と `Connection_incoming` は出なかった。これにより、保存済み link key DB による pairing-free outgoing reconnect 成功を実機で観測した。

Tidy status:
- classification: structure change
- decision: tidy after
- reason: pairing-free outgoing reconnect を実機で観測した後、`experimental` と銘打っていた CLI / function / static helper / trace 名は現在の振る舞いを表さなくなった。互換 alias を増やさず、正本を `--link-key-db` と `swbt_btstack_production_link_key_db_configure()` に揃える。
- verification: `just format`、`just build-debug`、`$env:CTEST_ARGS='-R "daemon_launch_options_test|btstack_production_hci_dump_test"'; just test-debug`、`just test-debug`、`just format-check`、`just windows-cross`、`just asan`

Refactor status:
- decision: refactor-done
- change: `--experimental-link-key-db` を `--link-key-db` へ置き換え、launch config field、BTstack bridge public function、static helper、trace を通常名へ変更した。過去 artifact の旧 trace / 旧 flag は実測証跡として残す。
- unchanged behavior: `--link-key-db` 未指定時は link key DB を接続しない。指定時は TLV-backed Classic link key DB を接続し、非 null `HCI_EVENT_LINK_KEY_NOTIFICATION` を `gap_store_link_key_for_bd_addr()` 経由で保存する。raw address と raw key は trace に出さない。
- verification: `just format`、`just build-debug`、`$env:CTEST_ARGS='-R "daemon_launch_options_test|btstack_production_hci_dump_test"'; just test-debug`、`just test-debug`、`just format-check`、`just windows-cross`、`just asan`
- notes: 実機再実行はこの refactor では行っていない。変更は CLI 名と trace 名を含むため、次に実機 script を使う場合は新しい `--link-key-db` と `btstack: link key db ...` trace を正本にする。

実機 post-refactor sleep/resume existing TLV reconnect rerun:

- date: 2026-06-26
- approval: user approved CSR8510 A10 / WinUSB adapter open、保存済み TOML config / TLV-backed Classic link key DB の再利用、Switch HOME から sleep / resume 済み状態での active reconnect request、Button A smoke、HCI dump / diagnostic trace 保存、cleanup 確認。Switch 側の Change Grip/Order 操作は含めない。
- artifact: `tmp/hardware/local_073/20260626-205341-link-key-db-sleep-resume-existing`
- source artifact: `tmp/hardware/local_073/20260626-195716-link-key-db-reconnect-retest`
- environment: Windows native PowerShell、CSR8510 A10 `USB\VID_0A12&PID_0001\9&12127A34&0&1`、Service `WinUSB`、backend `windows-winusb`、BTstack `075a0780f0fad7ff67d58ac19f46e8953656a752`、Switch2 firmware `22.1.0`。
- swbt: git HEAD `f77d8a58ecedd4a4a25b9fd27706168e020f26c3`。
- procedure: source artifact の `swbt-daemon.toml` と `swbt-link-key.tlv` を新 artifact へコピーした。`tmp/hardware/local_073/run-link-key-db-sleep-resume-existing.ps1` で daemon を `--config <artifact>/swbt-daemon.toml --link-key-db <artifact>/swbt-link-key.tlv` として起動し、Switch 側を操作せず `production: hid connection opened` を待った。HID open 後に Button A smoke と neutral cleanup を実行した。
- result: reconnect は pass。trace は `btstack: link key db open ok`、`production: active reconnect request ok`、`production: hid connection opened`、`production: shutdown neutral send ok` を各 1 件記録した。HCI dump は outgoing connection の `Connection_complete (status=0)` 1 件、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` 2 件、`responding to link key request` 1 件、`have link key db: 1` 1 件を記録した。`Connection_incoming`、`Connection_complete (status=8)`、`pairing started`、`pairing complete, status 00`、`L2CAP_EVENT_CHANNEL_OPENED status 0x8` は 0 件だった。Button A smoke は exit code 0。TLV file は before / after とも 88 bytes で、追加の link key notification 保存はなかった。
- cleanup: pass。Button A smoke 後に neutral state を accepted させ、daemon は `CTRL_BREAK_EVENT` で exit code 0、forced stop false、crash dump なしで終了した。
- log: raw Switch address と raw link key value は転記しない。`swbt-daemon.toml`、TLV file、HCI dump は scrub 対象である。

TDD status:
- source: user discussion, 2026-06-26: 通常名へ整理した後に、同じ保存済み config / TLV で pairing-free reconnect が維持されるか再試験する。
- use case: hardware operator は `--link-key-db` の通常名経路で保存済み link key DB を使い、Switch 側再登録画面を開かずに active reconnect request、saved link key DB response、HID open、Button A smoke を観測する。
- item: post-refactor sleep/resume rerun reaches HID open with `--link-key-db`, saved link key DB, and no incoming pairing。
- state: refactor-skipped
- commands:
  - hardware: `powershell -NoProfile -ExecutionPolicy Bypass -File .\tmp\hardware\local_073\run-link-key-db-sleep-resume-existing.ps1`
- notes: `--link-key-db` と trace `btstack: link key db open ok` の通常名経路で、`responding to link key request` と `have link key db: 1` が出た。`pairing complete, status 00` と `Connection_incoming` は出なかった。これにより、命名整理後も保存済み link key DB による pairing-free outgoing reconnect が維持されることを実機で観測した。

## 11. 実機実行条件

この work unit の実機実行は完了した。

実行済み範囲:

- CSR8510 A10 / WinUSB adapter open。
- initial pairing と HCI link key notification の TLV 保存。
- 保存済み TOML config / TLV-backed Classic link key DB の再利用。
- Switch HOME から sleep / resume 済み状態での active reconnect request。
- saved link key DB response、HID open、Button A smoke。
- neutral cleanup、HCI dump / diagnostic trace 保存、crash dump absence 確認。

## 12. 先送り事項

none for this work unit。

production / noop 起動モード、実機承認 env 整理、diagnostic path の CLI flag 化、adapter selector の設計候補は `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md` に分離した。

## 13. チェックリスト

- [x] source を user discussion、`local_071`、`local_045`、`docs/status.md` から特定した。
- [x] use case を `--config`、learned Switch address target、`--link-key-db`、pairing-free reconnect として定義した。
- [x] 設定ファイル schema とは別 work unit に分離した。
- [x] CLI parser の test list を実装前に再確認した。
- [x] red test または characterization test を追加した。
- [x] production default / noop explicit behavior、diagnostic CLI flag、hardware approval env 整理を `local_074` に分離した。
- [x] 通常名の `--link-key-db` 経路で実機再試験した。
- [x] docs / status を更新した。
- [x] software verification と実機未実行理由または実機結果を記録した。
