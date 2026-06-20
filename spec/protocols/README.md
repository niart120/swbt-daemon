# Protocol Specs

Switch HID、daemon IPC、report format、subcommand、SPI、rumble など、現在有効な protocol と wire behavior の spec を置く。

protocol 値、packet layout、timing、BTstack source selection に触れる場合は、`source-audit` の結果を参照できるようにする。

## 現在の spec

- [Daemon IPC Wire Protocol V1](daemon-ipc-v1.md): local TCP JSON Lines、request / response、owner model、state snapshot、status、error response、neutral fail-safe の通信仕様。
- [Switch HID Core](switch-hid-core.md): `0x30` input report、`0x21` subcommand reply、output report parser、virtual SPI、rumble、player lights、scheduler priority の current contract。
