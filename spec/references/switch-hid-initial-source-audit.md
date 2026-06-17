# Switch HID Initial Source Audit

## 1. 状態

draft。

この reference は、`spec/initial/` に置いた初期設計 docs が参照する
Switch HID report、subcommand、report period、BTstack port/run loop の根拠を
作業時点で確認した記録である。

これは規範ではない。
実装時には、対象値ごとに関連する protocol spec または work unit record へ
必要な根拠を再掲する。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| dekuNukem Nintendo Switch reverse engineering notes | `ac8093c84194b3232acb675ac1accce9bcb456a3` | `bluetooth_hid_notes.md`, `bluetooth_hid_subcommands_notes.md` |
| Linux `hid-nintendo.c` | `66affa37cfac0aec061cc4bcf4a065b0c52f7e19` | `drivers/hid/hid-nintendo.c` |
| joycontrol | `18a09da1a04306534ff9e1df8a1a69c0192a3244` | `joycontrol/protocol.py` |
| BTstack submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | `vendor/btstack` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| output report: rumble + subcommand | `0x01` | source fact | dekuNukem `bluetooth_hid_notes.md:5-24`; Linux `hid-nintendo.c:50-53` | stable for docs |
| output report: rumble only | `0x10` | source fact | dekuNukem `bluetooth_hid_notes.md:15,23,31-33`; Linux `hid-nintendo.c:50-53` | stable for docs |
| input report: subcommand reply | `0x21` | source fact | dekuNukem `bluetooth_hid_notes.md:108-110,139-147`; Linux `hid-nintendo.c:82-85` | stable for docs |
| input report: standard full mode | `0x30` | source fact | dekuNukem `bluetooth_hid_notes.md:116-118,139-147`; Linux `hid-nintendo.c:82-85` | stable for docs |
| standard full mode cadence | `60Hz`, `120Hz if Pro Controller` | source fact from reverse engineering notes | dekuNukem `bluetooth_hid_notes.md:116-118`; dekuNukem `bluetooth_hid_subcommands_notes.md:66-83` | configurable, not hardware-observed by swbt |
| Pro Controller Bluetooth input report period | `8ms` comment; observed `11ms` or `15ms` in one Linux-driver context | source fact from Linux driver comment | Linux `hid-nintendo.c:1384-1410` | configurable, not hardware-observed by swbt |
| initial swbt default report period | `8000us` | inference from `8ms` Linux comment and `120Hz` reverse engineering note | Linux `hid-nintendo.c:1384-1391`; dekuNukem `bluetooth_hid_notes.md:116-118`; `CMakeLists.txt` cache variable | configurable; not a stable hardware fact |
| fallback / example send delay | `0.015s` | implementation fact from joycontrol | joycontrol `protocol.py:124-135,174-190` | example only |
| subcommand IDs used in initial docs | `0x00`, `0x01`, `0x02`, `0x03`, `0x04`, `0x10`, `0x30`, `0x40`, `0x48`, `0x50` | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:7-87,146-162,234-244,306-309,379-385`; Linux `hid-nintendo.c:57-80` | stable for docs; implementation still needs value-specific tests |
| SPI flash read request shape | address + size, reply on `INPUT 0x21` | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:146-162`; joycontrol `protocol.py:312-339` | stable for docs; payload data not audited |
| rumble neutral sample | `00 01 40 40 00 01 40 40` | source fact from reverse engineering notes | dekuNukem `bluetooth_hid_notes.md:43-64` | documentation only; not implemented |
| BTstack libusb port | Unix-based system with dedicated USB Bluetooth dongle | source fact | `vendor/btstack/README.md:78-84` | source-selection candidate |
| BTstack windows-winusb port | Win32-based system with dedicated USB Bluetooth dongle | source fact | `vendor/btstack/README.md:94-97` | source-selection candidate |
| BTstack run loop timers | data sources + timers; timers can handle periodic events | source fact | `vendor/btstack/doc/manual/docs-template/how_to.md:398-419` | design support only |
| BTstack main-thread callback | `btstack_run_loop_execute_code_on_main_thread` may schedule callbacks on multi-threaded environments | source fact | `vendor/btstack/doc/manual/docs-template/how_to.md:440-458` | design support only; port-specific availability still needs implementation audit |

## 4. 未解決事項

- この監査は initial docs の根拠整理であり、protocol implementation の完了判定ではない。
- HID descriptor bytes、SPI payload contents、subcommand response payload、rumble encoding の実装用根拠は未監査である。
- `report_period_us = 8000` は実機観測ではない。実機での既定値決定には `docs/hardware-test-log.md` への記録が必要である。
- BTstack source file list はまだ CMake に取り込んでいない。取り込む work unit では、BTstack source selection を別途監査する。
- Windows WinUSB / Linux libusb の実機挙動は未検証である。
