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
