# Switch Player Lights Policy Source Audit

## 1. 状態

recorded。

この reference は `SET_PLAYER_LIGHTS` / `GET_PLAYER_LIGHTS` の policy core と dispatcher 連携で使う根拠監査である。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| dekuNukem Nintendo Switch reverse engineering notes | `ac8093c84194b3232acb675ac1accce9bcb456a3` | `bluetooth_hid_subcommands_notes.md` |
| Linux `hid-nintendo.c` | `66affa37cfac0aec061cc4bcf4a065b0c52f7e19` | `drivers/hid/hid-nintendo.c` |
| joycontrol | `18a09da1a04306534ff9e1df8a1a69c0192a3244` | `joycontrol/protocol.py`, `joycontrol/report.py` |
| swbt subcommand reply reference | current swbt implementation | `spec/references/switch-subcommand-reply-core.md` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| set player lights subcommand ID | `0x30` | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:234-244`; Linux `hid-nintendo.c:72`; joycontrol `report.py:220,228` | stable for implementation |
| set player lights payload size | first argument byte | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:234-244`; Linux `hid-nintendo.c:959-969` | stable for implementation |
| payload low nibble | keep player light on bits `0..3` | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:236-244`; Linux `hid-nintendo.c:959-969` | stable for implementation |
| payload high nibble | flash player light bits `0..3` | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:236-244`; Linux `hid-nintendo.c:959-969` | stable for implementation |
| on overrides flashing | on bit wins when both on and flash are set | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:244` | stable for software effective state |
| USB flash behavior | flash bits behave like on bits on USB | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:244` | recorded; swbt Bluetooth path does not implement USB-specific behavior |
| get player lights subcommand ID | `0x31` | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:247-249`; Linux `hid-nintendo.c:73` | stable for implementation |
| get player lights reply | ACK `0xB0`, subcommand `0x31`, one byte same bitfield | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:247-249` | stable for implementation |
| set player lights ACK | simple ACK `0x80`, subcommand `0x30` | implementation precedent | joycontrol `protocol.py:453-463`; `spec/references/switch-subcommand-reply-core.md` | stable for swbt policy |
| automatic player pattern table | eight patterns | implementation precedent | Linux `hid-nintendo.c:553-564,2255-2278` | reference only; swbt default does not assign a slot |

## 4. 実装判断

- swbt の default state は raw `0x00` とし、未割り当て player slot を暗黙に選ばない。
- `SET_PLAYER_LIGHTS` は 1 byte 以上の payload の先頭だけを読む。payload が空または `NULL` の場合は state を変更しない。
- state は requested raw byte、on mask、flash mask、effective flash mask を保持する。
- effective flash mask は `flash & ~on` とする。
- `GET_PLAYER_LIGHTS` は current raw byte を 1 byte reply data として返す。
- Linux の automatic player pattern table は slot assignment の参考に留め、この work unit では採用しない。

## 5. 未解決事項

- 実機 Switch が swbt の `0x30` ACK と `0x31` reply を受け入れることは未検証である。
- 実機 LED の点灯、点滅、USB-specific flash behavior は未検証である。
- 複数 controller の player slot assignment は未実装である。
