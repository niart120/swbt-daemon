# Virtual SPI Seed Data

## 1. 概要

local_006 で後続に残した production calibration と factory SPI data generator のうち、virtual SPI に初期 seed data を入れる helper を追加するための計画 record。

device type、serial、MAC、factory stick calibration、factory IMU calibration、controller color、user calibration magic を候補にする。

## 2. 対象範囲

- `swbt_switch_spi_t` に、根拠監査後に確定した seed profile を書き込む helper を追加する。
- device type、serial、MAC、factory stick calibration、factory IMU calibration、controller color、user calibration magic の seed 候補を整理する。
- seed profile の長さ、必須 field、重複 write、storage boundary を検証する。
- unseeded range は既存 virtual SPI の erased byte policy を維持する。
- dev/test 用の deterministic profile と production calibration の境界を明示する。

## 3. 対象外

- 実機 Pro Controller からの calibration 抽出。
- 個体ごとの factory calibration 生成。
- pairing data、LTK、Bluetooth address の永続化。
- patchram payload。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-spi-core.md`
- `work-units/complete/local_006/SWITCH_SPI_CORE.md`

## 5. 根拠監査

SPI address と seed payload は実装前に根拠監査が必要である。

device type、serial、MAC、factory stick calibration、factory IMU calibration、controller color、user calibration magic の値は未監査であり、断定しない。

既存の `switch-spi-core` は boundary と一部 address range を記録しているが、この work unit の seed data としては追加監査が必要である。

実機 Pro Controller として受け入れられる seed data は未検証である。

## 6. 設計メモ

- seed helper は既存の `swbt_switch_spi_write` を使い、直接 storage を触らない。
- seed data は固定配列へ直書きせず、根拠監査後に確定した profile 構造から書き込む。
- serial と MAC は caller-provided profile とし、既定値を置く場合も根拠監査後に決める。
- factory calibration は実機相当を名乗らず、dev/test seed として扱う。
- seed helper は Switch subcommand reply を生成しない。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/switch/switch_spi_seed.h`
- `swbt/switch/switch_spi_seed.c`
- `swbt/switch/switch_spi.h`
- `tests/switch_spi_seed_test.c`
- `spec/references/switch-virtual-spi-seed-data.md`
- `work-units/wip/local_021/VIRTUAL_SPI_SEED_DATA.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | source-audited seed profile writes expected ranges through `swbt_switch_spi_write` | new | unit | no |
| todo | serial, MAC, color, and calibration fields reject wrong lengths | edge | unit | no |
| todo | seed helper preserves unrelated erased ranges | regression | unit | no |
| todo | out-of-range seed entry returns explicit error before partial write | edge | unit | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、red、green、refactor、実機検証は実行していない。

実装後は `make debug CTEST_ARGS="-R switch_spi_seed_test"` を実行する。

変更範囲に応じて `make asan` または `make windows-cross` を実行する。

## 10. 実機実行条件

この work unit の unit test 実装では実機検証は不要である。

Switch が seed 済み virtual SPI を受け入れるかを確認する段階では実機検証が必要である。

実機検証を行う場合は、人間の明示承認を得る。

実機検証を行う場合は、専用 USB Bluetooth ドングルだけを使う。

実機検証を行う場合は、`SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。

実機検証を行う場合は、OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を `docs/hardware-test-log.md` に記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] 根拠監査を完了した。
- [ ] virtual SPI seed helper を実装した。
- [ ] テストを追加した。
- [ ] 検証を実行した。
- [ ] 実機状態を記録した。
