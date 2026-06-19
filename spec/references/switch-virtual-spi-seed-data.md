# Switch Virtual SPI Seed Data Source Audit

## 1. 状態

recorded。

この reference は、virtual SPI に初期 seed data を書き込む helper の根拠監査である。
対象は、既存の SPI address map で監査済みの範囲、seed helper が書き込む field 長、dev/test 用 deterministic profile の扱いである。

## 2. 参照元

| source | path |
|---|---|
| Switch SPI Core Source Audit | `spec/references/switch-spi-core.md` |
| swbt virtual SPI implementation | `swbt/switch/switch_spi.h`, `swbt/switch/switch_spi.c` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | status |
|---|---:|---|---|
| device type seed address | `0x6012` | `switch-spi-core.md` の source fact | stable address |
| Pro Controller device type value | `0x03` | `switch-spi-core.md` の source fact | stable value |
| factory IMU calibration seed length | `24` bytes | `0x6020`-`0x6037` の inclusive range | stable length; payload is dev/test seed |
| left stick calibration seed length | `9` bytes | `0x603d` から right stick 開始 `0x6046` 直前まで | stable length; payload is dev/test seed |
| right stick calibration seed length | `9` bytes | `0x6046`-`0x604e` の inclusive range | stable length; payload is dev/test seed |
| controller color seed length | `6` bytes | `0x6050`-`0x6055` の inclusive range | stable length; payload is dev/test seed |
| user calibration magic length | `2` bytes | `0x8010`/`0x801b`/`0x8026` with magic `b2 a1` | stable length and magic |
| serial number seed | not seeded | address exists as factory config section start, but production serial value and length are not audited here | deferred |
| MAC / Bluetooth address seed | not seeded | storage address, value, and persistence contract are not audited here | deferred |

## 4. 実装上の扱い

`swbt_switch_spi_seed_apply` は、profile の required field 長、extra entry の storage boundary、required field と extra entry の重複、extra entry 同士の重複を先に検証してから `swbt_switch_spi_write` を呼ぶ。

`swbt_switch_spi_seed_dev_profile` の calibration payload と controller color payload は、実機 Pro Controller の factory data として扱わない。
単体テストと開発時の deterministic seed であり、Switch が実機相当として受け入れることは未検証である。

serial number と MAC / Bluetooth address は、この work unit では erased byte のまま残す。

## 5. 未解決事項

- 実機 Pro Controller として受け入れられる factory calibration payload は未検証である。
- serial number と MAC / Bluetooth address の正しい SPI storage 位置、長さ、永続化責務は未監査である。
- seed 済み virtual SPI を Switch pairing / HID session で受け入れるかは実機未検証である。
