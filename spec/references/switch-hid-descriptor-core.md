# Switch HID Descriptor Core Source Audit

## 1. 状態

recorded。

この reference は Switch Pro Controller 相当の HID report descriptor bytes を
`swbt/switch/` に固定するための根拠監査である。

BTstack に渡す対象は HID report descriptor だけであり、USB device descriptor、
USB configuration descriptor、endpoint descriptor は実装対象に含めない。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| ToadKing pro controller descriptors gist | `b883a8ccfa26adcc6ba9905e75aeb4f2`, revision 1 | `pro.c` |
| DJm00n ControllersInfo | `bfc11b8f71df040388e4285d16bd6a7338a4e06a` | `switchpro/DescriptorDump_Pro_Controller (Nintendo Switch Pro Model HAC-013).txt` |
| dekuNukem Nintendo Switch reverse engineering notes | `ac8093c84194b3232acb675ac1accce9bcb456a3` | `bluetooth_hid_notes.md` |
| BTstack submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | `vendor/btstack/src/classic/hid_device.h` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| HID report descriptor bytes | ToadKing gist の 203 bytes | source fact | ToadKing gist `pro.c:134-225` | adopted byte-for-byte |
| HID report descriptor length | `203` bytes / `0x00CB` | source fact corroborated by real-device USB descriptor dump | ToadKing gist `pro.c:113-119`; DJm00n descriptor dump `HID Descriptor` | stable for implementation |
| report IDs | input `0x30`, `0x21`, `0x81`; output `0x01`, `0x10`, `0x80`, `0x82` | source fact from descriptor bytes; `0x01`, `0x10`, `0x21`, `0x30` are also described in protocol notes | ToadKing gist `pro.c:139-224`; dekuNukem `bluetooth_hid_notes.md` | implementation contract; unused IDs remain descriptor-only |
| BTstack descriptor ownership | caller supplies pointer and `uint16_t` size | source fact | BTstack `hid_device.h:59-92`; `spec/references/btstack-hid-device-registration.md` | existing bridge contract |
| Switch acceptability | accepted in limited bring-up | hardware observation | `docs/hardware-test-log.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | CSR8510 A10 / WinUSB / Switch2 firmware `22.1.0`; not generalized |

## 4. 実装判断

- `swbt/switch/switch_hid_descriptor.*` は ToadKing gist の HID report descriptor 203 bytes だけを持つ。
- USB device / configuration / endpoint descriptor は BTstack HID Device registration に渡さない。
- accessor は `const uint8_t *` と `size_t` だけを返す。呼び出し側がコピーする設計にはしない。
- unit test は descriptor の non-null、size、pointer stability、full byte fixture、主要 report ID marker を確認する。

## 5. 未解決事項

- `local_037` では CSR8510 A10、WinUSB、Switch2 firmware `22.1.0` の条件で、この descriptor を使う production HID session が pairing、HID L2CAP open、input / output report exchange まで進んだ。
- Bluetooth SDP parameters、class-of-device、pairing behavior はこの reference では固定しない。
- 実機で descriptor 調整が必要になった場合は、別 work unit で source audit と hardware log を更新する。
