# Switch Subcommand Core Source Audit

## 1. 状態

recorded。

この reference は Phase 2 の `switch_subcommand.*` 実装で使う output report parser と subcommand ID の根拠監査である。
対象は HIDP prefix を含まない output report `0x01`、`0x10`、主要 subcommand ID、SPI read request shape である。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| dekuNukem Nintendo Switch reverse engineering notes | `ac8093c84194b3232acb675ac1accce9bcb456a3` | `bluetooth_hid_notes.md`, `bluetooth_hid_subcommands_notes.md` |
| Linux `hid-nintendo.c` | `66affa37cfac0aec061cc4bcf4a065b0c52f7e19` | `drivers/hid/hid-nintendo.c` |
| joycontrol | `18a09da1a04306534ff9e1df8a1a69c0192a3244` | `joycontrol/report.py`, `joycontrol/protocol.py` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| output report: rumble + subcommand | `0x01` | source fact | dekuNukem `bluetooth_hid_notes.md:5-24`; Linux `hid-nintendo.c:50-53`; joycontrol `report.py:221-242` | stable for implementation |
| output report: rumble only | `0x10` | source fact | dekuNukem `bluetooth_hid_notes.md:15-24,31-33`; Linux `hid-nintendo.c:50-53`; joycontrol `report.py:231-234` | stable for implementation |
| output report `0x01` layout | id, packet counter, 8 rumble bytes, subcommand id, subcommand data | source fact | dekuNukem `bluetooth_hid_notes.md:9-19`; Linux `hid-nintendo.c:499-507` | stable for implementation; HIDP prefix excluded |
| output report `0x10` layout | id, packet counter, 8 rumble bytes | source fact | Linux `hid-nintendo.c:493-497`; dekuNukem `bluetooth_hid_notes.md:15-24,31-33` | stable for implementation; HIDP prefix excluded |
| subcommand IDs used by initial classifier | `0x00`, `0x01`, `0x02`, `0x03`, `0x04`, `0x05`, `0x06`, `0x07`, `0x08`, `0x10`, `0x11`, `0x20`, `0x21`, `0x22`, `0x30`, `0x31`, `0x38`, `0x40`, `0x41`, `0x42`, `0x43`, `0x48`, `0x50` | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:1-87,146-162`; Linux `hid-nintendo.c:57-80`; joycontrol `report.py:211-220` | stable for classification; not all are implemented by dispatcher |
| SPI flash read request shape | subcommand `0x10`; little-endian address + size | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:146-162`; joycontrol `report.py:285-304` | recorded for boundary; SPI payload validation remains separate |

## 4. 未解決事項

- `0x21` subcommand reply builder はこの work unit では扱わない。
- SPI flash read の address / size validation と payload は `switch_spi.*` の work unit で扱う。
- Rumble packet の semantic decode は `switch_rumble.*` の work unit で扱う。
- NFC/IR MCU output report `0x11` は未対応 report として分類し、実機 acceptability は未検証である。
