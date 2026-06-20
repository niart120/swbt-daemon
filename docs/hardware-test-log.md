# 実機テストログ

手動の実機検証結果をここに記録する。

## テンプレート

```markdown
## YYYY-MM-DD: <短い題名>

- OS:
- environment:
- dongle:
- USB VID/PID:
- driver:
- backend:
- BTstack:
- swbt:
- Switch firmware:
- approval scope:
- environment variables:
- IPC endpoint:
- report period:
- command / procedure:
- result:
- daemon log:
- artifact root:
- cleanup:
- notes:
```

Windows native 実機検証は `spec/operations/windows-native-preflight.md` の gate を満たしてから記録する。report period comparison を行う場合は、各 period を別の記録に分ける。

NyX `swbt_hardware_bringup` macro を使う場合は、`artifact root` に `run_context.json`、`ipc_session.json`、`hardware_log_draft.md`、capture path を辿れる場所を記録する。daemon log は swbt-daemon 側で保存し、NyX の request id と時刻で対応付ける。

## 2026-06-20: Windows native CSR8510 A10 preflight

- OS: Microsoft Windows [Version 10.0.26200.8655]
- environment: Windows native PowerShell、ブランチ `local-037-nyxpy-hardware-bringup`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`、hardware IDs `USB\VID_0A12&PID_0001&REV_8891`, `USB\VID_0A12&PID_0001`
- driver: Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`。`just windows-cross` でビルド成果物を作成
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `5a85016b2284a448ea7227a352b9e4928436f690`
- Switch firmware: 未記録
- approval scope: read-only Windows PnP device query only。daemon 起動、Bluetooth adapter open、Switch pairing、HID advertising、report loop、IPC input は未承認
- environment variables: daemon 未起動。実行時は `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_PORT=37637` を使う予定
- IPC endpoint: 予定値 `127.0.0.1:37637`。`Test-NetConnection 127.0.0.1:37637` は preflight 時点で `False`
- report period: 未実行。比較候補は `8000 / 8333 / 15000 / 16667 us`
- command / procedure: `just windows-cross`; `Get-PnpDevice` / `Get-PnpDeviceProperty` で CSR8510 A10 の VID/PID と WinUSB driver state を確認
- result: preflight 通過。専用 USB Bluetooth dongle 候補は WinUSB に割り当て済み。内蔵または常用側の `MediaTek Bluetooth Adapter` は別 adapter として検出したため対象外
- daemon log: 未作成
- artifact root: なし
- cleanup: daemon process は起動していない。Bluetooth adapter は開いていない
- notes: NyXpy handoff では Project NyX 側の `swbt_hardware_bringup` macro に同じ IPC port と metadata を渡す。現 daemon は endpoint を標準出力へ出さないため、自動 port ではなく固定 port から始める

## 2026-06-20: CSR8510 A10 hardware run approval

- OS: Microsoft Windows [Version 10.0.26200.8655]
- environment: Windows native PowerShell、ブランチ `local-037-nyxpy-hardware-bringup`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Service `WinUSB`、Class `USBDevice`、Provider `libwdi`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `5a85016b2284a448ea7227a352b9e4928436f690`
- Switch firmware: 未記録
- approval scope: ユーザは CSR8510 A10 を対象に実験を進めることを承認した。NyXpy 操作はユーザが行う。Codex は内蔵 `MediaTek Bluetooth Adapter` と常用 Bluetooth device を対象外にする
- environment variables: daemon 未起動。予定値は `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_PORT=37637`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: 未実行。最初の予定値は `8000 us`
- command / procedure: 実行前承認範囲と停止条件を記録
- result: daemon 未起動。現行 daemon は非対話背景プロセスから安全に停止する入口がないため、adapter open 前に停止経路を確認する
- daemon log: 未作成
- artifact root: なし
- cleanup: Bluetooth adapter は開いていない
- notes: 実機 daemon run に進む前に、強制終了ではない停止方法を確定する。停止経路の software gate は `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md` で扱う

## 2026-06-20: local_037 resumed CSR8510 A10 preflight

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`、hardware IDs `USB\VID_0A12&PID_0001&REV_8891`, `USB\VID_0A12&PID_0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`。`just windows-cross` で `swbt-daemon.exe` と `swbt-debug-client.exe` の link 成功
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `c090ab1cc463066a0f1bfa047b583f2ff0589b4a`
- Switch firmware: 未記録
- approval scope: read-only PnP query、Windows cross build、approval guard のみ。Bluetooth adapter open、Switch pairing、HID advertising、report loop、IPC input はこの entry では未実行
- environment variables: approval guard では `SWBT_DAEMON_BACKEND=production`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000` を設定し、`SWBT_RUN_HARDWARE` と `SWBT_HARDWARE_APPROVED` は未設定
- IPC endpoint: 予定値 `127.0.0.1:37637`。`Test-NetConnection 127.0.0.1:37637` は `False`
- report period: 未実行。最初の予定値は `8000 us`
- command / procedure: `just windows-cross`; `Get-PnpDevice` / `Get-PnpDeviceProperty` で CSR8510 A10 の current driver state を再確認; approval guard として `swbt-daemon.exe` を hardware approval なしで実行
- result: preflight 通過。current artifact は WinUSB backend で再生成済み。approval guard は exit code `1` で、hardware approval なしでは production backend が実機に進まない
- daemon log: 未作成
- artifact root: なし
- cleanup: daemon process は起動直後に失敗終了。Bluetooth adapter は開いていない
- notes: 次の実機 run は foreground console で起動し、Ctrl+C / Windows console control event による停止を観測する。NyXpy 操作はユーザが行い、swbt 側は `127.0.0.1:37637` を固定して渡す

## 2026-06-20: local_037 CSR8510 A10 8000us initial run crash

- OS: Microsoft Windows NT 10.0.26200.0。WER `Report.wer` の OS build は `10.0.26100.8655.amd64fre.ge_release.240331-1435`
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `c090ab1cc463066a0f1bfa047b583f2ff0589b4a`
- Switch firmware: 未記録
- approval scope: ユーザ承認済み。CSR8510 A10、`8000 us`、daemon 起動、HID advertising、Switch pairing、report loop、NyXpy IPC input、Ctrl+C cleanup 確認
- environment variables: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を起動し、stderr/stdout を `tmp/hardware/local_037/20260620-8000us-initial/daemon-8000us.log` へ保存する手順を実行
- result: crash。PowerShell の exit marker は `exit=-1073741819`。これは Windows exception `0xC0000005` である。`daemon-8000us.log` は作成されず、`daemon-8000us-exit.txt` のみ作成された。Windows Event Log は `APPCRASH`、faulting module `ntdll.dll`、exception code `0xc0000005`、fault offset `0x000000000002048a`、loaded module に `WINUSB.DLL` を記録した
- daemon log: `tmp/hardware/local_037/20260620-8000us-initial/daemon-8000us.log` は未作成
- artifact root: NyXpy は未実行。なし
- cleanup: process は APPCRASH で終了。Ctrl+C cleanup は未観測。adapter が正常に power-off / close された根拠はない
- notes: この entry は実機成功ではない。Switch pairing、HID advertising、report loop、NyXpy IPC input へ進まず、次回の実機再実行前に WinUSB startup crash の診断を追加する

## 2026-06-20: local_037 CSR8510 A10 8000us dump rerun

- OS: Microsoft Windows NT 10.0.26200.0。WER `Report.wer` の OS build は `10.0.26100.8655.amd64fre.ge_release.240331-1435`
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `c090ab1cc463066a0f1bfa047b583f2ff0589b4a`
- Switch firmware: 未記録
- approval scope: ユーザ承認済み。CSR8510 A10、`8000 us`、dump 取得のための daemon 再実行
- environment variables: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を再実行し、`tmp/hardware/local_037/20260620-8000us-dump-rerun` へ log / exit marker / WER LocalDumps の取得を試みた
- result: crash 再現。PowerShell の exit marker は `exit=-1073741819`。artifact directory には `daemon-8000us-exit.txt` だけが残り、`daemon-8000us.log` と `.dmp` は作成されなかった。HKCU `LocalDumps\swbt-daemon.exe` は `DumpFolder`, `DumpType=2`, `DumpCount=5` を確認できたが、WER の temporary `.tmp.mdmp` は copy 前に削除されていた。HKLM LocalDumps の設定は access denied で未設定
- daemon log: 未作成
- artifact root: NyXpy は未実行。なし
- cleanup: process は APPCRASH で終了。Ctrl+C cleanup は未観測。adapter が正常に power-off / close された根拠はない
- notes: 次の再実行前に daemon 側へ `SWBT_DIAGNOSTIC_TRACE_PATH` の起動トレースと `SWBT_CRASH_DUMP_PATH` の自前 minidump 出力を追加した診断ビルドを使う

## 2026-06-20: local_037 CSR8510 A10 8000us diagnostic rerun

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` diagnostic build。base commit は `c090ab1cc463066a0f1bfa047b583f2ff0589b4a`
- Switch firmware: 未記録
- approval scope: ユーザ承認済み。CSR8510 A10、`8000 us`、診断 trace / daemon 自前 dump 付きの daemon 再実行
- environment variables: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を再実行し、`tmp/hardware/local_037/20260620-8000us-diagnostic-rerun` へ exit marker / startup trace / minidump を保存する手順を実行
- result: crash 再現。PowerShell の exit marker は `exit=-1073741819`。artifact directory には `daemon-8000us-exit.txt` と `startup-trace.txt` だけが残り、`swbt-daemon-crash.dmp` は作成されなかった。`startup-trace.txt` の末尾は `btstack: hci close`, `btstack: run loop deinit` であり、`btstack: hci power on` へ到達していない
- daemon log: 未作成
- artifact root: NyXpy は未実行。なし
- cleanup: `hci_close` と `btstack_run_loop_deinit` に入った marker はあるが、process は APPCRASH で終了した。Ctrl+C cleanup は未観測。adapter が正常に power-off / close された根拠はない
- notes: crash 点は USB power-on ではなく、HID registration 失敗時 cleanup またはその直後に絞られた。次の診断 build では `sdp_register_service`、`hid_device_init`、`runtime_stop`、`platform_stop` の前後 marker と access violation 用 vectored dump handler を追加した

## 2026-06-20: local_037 CSR8510 A10 8000us detailed diagnostic rerun

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` detailed diagnostic build。base commit は `c090ab1cc463066a0f1bfa047b583f2ff0589b4a`
- Switch firmware: 未記録
- approval scope: ユーザ承認済み。CSR8510 A10、`8000 us`、詳細診断 trace / daemon 自前 dump 付きの daemon 再実行
- environment variables: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を再実行し、`tmp/hardware/local_037/20260620-8000us-detailed-diagnostic-rerun` へ exit marker / startup trace / minidump を保存した
- result: crash 再現。PowerShell の exit marker は `exit=-1073741819`。artifact directory には `daemon-8000us-exit.txt`、`startup-trace.txt`、`swbt-daemon-crash.dmp` が作成された。trace は `hid_registration: sdp record too large` を記録した。root cause は production HID service buffer が BTstack の生成する Switch SDP record を収容できないこと。software regression test で actual record length は `404`、修正前 capacity は `300` と確認した
- daemon log: 未作成
- artifact root: NyXpy は未実行。なし
- cleanup: `hci_close`、`btstack_run_loop_deinit`、`runtime: ipc stop` までは到達した。process は APPCRASH で終了したため、正常終了 cleanup ではない
- notes: `SWBT_DAEMON_PRODUCTION_HID_SERVICE_BUFFER_SIZE` を `512` へ増やし、BTstack `hid_create_sdp_record` の size-less API に対して scratch buffer 生成後に caller buffer へコピーする修正を入れた。`just debug` と `just windows-cross` は修正後 pass

## 2026-06-20: local_037 CSR8510 A10 8000us fixed rerun

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` SDP record buffer fix build。base commit は `c090ab1cc463066a0f1bfa047b583f2ff0589b4a`
- Switch firmware: 未記録
- approval scope: ユーザ承認済み。CSR8510 A10、`8000 us`、SDP record buffer fix 後の daemon 再実行
- environment variables: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を再実行し、`tmp/hardware/local_037/20260620-234720-8000us-fixed-rerun` へ exit marker / startup trace / minidump path を保存した
- result: 前回の `hid_registration: sdp record too large` は再発しなかった。daemon は手動 `Ctrl+C` で停止し、PowerShell の exit marker は `exit=-1` だった。artifact directory には `daemon-8000us-exit.txt` と `startup-trace.txt` が作成され、`swbt-daemon-crash.dmp` と `daemon-8000us.log` は作成されなかった。trace は `hid_registration: sdp register service`、`hid_registration: hid device init`、`hid_registration: ok`、`btstack: hci power on ok`、`production: run loop execute` まで到達した
- daemon log: 未作成
- artifact root: NyXpy は未実行。なし
- cleanup: 手動 `Ctrl+C` 後、trace は `btstack: hci power off`、`production: run loop returned`、`runtime: stop enter` まで到達した。`runtime: stop done` は記録されていないため、cleanup 完了は未確認である
- notes: SDP record buffer fix の実機再実行としては green。Switch pairing、Switch 側での HID advertising 視認、NyXpy IPC input、report loop の実機 input 反映、neutral fail-safe は未観測である

## 2026-06-21: local_037 CSR8510 A10 8000us cleanup direct rerun

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `88f0188aa17be524446a25ea0763aacc35ce5c88`
- Switch firmware: 未記録
- approval scope: ユーザ承認済み。CSR8510 A10、`8000 us`、`Tee-Object` なしの foreground daemon 直接起動、手動 `Ctrl+C` cleanup 確認
- environment variables: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を `Tee-Object` なしで直接起動し、10 秒程度の観測後に同じ console で `Ctrl+C` した。`tmp/hardware/local_037/20260620-235943-8000us-cleanup-direct-rerun` へ exit marker / startup trace / minidump path を保存した
- result: cleanup 完了を確認。PowerShell の exit marker は `exit=0`。artifact directory には `daemon-8000us-exit.txt` と `startup-trace.txt` が作成され、`swbt-daemon-crash.dmp` と daemon stdout / stderr log は作成されなかった。trace は `btstack: hci power on ok`、`production: run loop execute` まで到達した後、手動 `Ctrl+C` により `btstack: hci power off`、`production: run loop returned`、`runtime: report timer stop`、`runtime: output handler stop`、`runtime: hid stop`、`production: hid stop`、`production: platform stop`、`btstack: hci close done`、`btstack: run loop deinit done`、`runtime: ipc stop`、`runtime: stop done`、`production: runtime stop done` まで到達した
- daemon log: 未作成。今回は `Tee-Object` を使わず、`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace を正本にする
- artifact root: NyXpy は未実行。なし
- cleanup: pass。手動 `Ctrl+C` から HCI power-off、BTstack close / run loop deinit、IPC stop、runtime stop done まで到達し、process exit は `0`
- notes: pairing、Switch 側での HID advertising 視認、NyXpy IPC input、report loop の実機 input 反映は未観測である。`Tee-Object` pipeline を使った前回の `exit=-1` は cleanup 未完了の根拠として扱わず、直接起動結果を cleanup 判定の根拠にする

## 2026-06-21: local_037 CSR8510 A10 8000us pairing attempt on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `248049f725688afcc2efca48bfa06bf87373fbba`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、`8000 us`、daemon 起動、Switch2 の pairing 画面での HID advertising / connection state 観測、手動 `Ctrl+C` cleanup 確認。NyXpy IPC input は未実行
- environment variables: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Switch2 側で pairing 画面を観測した。`tmp/hardware/local_037/20260621-000629-8000us-pairing` へ exit marker / startup trace / minidump path を保存した
- result: Switch2 側には何も表示されなかった。daemon 側は `btstack: hci power on ok` と `production: run loop execute` まで到達し、手動 `Ctrl+C` 後に `runtime: stop done` と `production: runtime stop done` まで到達した。PowerShell の exit marker は `exit=0`。artifact directory には `daemon-8000us-exit.txt` と `startup-trace.txt` が作成され、`swbt-daemon-crash.dmp` と daemon stdout / stderr log は作成されなかった
- daemon log: 未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace を正本にする
- artifact root: NyXpy は未実行。なし
- cleanup: pass。手動 `Ctrl+C` から HCI power-off、BTstack close / run loop deinit、IPC stop、runtime stop done まで到達し、process exit は `0`
- notes: pairing / HID advertising の実機結果は red。daemon は power-on と run loop へ到達しているため、次の切り分けは BTstack の discoverable / connectable / class of device / local name / HID service advertising 状態を trace で確認するのが妥当である。Switch2 22.1.0 固有の観測であり、他の Switch firmware へ一般化しない

## 2026-06-21: local_037 CSR8510 A10 8000us GAP discovery pairing rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `4fa1141c2899f49506d587c547924738eb79148f`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、Classic GAP discovery config 修正後、`8000 us`、Switch2 の pairing 画面での HID advertising / connection state 観測、手動 `Ctrl+C` cleanup 確認。NyXpy IPC input は未実行
- environment variables: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Switch2 側で pairing 画面を観測した。`tmp/hardware/local_037/20260621-004439-8000us-gap-discovery-pairing` へ exit marker / startup trace / minidump path を保存した
- result: Switch2 側の pairing 画面は動かなかった。daemon trace では `btstack: classic discovery configure ok` が `btstack: hci power on` より前に記録され、`hid_registration: ok`、`btstack: hci power on ok`、`production: run loop execute` まで到達した。PowerShell の exit marker は `exit=0`。artifact directory には `daemon-8000us-exit.txt` と `startup-trace.txt` が作成され、`swbt-daemon-crash.dmp` と daemon stdout / stderr log は作成されなかった
- daemon log: 未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace を正本にする
- artifact root: NyXpy は未実行。なし
- cleanup: pass。手動 `Ctrl+C` から HCI power-off、BTstack close / run loop deinit、IPC stop、runtime stop done まで到達し、process exit は `0`
- notes: Classic GAP discovery config は production code path へ入ったが、Switch2 22.1.0 の pairing 画面には出なかった。次の切り分けでは `SWBT_HCI_DUMP_TRACE_PATH` を追加し、HCI command / event として inquiry scan、page scan、local name、class of device の設定が controller へ送られているかを確認する
