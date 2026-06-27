# BTstack upstream

`swbt-daemon` は BTstack をサードパーティ依存として利用する。

配置先は次の通り。

```text
vendor/btstack
```

## 現在の基準

このプロジェクトは現在、Git submodule として BTstack `v1.8.2` に固定している。

```text
upstream repository: https://github.com/bluekitchen/btstack
submodule path: vendor/btstack
base tag: v1.8.2
base commit: 075a0780f0fad7ff67d58ac19f46e8953656a752
```

submodule を更新した場合は、正確な submodule commit をここに記録する。

## 更新方針

- 可能な限り Switch 固有の挙動は BTstack の外に置く。
- integration code は `swbt/btstack_bridge/` を優先する。
- BTstack HID validation を全体で無効化しない。
- BTstack patch または fork が必要になった場合は、このファイルに記録する。

## local generated patches

### WinUSB USB location selector source generation

`local_077` で、Windows WinUSB transport に USB location path selector を追加するため、build directory に patch 済み source を生成する処理を追加した。`vendor/btstack` submodule 自体は変更しない。

対象:

- input: `vendor/btstack/platform/windows/hci_transport_h2_winusb.c`
- generator: `cmake/btstack_winusb_location_patch.cmake`
- generated source: `${CMAKE_BINARY_DIR}/generated/btstack/platform/windows/hci_transport_h2_winusb_location.c`

理由:

- Linux libusb transport には `hci_transport_usb_set_bus_and_path(uint8_t bus, int len, uint8_t *port_numbers)` があり、USB bus + port path で対象アダプターを絞れる。
- Windows WinUSB transport には同等の selector API がなく、current pinned source は USB device interface を列挙した後に全候補へ `usb_try_open_device(DevicePath)` を試す。
- swbt の production 実機経路では、同じ VID/PID の Bluetooth USB dongle が複数ある host でも、専用 USB 経路のアダプターだけを開く必要がある。

変更内容:

- `hci_transport_usb_set_location_path(const char *location_path)` を追加した。
- WinUSB `usb_open()` の device interface 列挙で `SPDRP_LOCATION_PATHS` を読み、指定された location path と一致した候補だけを `usb_try_open_device(...)` へ渡す。
- location path 未指定時の既存列挙挙動は維持する。
- `CMakeLists.txt` の `windows-winusb` backend source selection では、pinned submodule source ではなく生成済み source を使う。

制約:

- これは pinned BTstack source から生成する最小 patch であり、Bluetooth protocol、HID report、libusb backend source は変更しない。
- `SPDRP_LOCATION_PATHS` の文字列表現と operator が preflight で取得する値の一致は、Windows native read-only preflight と実機実行時の記録で確認する。
