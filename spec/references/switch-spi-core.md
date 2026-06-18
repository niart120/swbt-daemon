# Switch SPI Core Source Audit

## 1. 状態

recorded。

この reference は Phase 2 の `switch_spi.*` 実装で使う SPI flash read boundary と主要 SPI address の根拠監査である。
対象は SPI address limit、read size limit、factory configuration / calibration、controller color、user calibration magic の address range である。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| dekuNukem Nintendo Switch reverse engineering notes | `ac8093c84194b3232acb675ac1accce9bcb456a3` | `spi_flash_notes.md`, `bluetooth_hid_subcommands_notes.md` |
| joycontrol | `18a09da1a04306534ff9e1df8a1a69c0192a3244` | `joycontrol/report.py` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| SPI flash address limit | `0x80000` exclusive | source fact | joycontrol `report.py:299-307`; dekuNukem `spi_flash_notes.md:3-15` | stable for boundary check |
| SPI read max size | `0x1d` bytes | source fact | dekuNukem `bluetooth_hid_subcommands_notes.md:146-162`; joycontrol `report.py:299-305` | stable for boundary check |
| factory config section | `0x6000` config/calibration section | source fact | dekuNukem `spi_flash_notes.md:114-137` | stable address map |
| device type address | `0x6012`; Pro Controller value `0x03` | source fact | dekuNukem `spi_flash_notes.md:116-121`; joycontrol `controller.py:4-8` | stable address/value; caller-seeded in swbt |
| factory IMU calibration range | `0x6020`-`0x6037` | source fact | dekuNukem `spi_flash_notes.md:122-124`, `spi_flash_notes.md:150-170` | stable address map; sample values only |
| stick calibration range | `0x603d`-`0x604e` | source fact | dekuNukem `spi_flash_notes.md:125-128`, `spi_flash_notes.md:143-149` | stable address map; sample values only |
| controller color range | `0x6050`-`0x6055` | source fact | dekuNukem `spi_flash_notes.md:129-130` | stable address map; caller-seeded in swbt |
| user calibration magic ranges | `0x8010`/`0x801b`/`0x8026` with magic `b2 a1` | source fact | dekuNukem `spi_flash_notes.md:136-144` | stable address map; payload remains caller-seeded |

## 4. 未解決事項

- 実機 Pro Controller として受け入れられる calibration payload は未検証である。
- この work unit は caller-seeded virtual SPI を読む。実機相当の factory data generator は実装しない。
- SPI read subcommand `0x21` reply builder は別 work unit で扱う。
