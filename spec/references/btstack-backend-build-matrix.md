# BTstack Backend Build Matrix

## 1. 状態

recorded。

この reference は Phase 4 の `libusb build` と `windows-winusb MinGW build` の検証結果を記録する。

## 2. 対象

| build path | preset / target | backend | result |
|---|---|---|---|
| Linux debug build/test | `make debug` / `linux-debug` | `libusb` | pass, CTest 13/13 |
| Windows MinGW cross build | `make windows-cross` / `windows-mingw-debug` | `windows-winusb` | pass, `swbt-daemon.exe` linked |
| Full non-hardware verification | `make verify` | `libusb`, `windows-winusb` | pass |

## 3. 根拠

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| Linux preset backend | `SWBT_BACKEND=libusb` | implementation fact | `CMakePresets.json` `linux-debug`; `make debug` configure output | verified |
| Windows preset backend | `SWBT_BACKEND=windows-winusb` | implementation fact | `CMakePresets.json` `windows-mingw-debug`; `make windows-cross` configure output | verified |
| libusb selected sources | 182 selected sources | implementation fact | `make debug` configure output; `tests/cmake/btstack_sources_test.cmake` | verified |
| windows-winusb selected sources | 173 selected sources | implementation fact | `make windows-cross` configure output; `tests/cmake/btstack_sources_test.cmake` | verified |
| source selection audit | `btstack-source-selection.md` | source fact and implementation fact | `spec/references/btstack-source-selection.md` | recorded |

## 4. 未解決事項

- この reference 自体は build success を記録する。後続の `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `docs/hardware-test-log.md` では、CSR8510 A10、WinUSB、Switch2 firmware `22.1.0` の条件で Windows native execution、WinUSB driver assignment、Bluetooth dongle recognition、Switch pairing、HID L2CAP open、report loop を観測した。
- build matrix は実機 report loop、latency、jitter、別 adapter / firmware の互換性を証明しない。
