# Switch SPI Core

## 1. 概要

Phase 2: Switch protocol core のうち、`swbt/switch/switch_spi.*` と virtual SPI unit test を実装する work unit。

Switch の SPI flash read subcommand に応答するための下地として、caller が seed した仮想 SPI 領域を address / size 境界に従って読む。

## 2. 対象範囲

- virtual SPI storage を初期化する。
- caller-provided bytes を SPI address に seed する。
- SPI read request の address、size、output capacity を検証する。
- device type、controller color、factory calibration range の read を unit test で確認する。
- unseeded range は default erased byte として `0xff` を返す。

## 3. 対象外

- 実機相当の calibration generator。
- `0x21` subcommand reply builder。
- pairing data、LTK、Bluetooth address の永続化。
- patchram payload。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-hid-initial-source-audit.md`
- `spec/references/switch-spi-core.md`

## 5. 根拠監査

recorded。

SPI address map と read boundary を実装するため、`source-audit` に従って `spec/references/switch-spi-core.md` に根拠を記録した。
実機 acceptability は未検証であり、この work unit では caller-seeded virtual SPI read までを扱う。

## 6. 設計メモ

- `SWBT_SWITCH_SPI_ADDRESS_LIMIT` は joycontrol の validation と dekuNukem の map に合わせて `0x80000` exclusive とする。
- local storage は initial spec の仮想 SPI 方針に合わせ、初期に必要な config/user calibration range を含む `0x10000` bytes とする。
- storage 外だが address limit 内の read は erased byte `0xff` を返す。
- 実機で妥当な calibration payload は未固定であり、caller が seed した bytes を返すだけにする。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/switch/switch_spi.h`
- `swbt/switch/switch_spi.c`
- `tests/switch_spi_test.c`
- `spec/references/switch-spi-core.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/complete/local_006/SWITCH_SPI_CORE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | initialized SPI returns `0xff` for unseeded device type range | new | unit | no |
| refactor-done | caller-seeded device type, color, and factory calibration bytes are returned by address | new | unit | no |
| refactor-done | address limit, read size limit, and output capacity return explicit errors | edge | unit | no |
| refactor-done | write beyond local storage returns explicit error | edge | unit | no |
| refactor-done | Phase 2 TODO の `switch_spi.*` を完了状態に更新する | regression | docs | no |

## 9. 検証

TDD status:

- item: initialized SPI returns `0xff` for unseeded device type range, and seeded ranges can be read by address
- state: refactor-done
- commands:
  - red: `make debug CTEST_ARGS="-R switch_spi_test"` failed as expected because `swbt/switch/switch_spi.c` did not exist.
  - green: `make debug CTEST_ARGS="-R switch_spi_test"` passed.
  - refactor: `make verify` passed.
- notes: implementation returns caller-seeded bytes; it does not generate production calibration payloads.

Verification:

- `make verify`: passed. format-check、clang-tidy、debug、ASan/UBSan、Windows MinGW cross build が完了した。
- `make debug`: passed. `swbt_smoke_test`, `btstack_sources_cmake_test`, `switch_report_test`, `switch_subcommand_test`, `switch_spi_test` passed.
- `make asan`: passed. `swbt_smoke_test`, `btstack_sources_cmake_test`, `switch_report_test`, `switch_subcommand_test`, `switch_spi_test` passed under ASan/UBSan.
- `make windows-cross`: passed. MinGW debug build completed for `swbt-daemon.exe`, `swbt_smoke_test.exe`, `switch_report_test.exe`, `switch_subcommand_test.exe`, and `switch_spi_test.exe`.

## 10. 実機実行条件

実機検証は不要。

この work unit は virtual SPI の unit test のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。
実機 acceptability は reply builder / BTstack bridge / hardware work unit で確認する。

## 11. チェックリスト

- [x] 根拠監査を記録した。
- [x] red を確認した。
- [x] `switch_spi.*` を追加した。
- [x] virtual SPI unit test を追加した。
- [x] `make debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] Phase 2 TODO を更新した。
- [x] 実機状態を記録した。
