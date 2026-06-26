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

## 2026-06-21: local_037 CSR8510 A10 8000us HCI dump pairing rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `74c68a74cb02574bea430ce7e7c39df499b478b9`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、HCI dump text 付き、Classic GAP discovery config 修正後、`8000 us`、Switch2 pairing 画面での HID advertising / connection state 観測、手動 `Ctrl+C` cleanup 確認。NyXpy IPC input は未実行
- environment variables: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Switch2 側で pairing 画面を観測した。`tmp/hardware/local_037/20260621-005526-8000us-hci-dump-pairing` へ exit marker / startup trace / HCI dump text を保存した
- result: Switch2 側の pairing 画面は動かなかった。daemon は `btstack: hci dump open ok`、`btstack: classic discovery configure ok`、`hid_registration: ok`、`btstack: hci power on ok`、`production: run loop execute` まで到達し、PowerShell の exit marker は `exit=0` だった。HCI dump では CSR8510 A10 `00:1B:DC:F9:9F:7D` を開き、class of device `0x002508`、local name / EIR `Pro Controller`、default link policy `0x0005`、`Write Scan Enable` value `0x03` が status `0x00` で controller に届いた。Switch 側と思われる `C8:48:05:F7:B5:21` から incoming connection が複数回あり、connection complete は status `0` だった。その後 SSP pairing は requested level `2` で開始し、`HCI_EVENT_USER_CONFIRMATION_REQUEST` の直後に `Simple Pairing Complete` status `0x13` と disconnection reason `0x13` で切断された。dump 上では `User Confirmation Request Reply` opcode `0x042c` は見えていない
- daemon log: 未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: `tmp/hardware/local_037/20260621-005526-8000us-hci-dump-pairing`
- cleanup: pass。手動 `Ctrl+C` から HCI power-off、BTstack close / run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達し、process exit は `0`
- notes: これにより「発見可能化が controller に届いていない」仮説は棄却する。次の修正対象は production packet handler が `HCI_EVENT_USER_CONFIRMATION_REQUEST` を処理せず、BTstack HID examples と同じ `gap_ssp_confirmation_response` を返していない点である。Switch2 22.1.0 / CSR8510 A10 の観測であり、他環境へ一般化しない

## 2026-06-21: local_037 CSR8510 A10 8000us SSP confirmation HCI dump pairing rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `8c447c3 fix(daemon): SSP user confirmation に応答する`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、SSP confirmation fix 後、HCI dump text 付き、`8000 us`、Switch2 pairing 画面での HID advertising / connection state 観測、手動 `Ctrl+C` cleanup 確認。NyXpy IPC input は未実行
- environment variables: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Switch2 側で pairing 画面を観測した。`tmp/hardware/local_037/20260621-010951-8000us-ssp-confirm-hci-dump-pairing` へ exit marker / startup trace / HCI dump text を保存した
- result: Switch2 側の pairing 画面は変化しなかった。daemon は `btstack: hci dump open ok`、`btstack: classic discovery configure ok`、`hid_registration: ok`、`btstack: hci power on ok`、`production: run loop execute` まで到達し、PowerShell の exit marker は `exit=0` だった。HCI dump では `C8:48:05:F7:B5:21` から incoming connection があり、connection complete は status `0` だった。SSP pairing は requested level `2` で開始し、`HCI_EVENT_USER_CONFIRMATION_REQUEST` の直後に `Simple Pairing Complete` status `0x13` と disconnection reason `0x13` で切断された。dump 上では `User Confirmation Request Reply` opcode `0x042c` はまだ見えていない
- daemon log: 未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: `tmp/hardware/local_037/20260621-010951-8000us-ssp-confirm-hci-dump-pairing`
- cleanup: pass。手動 `Ctrl+C` から HCI power-off、BTstack close / run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達し、process exit は `0`
- notes: SSP confirmation handler の実装だけでは HCI command `0x042c` が送られなかった。BTstack `hid_device_register_packet_handler` は HID device callback を保存するだけで HCI event handler 登録は行わない。BTstack HID examples と同様に `hci_add_event_handler` で production packet handler を HCI events にも登録する必要がある。Switch2 22.1.0 / CSR8510 A10 の観測であり、他環境へ一般化しない

## 2026-06-21: local_037 CSR8510 A10 8000us HCI event handler pairing rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `016b8e9 fix(btstack): HID handler を HCI event にも登録する`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、HCI event handler registration fix 後、HCI dump text 付き、`8000 us`、Switch2 pairing 画面での HID advertising / connection state 観測、手動 `Ctrl+C` cleanup 確認。NyXpy IPC input は未実行
- environment variables: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Switch2 側で pairing 画面を観測した。`tmp/hardware/local_037/20260621-012253-8000us-hci-event-handler-hci-dump-pairing` へ exit marker / startup trace / HCI dump text を保存した
- result: Switch2 側の画面は変化しなかった。daemon は `btstack: hci dump open ok`、`btstack: classic discovery configure ok`、`hid_registration: ok`、`btstack: hci power on ok`、`production: run loop execute` まで到達し、PowerShell の exit marker は `exit=0` だった。HCI dump では `User Confirmation Request Reply` opcode `0x042c` が送信され、`Simple Pairing Complete` は status `0x00` になった。その後 security level `2`、PSM `0x11` と PSM `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` まで到達した。swbt から Switch 側へ 57 byte の ACL packet が 548 個送信されている。一方、Switch 側からの `packet type=0x02 in=1` は L2CAP setup までで、HID output report / subcommand は観測されなかった
- daemon log: 未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: `tmp/hardware/local_037/20260621-012253-8000us-hci-event-handler-hci-dump-pairing`
- cleanup: pass。手動 `Ctrl+C` から HCI power-off、BTstack close / run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達し、process exit は `0`
- notes: Bluetooth SSP pairing と HID control / interrupt L2CAP channel open は pass。Switch UI 上の controller 採用、HID output report / subcommand 受信、NyXpy IPC input 反映は未観測である。次の切り分けは、Switch が HID output report を送らない理由、または swbt の input report / descriptor / SDP attributes が Switch2 22.1.0 の controller adoption 条件を満たしていない理由に移る。Switch2 22.1.0 / CSR8510 A10 の観測であり、他環境へ一般化しない

## 2026-06-21: local_037 CSR8510 A10 8000us single-shot IPC input HCI dump pairing rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、ブランチ `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `543524d`; production binary behavior is unchanged from `016b8e9 fix(btstack): HID handler を HCI event にも登録する`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、HCI event handler registration fix 後、HCI dump text 付き、`8000 us`、Switch2 pairing 画面での HID advertising / connection state 観測、単発 IPC input の反映確認、手動 `Ctrl+C` cleanup 確認。NyXpy IPC input は未実行
- environment variables: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: 予定値 `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Switch2 側で pairing 画面を観測した。単発 IPC input を試したが、client command と client stdout は artifact に未保存。`tmp/hardware/local_037/20260621-013528-8000us-hci-event-handler-hci-dump-pairing` へ exit marker / startup trace / HCI dump text を保存した
- result: Switch2 側の画面は変化しなかった。daemon は `btstack: hci dump open ok`、`btstack: classic discovery configure ok`、`hid_registration: ok`、`btstack: hci power on ok`、`production: run loop execute` まで到達し、PowerShell の exit marker は `exit=0` だった。HCI dump では `User Confirmation Request Reply` opcode `0x042c`、`Simple Pairing Complete` status `0x00`、security level `2`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` を再確認した。swbt から Switch 側へ 57 byte の ACL packet が `1877` 個送信されたが、timer byte を除いた input report state は `0000000000088000088000000000000000000000000000000000000000000000000000000000000000000000000000` の 1 種類だけだった。単発 input は HCI dump 上の非 neutral report として観測されなかった。Switch 側からの `packet type=0x02 in=1` は `8` 件で、L2CAP setup payload だけだった
- daemon log: 未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: `tmp/hardware/local_037/20260621-013528-8000us-hci-event-handler-hci-dump-pairing`
- cleanup: pass。手動 `Ctrl+C` から HCI power-off、BTstack close / run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達し、process exit は `0`
- notes: 今回の実機観測は「入力が Switch に無視された」ではなく、「単発 IPC input が 8000 us report loop に捕捉された根拠がない」と扱う。`swbt-debug-client` は `set_state` 後に `release` するため、held state を作らない単発実行では次の report tick 前に neutral へ戻る可能性がある。次の入力反映確認は NyXpy または held-state capable client で、非 neutral state を 1 秒以上維持し、同時に HCI dump の timer byte 除外 state を再集計する

## 2026-06-21: local_037 NyXPy held-input IPC timeout diagnosis

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: pre-fix `2e08336`; IPC pump fix はこの commit に含める
- Switch firmware: Switch2 `22.1.0`
- approval scope: NyXPy 操作はユーザが実行した。Codex は software diagnosis / fix だけを実施し、この項目では新しい実機 daemon run を実行していない
- environment variables: planned daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: ユーザが Project NyX 側で `uv run nyxpy run swbt_hardware_bringup ...` を実行した。NyXPy stdout は `swbt hardware bring-up IPC probe initialized: scenario=held_input_probe, steps=5` の後に `MacroRuntime | runtime.failed | macro failed` と `timed out` を記録した。Codex は swbt production IPC path を確認した
- result: software-gate red。production daemon は IPC listener を開始していたが、BTstack run loop 中に pending connection の accept と JSON Lines request serve を回していなかった。そのため TCP connect は OS backlog で成立し得る一方、NyXPy は `hello_ok` などの daemon response を受け取れず timeout したと判断した。修正として `swbt_daemon_ipc_runner_poll_once` と production BTstack の 1 ms IPC pump timer を追加した。`just debug` は 31/31 pass、`just windows-cross` は pass。修正後の実機 rerun は未実行
- daemon log: この timeout では daemon 側 artifact は提示されていない。NyXPy stdout はユーザ提供
- artifact root: NyXPy artifact path は未提示。Codex はこの項目で新しい daemon hardware artifact を作成していない
- cleanup: この項目では新しい実機 daemon run を実行していないため、追加 cleanup はない
- notes: この項目は held input が Switch に反映されることを示さない。次の rerun では修正後 binary を使い、startup trace に `btstack: ipc pump start ok` が出ること、NyXPy が timeout せず IPC response を受け取ること、HCI dump の timer byte 除外 state に非 neutral state が出ることを別々に確認する

## 2026-06-21: local_037 CSR8510 A10 8000us NyXPy held Button A rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `2cd003b fix(daemon): production IPC を run loop で処理する`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、production IPC pump fix 後、HCI dump text 付き、`8000 us`、Switch2 controller pairing 画面での NyXPy held input 反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を実行した。daemon artifact は `tmp/hardware/local_037/20260621-022214-8000us-held-input-nyxpy`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T022219_74b5`
- result: IPC と HCI report 送信は pass。NyXPy の `ipc_session.json` は `hello_ok`、`acquired`、`state_accepted` for `seq=3` Button A、`state_accepted` for `seq=4` neutral、cleanup `release_sent=true` を記録した。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、`production: run loop execute`、手動停止後の `production: runtime stop done` まで到達した。HCI dump は `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0`、57 byte input report `1093` 個を記録した。timer byte を除外して集計すると state は 2 種類で、neutral `1034` 個、Button A `0x000008` が `59` 個だった。Button A report は HCI dump の report index `469` から `527`、line `1214` から `1330` に出ている
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-022214-8000us-held-input-nyxpy`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T022219_74b5`
- cleanup: pass。daemon exit marker は `exit=0`。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: Switch2 の画面は baseline、Button A、neutral の NyXPy capture で同じ「使いたいコントローラーの L + R を押してください」画面だった。したがって今回の red は「IPC input が daemon または HCI に届かない」ではない。Button A 単体がこの画面の期待入力ではないこと、または Switch2 がこの virtual controller の input report を UI 入力としてまだ採用していないことが残る。次の入力反映確認は L+R 同時押しを held state として送る。swbt の定義では `SWBT_BUTTON_R = 1u << 6`、`SWBT_BUTTON_L = 1u << 22` なので L+R は `0x00400040`、decimal `4194368`

## 2026-06-21: local_037 CSR8510 A10 8000us HIDP input header NyXPy held L+R rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `978790d` plus dirty HIDP input header changes; later truncated-report software fix was not in this hardware run
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、HIDP input header fix 後、HCI dump text 付き、`8000 us`、Switch2 controller pairing 画面での NyXPy held L+R 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を `probe_label=l_plus_r`、`probe_buttons=0x00400040` で実行した。daemon artifact は `tmp/hardware/local_037/20260621-114529-8000us-hidp-input-header-rerun`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T114953_604d`
- result: Switch2 側の画面は変化しなかった。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達し、exit marker は `exit=0` だった。NyXPy `ipc_session.json` は `hello_ok`、`acquired`、L+R `state_accepted`、neutral `state_accepted`、cleanup `release_sent=true` を記録した。HCI dump は incoming connection、`pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` を記録した。swbt から Switch 側への `a1 30` input report は `17345` 件あり、NyXPy L+R state は `a130...00400040...` として HCI dump に現れた。Switch 側からは `a2 01 ...` output report が `886` 件来たが、同数の `hid_device.c.636: Ignore invalid report data packet, invalid size` で BTstack が捨てていた
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-114529-8000us-hidp-input-header-rerun`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T114953_604d`
- cleanup: pass。daemon exit marker は `exit=0`。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: HIDP input header fix 後、Switch 側は HID output report を送る段階まで進んだ。画面が変化しない次の直接原因候補は、BTstack が Switch2 からの短い output report を descriptor size 不一致として callback 前に破棄している点である。次の software gate では `vendor/btstack` を直接変更せず、BTstack の `hid_device_accept_truncated_hid_reports(true)` を production HID adapter で有効化する

## 2026-06-21: local_037 CSR8510 A10 8000us truncated output report NyXPy L+R rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `978790d` plus dirty HIDP input header and truncated-report acceptance changes; later device-info reply fix was not in this hardware run
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、truncated HID report acceptance fix 後、HCI dump text 付き、`8000 us`、Switch2 controller pairing 画面での NyXPy held L+R 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を `probe_label=l_plus_r`、`probe_buttons=0x00400040` で実行した。daemon artifact は `tmp/hardware/local_037/20260621-120325-8000us-hidp-input-header-rerun`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T120333_9d41`
- result: Switch2 側の画面は変化しなかった。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達し、exit marker は `exit=0` だった。NyXPy `ipc_session.json` は `hello_ok`、`acquired`、L+R `state_accepted`、neutral `state_accepted`、cleanup `release_sent=true` を記録した。HCI dump は incoming connection、`pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` を記録した。swbt から Switch 側への `a1 30` input report は `2539` 件、Switch 側からの `a2 01` output report は `130` 件だった。`hid_device.c.636: Ignore invalid report data packet, invalid size` は `0` 件になった。`a2 01` の subcommand は全件 `0x02` request device info で、swbt からの `a1 21` subcommand reply は `0` 件だった
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-120325-8000us-hidp-input-header-rerun`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T120333_9d41`
- cleanup: pass。daemon exit marker は `exit=0`。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: truncated HID report acceptance により BTstack の size rejection は解消した。画面が変化しない次の直接原因候補は、Switch2 が繰り返す subcommand `0x02` request device info に swbt が未応答で、初期化列が次へ進まない点である。次の software gate では device info reply を実装し、production では BTstack の local BD_ADDR を reply data の controller address として使う

## 2026-06-21: local_037 CSR8510 A10 8000us device info reply NyXPy L+R rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `978790d` plus dirty HIDP input header、truncated-report acceptance、device-info reply changes; later low-power-mode ACK fix was not in this hardware run
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、device info reply fix 後、HCI dump text 付き、`8000 us`、Switch2 controller pairing 画面での NyXPy held L+R 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を `probe_label=l_plus_r`、`probe_buttons=0x00400040` で実行した。daemon artifact は `tmp/hardware/local_037/20260621-121941-8000us-device-info-rerun`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T122239_f34e`
- result: Switch2 側の画面は変化しなかった。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達し、exit marker は `exit=0` だった。NyXPy `ipc_session.json` は `hello_ok`、`acquired`、L+R `state_accepted`、neutral `state_accepted`、cleanup `release_sent=true` を記録した。HCI dump では incoming connection、`pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` を再確認した。BTstack `invalid size` は `0` 件。swbt から Switch 側への `a1 30` input report は `14294` 件、Switch 側からの `a2 01` subcommand は `0x02` が `2` 件、`0x08` が `723` 件だった。swbt は `0x02` に対して `a1 21 ... 82 02 04 00 03 02 00 1b dc f9 9f 7d 01 01` を `1` 件返したが、`0x08` に対する `a1 21 ... 80 08` は出ていない
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-121941-8000us-device-info-rerun`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T122239_f34e`
- cleanup: pass。daemon exit marker は `exit=0`。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: request device info reply により、Switch2 は device info 待ちから次の subcommand `0x08` へ進んだ。画面が変化しない次の直接原因候補は、Switch2 が繰り返す subcommand `0x08` low power / shipment state に swbt が未応答で、初期化列が次へ進まない点である。次の software gate では `0x08` に ACK `0x80` の simple ACK を返す

## 2026-06-21: local_037 CSR8510 A10 8000us low power ACK NyXPy L+R rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `978790d` plus dirty HIDP input header、truncated-report acceptance、device-info reply、low-power ACK changes; later daemon default report-options fix was not in this hardware run
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、low power mode ACK fix 後、HCI dump text 付き、`8000 us`、Switch2 controller pairing 画面での NyXPy held L+R 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を `probe_label=l_plus_r`、`probe_buttons=0x00400040` で実行した。daemon artifact は `tmp/hardware/local_037/20260621-123338-8000us-device-info-rerun`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T123344_f287`
- result: Switch2 側の画面は変化しなかった。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達し、exit marker は `exit=0` だった。NyXPy `ipc_session.json` は `hello_ok`、`acquired`、L+R `state_accepted`、neutral `state_accepted`、cleanup `release_sent=true` を記録した。HCI dump では `hid_device.c.636: Ignore invalid report data packet, invalid size` は `0` 件。Switch 側からの `a2 01` subcommand は `0x02` が `1` 件、`0x08` が `78` 件だった。swbt は `0x02` に対して `a1 21 ... 82 02` を `1` 件、`0x08` に対して `a1 21 ... 80 08` を `77` 件返した。先頭の `0x08` は HID channel open 前に届き、BTstack は `acl_handler called with non-registered handle 72!` を記録した。swbt から Switch 側への `a1 30` input report は `1461` 件で、buttons は neutral `1421` 件、L+R `0x400040` が `40` 件だった。`a1 30` の battery/connection byte と vibrator byte は全件 `0x00` だった
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-123338-8000us-device-info-rerun`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T123344_f287`
- cleanup: pass。daemon exit marker は `exit=0`。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: low power mode ACK により、`0x08` への未応答は解消した。ただし Switch2 は `0x10` SPI read や `0x03` report mode へ進まず、`0x08` を繰り返した。今回の HCI dump では production report prefix の battery/connection と vibrator が `0x00` のままだった。joycontrol は同じ prefix で battery/connection `0x8e` と vibrator `0x80` を使うため、次の software gate では daemon default report options を `0x8e` / `0x80` にする

## 2026-06-21: local_037 CSR8510 A10 8000us report options default NyXPy L+R rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `978790d` plus dirty HIDP input header、truncated-report acceptance、device-info reply、low-power ACK、daemon report-options default changes。Project NyX `run_context.json` の `swbt_commit` は `unrecorded`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、daemon report-options default fix 後、HCI dump text 付き、`8000 us`、Switch2 controller pairing 画面での NyXPy held L+R 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を `probe_label=l_plus_r`、`probe_buttons=0x00400040`、notes `report options default rerun` で実行した。daemon artifact は `tmp/hardware/local_037/20260621-125102-8000us-report-options-rerun`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T125113_a937`
- result: Switch2 側の画面は変化しなかった。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達し、exit marker は `exit=0` だった。NyXPy `ipc_session.json` は `hello_ok`、`acquired`、L+R `state_accepted`、neutral `state_accepted`、cleanup `release_sent=true` を記録した。HCI dump では `hid_device.c.636: Ignore invalid report data packet, invalid size` は `0` 件。Switch 側からの `a2 01` subcommand は `0x02` が `1` 件、`0x08` が `64` 件だった。swbt は `0x02` に対して `a1 21 ... 82 02` を `1` 件、`0x08` に対して `a1 21 ... 80 08` を `63` 件返した。先頭の `0x08` は HID channel open 前に届き、BTstack は `acl_handler called with non-registered handle 71!` を記録した。swbt から Switch 側への `a1 30` input report は `1191` 件で、buttons は neutral `1152` 件、L+R `0x400040` が `39` 件だった。`a1 30` と `a1 21` の battery/connection byte は全件 `0x8e`、vibrator byte は全件 `0x80` だった
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-125102-8000us-report-options-rerun`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T125113_a937`
- cleanup: pass。daemon exit marker は `exit=0`。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: report-options default fix により、前回の `0x00` prefix 問題は解消した。ただし Switch2 は `0x10` SPI read や `0x03` report mode へ進まず、`0x08` を繰り返した。joycontrol は `SET_INPUT_REPORT_MODE` (`0x03`) を受けてから continuous `0x30` report loop に入るため、次の software gate は subcommand reply 経路を残したまま、periodic `0x30` を `0x03` 受信まで送らない制御に置く

## 2026-06-21: local_037 CSR8510 A10 8000us report-mode gate NyXPy L+R rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `978790d` plus dirty HIDP input header、truncated-report acceptance、device-info reply、low-power ACK、daemon report-options default、report-mode gate changes
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、report-mode gate fix 後、HCI dump text 付き、`8000 us`、Switch2 controller pairing 画面での NyXPy held L+R 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を実行した。daemon artifact は `tmp/hardware/local_037/20260621-132059-8000us-report-mode-gate-rerun`
- result: Switch2 側の画面は変化しなかった。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達し、exit marker は `exit=0` だった。HCI dump では `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` を再確認した。report-mode gate により outgoing `a1 30` は `0` 件、outgoing `a1 21` は `0` 件だった。Switch 側からの `a2 01` output report も `0` 件で、interrupt channel open 後の ACL packet は観測されなかった。BTstack `invalid size` と `non-registered handle` はどちらも `0` 件だった
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-132059-8000us-report-mode-gate-rerun`
- cleanup: pass。daemon exit marker は `exit=0`。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達した
- notes: `SET_INPUT_REPORT_MODE` (`0x03`) まで periodic `0x30` を完全に止めると、Switch2 は subcommand sequence 自体を開始しなかった。したがって、前回の `0x08` 反復の直接原因を「`0x03` 前の `0x30` 送信」とみなす仮説は支持されない。次の候補は、初期 `0x30` は出して Switch 側の output report を誘発しつつ、subcommand reply 後の periodic `0x30` を抑制または遅延する制御である

## 2026-06-21: local_037 CSR8510 A10 8000us reply holdoff NyXPy L+R rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `978790d` plus dirty HIDP input header、truncated-report acceptance、device-info reply、low-power ACK、daemon report-options default、reply periodic holdoff changes
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、reply periodic holdoff fix 後、HCI dump text 付き、`8000 us`、Switch2 controller pairing 画面での NyXPy held L+R 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を実行した。daemon artifact は `tmp/hardware/local_037/20260621-133628-8000us-report-mode-gate-rerun`。directory 名には前実験名が残っているが、HCI dump では `a1 30` が送信されているため reply holdoff 後の断面として扱う
- result: Switch2 側の画面は変化しなかった。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達し、exit marker は `exit=0` だった。HCI dump では `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` を再確認した。BTstack `invalid size` と `non-registered handle` はどちらも `0` 件だった。Switch 側からの `a2 01` subcommand は `0x02` が `1` 件、`0x08` が `53` 件だった。swbt は `0x02` に `a1 21 ... 82 02` を `1` 件、`0x08` に `a1 21 ... 80 08` を `53` 件返した。`a1 30` input report は `996` 件で、buttons は neutral `959` 件、L+R `0x400040` が `37` 件だった。`a1 30` と `a1 21` の battery/connection byte は全件 `0x8e`、vibrator byte は全件 `0x80` だった
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-133628-8000us-report-mode-gate-rerun`
- cleanup: pass。daemon exit marker は `exit=0`。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達した
- notes: reply holdoff により、前回見えていた channel 登録前の `0x08` drop はこの断面では消えた。ただし Switch2 は `0x08` ACK を受けても `0x10` SPI read や `0x03` report mode へ進まず、画面も変化しなかった。初期 `0x30` の有無、report prefix、BTstack size validation、先頭 `0x08` drop は現時点の直接原因候補から下げる。次の候補は `0x08` reply semantics、または直前の request device info identity / payload を Switch2 が受理していない可能性である

## 2026-06-21: local_037 CSR8510 A10 8000us mizuyoukanao device info profile NyXPy L+R rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `1971368`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`、HCI dump text 付き、`8000 us`、Switch2 controller pairing 画面での NyXPy held L+R 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を `probe_label=l_plus_r`、`probe_buttons=0x00400040`、notes `device_info_mizuyoukanao_pro_rerun` で実行した。daemon artifact は `tmp/hardware/local_037/20260621-140355-8000us-device-info-mizuyoukanao-pro-rerun`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T140402_a9e1`
- result: Switch2 側の画面は変化しなかった。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達した。HCI dump では `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` を再確認した。BTstack `invalid size` と `non-registered handle` はどちらも `0` 件だった。Switch 側からの `a2 01` subcommand は `0x02` が `1` 件、`0x08` が `152` 件だった。swbt は `0x02` に `a1 21 ... 82 02 03 48 03 02 00 1b dc f9 9f 7d 03 02` を `1` 件、`0x08` に `a1 21 ... 80 08` を `152` 件返した。`a1 30` input report は `2818` 件で、buttons は neutral `2779` 件、L+R `0x400040` が `39` 件だった。`a1 30` の battery/connection byte は全件 `0x8e`、vibrator byte は全件 `0x80` だった。NyXPy `ipc_session.json` は `hello_ok`、`acquired`、L+R `state_accepted`、neutral `state_accepted`、cleanup `release_sent=true` を記録した
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-140355-8000us-device-info-mizuyoukanao-pro-rerun`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T140402_a9e1`
- cleanup: pass。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: `mizuyoukanao-pro` profile override は実際に `0x02` reply data を変更したが、Switch2 は `0x08` 反復から `0x10` SPI read や `0x03` report mode へ進まなかった。これにより、firmware bytes と device info tail bytes だけが直接原因である可能性は下げる。次の候補は `0x08` reply data / state semantics、または device info 以外の identity 情報である

## 2026-06-21: local_037 CSR8510 A10 8000us shared subcommand timer NyXPy L+R rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `a186dcb`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`、shared subcommand timer fix 後、HCI dump text 付き、`8000 us`、Switch2 controller pairing 画面での NyXPy held L+R 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を `probe_label=l_plus_r`、`probe_buttons=0x00400040`、notes `subcommand_reply_timer_shared_rerun` で実行した。daemon artifact は `tmp/hardware/local_037/20260621-143010-8000us-subcommand-reply-timer-rerun`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T143135_7049`
- result: Switch2 側の画面は変化しなかった。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達した。exit marker は残っていないが、trace 上の cleanup は pass と扱う。NyXPy `ipc_session.json` は `hello_ok`、`acquired`、L+R `state_accepted`、neutral `state_accepted`、cleanup `release_sent=true` を記録した。HCI dump では `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0`、BTstack `invalid size` `0` 件、`non-registered handle` `0` 件だった。Switch 側からの `a2 01` subcommand は `0x02` が `1` 件、`0x08` が `1` 件、`0x10` が `2` 件、`0x03` が `1` 件、`0x04` が `369` 件だった。swbt は `0x02`、`0x08`、`0x10` 2 件、`0x03` に対して outgoing `a1 21` を計 `5` 件返し、`0x04` には返していない。`a1 21` の timer byte は `08`、`0b`、`0c`、`0d`、`0e` で、`a1 30` と同じ timer 系列を共有した。`a1 30` input report は `7245` 件で、buttons は neutral `7177` 件、L+R `0x400040` が `68` 件だった。`a1 30` の battery/connection byte は全件 `0x8e`、vibrator byte は全件 `0x80` だった
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-143010-8000us-subcommand-reply-timer-rerun`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T143135_7049`
- cleanup: pass by trace。daemon exit marker は未作成。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: shared subcommand timer fix により、固定 `0x00` timer 仮説は下げる。Switch2 は `0x08` 反復を抜け、SPI read `0x10` 2 件と report mode `0x03` へ進んだ。次の直接原因候補は、Switch2 が繰り返す `0x04` trigger buttons elapsed time subcommand に swbt が未応答で、初期化列が `0x48` / `0x40` / `0x30` へ進まない点である。次の software gate では `0x04` reply の ACK byte と payload semantics を根拠監査してから実装する

## 2026-06-21: local_037 CSR8510 A10 8000us trigger elapsed reply NyXPy L+R rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `f7b75d6`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`、trigger buttons elapsed reply fix 後、HCI dump text 付き、`8000 us`、Switch2 controller pairing 画面での NyXPy held L+R 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を `probe_label=l_plus_r`、`probe_buttons=0x00400040`、notes `trigger_elapsed_reply_rerun` で実行した。daemon artifact は `tmp/hardware/local_037/20260621-150120-8000us-trigger-elapsed-rerun`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T150130_23c9`
- result: Switch2 側の画面変化をユーザが観測した。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達した。exit marker は残っていないが、trace 上の cleanup は pass と扱う。HCI dump では `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0`、BTstack `invalid size` `0` 件だった。Switch 側からの `a2 01` subcommand は `0x02` が `1` 件、`0x08` が `1` 件、`0x10` が `8` 件、`0x03` が `1` 件、`0x04` が `1` 件、`0x40` が `1` 件、`0x30` が `1` 件、`0x48` が `1` 件、`0x21` が `209` 件だった。swbt は `0x04` に `a1 21 ... 83 04 2c 01 2c 01 00 ...` を返し、その後 Switch2 は追加 SPI read、IMU enable `0x40`、player lights `0x30`、vibration enable `0x48` へ進んだ。outgoing `a1 30` input report は `4117` 件で、buttons は neutral `4057` 件、L+R `0x400040` が `60` 件だった。`a1 30` の battery/connection byte は全件 `0x8e`、vibrator byte は全件 `0x80` だった
- NyXPy result: `run_context.json` は scenario `held_input_probe`、notes `trigger_elapsed_reply_rerun`、probe `l_plus_r` buttons `4194368` を記録した。`ipc_session.json` は `hello_ok`、`acquired`、L+R `state_accepted`、neutral `state_accepted`、cleanup `release_sent=true`、`socket_closed=true`、`command_release_called=true` を記録した。capture は baseline、L+R、neutral のいずれも controller 1 の枠を表示し、L+R prompt 自体は残っている
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-150120-8000us-trigger-elapsed-rerun`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T150130_23c9`
- cleanup: pass by trace。daemon exit marker は未作成。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done、production runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: `0x04` trigger buttons elapsed reply は Switch2 22.1.0 / CSR8510 A10 の今回条件で初期化列を前進させた。`SET_PLAYER_LIGHTS` (`0x30`) と capture の controller 1 表示から、Switch UI 上の controller 採用は pass と扱う。一方、capture は L+R prompt のままで、held L+R による画面遷移までは確認できていない。次の直接原因候補は、Switch2 が繰り返す `0x21` NFC/IR MCU config subcommand に swbt が未応答である点、または pairing 画面で必要な最終入力が L+R ではなく A へ移った点である

## 2026-06-21: local_037 CSR8510 A10 8000us NFC/IR MCU config reply NyXPy L+R plus Button A rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `94cc04c`; daemon code for `0x21` reply was introduced by `19c6c75`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`、NFC/IR MCU config reply fix 後、HCI dump text 付き、`8000 us`、Switch2 controller pairing 画面での NyXPy held L+R and Button A 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を `probe_label=l_plus_r`、`probe_buttons=0x00400040`、`probe_states=["button_a"]`、notes `nfc_mcu_a_followup_rerun` で実行した。daemon artifact は `tmp/hardware/local_037/20260621-201840-8000us-nfc-mcu-a-followup-rerun`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T201845_e709`
- result: Switch2 側の画面変化をユーザが観測し、決定ボタンによる接続設定画面終了まで到達した。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達した。HCI dump では `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、BTstack `invalid size` `0` 件だった。`non-registered handle` は run 初期に `1` 件あるが、current connection の pairing / HID sequence は継続している。Switch 側からの `a2 01` subcommand は `0x02` が `1` 件、`0x08` が `1` 件、`0x10` が `8` 件、`0x03` が `1` 件、`0x04` が `1` 件、`0x40` が `1` 件、`0x48` が `1` 件、`0x21` が `1` 件、`0x30` が `2` 件だった。swbt は `0x21` に `a1 21 ... a0 21 01 00 ff 00 08 00 1b 01 ... c8` を `1` 件返し、`0x21` 反復は消えた。outgoing `a1 21` replies は `82/02` `1` 件、`80/08` `1` 件、`90/10` `8` 件、`80/03` `1` 件、`83/04` `1` 件、`80/40` `1` 件、`80/48` `1` 件、`a0/21` `1` 件、`80/30` `2` 件だった。outgoing `a1 30` input report は `11410` 件で、buttons は neutral `11260` 件、L+R `0x400040` `64` 件、Button A `0x000008` `86` 件だった
- NyXPy result: `run_context.json` は scenario `held_input_probe`、step count `7`、notes `nfc_mcu_a_followup_rerun`、probe `l_plus_r` buttons `4194368`、probe states `button_a` を記録した。`ipc_session.json` は `hello_ok`、`acquired`、L+R `state_accepted`、L+R neutral `state_accepted`、Button A `state_accepted`、Button A neutral `state_accepted`、cleanup `release_sent=true`、`socket_closed=true`、`command_release_called=true` を記録した。capture は baseline と L+R が L+R prompt、Button A と Button A neutral が controller settings screen を表示した
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-201840-8000us-nfc-mcu-a-followup-rerun`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T201845_e709`
- cleanup: pass by trace。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done、production runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: `0x21` NFC/IR MCU config reply は Switch2 22.1.0 / CSR8510 A10 の今回条件で `0x21` 反復を解消した。`0x30` subcommand、Button A の `a1 30` report、capture の画面遷移から、Switch UI 上の IPC input 反映は pass と扱う。NFC/IR semantic state は未実装であり、この結果は pairing / controller setup sequence を進める ACK / payload の確認である。残件は report period comparison、owner disconnect / heartbeat timeout の neutral fail-safe、bonded reconnect persistence である

## 2026-06-21: local_037 CSR8510 A10 8333us report period NyXPy Button A rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `493235e`; daemon code for `0x21` reply was introduced by `19c6c75`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`、HCI dump text 付き、`8333 us` report period、Switch2 controller pairing 画面での NyXPy Button A 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8333`, `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8333 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を notes `report_period_8333us_rerun`、`probe_states=button_a` で実行した。daemon artifact は `tmp/hardware/local_037/20260621-203339-8333us-report-period-rerun`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T203401_7b18`
- result: Switch2 側の画面遷移をユーザが観測した。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達した。HCI dump では `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、BTstack `invalid size` `0` 件、`non-registered handle` `0` 件だった。Switch 側からの `a2 01` subcommand は `0x02` が `1` 件、`0x08` が `1` 件、`0x10` が `8` 件、`0x03` が `1` 件、`0x04` が `1` 件、`0x40` が `1` 件、`0x48` が `1` 件、`0x21` が `1` 件、`0x30` が `2` 件だった。outgoing `a1 21` replies は `82/02` `1` 件、`80/08` `1` 件、`90/10` `8` 件、`80/03` `1` 件、`83/04` `1` 件、`80/40` `1` 件、`80/48` `1` 件、`a0/21` `1` 件、`80/30` `2` 件だった。outgoing `a1 30` input report は `4191` 件で、buttons は neutral `4045` 件、L+R `0x400040` `63` 件、Button A `0x000008` `83` 件だった
- NyXPy result: `run_context.json` は scenario `held_input_probe`、step count `7`、notes `report_period_8333us_rerun`、probe states `button_a` を記録した。`ipc_session.json` は L+R `state_accepted`、L+R neutral `state_accepted`、Button A `state_accepted`、Button A neutral `state_accepted`、cleanup `release_sent=true`、`socket_closed=true`、`command_release_called=true` を記録した
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-203339-8333us-report-period-rerun`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T203401_7b18`
- cleanup: pass by trace。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done、production runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: `8333 us` は今回条件で controller setup sequence、subcommand replies、NyXPy state acceptance、Switch UI 画面遷移まで pass と扱う。ただし、この entry は画面遷移を基準にした粗い受理確認であり、report jitter、入力遅延、取りこぼし率の厳密な測定ではない

## 2026-06-21: local_037 CSR8510 A10 15000us report period NyXPy Button A rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `493235e`; daemon code for `0x21` reply was introduced by `19c6c75`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`、HCI dump text 付き、`15000 us` report period、Switch2 controller pairing 画面での NyXPy Button A 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=15000`, `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `15000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を notes `report_period_15000us_rerun`、`probe_states=button_a` で実行した。daemon artifact は `tmp/hardware/local_037/20260621-203531-15000us-report-period-rerun`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T203538_6d57`
- result: Switch2 側の画面遷移をユーザが観測した。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達した。HCI dump では `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、BTstack `invalid size` `0` 件、`non-registered handle` `0` 件だった。Switch 側からの `a2 01` subcommand は `0x02` が `1` 件、`0x08` が `1` 件、`0x10` が `8` 件、`0x03` が `1` 件、`0x04` が `1` 件、`0x40` が `1` 件、`0x48` が `1` 件、`0x21` が `1` 件、`0x30` が `2` 件だった。outgoing `a1 21` replies は `82/02` `1` 件、`80/08` `1` 件、`90/10` `8` 件、`80/03` `1` 件、`83/04` `1` 件、`80/40` `1` 件、`80/48` `1` 件、`a0/21` `1` 件、`80/30` `2` 件だった。outgoing `a1 30` input report は `2181` 件で、buttons は neutral `2052` 件、L+R `0x400040` `62` 件、Button A `0x000008` `67` 件だった
- NyXPy result: `run_context.json` は scenario `held_input_probe`、step count `7`、notes `report_period_15000us_rerun`、probe states `button_a` を記録した。`ipc_session.json` は L+R `state_accepted`、L+R neutral `state_accepted`、Button A `state_accepted`、Button A neutral `state_accepted`、cleanup `release_sent=true`、`socket_closed=true`、`command_release_called=true` を記録した
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-203531-15000us-report-period-rerun`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T203538_6d57`
- cleanup: pass by trace。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done、production runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: `15000 us` は今回条件で controller setup sequence、subcommand replies、NyXPy state acceptance、Switch UI 画面遷移まで pass と扱う。ただし、この entry は画面遷移を基準にした粗い受理確認であり、report jitter、入力遅延、取りこぼし率の厳密な測定ではない

## 2026-06-21: local_037 CSR8510 A10 16667us report period NyXPy Button A rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`、Project NyX branch `feat/swbt-hardware-bringup-macro`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `493235e`; daemon code for `0x21` reply was introduced by `19c6c75`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`、HCI dump text 付き、`16667 us` report period、Switch2 controller pairing 画面での NyXPy Button A 入力反映確認、手動停止 cleanup 確認。NyXPy 操作はユーザが実行した
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=16667`, `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `16667 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、Project NyX 側で `swbt_hardware_bringup` macro の `held_input_probe` を notes `report_period_16667us_rerun`、`probe_states=button_a` で実行した。daemon artifact は `tmp/hardware/local_037/20260621-203753-16667us-report-period-rerun`。NyXPy artifact は `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T203807_b671`
- result: Switch2 側の画面遷移をユーザが観測した。daemon startup trace は `btstack: ipc pump start ok`、`hid_registration: ok`、`btstack: hci power on ok`、手動停止後の `production: runtime stop done` まで到達した。HCI dump では `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、BTstack `invalid size` `0` 件、`non-registered handle` `0` 件だった。Switch 側からの `a2 01` subcommand は `0x02` が `1` 件、`0x08` が `1` 件、`0x10` が `8` 件、`0x03` が `1` 件、`0x04` が `1` 件、`0x40` が `1` 件、`0x48` が `1` 件、`0x21` が `1` 件、`0x30` が `2` 件だった。outgoing `a1 21` replies は `82/02` `1` 件、`80/08` `1` 件、`90/10` `8` 件、`80/03` `1` 件、`83/04` `1` 件、`80/40` `1` 件、`80/48` `1` 件、`a0/21` `1` 件、`80/30` `2` 件だった。outgoing `a1 30` input report は `1678` 件で、buttons は neutral `1556` 件、L+R `0x400040` `61` 件、Button A `0x000008` `61` 件だった
- NyXPy result: `run_context.json` は scenario `held_input_probe`、step count `7`、notes `report_period_16667us_rerun`、probe states `button_a` を記録した。`ipc_session.json` は L+R `state_accepted`、L+R neutral `state_accepted`、Button A `state_accepted`、Button A neutral `state_accepted`、cleanup `release_sent=true`、`socket_closed=true`、`command_release_called=true` を記録した
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-203753-16667us-report-period-rerun`、NyXPy `../Project_NyX/resources/swbt_hardware_bringup/artifacts/20260621T203807_b671`
- cleanup: pass by trace。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done、production runtime stop done まで到達した。NyXPy artifact は cleanup `release_sent=true` と socket close を記録した
- notes: `16667 us` は今回条件で controller setup sequence、subcommand replies、NyXPy state acceptance、Switch UI 画面遷移まで pass と扱う。ただし、この entry は画面遷移を基準にした粗い受理確認であり、report jitter、入力遅延、取りこぼし率の厳密な測定ではない

## 2026-06-21: local_037 CSR8510 A10 8000us owner disconnect neutral rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `45fa03a`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`、HCI dump text 付き、`8000 us` report period、`swbt-debug-client` による Button A owner disconnect neutral fail-safe 確認、手動停止 cleanup 確認
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、別 PowerShell で `build/windows-mingw-debug/swbt-debug-client.exe --port 37637 --button a --seq 9003 --hold-ms 3000 --skip-release` を実行した。client stdout / stderr は `owner-disconnect-client.log` に保存した。client 終了後に `Start-Sleep -Seconds 5` で owner socket close 後の report loop を残し、その後 daemon を手動 `Ctrl+C` で停止した
- result: owner disconnect neutral fail-safe pass。client log は `owner.present=true`、`owner_id=00000001`、`last_seq=9003`、`state.buttons=8`、`client_exit=0` を記録した。HCI dump では `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、BTstack `invalid size` `0` 件、`non-registered handle` `0` 件だった。outgoing `a1 30` input report は `934` 件で、buttons は neutral `000000` が `451` 件、Button A `080000` が `330` 件、owner socket close 後の neutral `000000` が `153` 件だった。`080000` は IPC state `buttons=8` の little-endian 3 byte 表現である
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-211311-8000us-owner-disconnect-rerun`
- cleanup: pass by trace。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done、production runtime stop done まで到達した。PowerShell exit marker は `exit=0`
- notes: この entry は owner socket close による neutralization の確認である。Bluetooth L2CAP close は手動停止時に記録されており、owner disconnect と同義ではない

## 2026-06-21: local_037 CSR8510 A10 8000us heartbeat timeout neutral rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `fe000d5`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`、`SWBT_IPC_HEARTBEAT_TIMEOUT_MS=1000`、HCI dump text 付き、`8000 us` report period、`swbt-debug-client` による Button A heartbeat timeout neutral fail-safe 確認、手動停止 cleanup 確認
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`, `SWBT_IPC_HEARTBEAT_TIMEOUT_MS=1000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、別 PowerShell で `build/windows-mingw-debug/swbt-debug-client.exe --port 37637 --button a --seq 9101 --hold-ms 3000 --skip-release` を実行した。client stdout / stderr は `heartbeat-timeout-client.log` に保存した。client 終了後に `Start-Sleep -Seconds 3` で timeout 後の report loop を残し、その後 daemon を手動 `Ctrl+C` で停止した
- result: heartbeat timeout neutral fail-safe pass。client log は `owner.present=true`、`owner_id=00000001`、`last_seq=9101`、`state.buttons=8`、`client_exit=0` を記録した。HCI dump では `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、BTstack `invalid size` `0` 件、`non-registered handle` `0` 件だった。outgoing `a1 30` input report は `1354` 件で、buttons は neutral `000000` が `1274` 件、Button A `080000` が `80` 件だった。区間は neutral `449` 件、Button A `80` 件、timeout 後の neutral `825` 件の順に並んだ。`080000` は IPC state `buttons=8` の little-endian 3 byte 表現である
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-213315-8000us-heartbeat-timeout-rerun`
- cleanup: pass by trace。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done、production runtime stop done まで到達した。PowerShell exit marker は `exit=0`
- notes: client は `--hold-ms 3000 --skip-release` で owner socket を保持したまま release を送らない。Button A report は `80` 件で終わり、その後 neutral report が `825` 件続いたため、owner socket close ではなく `SWBT_IPC_HEARTBEAT_TIMEOUT_MS=1000` による neutralization と扱う。ただし HCI dump text は timestamp を持たないため、timeout が厳密に 1000 ms で発火したことはこの entry では測定していない

## 2026-06-21: local_037 CSR8510 A10 8000us shutdown while Button A owner is held on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: branch `local-037-hardware-verification` at `29434b2`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`、`SWBT_IPC_HEARTBEAT_TIMEOUT_MS=0`、HCI dump text 付き、`8000 us` report period、`swbt-debug-client` による Button A held owner 中の daemon shutdown neutral fail-safe 確認、手動 `Ctrl+C` cleanup 確認
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`, `SWBT_IPC_HEARTBEAT_TIMEOUT_MS=0`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、別 PowerShell で HCI dump の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` が `2` 件になるまで待ってから、`build/windows-mingw-debug/swbt-debug-client.exe --port 37637 --button a --seq 9204 --hold-ms 60000 --skip-release` を実行した。status JSON 表示後、client の `hold-ms` が尽きる前に daemon を手動 `Ctrl+C` で停止した。client stdout は表示優先のため artifact には保存せず、`shutdown-neutral-client.log` は `client_exit=0` だけを記録した
- result: shutdown 中の Switch-facing neutral report は未達。HCI dump では `pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、BTstack `invalid size` `0` 件だった。`non-registered handle` は pairing 前に `1` 件あるが、current connection は継続した。outgoing `a1 30` input report は `1094` 件で、buttons は neutral `000000` が `696` 件、Button A `080000` が `398` 件だった。区間は neutral `696` 件、Button A `398` 件で、Button A の次の行が `hci_power_control: 0` だった。shutdown 前または shutdown 中の trailing neutral `000000` report は観測されなかった
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-215647-8000us-shutdown-neutral-rerun`
- cleanup: pass by trace。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done、production runtime stop done まで到達した。PowerShell exit marker は `exit=0`
- notes: `swbt_daemon_runtime_stop` は内部 state を neutral に戻すが、production shutdown request は先に HCI power-off と run loop exit を進める。今回の HCI dump では Button A report の直後に HCI power-off に入っており、Switch-facing neutral report を送れていない。shutdown cleanup は正常だが、非 neutral owner が残る shutdown fail-safe は Switch-facing behavior として未達である

## 2026-06-21: local_037 CSR8510 A10 8000us shutdown neutral postfix rerun on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `local-037-hardware-verification`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `dc74af407db536cda4bcd191649a683cc71c83fc`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`、`SWBT_IPC_HEARTBEAT_TIMEOUT_MS=0`、HCI dump text 付き、`8000 us` report period、`swbt-debug-client` による Button A held owner 中の shutdown trailing neutral report 確認、手動 `Ctrl+C` cleanup 確認
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`, `SWBT_IPC_HEARTBEAT_TIMEOUT_MS=0`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: foreground PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を直接起動し、別 PowerShell で HCI dump の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` が `2` 件になるまで待ってから、`build/windows-mingw-debug/swbt-debug-client.exe --port 37637 --button a --seq 9301 --hold-ms 60000 --skip-release` を実行した。status JSON 表示後、client の `hold-ms` が尽きる前に daemon を手動 `Ctrl+C` で停止した
- result: shutdown trailing neutral report pass。startup trace は `production: shutdown neutral send`、`production: shutdown neutral send ok` を `btstack: hci power off` より前に記録した。HCI dump では `pairing complete, status 00` `1` 件、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、BTstack `invalid size` `0` 件、`non-registered handle` `0` 件だった。outgoing `a1 30` input report は `813` 件で、buttons は neutral `000000` が `162` 件、Button A `080000` が `651` 件だった。区間は neutral `161` 件、Button A `651` 件、shutdown 直前の trailing neutral `1` 件である。末尾は line `2039` Button A `080000`、line `2040` neutral `000000`、line `2041` `hci_power_control: 0` の順だった
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260621-221559-8000us-shutdown-neutral-postfix-rerun`
- cleanup: pass by trace。startup trace は HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done、production runtime stop done まで到達した。PowerShell exit marker は `exit=0`、client marker は `client_exit=0`
- notes: この entry は shutdown request が HCI power-off 前に Switch-facing neutral report を 1 件送ることを確認した実機観測である。HCI dump text には timestamp がないため、shutdown request から neutral report 送信までの時間は測定していない

## 2026-06-22: local_045 env dependency audit 8000us Button A smoke on Switch2

- OS: Microsoft Windows NT 10.0.26200.0
- environment: Windows native PowerShell、swbt branch `refactor/env-dependency-audit`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `ed6039bca3ac07667a1e10f43116e1897ee5a78d`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、adapter open、HID advertising / connectable、Switch pairing、L2CAP 接続、`8000 us` report loop、`swbt-debug-client` による Button A 3 秒入力と release、HCI dump / diagnostic trace 保存、cleanup 確認
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: preflight として `just windows-cross` pass。PowerShell から `build/windows-mingw-debug/swbt-daemon.exe` を `CREATE_NEW_PROCESS_GROUP` 付きで起動し、HCI dump の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` が 2 件になるまで待った。その後 `build/windows-mingw-debug/swbt-debug-client.exe --port 37637 --button a --seq 9401 --hold-ms 3000` を実行した。client の release 後に `GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT)` で daemon の console control handler 経由の cleanup を要求した
- result: Button A smoke pass。client log は `owner.present=true`、`owner_id=00000001`、`last_seq=9401`、`state.buttons=8`、`client_exit=0` を記録した。HCI dump では `pairing complete, status 00` `1` 件、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、BTstack `invalid size` `0` 件、`non-registered handle` `0` 件だった。outgoing `a1 30` input report は `441` 件で、buttons は neutral `000000` が `331` 件、Button A `080000` が `110` 件だった。区間は neutral `13` 件、Button A `110` 件、release 後 neutral `318` 件の順に並んだ
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_037/20260622-003424-8000us-env-audit-smoke`
- cleanup: pass by trace。startup trace は `production: shutdown neutral send`、`production: shutdown neutral send ok`、HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done、production runtime stop done まで到達した。PowerShell exit marker は `exit=0`、`cleanup_forced=False`、client marker は `client_exit=0`。crash dump は作成されなかった
- notes: この entry は `local_045` の環境変数依存監査後に、診断系 environment variable と実機 gate を明示した production run が現行 branch で実機接続と Button A / release まで進むことを確認した smoke である。Switch UI capture は今回取得していないため、画面遷移の根拠としては扱わない

## 2026-06-22: local_049 swbt-pro default 8000us run on Switch2

- OS: Microsoft Windows [Version 10.0.26200.8655]
- environment: Windows native PowerShell、swbt branch `work/swbt-device-info-profile`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `728a3ce57ebd3df8cdff3ba0f81aa6972455c32b`
- Switch firmware: Switch2 `22.1.0`
- approval scope: ユーザ承認済み。CSR8510 A10、adapter open、HID advertising / connectable、Switch pairing、L2CAP 接続、`8000 us` report loop、`SWBT_DEVICE_INFO_PROFILE` 未指定の `swbt-pro` default、`swbt-debug-client` による L+R 3 秒入力と Button A 3 秒入力、HCI dump / diagnostic trace 保存、cleanup 確認
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`。`SWBT_DEVICE_INFO_PROFILE` は未指定
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: preflight として `just windows-cross` pass。PowerShell で `SWBT_DEVICE_INFO_PROFILE` を未設定にして `build/windows-mingw-debug/swbt-daemon.exe` を foreground 起動した。別 PowerShell で `build/windows-mingw-debug/swbt-debug-client.exe --port 37637 --button l --button r --seq 4901 --hold-ms 3000` と `build/windows-mingw-debug/swbt-debug-client.exe --port 37637 --button a --seq 4902 --hold-ms 3000` を実行した。ユーザは Switch 側でコントローラー登録から Button A による画面遷移まで到達したことを観測した。入力確認後、daemon を手動停止した
- result: `swbt-pro` default run pass。HCI dump では `pairing complete, status 00` `1` 件、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、BTstack `invalid size` `0` 件だった。`non-registered handle` は pairing 前に `1` 件あるが、current connection は継続した。request device info reply は `a1 21 ... 82 02 04 00 03 02 00 1b dc f9 9f 7d 01 01` を `1` 件記録し、`SWBT_DEVICE_INFO_PROFILE` 未指定で `swbt-pro` bytes が返った。incoming subcommand は `0x02` `1`、`0x08` `1`、`0x10` `8`、`0x03` `1`、`0x04` `1`、`0x40` `1`、`0x48` `1`、`0x21` `1`、`0x30` `2`。outgoing `a1 21` replies は `82/02` `1`、`80/08` `1`、`90/10` `8`、`80/03` `1`、`83/04` `1`、`80/40` `1`、`80/48` `1`、`a0/21` `1`、`80/30` `2`。outgoing `a1 30` input report は `2448` 件で、buttons は neutral `000000` `1925` 件、L+R `400040` `194` 件、Button A `080000` `329` 件だった
- debug client result: L+R client は `state.buttons=4194368`、`client_lr_exit=0`。Button A client は `state.buttons=8`、`client_a_exit=0`
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: daemon `tmp/hardware/local_049/20260622-202545-8000us-swbt-pro-default`
- cleanup: pass by trace。startup trace は `production: shutdown neutral send`、`production: shutdown neutral send ok`、HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done、production runtime stop done まで到達した。PowerShell exit marker は `exit=0`、client markers は `client_lr_exit=0` と `client_a_exit=0`
- notes: この entry は `local_048` で `mizuyoukanao-pro` を削除し、`swbt-pro` を daemon default にした後の実機再実行である。`SWBT_DEVICE_INFO_PROFILE=swbt-pro` の明示指定 run は、未指定 default run で `swbt-pro` reply と Switch UI 入力反映まで確認できたため実施していない。この結果は CSR8510 A10 / WinUSB / Switch2 firmware `22.1.0` の当該条件の hardware observation であり、別 adapter / firmware の一般互換性ではない

## 2026-06-23: local_057 architecture cutover H1 on Switch2

- OS: Microsoft Windows 11 Pro、Version `10.0.26200`、Build `26200`、64-bit
- environment: Windows native PowerShell、swbt branch `codex/architecture-cutover`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`、Service `WinUSB`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: git HEAD `a45930304515d0eeb2b3f909961584da17ae5f31` plus uncommitted `codex/architecture-cutover` working tree diff。artifact の `git-status-short.txt` に差分一覧を保存した
- Switch firmware: Switch2 `22.1.0`。既存実機環境の継続値であり、今回の artifact では本体画面を再読していない
- approval scope: ユーザ承認済み。CSR8510 A10、WinUSB、pairing / HID advertising / periodic input report loop、owner disconnect、daemon shutdown、`SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、HCI dump / diagnostic trace 保存を含む H1 scope
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: preflight として `just debug` pass、`just windows-cross` pass。PowerShell で `build/windows-mingw-debug/swbt-daemon.exe` を `CREATE_NEW_PROCESS_GROUP` 付きで起動し、HCI dump の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` が 2 件になるまで待った。`swbt-debug-client --button a --seq 1 --hold-ms 1000 --skip-release`、`swbt-debug-client --button a --seq 2 --hold-ms 1000 --skip-release` を順に実行し、owner disconnect 後の neutral と再度 Button A を観測した。shutdown held state は PowerShell から daemon IPC へ直接 `hello`、`acquire`、`set_state(seq=3, buttons=8)` を送り、TCP connection を保持したまま `GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT)` で daemon shutdown を要求した
- failed attempts recorded: `20260623-104806`, `20260623-105050`, `20260623-105219` は shutdown request 前に `production: hid connection closed` へ進み、`production: neutral send timer stopped` または `shutdown neutral send failed` を記録した。これらは H1 fail として扱い、最終 pass には含めない
- result: architecture cutover H1 pass。最終 artifact `tmp/hardware/local_057/20260623-105416-architecture-cutover-h1` では daemon trace が `production: shutdown requested`、`production: shutdown neutral send`、`production: neutral send adapter ok`、`production: shutdown neutral send ok`、`btstack: hci power off` の順を記録した。HCI dump では `pairing complete, status 00` `1` 件、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、`invalid size` `0` 件、`non-registered handle` `0` 件だった。outgoing `a1 30` input report pattern は Button A `080000` が `109` 件、neutral `000000` が `205` 件だった。末尾は line `953` Button A `080000`、line `954` trailing neutral `000000`、line `955` `hci_power_control: 0` の順である
- daemon log: daemon stdout / stderr log は未作成。`SWBT_DIAGNOSTIC_TRACE_PATH` の startup trace と `SWBT_HCI_DUMP_TRACE_PATH` の HCI dump text を正本にする
- artifact root: `tmp/hardware/local_057/20260623-105416-architecture-cutover-h1`
- cleanup: pass by trace。daemon exit code `0`。trace は HCI power-off、run loop returned、power off cleanup、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、host stop done まで到達した。crash dump は作成されなかった
- notes: HCI dump には `usb_open: Device open failed` が `16` 件ある。これは BTstack の USB device 探索中に対象外 interface / device を試した記録であり、current connection の `invalid size` / `non-registered handle` とは分けて扱う。この entry の pass 判定は、Button A report、owner disconnect 後 neutral、再度 Button A、shutdown trailing neutral、cleanup trace の組み合わせに基づく hardware observation である

## 2026-06-24: local_065 daemon restart reconnect boundary on Switch2

- OS: Microsoft Windows 11 Pro、Version `10.0.26200`、Build `26200`、64-bit
- environment: Windows native PowerShell、swbt branch `docs/config-file-work-units`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`、Service `WinUSB`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `371914f5fd65b23c9f59dcd7366a3cda2edd410b`
- Switch firmware: Switch2 `22.1.0`。既存実機環境の継続値であり、今回の artifact では本体画面を再読していない
- approval scope: ユーザ承認済み。CSR8510 A10、WinUSB、adapter open、HID advertising、initial pairing、daemon restart、Switch 側操作なしの reconnect 待ち、`8000 us` report loop、HCI dump / diagnostic trace 保存、cleanup 確認
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`。`SWBT_DEVICE_INFO_PROFILE` は未指定
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: preflight として `just windows-cross` pass。PowerShell で `tmp/hardware/local_065/run-daemon-restart-reconnect.ps1` を実行した。script は artifact root を daemon working directory とし、`initial-pairing` で L2CAP open と Button A smoke を確認後に daemon を `CTRL_BREAK_EVENT` で停止した。その後、同じ working directory と同じ `swbt-bond-*.tlv` を残したまま `daemon-restart-reconnect` を起動し、Switch 側を操作せず L2CAP open を待って Button A smoke を実行した
- result: bonded reconnect は未達。`initial-pairing` と `daemon-restart-reconnect` はどちらも PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件と Button A client exit `0` を記録したが、restart 側 HCI dump にも `pairing complete, status 00` が `1` 件出た。両 run の HCI dump は `Remote not bonding, dropping local flag` を記録し、`swbt-bond-00-1b-dc-f9-9f-7d.tlv` は `8` bytes のまま、restart 後も更新されなかった。現行実装では TLV-backed DB は接続されるが、Switch2 `22.1.0` が bonding を要求しないため link key material が保存されず、daemon restart 後の接続は既存 bond ではなく再 pairing と扱う
- daemon log: daemon stdout / stderr log は未作成。各 run の `daemon-trace.txt` と `hci-dump.txt`、artifact root の `summary.json` を正本にする
- artifact root: `tmp/hardware/local_065/20260624-234907-daemon-restart-reconnect`
- cleanup: pass by trace。両 run とも daemon exit code `0`、forced stop `false`、crash dump なし。trace は `btstack: bond cache configured`、`btstack: hci power off`、`btstack: bond cache close` を各 `1` 件記録した
- notes: raw TLV content と link key value は表示、転記していない。この entry は `CSR8510 A10` / WinUSB / Switch2 firmware `22.1.0` / swbt commit `371914f` の hardware observation であり、別 firmware や別 adapter の一般結果ではない

## 2026-06-25: local_065 Switch sleep/resume reconnect boundary on Switch2

- OS: Microsoft Windows 11 Pro、Version `10.0.26200`、Build `26200`、64-bit
- environment: Windows native PowerShell、swbt branch `docs/config-file-work-units`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Class `USBDevice`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`、Service `WinUSB`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: `6633b1b7a417c4da1858a4751dc7c20d02677b47`
- Switch firmware: Switch2 `22.1.0`。既存実機環境の継続値であり、今回の artifact では本体画面を再読していない
- approval scope: ユーザ承認済み。CSR8510 A10、WinUSB、adapter open、HID advertising、initial pairing、Switch sleep / resume、resume 後 reconnect 待ち、`8000 us` report loop、HCI dump / diagnostic trace 保存、cleanup 確認
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`。`SWBT_DEVICE_INFO_PROFILE` は未指定
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: preflight として `just windows-cross` pass。PowerShell で `tmp/hardware/local_065/run-sleep-resume-reconnect.ps1` を実行した。script は initial L2CAP open と Button A smoke を確認後、operator action window で Switch を sleep / resume し、同一 daemon process 内で HCI dump の L2CAP open count が `4` になるまで待った。その後、resume 後の Button A smoke を実行した
- result: sleep / resume 後の接続復帰と Button A smoke は成立したが、bonded reconnect は未達。`final_l2cap_open_count=4`、`after_resume_client_ran=true`。HCI dump は初回接続で `pairing complete, status 00` を line `209`、resume 後の再接続で同じ `pairing complete, status 00` を line `1031` に記録した。`Remote not bonding, dropping local flag` も line `199` と line `1021` の `2` 件だった。`swbt-bond-00-1b-dc-f9-9f-7d.tlv` は `8` bytes のままで、link key material は保存されなかった。したがって、Switch sleep / resume 後の復帰は既存 bond ではなく再 pairing と扱う
- daemon log: daemon stdout / stderr log は未作成。`sleep-resume-reconnect/daemon-trace.txt`、`sleep-resume-reconnect/hci-dump.txt`、artifact root の `summary.json` を正本にする
- artifact root: `tmp/hardware/local_065/20260625-000432-sleep-resume-reconnect`
- cleanup: pass by trace。daemon exit code `0`、forced stop `false`、crash dump なし。trace は `btstack: bond cache configured`、`production: hid connection closed`、`btstack: hci power off`、`btstack: bond cache close` を記録した
- notes: raw TLV content と link key value は表示、転記していない。この entry は `CSR8510 A10` / WinUSB / Switch2 firmware `22.1.0` / swbt commit `6633b1b` の hardware observation であり、別 firmware や別 adapter の一般結果ではない

## 2026-06-26: local_073 active reconnect request boundary on Switch2

- OS: Microsoft Windows NT `10.0.26200.0`
- environment: Windows native PowerShell、swbt branch `work/local-073-config-path-reconnect`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Class `USBDevice`、Service `WinUSB`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: git HEAD `d3f2b9cc6b4adf5de2acf252f94f0ca743707bed` plus uncommitted `CMakeLists.txt` MinGW static link fix (`add_link_options(-static)`)。artifact 生成後、この差分は branch に残した
- Switch firmware: Switch2 `22.1.0`。既存実機環境の継続値であり、今回の artifact では本体画面を再読していない
- approval scope: ユーザ承認済み。CSR8510 A10、WinUSB、adapter open、initial pairing、learned Switch address の TOML config 書き戻し、daemon restart、保存済み address からの active reconnect request、Switch 側の再接続操作、Button A smoke、HCI dump / diagnostic trace 保存、cleanup 確認
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`。daemon argument は `--config tmp/hardware/local_073/20260626-171348-active-reconnect/swbt-daemon.toml`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: 初回起動前に `just windows-cross` を実行したが、Windows host 起動時に `libgcc_s_seh-1.dll` 不在が露出したため、MinGW build に `add_link_options(-static)` を追加して `just windows-cross` を再実行した。`swbt-daemon.exe --definitely-invalid-option` が exit code `1` で即終了することを確認した後、空の TOML config を作成し、`swbt-daemon.exe --config <config>` を production env 付きで起動した。初回 run は Switch 側 pairing、learned address 保存、Button A smoke を実行した。その後 daemon を停止し、同じ config で restart run を起動した。restart run では Switch 側の pairing 操作をせず、daemon power on 直後の active reconnect request と、ユーザによる Switch 側の登録済み controller 再接続操作を観測した
- result: learned address config writeback は成功した。初回 run の HCI dump は `pairing complete, status 00` `1` 件、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件を記録し、trace は `production: learned switch address save ok` を `1` 件記録した。最初の restart run は trace に `production: active reconnect request ok` を `1` 件記録したが、HCI dump は outgoing connection で `Connection_complete (status=8)` `1` 件、control PSM `0x11` の `L2CAP_EVENT_CHANNEL_OPENED status 0x8` `1` 件を記録した。ユーザの Switch 側再接続操作後は incoming connection が発生し、`pairing complete, status 00` `2` 件、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `4` 件を記録したため、この操作は active reconnect 成功ではなく incoming pairing path と扱う。後続の manual rerun `tmp/hardware/local_073/manual-20260626-174907-active-reconnect` では、Switch 側操作前に daemon 起点の outgoing connection が `Connection_complete (status=0)`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0`、trace の `production: hid connection opened` まで到達した。`Connection_incoming`、`Connection_complete (status=8)`、`L2CAP_EVENT_CHANNEL_OPENED status 0x8` は記録されなかった。HCI dump は `a1 30` input report `9532` 件、Switch 側 `a2 01` output report `17` 件、`a1 21` reply `17` 件を記録した。一方、同じ manual rerun でも `have link key db: 0`、`pairing started, ssp 1, initiator 1`、`pairing complete, status 00` `1` 件が出たため、これは daemon initiated active reconnect transport の成功であり、保存済み link key による pairing-free reconnect ではない
- daemon log: daemon stdout / stderr log は空。`initial2-trace.txt`、`initial2-hci-dump.txt`、`restart-trace.txt`、`restart-hci-dump.txt`、manual rerun の `restart-trace.txt`、`restart-hci-dump.txt` を正本にする
- artifact root: `tmp/hardware/local_073/20260626-171348-active-reconnect`, `tmp/hardware/local_073/manual-20260626-174907-active-reconnect`
- cleanup: pass by trace。最初の restart run は停止前に `swbt-debug-client.exe --port 37637 --seq 3 --hold-ms 100` で neutral state を accepted させた。`GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT)` は `sent=True`、`still_running=false` を記録した。manual rerun は trace で `production: shutdown neutral send ok`、`btstack: hci power off`、`production: run loop returned`、`btstack: hci close done`、`btstack: run loop deinit done`、`btstack: hci dump close done`、`production: host stop done` まで到達した。crash dump は作成されなかった
- notes: config file 上の raw Switch address は表示、転記していない。この entry は `CSR8510 A10` / WinUSB / Switch2 firmware `22.1.0` / swbt commit `d3f2b9c` plus MinGW static link diff の hardware observation であり、別 firmware や別 adapter の一般結果ではない

## 2026-06-26: local_073 experimental link key DB reconnect boundary on Switch2

- OS: Microsoft Windows NT `10.0.26200.0`
- environment: Windows native PowerShell、swbt branch `work/local-073-config-path-reconnect`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Class `USBDevice`、Service `WinUSB`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: git HEAD `384e5d4`
- Switch firmware: Switch2 `22.1.0`。既存実機環境の継続値であり、今回の artifact では本体画面を再読していない
- approval scope: ユーザ承認済み。CSR8510 A10、WinUSB、adapter open、initial pairing、experimental TLV-backed Classic link key DB、daemon restart、保存済み address からの active reconnect request、HCI dump / diagnostic trace 保存、cleanup 確認
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`。daemon argument は `--config <artifact>/swbt-daemon.toml --experimental-link-key-db <artifact>/swbt-link-key.tlv`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: `tmp/hardware/local_073/20260626-191642-link-key-db-reconnect` を artifact root として、空の TOML config と experimental link key DB path を指定した initial run を起動した。initial run では Switch 側 pairing を実行し、restart run では同じ config と TLV path で Switch 側操作なしの active reconnect request を観測した
- result: experimental DB open は initial / restart の両方で pass。initial run は trace の `btstack: experimental link key db open ok`、`production: hid connection opened`、`production: learned switch address save ok` を記録した。initial HCI dump は `responding to link key request, have link key db: 1`、`pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` を記録したが、同時に `Remote not bonding, dropping local flag` も記録した。`swbt-link-key.tlv` は `8` bytes のままで、link key material は保存されなかった。restart run は trace の `btstack: experimental link key db open ok` と `production: active reconnect request ok` を記録したが、`production: hid connection opened` は記録しなかった。restart HCI dump は `Create outgoing HID Control` の後、`Connection_complete (status=22)` と control PSM `0x11` の `L2CAP_EVENT_CHANNEL_OPENED status 0x16` を記録した。restart run では `responding to link key request` と `pairing complete, status 00` は記録されていない
- daemon log: daemon stdout / stderr log は空。`initial-trace.txt`、`initial-hci-dump.txt`、`restart-trace.txt`、`restart-hci-dump.txt` を正本にする
- artifact root: `tmp/hardware/local_073/20260626-191642-link-key-db-reconnect`
- cleanup: initial inspection 時点では `swbt-daemon.exe` process が残り、TLV file は process lock されていた。その後 `Get-Process swbt-daemon -ErrorAction SilentlyContinue` が空であることを確認した。TLV file は `BTstack` header の 8 bytes のみで、raw link key value は含まれていない
- notes: config file 上の raw Switch address、HCI dump 上の raw address、raw link key value は表示、転記していない。この entry は `CSR8510 A10` / WinUSB / Switch2 firmware `22.1.0` / swbt commit `384e5d4` の hardware observation であり、別 firmware や別 adapter の一般結果ではない

## 2026-06-26: local_073 experimental link key notification persistence retest on Switch2

- OS: Microsoft Windows NT `10.0.26200.0`
- environment: Windows native PowerShell、swbt branch `work/local-073-config-path-reconnect`
- dongle: CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`
- USB VID/PID: `0A12:0001`
- driver: Status `OK`、Class `USBDevice`、Service `WinUSB`、Provider `libwdi`、INF `oem75.inf`、DriverVersion `6.1.7600.16385`
- backend: `windows-winusb`
- BTstack: `075a0780f0fad7ff67d58ac19f46e8953656a752`
- swbt: git HEAD `47ea721`
- Switch firmware: Switch2 `22.1.0`。既存実機環境の継続値であり、今回の artifact では本体画面を再読していない
- approval scope: ユーザ承認済み。CSR8510 A10、WinUSB、adapter open、initial pairing、experimental TLV-backed Classic link key DB、HCI link key notification 明示保存、daemon restart、保存済み address からの active reconnect request、Button A smoke、HCI dump / diagnostic trace 保存、cleanup 確認
- environment variables: daemon side `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`。daemon argument は `--config <artifact>/swbt-daemon.toml --experimental-link-key-db <artifact>/swbt-link-key.tlv`
- IPC endpoint: `127.0.0.1:37637`
- report period: `8000 us`
- command / procedure: `tmp/hardware/local_073/20260626-195716-link-key-db-reconnect-retest` を artifact root として、空の TOML config と experimental link key DB path を指定した initial run を起動した。initial run では Switch 側 pairing を実行し、restart run では同じ config と TLV path で Switch 側操作なしの active reconnect request を観測した。両 run で Button A smoke と shutdown neutral send を実行した
- result: initial run は trace の `btstack: experimental link key db stored notification`、`production: hid connection opened`、`production: learned switch address save ok` を記録し、`swbt-link-key.tlv` は `48` bytes へ増えた。restart run は trace の `production: active reconnect request ok`、`btstack: experimental link key db stored notification`、`production: hid connection opened`、`production: learned switch address save ok` を記録し、`swbt-link-key.tlv` は `88` bytes へ増えた。Button A smoke は initial / restart の両方で exit code `0`
- reconnect boundary: restart run は `hid connection opened` まで到達したが、HCI dump 上は outgoing connection が `Connection_complete (status=8)` と control PSM `0x11` の `L2CAP_EVENT_CHANNEL_OPENED status 0x8` で一度失敗した後、incoming connection で `pairing started, ssp 1, initiator 0`、`Remote not bonding, dropping local flag`、`pairing complete, status 00`、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` を記録した。したがって今回の結果は link key notification の保存成功と incoming SSP pairing による接続成功であり、保存済み link key だけによる pairing-free reconnect 成功ではない
- daemon log: daemon stdout / stderr log は空。`initial-pairing/initial-pairing-trace.txt`、`initial-pairing/initial-pairing-hci-dump.txt`、`daemon-restart-reconnect/daemon-restart-reconnect-trace.txt`、`daemon-restart-reconnect/daemon-restart-reconnect-hci-dump.txt`、`summary.json` を正本にする
- artifact root: `tmp/hardware/local_073/20260626-195716-link-key-db-reconnect-retest`
- cleanup: pass。initial / restart の両方で neutral state を accepted させた後、`CTRL_BREAK_EVENT` により daemon は exit code `0` で終了した。`Get-Process swbt-daemon -ErrorAction SilentlyContinue` は空で、crash dump は作成されなかった
- notes: config file 上の raw Switch address、HCI dump 上の raw address、raw link key value は表示、転記していない。この entry は `CSR8510 A10` / WinUSB / Switch2 firmware `22.1.0` / swbt commit `47ea721` の hardware observation であり、別 firmware や別 adapter の一般結果ではない
