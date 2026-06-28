# BTstack Daemon Entrypoint Source Audit

## 1. 状態

recorded。

この reference は `swbt-daemon` production backend が使う BTstack run loop、HCI power、HID Device event、WinUSB / libusb transport 境界の根拠監査である。

2026-06-28 時点の current implementation では、backend selection は `--backend production|noop` と既定 production で扱う。`SWBT_DAEMON_BACKEND`、`SWBT_RUN_HARDWARE`、`SWBT_HARDWARE_APPROVED` は起動 mode selector ではない。実機承認は `hardware-harness` と `spec/operations/windows-native-preflight.md` の運用 gate として扱う。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| BTstack submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | `vendor/btstack` |
| swbt implementation | current work unit | `swbt/daemon/production_runner.*`, `swbt/btstack_bridge/production_btstack_impl.*` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| Windows WinUSB port startup order | memory init, Windows run loop init, `hci_init(hci_transport_usb_instance(), NULL)`, app setup, run loop execute | source fact | `vendor/btstack/port/windows-winusb/main.c:138-168` | stable at pinned commit |
| libusb port startup order | memory init, POSIX run loop init, `hci_init(hci_transport_usb_instance(), NULL)`, app setup, run loop execute | source fact | `vendor/btstack/port/libusb/main.c:334-389` | stable at pinned commit |
| Windows USB transport provider | `hci_transport_usb_instance()` returns `H2_WINUSB` transport with `usb_open` / `usb_close` | source fact | `vendor/btstack/platform/windows/hci_transport_h2_winusb.c:1459-1473` | WinUSB adapter open boundary |
| libusb transport provider | `hci_transport_usb_instance()` returns `H2_LIBUSB` transport with `usb_open` / `usb_close` | source fact | `vendor/btstack/platform/libusb/hci_transport_h2_libusb.c:1518-1533` | Linux build boundary |
| HCI power-on opens adapter | `hci_power_control(HCI_POWER_ON)` reaches transport `init` and `open()` through `hci_power_control_on` | source fact | `vendor/btstack/src/hci.c:5831-5860`, `vendor/btstack/src/hci.c:6138-6168` | approval gate must run before power-on |
| HCI power-off closes adapter | power-off path calls transport `close()` | source fact | `vendor/btstack/src/hci.c:5867-5884` | cleanup boundary |
| HCI close | `hci_close()` discards connections, powers off, and releases stack memory when malloc is enabled | source fact | `vendor/btstack/src/hci.c:5705-5720` | production platform cleanup |
| run loop init / execute / trigger exit / deinit | BTstack run loop API stores selected loop, executes it, can trigger exit, and can deinit | source fact | `vendor/btstack/src/btstack_run_loop.c:308-333`, `vendor/btstack/src/btstack_run_loop.h:223,338,363,369` | lifecycle boundary |
| HID Device deinit | clears callbacks, singleton state, descriptor pointer, descriptor size, and HID cid | source fact | `vendor/btstack/src/classic/hid_device.c:859-878` | HID cleanup boundary |
| Windows platform include capitalization | `hci_transport_h2_winusb.c` includes `<Windows.h>`, `<SetupAPI.h>`, and `<Winusb.h>`; MinGW headers are lowercase on the Dev Container filesystem | source fact / build observation | `vendor/btstack/platform/windows/hci_transport_h2_winusb.c:56,70,71`, `cmake/mingw-compat-include/*` | cross-build compatibility boundary |
| HID meta event code | `HCI_EVENT_HID_META = 0xef` | source fact | `vendor/btstack/src/btstack_defines.h:2123` | event parser constant |
| HID connection / can-send / close subevents | opened `0x02`, closed `0x03`, can-send `0x04` | source fact | `vendor/btstack/src/btstack_defines.h:4203-4217` | event parser constants |
| HID event field offsets | opened/closed/can-send events expose `hid_cid` at byte offset 3; opened status is at byte offset 5 | source fact | `vendor/btstack/src/btstack_event.h:15072-15133` | event parser layout |
| HID can-send generation | `hid_device_request_can_send_now_event` generates `HID_SUBEVENT_CAN_SEND_NOW`; implementation emits it after pending request can send | source fact | `vendor/btstack/src/classic/hid_device.h:134-143`, `vendor/btstack/src/classic/hid_device.c:823-829` | send-ready bridge boundary |

## 4. 実装判断

- `swbt-daemon` は既定で production backend を選ぶ。Bluetooth アダプターを開かない test / smoke は `--backend noop` または `--backend=noop` を明示する。
- production backend は `--adapter-location winusb:<location-path>` または `--adapter-location libusb:<bus>:<port-path>` を受け付け、selector がない場合は adapter open 前に失敗する。
- adapter open は `hci_power_control(HCI_POWER_ON)` の先にあるため、production runner は `--adapter-location` が設定済みであることを power-on 前に確認する。
- 実機承認は実装上の環境変数 gate ではなく、Bluetooth adapter open、Switch pairing、HID advertising、report loop、IPC input、cleanup confirmation の許可範囲を実行前に明示する運用 gate として扱う。
- `swbt/daemon/production_runner.*` は fake ports で検証できる composition 層にし、BTstack API の実呼び出しは `swbt/btstack_bridge/production_btstack_impl.*` に置く。
- BTstack HID packet handler は user context を持たないため、production backend は単一 active backend pointer を持つ。これは現行 scope の単一 controller 制約と一致する。
- event parser は上表の BTstack event layout に基づく最小 parse とし、BTstack inline helper へ依存しない。
- daemon link 用 BTstack source は broad source selection から platform helper を除き、選択 backend の transport と run loop だけを戻す。source selection の監査用一覧は `btstack_sources_cmake_test` で維持する。
- Windows filesystem 上の MinGW cross build では、BTstack の uppercase include が小文字の MinGW system header を解決できない。`windows-winusb` + MinGW の daemon link target だけに `cmake/mingw-compat-include/*` を追加し、`include_next` で system header へ渡す。`vendor/btstack` は編集しない。

## 5. 関連 work units

- `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`
- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`
- `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md`
- `work-units/complete/local_094/DOCS_PRODUCTION_CURRENT_STATE_CLEANUP.md`

## 6. 未解決事項

- Switch pairing、HID advertising が受け入れられること、periodic report loop の実測値、NyXpy macro artifact はこの reference では証明しない。
- libusb build は software gate であり、Windows native WinUSB runtime は `local_037` の実機記録で扱う。
- HID SDP の `hid_device_subclass` などが Switch 実機で最適値かどうかは未検証である。現時点では software composition と build/link gate の値として扱う。
