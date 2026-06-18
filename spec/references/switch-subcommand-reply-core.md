# Switch Subcommand Reply Core Source Audit

## 1. 状態

recorded。

この reference は Phase 4 の input report `0x21` subcommand reply builder で使う report layout と ACK/data offset の根拠監査である。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| dekuNukem Nintendo Switch reverse engineering notes | `ac8093c84194b3232acb675ac1accce9bcb456a3` | `bluetooth_hid_notes.md`, `bluetooth_hid_subcommands_notes.md` |
| Linux `hid-nintendo.c` | `66affa37cfac0aec061cc4bcf4a065b0c52f7e19` | `drivers/hid/hid-nintendo.c` |
| joycontrol | `18a09da1a04306534ff9e1df8a1a69c0192a3244` | `joycontrol/report.py` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| subcommand reply input report ID | `0x21` | source fact | dekuNukem `bluetooth_hid_notes.md:108-110`; Linux `hid-nintendo.c:84`; joycontrol `report.py:38-44` | stable for implementation |
| standard input report prefix | report ID, timer, battery/connection, buttons, left/right sticks, vibrator input | source fact | dekuNukem `bluetooth_hid_notes.md:139-148`; Linux `hid-nintendo.c:526-534` | stable for implementation |
| ACK byte offset | byte `13`, MSB indicates ACK/NACK | source fact | dekuNukem `bluetooth_hid_notes.md:149,156`; Linux `hid-nintendo.c:511-514` | stable for implementation |
| reply-to subcommand ID offset | byte `14` | source fact | dekuNukem `bluetooth_hid_notes.md:150`; Linux `hid-nintendo.c:511-514` | stable for implementation |
| reply data offset and max size | bytes `15..49`, max `35` bytes | source fact | dekuNukem `bluetooth_hid_notes.md:151`; Linux `hid-nintendo.c:511-514`, `hid-nintendo.c:542` | stable for implementation |
| SPI read ACK/data shape | ACK `0x90`, subcommand `0x10`, echoed address/size then read data | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:146-162`; Linux `hid-nintendo.c:991-1017`; joycontrol `report.py:161-175` | caller-provided data payload in this work unit |

## 4. 未解決事項

- 実機 Switch で要求される exact ACK subset は未検証である。
- Subcommand dispatcher と BTstack send path はこの work unit では扱わない。
