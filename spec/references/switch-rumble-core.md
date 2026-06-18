# Switch Rumble Core Source Audit

## 1. 状態

recorded。

この reference は Phase 2 の `switch_rumble.*` 実装で使う rumble payload size と neutral payload の根拠監査である。
対象は output report 内の 8-byte rumble payload、neutral sample、raw state 分離である。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| dekuNukem Nintendo Switch reverse engineering notes | `ac8093c84194b3232acb675ac1accce9bcb456a3` | `bluetooth_hid_notes.md` |
| Linux `hid-nintendo.c` | `66affa37cfac0aec061cc4bcf4a065b0c52f7e19` | `drivers/hid/hid-nintendo.c` |
| joycontrol | `18a09da1a04306534ff9e1df8a1a69c0192a3244` | `joycontrol/report.py` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| rumble payload size | `8` bytes | source fact | Linux `hid-nintendo.c:493-507`; joycontrol `report.py:266-267` | stable for implementation |
| neutral rumble payload | `00 01 40 40 00 01 40 40` | source fact | dekuNukem `bluetooth_hid_notes.md:43-47` | stable for neutral detection |
| rumble payload position in output report | after output report id and packet counter | source fact | dekuNukem `bluetooth_hid_notes.md:9-19`; Linux `hid-nintendo.c:493-507` | already parsed by `switch_subcommand.*` |
| raw rumble state is separate from controller input state | design fact | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md:815-825` | stable design boundary |

## 4. 未解決事項

- Rumble frequency/amplitude semantic decode はこの work unit では扱わない。
- Actuator-safe amplitude conversion は未実装である。
- 実機での rumble effect は未検証である。
