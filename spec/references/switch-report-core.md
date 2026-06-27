# Switch Report Core Source Audit

## 1. 状態

recorded。

この reference は Phase 2 の `switch_report.*` 実装で使う standard full input report の根拠監査である。
対象は input report `0x30`、button status 3 bytes、12-bit stick packing、6-Axis payload の基本 layout である。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| dekuNukem Nintendo Switch reverse engineering notes | `ac8093c84194b3232acb675ac1accce9bcb456a3` | `bluetooth_hid_notes.md` |
| Linux `hid-nintendo.c` | `66affa37cfac0aec061cc4bcf4a065b0c52f7e19` | `drivers/hid/hid-nintendo.c` |
| joycontrol | `18a09da1a04306534ff9e1df8a1a69c0192a3244` | `joycontrol/report.py`, `joycontrol/controller_state.py` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| standard full input report ID | `0x30` | source fact | dekuNukem `bluetooth_hid_notes.md:111-119`; Linux `hid-nintendo.c:80-84` | stable for implementation |
| standard input report byte layout | report ID, timer, battery/connection, 3 button bytes, left stick, right stick, vibrator, payload | source fact | dekuNukem `bluetooth_hid_notes.md:123-148`; Linux `hid-nintendo.c:455-470` | stable for implementation |
| standard full report size | `49` bytes, report ID included | inference from dekuNukem `0..48` byte table and Linux packed input report fields | dekuNukem `bluetooth_hid_notes.md:123-148`; Linux `hid-nintendo.c:455-470` | implementation contract; no HIDP prefix |
| button status bytes | right/shared/left bytes at indexes `3`, `4`, `5` | source fact | dekuNukem `bluetooth_hid_notes.md:151-159`; Linux `hid-nintendo.c:340-430` | stable for implementation |
| stick packing | 3 bytes per stick; `x = b0 | ((b1 & 0x0f) << 8)`, `y = (b1 >> 4) | (b2 << 4)` | source fact | dekuNukem `bluetooth_hid_notes.md:163-170` | stable for implementation |
| 6-Axis payload | 3 frames; each frame is accel x/y/z followed by gyro x/y/z as `int16le` | source fact | dekuNukem `bluetooth_hid_notes.md:143-148`; Linux `hid-nintendo.c:455-470` | stable for implementation |
| joycontrol misc/status bytes | `0x8e` battery/connection, `0x80` vibrator in one software implementation | implementation fact | joycontrol `report.py:37-88` | production default source; swbt builder takes caller-provided values |
| daemon default report options | `0x8e` battery/connection, `0x80` vibrator | implementation policy based on joycontrol implementation fact; hardware observation in later bring-up | joycontrol `report.py:37-88`; `swbt/daemon/config.c`; `tests/daemon_process_test.c`; `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | production daemon default; observed on CSR8510 A10 / Switch2 22.1.0 |

## 4. 未解決事項

- HID descriptor bytes はこの work unit では扱わない。
- Stick calibration と sensor calibration はこの work unit では扱わない。`swbt_state_t` の値を 12-bit stick raw value として pack する。
- report builder は `battery_connection` と `vibrator_report` の既定値を持たない。builder caller が明示的に渡す。
- production daemon default は、joycontrol の `set_misc()` / `set_vibrator_input()` と同じ battery/connection `0x8e`、vibrator `0x80` を使う。`local_037` では CSR8510 A10、WinUSB、Switch2 firmware `22.1.0` の条件で、この default を含む `0x30` input report loop と Button A / neutral report の受理を観測した。
- 別 adapter / firmware での acceptability、report jitter、入力遅延、取りこぼし率は未検証である。
