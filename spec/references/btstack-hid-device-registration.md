# BTstack HID Device Registration Source Audit

## 1. 状態

recorded。

この reference は Phase 4 の HID Device registration 境界で使う BTstack Classic HID Device API の根拠監査である。
Switch Pro Controller 固有の HID descriptor bytes は扱わない。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| BTstack submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | `vendor/btstack` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| HID SDP record params | `hid_sdp_record_t` fields include subclass, country code, virtual cable, remote wake, reconnect initiate, normally connectable, boot device, SSR values, supervision timeout, descriptor pointer/size, device name | source fact | `vendor/btstack/src/classic/hid_device.h:59-74` | stable BTstack API at pinned commit |
| HID SDP record creation | `hid_create_sdp_record(service, handle, params)` | source fact | `vendor/btstack/src/classic/hid_device.h:76-84` | stable BTstack API at pinned commit |
| HID Device init | `hid_device_init(boot_protocol_mode_supported, hid_descriptor_len, hid_descriptor)` | source fact | `vendor/btstack/src/classic/hid_device.h:86-92` | stable BTstack API at pinned commit |
| HID packet handler registration | `hid_device_register_packet_handler(callback)` | source fact | `vendor/btstack/src/classic/hid_device.h:94-98` | stable BTstack API at pinned commit |
| HID Device example order | `sdp_init`, create/register HID SDP record, `hid_device_init`, packet handler registration | source fact | `vendor/btstack/example/hid_keyboard_demo.c:470-510` | example-driven registration sequence |
| SDP record length guard | Example checks `de_get_len(hid_service_buffer) <= sizeof(hid_service_buffer)` after record creation | source fact | `vendor/btstack/example/hid_keyboard_demo.c:492-494` | guard pattern only; caller must still provide a sufficiently large buffer |

## 4. 未解決事項

- Switch Pro Controller HID descriptor bytes は未監査であり、この work unit では実装しない。
- HID class-of-device / subclass の Switch 実機 acceptability は未検証である。
- BTstack の実機 advertising、pairing、connection callback はこの reference では扱わない。
