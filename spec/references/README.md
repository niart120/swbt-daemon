# References

upstream 調査、外部資料、実測値の背景、根拠の要約を置く。

このディレクトリは規範ではない。
実装が従う判断は `spec/architecture/`、`spec/protocols/`、`spec/operations/` のいずれかに置き、ここから参照する。

## Entries

- `btstack-hid-device-registration.md`: BTstack Classic HID Device registration API と example order の根拠監査。
- `btstack-backend-build-matrix.md`: Phase 4 の libusb / windows-winusb build matrix と検証結果。
- `btstack-output-report-parser-bridge.md`: BTstack Classic HID report data callback と swbt output report parser 接続の根拠監査。
- `btstack-periodic-input-report-core.md`: periodic input report scheduler core の `0x30` report、period、BTstack send/timer 境界の根拠監査。
- `btstack-subcommand-reply-send-queue.md`: subcommand reply send queue core の queue policy と report size の根拠監査。
- `switch-hid-initial-source-audit.md`: `spec/initial/` の Switch HID / BTstack 初期設計値に対する documentation-level の根拠監査。
- `switch-subcommand-dispatcher-core.md`: output report parser、subcommand reply builder、virtual SPI、player lights state を接続する dispatcher core の根拠監査。
- `switch-subcommand-reply-core.md`: input report `0x21` subcommand reply layout と ACK/data offset の根拠監査。
- `switch-player-lights-policy.md`: `SET_PLAYER_LIGHTS` / `GET_PLAYER_LIGHTS` の policy core と dispatcher 連携の根拠監査。

## Current spec からの参照

- `spec/protocols/switch-hid-core.md` は Switch HID、subcommand、SPI、rumble、player lights、scheduler priority policy、未完成の production send path 境界の current spec として、このディレクトリの個別根拠監査を参照する。
- `spec/architecture/daemon-runtime-boundaries.md` は BTstack bridge と daemon runtime の current boundary として、BTstack 関連 reference と完了済み work unit record を参照する。
