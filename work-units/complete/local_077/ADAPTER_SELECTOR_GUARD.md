# USB Location Adapter Selector Guard

## 1. 概要

production 既定化後に、BTstack USB transport がどの Bluetooth アダプターを開くかを swbt 側で明示できない問題を扱う work unit。

この work unit では、VID/PID ではなく USB topology 上の location を selector にする。Windows WinUSB と Linux libusb の両方で「この USB 経路に刺さっているデバイスだけを開く」という同じ運用モデルへ揃える。

採用する CLI surface の第一候補は次である。

```console
swbt-daemon --adapter-location "winusb:<windows-location-path>"
swbt-daemon --adapter-location "libusb:<bus>:<port-path>"
```

`--adapter-location` がない production 起動は、対象アダプターが不明な状態として adapter open 前に失敗させる。Bluetooth アダプターを開かない smoke / test では、従来通り `--backend noop` を明示する。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`: production 既定化後も、BTstack USB transport がどの adapter を開くかは swbt 側で明示 selector を持たない、という先送り事項。
- `spec/operations/windows-native-preflight.md`: 実機実行では専用 USB Bluetooth ドングルと WinUSB driver assignment を記録し、内蔵 Bluetooth と常用ドングルを使わない。
- `docs/hardware-test-log.md`: CSR8510 A10 + WinUSB を既知の実機構成として記録している。
- user discussion, 2026-06-27: VID/PID は同じ型番の複数ドングルを区別できないため selector として弱い。Windows の長い device interface path や短い fingerprint は運用が重い。Linux の bus + port path と同じ発想で、Windows も USB location path に寄せる方針を採る。

use case:

- actor: hardware operator、maintainer。
- 入力または状態: Windows native production run、Linux production run、同じ VID/PID の Bluetooth USB dongle が複数存在し得る host、専用 USB port に挿したドングル。
- 期待する観測結果:
  - production run は `--adapter-location` で指定した USB 経路の adapter だけを開く。
  - selector が未指定、不正、または一致なしの場合は、BTstack の adapter open 前に失敗する。
  - VID/PID は診断表示や preflight 記録の補助情報として残すが、selector の主キーにはしない。
  - ドングルを別ポートへ移した場合、operator が preflight で location を確認し、CLI 指定を更新できる。
- 制約:
  - Bluetooth adapter open、Switch pairing、HID advertising、report loop は人間の明示承認なしに実行しない。
  - `vendor/btstack` の直接編集はしない。Windows WinUSB の location match に必要な最小 patch は build directory へ生成する source に閉じ込め、この work unit の根拠監査つき対象に含める。

source から use case への変換:

VID/PID は「対象チップ系列の許可」には使えるが、同一 VID/PID が複数ある host で一意性を持たない。device interface path は Windows WinUSB の open 対象に直結するが、人間が扱う selector として読みにくい。USB location は個体 identity ではなく port identity だが、専用ドングル運用では「この USB 経路だけを使う」という事故防止に合う。Linux 側も既存 BTstack API が bus + port path を持つため、Windows / Linux を同じ運用モデルへ寄せられる。

## 3. 対象範囲

- `--adapter-location` の CLI parser と launch config 接続。
- production backend が adapter location 未指定で adapter open へ進まない guard。
- `libusb:<bus>:<port-path>` parser と `hci_transport_usb_set_bus_and_path(...)` への接続。
- `winusb:<windows-location-path>` parser と Windows WinUSB transport の location match 接続。
- Windows WinUSB transport の生成済み source で、列挙した USB device interface の PnP location path を取得し、一致した interface だけを open 対象にする最小変更。
- selector mismatch、missing selector、unsupported backend prefix、malformed selector の software test。
- Windows native preflight / docs の更新。preflight では adapter VID/PID、driver state、location path、実行コマンドを記録する。
- Linux 実機経路の docs 更新。preflight では bus、port path、VID/PID、driver / permission 状態を記録する。

## 4. 対象外

- VID/PID を selector の主キーにする実装。
- device interface path を人間向け CLI の主キーにする実装。内部 fallback や診断記録としては使ってよい。
- fingerprint 生成、短縮 ID、adapter alias、config alias。
- `swbt-daemon adapters list` などの管理 subcommand。必要なら `local_078` 以降で扱う。
- USB serial number による個体 identity selector。
- Bluetooth adapter driver の自動切替。
- Zadig / WinUSB の設定変更自動化。
- macOS の adapter selector 実装。
- 複数 controller 同時接続。
- Switch protocol、HID report、pairing sequence の変更。

## 5. 関連 spec / docs

- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `docs/upstream-btstack.md`

## 6. 根拠監査

### 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---|---|---|---|
| libusb selector API | `hci_transport_usb_set_bus_and_path(uint8_t bus, int len, uint8_t *port_numbers)` が公開されている | source fact | `vendor/btstack/src/hci_transport_usb.h:62-67`, `vendor/btstack/platform/libusb/hci_transport_h2_libusb.c:417-429` | current pinned BTstack |
| libusb location match | `usb_path_len` がある場合、libusb device の bus number と port numbers を照合してから open する | source fact | `vendor/btstack/platform/libusb/hci_transport_h2_libusb.c:1089-1119` | current pinned BTstack |
| WinUSB current selection | WinUSB transport は USB device interface を列挙し、`DevicePath` を取得した後、現状では全候補に `usb_try_open_device(DevicePath)` を試す | source fact | `vendor/btstack/platform/windows/hci_transport_h2_winusb.c:1117-1230` | current pinned BTstack; selector なし |
| WinUSB VID/PID hard-code | WinUSB transport には無効化された VID/PID filter 例があるが、current implementation では使われていない | source fact | `vendor/btstack/platform/windows/hci_transport_h2_winusb.c:1204-1221` | current pinned BTstack |
| Windows location path | `DEVPKEY_Device_LocationPaths` は device tree 上の device instance location を表す string list で、`SetupDiGetDeviceProperty` などで取得できる | source fact | Microsoft Learn `DEVPKEY_Device_LocationPaths` <https://learn.microsoft.com/en-us/windows-hardware/drivers/install/devpkey-device-locationpaths>, `SetupDiGetDevicePropertyW` <https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdevicepropertyw> | Windows Vista 以降 |
| WinUSB implemented selector | `hci_transport_usb_set_location_path(const char *)` で location path を保持し、`usb_open()` の列挙中に `SPDRP_LOCATION_PATHS` と完全一致した候補だけを `usb_try_open_device(...)` へ渡す generated source を使う | implementation fact / source audit / hardware observation | `cmake/btstack_winusb_location_patch.cmake`, `CMakeLists.txt`, `vendor/btstack/platform/windows/hci_transport_h2_winusb.c`, `docs/upstream-btstack.md`, `docs/hardware-test-log.md` | implemented; Windows native CSR8510 A10 runtime probe passed |
| selector semantics | USB location selector は個体 identity ではなく USB port / hub 経路 identity である | inference | libusb bus/path API、Windows location path property の意味 | 専用ポート運用として採用 |

### 未解決事項

- Linux libusb の selector が実機で専用 USB Bluetooth dongle だけを開くことは hardware-gated item として残る。
- Windows WinUSB の selector は CSR8510 A10 で adapter open と HCI power on まで確認したが、Switch pairing、Switch connection、IPC input、report loop はこの work unit では確認していない。

## 7. 設計メモ

- selector 名は `--adapter-location` を第一候補にする。`--adapter-id` は抽象的すぎるため、この work unit では採用しない。
- Windows selector は `winusb:<windows-location-path>` とする。例: `winusb:PCIROOT(0)#PCI(... )#USBROOT(0)#USB(3)`。`DEVPKEY_Device_LocationPaths` は string list なので、WinUSB transport は列挙した location path のいずれかが selector と完全一致した interface だけを open 対象にする。
- Linux selector は `libusb:<bus>:<port-path>` とする。例: `libusb:1:3.2`。
- `winusb:` prefix は Windows WinUSB backend のみ、`libusb:` prefix は libusb backend のみで受け付ける。現在の build backend と一致しない prefix は adapter open 前に失敗させる。
- `--adapter-location` は production backend 用の guard である。`--backend noop` では指定不要にする。
- location selector は「この USB 経路にいる device」を指定する。ドングルを別ポートへ移すと selector が変わり得る。これは専用ポート運用では受け入れる。
- 同一 location に複数 device が一致する状態は異常として失敗させる。ただし通常の USB device interface 列挙では、同一対象の複数 interface が出る可能性を考慮し、WinUSB 側の matching unit test で扱う。
- VID/PID は selector にしない。preflight と error message で「operator が見て判断する補助情報」として出す。
- Windows 実装は current BTstack public API だけでは完結しない。`hci_transport_usb_set_bus_and_path` 相当の WinUSB location setter を、build 時に生成する WinUSB transport source へ追加する。
- Linux 実装は current BTstack public API で完結するため、`vendor/btstack/platform/libusb/hci_transport_h2_libusb.c` の変更は原則不要である。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `swbt/daemon/launch_options.*`
- `swbt/btstack_bridge/production_btstack.*`
- `swbt/btstack_bridge/usb_adapter_location.*`
- `cmake/btstack_winusb_location_patch.cmake`
- `docs/upstream-btstack.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `docs/status.md`
- `tests/daemon_launch_options_test.c`
- `tests/daemon_production_backend_test.c`
- `tests/btstack_*`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | CLI parser accepts `--adapter-location winusb:<location>` and `--adapter-location libusb:<bus>:<path>` | new | unit | no |
| done | malformed adapter location is rejected before launch config is prepared | edge | unit | no |
| done | production backend fails before BTstack platform start when adapter location is missing | edge | integration | no |
| done | noop backend does not require adapter location | regression | integration | no |
| done | libusb adapter location config parses bus and port numbers for `hci_transport_usb_set_bus_and_path` before USB transport init | new | bridge unit / cross build | no |
| done | WinUSB adapter location config installs a location-path selector and skips non-matching USB interfaces before `usb_try_open_device(...)` | new | bridge / transport | no |
| done | WinUSB adapter location missing guard fails without calling HCI power on or run loop execute | edge | integration | no |
| done | unsupported backend prefix for the current build fails before adapter open | edge | unit / integration | no |
| done | Windows preflight records location path, VID/PID, driver state, and selected command | regression | docs | no |
| done | Linux status docs record bus, port path, VID/PID, permission state, and selected command expectation | regression | docs | no |
| done | selected location policy is checked against a dedicated USB Bluetooth dongle on Windows WinUSB | behavior | hardware | yes |
| hardware-gated | selected location policy is checked against a dedicated USB Bluetooth dongle on Linux libusb | behavior | hardware | yes |

## 10. 検証

TDD status:
- source: user discussion, 2026-06-27。Windows / Linux の selector を USB location approach へ揃える。
- use case: 同じ VID/PID の adapter が複数あり得る host で、production daemon が指定された USB 経路以外を開かない。
- item: USB location adapter selector guard。
- state: green。
- commands:
  - `git status --short --branch`
  - `Get-Content` / `rg` による BTstack WinUSB / libusb source audit。
  - red: `just build-debug`。期待通り `btstack_bridge/usb_adapter_location.h`、`adapter_location` field、production backend guard API が未実装で失敗。
  - green: `just build-debug` pass。
  - `just test-debug` pass。43/43 tests passed。
  - `just windows-cross` pass。Windows WinUSB backend の generated source compile / link を確認。
  - `scripts/format.sh` pass。
  - `scripts/check-format.sh` pass。
  - `just verify` pass。format-check、clang-tidy、fresh debug test、ASan、Windows MinGW cross build を確認。
  - `git diff --check` pass。Windows checkout 由来の LF to CRLF warning のみ。
  - hardware: `tmp/hardware/local_077/run-adapter-location-open.ps1` pass。Windows WinUSB CSR8510 A10 を `--adapter-location winusb:PCIROOT(0)#PCI(0201)#PCI(0000)#PCI(0C00)#PCI(0000)#USBROOT(0)#USB(9)#USB(1)` で短時間起動し、`btstack: hci power on ok`、`usb_open: done, r = 0`、cleanup exit code `0` を確認。
- notes: Windows hardware probe は Bluetooth adapter open と短時間の HID advertising 可能性を含む。Switch 操作、Switch pairing、IPC input、report loop は実行していない。HCI dump では non-matching USB device interface `16` 件が location selector で除外され、CSR8510 A10 だけが open 対象になった。

## 11. 実機実行条件

この work unit では、software 実装と単体テストに加えて Windows WinUSB の短時間 adapter open probe を実行した。Linux libusb と Switch-facing 疎通は未実行である。

実機実行前条件:

- `hardware-harness` を読む。
- 人間の明示承認を得る。
- 承認範囲を Bluetooth adapter open、Switch pairing、HID advertising、report loop、IPC input、cleanup confirmation のどこまで含むか明示する。
- 専用 USB Bluetooth dongle を使い、内蔵 Bluetooth と常用ドングルを対象外にする。
- Windows では WinUSB driver assignment、location path、VID/PID、adapter identity を記録する。
- Linux では libusb permission、bus、port path、VID/PID、adapter identity を記録する。
- `docs/hardware-test-log.md` へ OS、dongle VID/PID、driver / permission、BTstack commit、swbt commit、Switch firmware、実行コマンド、結果、cleanup を記録する。

## 12. 先送り事項

- adapter list / management subcommand:
  - 理由: local_077 は production run を fail-closed にする selector guard が目的であり、CLI 管理面を広げると scope が増える。
  - 次の置き場: `work-units/complete/local_078/DAEMON_CLI_MANAGEMENT_SURFACE.md`。
- config alias / short adapter name:
  - 理由: まず location selector を実装し、preflight で operator が確認して渡せることを固定する。alias は selector の永続運用が必要になってから扱う。
  - 次の置き場: `spec/dev-journal.md` または後続 work unit。
- USB serial number selector:
  - 理由: 安価な Bluetooth dongle では serial がない、または運用上信用しづらい可能性がある。local_077 の目的は専用 USB 経路へ fail-closed にすることで足りる。
  - 次の置き場: 追加の実機観測または adapter inventory が必要になった時点の work unit。

## 13. チェックリスト

- [x] source を `local_074` の先送り事項と 2026-06-27 の設計議論から特定した。
- [x] use case を Windows / Linux 共通の USB location selector guard として再定義した。
- [x] VID/PID selector、device path CLI、fingerprint、alias を対象外に整理した。
- [x] BTstack / WinUSB / libusb の adapter selection 境界を初期調査した。
- [x] TDD Test List を USB location selector に合わせて再作成した。
- [x] `--adapter-location` parser を実装した。
- [x] production missing selector guard を実装した。
- [x] libusb selector 接続を実装した。
- [x] WinUSB location selector 接続を実装した。
- [x] docs / preflight を更新した。
- [x] software verification と Windows WinUSB hardware observation を記録した。
