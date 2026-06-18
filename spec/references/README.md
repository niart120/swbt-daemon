# References

upstream 調査、外部資料、実測値の背景、根拠の要約を置く。

このディレクトリは規範ではない。
実装が従う判断は `spec/architecture/`、`spec/protocols/`、`spec/operations/` のいずれかに置き、ここから参照する。

## Entries

- `btstack-hid-device-registration.md`: BTstack Classic HID Device registration API と example order の根拠監査。
- `btstack-output-report-parser-bridge.md`: BTstack Classic HID report data callback と swbt output report parser 接続の根拠監査。
- `btstack-periodic-input-report-core.md`: periodic input report scheduler core の `0x30` report、period、BTstack send/timer 境界の根拠監査。
- `switch-hid-initial-source-audit.md`: `spec/initial/` の Switch HID / BTstack 初期設計値に対する documentation-level の根拠監査。
- `switch-subcommand-reply-core.md`: input report `0x21` subcommand reply layout と ACK/data offset の根拠監査。
