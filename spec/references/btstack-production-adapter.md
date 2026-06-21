# BTstack Production Adapter Source Audit

## 1. 状態

recorded。

この reference は `swbt_btstack_hid_registration_backend_t` を pinned BTstack の
Classic HID Device / SDP API へ接続する production adapter の根拠監査である。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| BTstack submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | `vendor/btstack` |
| BTstack HID Device header | same as submodule | `vendor/btstack/src/classic/hid_device.h` |
| BTstack SDP server header | same as submodule | `vendor/btstack/src/classic/sdp_server.h` |
| BTstack SDP util header | same as submodule | `vendor/btstack/src/classic/sdp_util.h` |
| BTstack HID keyboard example | same as submodule | `vendor/btstack/example/hid_keyboard_demo.c` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| HID SDP record type | `hid_sdp_record_t` | source fact | `vendor/btstack/src/classic/hid_device.h:59-74` | stable at pinned commit |
| HID SDP record creation | `hid_create_sdp_record(uint8_t *, uint32_t, const hid_sdp_record_t *)` | source fact | `vendor/btstack/src/classic/hid_device.h:76-84` | stable at pinned commit |
| HID Device init | `hid_device_init(bool, uint16_t, const uint8_t *)` | source fact | `vendor/btstack/src/classic/hid_device.h:86-92` | stable at pinned commit |
| HID packet handler registration | `hid_device_register_packet_handler(btstack_packet_handler_t)` | source fact | `vendor/btstack/src/classic/hid_device.h:94-98` | stable at pinned commit |
| SDP init | `sdp_init(void)` | source fact | `vendor/btstack/src/classic/sdp_server.h:72` | stable at pinned commit |
| SDP service registration | `sdp_register_service(const uint8_t *)` | source fact | `vendor/btstack/src/classic/sdp_server.h:81` | stable at pinned commit |
| SDP handle allocation | `sdp_create_service_record_handle(void)` | source fact | `vendor/btstack/src/classic/sdp_server.h:98` | stable at pinned commit |
| SDP record length | `de_get_len(const uint8_t *)` returns `uint32_t` | source fact | `vendor/btstack/src/classic/sdp_util.h:99`; `vendor/btstack/example/hid_keyboard_demo.c:492-494` | stable guard pattern |
| Adapter include set | `classic/hid_device.h`, `classic/sdp_server.h`, `classic/sdp_util.h` | source fact | headers above | implementation contract |
| BTstack source selection | adapter consumes selected include dirs; backend link libs are not propagated by this work unit | implementation fact | `cmake/btstack_sources.cmake`; `spec/references/btstack-source-selection.md` | adapter compile boundary |

## 4. 実装判断

- production adapter は `swbt/btstack_bridge/` に置き、public header へ BTstack header を含めない。
- adapter は `swbt_btstack_hid_registration_backend_t` の function table を返す。
- unit test は BTstack symbols を test stub で解決し、adapter が registration core から呼ばれることを確認する。
- `swbt_core` と adapter test は selected BTstack include dirs だけを参照する。backend link libs は daemon runtime integration で必要になった時点で扱う。
- この work unit では daemon main から adapter を呼ばない。実機 advertising、pairing、run loop は後続 work unit に残す。

## 5. 未解決事項

- 実 BTstack runtime 上での advertising、pairing、HID connection は、後続の `local_037` で CSR8510 A10、WinUSB、Switch2 firmware `22.1.0` の条件に限って観測済みである。別 adapter / firmware の互換性は未検証である。
- BTstack selected sources を実行ファイルへ常時 link する設計はこの reference では固定しない。
- Windows native WinUSB runtime 挙動は cross build だけでは証明しない。限定実機観測は `docs/hardware-test-log.md` を正本にする。
