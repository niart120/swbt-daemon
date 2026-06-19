# Virtual SPI Seed Data

## 1. 概要

local_006 で後続に残した factory SPI data generator のうち、virtual SPI に初期 seed data を入れる helper を追加した。

この work unit では、監査済み address range へ dev/test 用 deterministic profile を書き込む API を作った。
serial number と MAC / Bluetooth address は、値、長さ、永続化責務をこの work unit で監査できていないため seed しない。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_006/SWITCH_SPI_CORE.md` の後続事項。
- `spec/references/switch-spi-core.md` の未解決事項。

use case:

- 境界: virtual SPI storage と seed helper。
- 入力: dev/test 用 seed profile、または caller-provided extra entry。
- 期待する観測結果: 監査済み range にだけ seed data が入り、未seed範囲は erased byte のまま残る。
- 制約: seed helper は `swbt_switch_spi_write` を使い、storage を直接変更しない。
- 対象外: 実機 Pro Controller の factory calibration 生成、serial number、MAC / Bluetooth address、pairing data。

## 3. 対象範囲

- `swbt_switch_spi_t` に deterministic seed profile を書き込む helper を追加する。
- device type、factory stick calibration、factory IMU calibration、controller color、user calibration magic の seed 長を検証する。
- extra entry の storage boundary と重複 write を検証する。
- unseeded range は既存 virtual SPI の erased byte policy を維持する。
- dev/test 用 seed と production calibration の境界を根拠監査に記録する。

## 4. 対象外

- 実機 Pro Controller からの calibration 抽出。
- 個体ごとの factory calibration 生成。
- serial number と MAC / Bluetooth address の seed 実装。
- pairing data、LTK、Bluetooth address の永続化。
- patchram payload。
- 実機 pairing、HID advertising、report loop。

## 5. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-spi-core.md`
- `spec/references/switch-virtual-spi-seed-data.md`
- `work-units/complete/local_006/SWITCH_SPI_CORE.md`

## 6. 根拠監査

`source-audit` を使い、`spec/references/switch-virtual-spi-seed-data.md` を追加した。

device type address/value、factory IMU calibration range、stick calibration range、controller color range、user calibration magic は `spec/references/switch-spi-core.md` の既存根拠に従った。

controller color payload と calibration payload は dev/test seed であり、実機 Pro Controller factory data とは断定しない。
serial number と MAC / Bluetooth address は未監査として seed しない。

実機 Pro Controller として受け入れられる seed data は未検証である。

## 7. 設計メモ

- `swbt_switch_spi_seed_apply` は required field 長、extra entry の boundary、required field と extra entry の重複、extra entry 同士の重複を先に検証してから write する。
- seed helper は既存の `swbt_switch_spi_write` を使い、直接 storage を触らない。
- `swbt_switch_spi_seed_dev_profile` は static data への pointer を持つ profile を返す。
- factory calibration は実機相当を名乗らず、dev/test seed として扱う。
- seed helper は Switch subcommand reply を生成しない。

## 8. 対象ファイル

- `CMakeLists.txt`
- `swbt/switch/switch_spi_seed.h`
- `swbt/switch/switch_spi_seed.c`
- `tests/switch_spi_seed_test.c`
- `spec/references/switch-virtual-spi-seed-data.md`
- `work-units/complete/local_021/VIRTUAL_SPI_SEED_DATA.md`

## 9. TDD Test List

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | source-audited seed profile writes expected ranges through `swbt_switch_spi_write` | new | unit | no |
| done | color and calibration fields reject wrong lengths | edge | unit | no |
| done | seed helper preserves unrelated erased ranges | regression | unit | no |
| done | out-of-range seed entry returns explicit error before partial write | edge | unit | no |
| done | overlapping extra entry returns explicit error before partial write | edge | unit | no |
| deferred | serial and MAC fields reject wrong lengths | edge | unit | no |

## 10. 検証

- red: `just debug` failed as expected because `tests/switch_spi_seed_test.c` included missing `switch/switch_spi_seed.h`.
- green: `just debug` passed. 19 tests passed, including `switch_spi_seed_test`.
- first verify: `just verify` failed in clang-tidy because `ranges_overlap` had easily swappable adjacent parameters.
- final verify: `just verify` passed. Format check, clang-tidy, linux debug tests, ASan/UBSan tests, and Windows MinGW cross build passed.

## 11. 実機実行条件

この work unit の実装と unit test では実機検証は不要である。
Bluetooth adapter、Switch pairing、HID advertising、report loop に触れていない。

Switch が seed 済み virtual SPI を受け入れるかを確認する段階では実機検証が必要である。
実機検証を行う場合は、人間の明示承認を得る。
実機検証を行う場合は、専用 USB Bluetooth ドングルだけを使う。
実機検証を行う場合は、`SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。
実機検証を行う場合は、OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を `docs/hardware-test-log.md` に記録する。

## 12. 先送り事項

- serial number と MAC / Bluetooth address の seed は未実装である。storage 位置、長さ、永続化責務が未監査であり、この work unit で実機相当の値を置くと確認済み事実と推定が混ざるため先送りする。後続 work unit の source は `spec/references/switch-virtual-spi-seed-data.md` の未解決事項に置く。
- seed 済み virtual SPI が Switch pairing / HID session で受け入れられるかは実機未検証である。実機承認を得た work unit で `docs/hardware-test-log.md` に記録する。

## 13. チェックリスト

- [x] work unit record を新形式へ更新した。
- [x] 根拠監査を記録した。
- [x] virtual SPI seed helper を実装した。
- [x] テストを追加した。
- [x] red を確認した。
- [x] green を確認した。
- [x] `just verify` を実行した。
- [x] 実機未実行理由を記録した。
