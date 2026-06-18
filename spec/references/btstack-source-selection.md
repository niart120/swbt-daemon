# BTstack Source Selection Audit

## 1. 状態

draft。

この reference は Phase 1: BTstack source selection の根拠監査である。
対象は親リポジトリが固定している BTstack submodule `v1.8.2` / `075a0780f0fad7ff67d58ac19f46e8953656a752`。

これは規範ではない。
実装時の選択結果は `cmake/btstack_sources.cmake` と work unit record に記録する。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| BTstack submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | `vendor/btstack` |
| BTstack libusb port CMake | same as submodule | `vendor/btstack/port/libusb/CMakeLists.txt` |
| BTstack windows-winusb port CMake | same as submodule | `vendor/btstack/port/windows-winusb/CMakeLists.txt` |
| BTstack libusb port README | same as submodule | `vendor/btstack/port/libusb/README.md` |
| BTstack windows-winusb port README | same as submodule | `vendor/btstack/port/windows-winusb/README.md` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| Linux backend candidate | `libusb` | source fact | `vendor/btstack/port/libusb/README.md:1-8`, `vendor/btstack/port/libusb/CMakeLists.txt:63-65`, `vendor/btstack/port/libusb/CMakeLists.txt:144-151` | stable source-selection candidate |
| Windows backend candidate | `windows-winusb` | source fact | `vendor/btstack/port/windows-winusb/README.md:1-5`, `vendor/btstack/port/windows-winusb/README.md:7-19`, `vendor/btstack/port/windows-winusb/CMakeLists.txt:52-55`, `vendor/btstack/port/windows-winusb/CMakeLists.txt:120-121` | stable source-selection candidate |
| Common BTstack source family | `vendor/btstack/src/*.c` except `src/hci_transport_*.c` | source fact | `vendor/btstack/port/libusb/CMakeLists.txt:51`, `vendor/btstack/port/libusb/CMakeLists.txt:111-112`; `vendor/btstack/port/windows-winusb/CMakeLists.txt:43` | stable for CMake source selection |
| Classic source family | `vendor/btstack/src/classic/*.c` | source fact | `vendor/btstack/port/libusb/CMakeLists.txt:54`, `vendor/btstack/port/windows-winusb/CMakeLists.txt:45` | stable for CMake source selection |
| BLE source family | `vendor/btstack/src/ble/*.c` except `le_device_db_memory.c` plus `src/ble/gatt-service/*.c` | source fact | `vendor/btstack/port/libusb/CMakeLists.txt:52-53`, `vendor/btstack/port/libusb/CMakeLists.txt:114-115`; `vendor/btstack/port/windows-winusb/CMakeLists.txt:46-59` | stable for CMake source selection |
| libusb platform source family | `vendor/btstack/platform/libusb/*.c`, `vendor/btstack/platform/posix/*.c` except `le_device_db_fs.c` | source fact | `vendor/btstack/port/libusb/CMakeLists.txt:63-64`, `vendor/btstack/port/libusb/CMakeLists.txt:117-118` | stable for CMake source selection |
| windows-winusb platform source family | `vendor/btstack/platform/windows/*.c` | source fact | `vendor/btstack/port/windows-winusb/CMakeLists.txt:52` | stable for CMake source selection |
| chipset source families | `chipset/realtek/*.c` and `chipset/zephyr/*.c` for libusb; `chipset/zephyr/*.c` for windows-winusb | source fact | `vendor/btstack/port/libusb/CMakeLists.txt:65-66`, `vendor/btstack/port/windows-winusb/CMakeLists.txt:53` | stable for CMake source selection |
| third-party source families | `bluedroid`, `md5`, `micro-ecc`, `hxcmod-player`, `lc3-google`, `qr-code-generator`, `rijndael`, `yxml` | source fact | `vendor/btstack/port/libusb/CMakeLists.txt:56-67`, `vendor/btstack/port/windows-winusb/CMakeLists.txt:44-56` | stable for CMake source selection |
| swbt exclusion: port `main.c` | excluded from `cmake/btstack_sources.cmake` | inference from upstream examples and swbt executable ownership | `vendor/btstack/port/libusb/CMakeLists.txt:171-209`, `vendor/btstack/port/windows-winusb/CMakeLists.txt:96-121`; `apps/swbt-daemon/main.c` | stable swbt boundary |
| swbt source-selection test | CTest checks required HCI, L2CAP, Classic HID, backend transport, run loop, include dirs, link boundary, and port main exclusion | implementation fact | `tests/cmake/btstack_sources_test.cmake` | covered by unit-level CMake test |

## 4. 未解決事項

- Phase 1 は source selection を固定する。BTstack HID Device bridge から API を呼ぶ実装は Phase 4 の対象である。
- `swbt_btstack_source_selection` は source selection property と include/link 境界を保持する `INTERFACE` target であり、現時点では BTstack source をコンパイルしない。
- Windows `windows-winusb` cross build は CMake source selection を設定できることの確認であり、Windows native 実機動作を証明しない。
- 実機での WinUSB driver、pairing、report loop は未検証であり、`docs/hardware-test-log.md` にはまだ記録しない。
