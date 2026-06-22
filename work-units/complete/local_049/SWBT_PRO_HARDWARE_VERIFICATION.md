# Swbt Pro Hardware Verification

## 1. 概要

`local_048` で `swbt-daemon` の規定 device info profile を `swbt-pro` として定義し、daemon の既定値にした。この work unit では、その変更後の実機経路を Windows native + 専用 USB Bluetooth dongle + WinUSB + Switch2 firmware `22.1.0` で再確認する。

完了後は、`SWBT_DEVICE_INFO_PROFILE` 未指定の production 実行で `swbt-pro` が使われ、pairing、HID L2CAP、subcommand reply、IPC input、cleanup まで既存 bring-up 経路を通るかを `docs/hardware-test-log.md` に記録する。実機結果は対象構成の観測として扱い、別 adapter、別 firmware、長時間安定性へ一般化しない。

## 2. 起点 / ユースケース

source: ユーザ要求。`local_048` は complete に移したため、`swbt-pro` の実機検証は新しい work unit として切る。

source: `work-units/complete/local_048/SWBT_DEVICE_INFO_PROFILE_DEFINITION.md` は、`swbt-pro` profile で Switch2 pairing 画面の Button A 入力反映を確認する hardware item を deferred とした。

source: `docs/status.md` は、`mizuyoukanao-pro` 削除後に未指定または `SWBT_DEVICE_INFO_PROFILE=swbt-pro` で同じ bring-up 経路を通るかを未確認項目としている。

use case: maintainer が `swbt-pro` を daemon default として production 実行したとき、過去の `mizuyoukanao-pro` 由来の実験条件なしに、既知の CSR8510 A10 / WinUSB / Switch2 22.1.0 構成で controller setup sequence と Switch UI 入力反映まで進むことを確認できる。

source から use case への変換: `local_048` は非実機の定義、実装、文書更新で閉じた。実機観測は承認、adapter identity、WinUSB state、daemon trace、HCI dump、cleanup を伴う別 work unit として扱う。

## 3. 対象範囲

- Windows native 実機検証前の preflight を記録する。
- 専用 USB Bluetooth dongle の identity、USB VID/PID、WinUSB driver state を確認する。
- `just windows-cross` で Windows native artifact を作成する。
- `SWBT_DEVICE_INFO_PROFILE` 未指定で production daemon を実行し、daemon default として `swbt-pro` を使う経路を検証する。
- 必要に応じて `SWBT_DEVICE_INFO_PROFILE=swbt-pro` 明示指定でも同じ経路を確認する。
- Switch pairing、HID L2CAP open、request device info reply、後続 subcommand reply sequence、`0x30` input report、IPC input、cleanup を観測する。
- `docs/hardware-test-log.md` に OS、dongle、driver、backend、BTstack commit、swbt commit、Switch firmware、approval scope、environment variables、report period、result、artifact path、cleanup を記録する。
- この record の TDD Test List、検証結果、実機状態、先送り事項を更新する。

## 4. 対象外

- `swbt-pro` bytes、HID descriptor、subcommand reply payload、report packing、rumble、SPI data、report period default の変更。
- `mizuyoukanao-pro` の復活、互換 selector、過去 profile との比較 run。
- BTstack submodule の変更。
- production backend selection の既定化。
- 別 adapter、Linux libusb 経路、bonded reconnect、長時間安定性、report jitter / latency / drop-rate の厳密測定。
- Project NyX 側 macro の実装変更。

## 5. 関連 spec / docs

- `work-units/complete/local_048/SWBT_DEVICE_INFO_PROFILE_DEFINITION.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `spec/protocols/switch-hid-core.md`
- `spec/references/switch-subcommand-dispatcher-core.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`

## 6. 根拠監査

この work unit は新しい Switch protocol byte、BTstack source selection、WinUSB/libusb 実装値、report period 採用値を追加しない。`swbt-pro` の profile bytes と daemon default 化は `local_048` で根拠監査済みであり、この work unit ではその実機観測だけを扱う。

実機観測では、source fact、implementation fact、hardware observation、inference を分ける。`docs/hardware-test-log.md` に記録した条件なしに、結果を一般互換性として扱わない。

## 7. 設計メモ

主検証は `SWBT_DEVICE_INFO_PROFILE` 未指定で行う。これは `local_048` の変更点が「規定 profile を daemon default にする」ことだからである。明示指定の `SWBT_DEVICE_INFO_PROFILE=swbt-pro` は、未指定 run の結果が曖昧な場合だけ補助確認として使う。

`mizuyoukanao-pro` は削除済みのため、この work unit では指定しない。過去ログとの比較は `docs/hardware-test-log.md` と `local_037` の既存記録を参照するに留める。

実機操作は `hardware-harness` と `spec/operations/windows-native-preflight.md` に従い、承認範囲が Bluetooth adapter open、pairing、advertising、report loop、IPC input、cleanup のどれを含むかを記録してから開始する。

## 8. 対象ファイル

- `work-units/complete/local_049/SWBT_PRO_HARDWARE_VERIFICATION.md`
- `docs/hardware-test-log.md`
- `docs/status.md`
- `README.md`
- `spec/protocols/switch-hid-core.md`
- `spec/references/switch-subcommand-dispatcher-core.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | preflight が dedicated dongle identity、USB VID/PID、WinUSB driver state、approval scope を記録する | characterization | docs | yes |
| green | `just windows-cross` で Windows native daemon artifact を作成できる | regression | build | no |
| green | `SWBT_DEVICE_INFO_PROFILE` 未指定の production run が request device info で `swbt-pro` default reply を出す | characterization | hardware | yes |
| green | `swbt-pro` default run が pairing、HID L2CAP open、known subcommand reply sequence、`0x30` input report loop まで進む | characterization | hardware | yes |
| green | IPC から Button A または画面に合う probe input を送り、Switch UI 上の入力反映を確認する | characterization | hardware | yes |
| green | daemon shutdown が cleanup trace と必要な neutral fail-safe を記録し、process exit が成功する | edge | hardware | yes |
| green | `docs/hardware-test-log.md` が実行条件、artifact path、result、cleanup を条件付き観測として記録する | regression | docs | yes |
| deferred | 別 adapter、Linux libusb、bonded reconnect、長時間安定性、report jitter / latency / drop-rate を測る | characterization | hardware | yes |

## 10. 検証

Record 作成時点では実機を実行していなかった。2026-06-22 に `swbt-pro` default run を実行し、結果を以下に追記した。

- `git branch --show-current`: `work/swbt-device-info-profile`。
- `git status --short`: clean before record creation。
- `Get-ChildItem -Name work-units\wip` / `Get-ChildItem -Name work-units\complete`: `local_049` が未使用であることを確認した。
- `work-units/complete/local_048/SWBT_DEVICE_INFO_PROFILE_DEFINITION.md`: deferred hardware item と実機実行条件を確認した。
- `spec/operations/windows-native-preflight.md`: 承認、専用ドングル、WinUSB state、hardware log 記録 gate を確認した。
- `spec/operations/windows-hardware-bringup-sequence.md`: Windows native bring-up の software gate が既存完了 work unit に閉じていることを確認した。
- `docs/hardware-test-log.md`: 既存の hardware log template と CSR8510 A10 / WinUSB / Switch2 記録形式を確認した。
- `just windows-cross`: pass。Windows native daemon artifact を作成した。
- preflight: `tmp/hardware/local_049/20260622-202545-8000us-swbt-pro-default` に OS、swbt commit、BTstack commit、Pnp device、Pnp properties を保存した。CSR8510 A10 は `USB\VID_0A12&PID_0001\9&12127A34&0&1`、Service `WinUSB`、Class `USBDevice`、Provider `libwdi`、DriverVersion `6.1.7600.16385`。
- hardware run: `SWBT_DEVICE_INFO_PROFILE` を未指定にして `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH` を設定し、`build/windows-mingw-debug/swbt-daemon.exe` を foreground 起動した。
- IPC input: `build/windows-mingw-debug/swbt-debug-client.exe --port 37637 --button l --button r --seq 4901 --hold-ms 3000` と `build/windows-mingw-debug/swbt-debug-client.exe --port 37637 --button a --seq 4902 --hold-ms 3000` を実行した。client logs は L+R `state.buttons=4194368`、Button A `state.buttons=8`、`client_lr_exit=0`、`client_a_exit=0` を記録した。
- user observation: Switch 側でコントローラー登録から Button A による画面遷移まで到達した。
- HCI dump: `pairing complete, status 00` `1` 件、PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、BTstack `invalid size` `0` 件。`non-registered handle` は pairing 前に `1` 件あるが、current connection は継続した。
- hardware condition correction: ユーザ補足により、今回も過去の pass run と同じハードウェアおよび firmware 条件、すなわち CSR8510 A10 / WinUSB / Switch2 firmware `22.1.0` で実行したことを記録した。
- `swbt-pro` default reply: HCI dump line `303` は `a1 21 ... 82 02 04 00 03 02 00 1b dc f9 9f 7d 01 01` を記録した。`SWBT_DEVICE_INFO_PROFILE` 未指定の production run で `swbt-pro` bytes が返った。
- subcommand sequence: incoming subcommand は `0x02` `1`、`0x08` `1`、`0x10` `8`、`0x03` `1`、`0x04` `1`、`0x40` `1`、`0x48` `1`、`0x21` `1`、`0x30` `2`。outgoing `a1 21` replies は `82/02` `1`、`80/08` `1`、`90/10` `8`、`80/03` `1`、`83/04` `1`、`80/40` `1`、`80/48` `1`、`a0/21` `1`、`80/30` `2`。
- input reports: outgoing `a1 30` は `2448` 件で、buttons は neutral `000000` `1925` 件、L+R `400040` `194` 件、Button A `080000` `329` 件だった。
- cleanup: startup trace は `production: shutdown neutral send`、`production: shutdown neutral send ok`、HCI power-off、report timer stop、output handler stop、HID stop、BTstack close、run loop deinit、HCI dump close、IPC stop、runtime stop done、production runtime stop done まで到達した。daemon exit marker は `exit=0`。
- docs: `docs/hardware-test-log.md`、`docs/status.md`、`spec/protocols/switch-hid-core.md`、`spec/references/switch-subcommand-dispatcher-core.md` を今回の `swbt-pro` default hardware observation に更新した。
- skipped by design: `SWBT_DEVICE_INFO_PROFILE=swbt-pro` 明示指定 run は、未指定 default run で `swbt-pro` reply と Switch UI 入力反映まで確認できたため実施しない。

## 11. 実機実行条件

実機を伴うため、実行前にユーザの明示承認を必要とする。承認範囲は少なくとも次を分けて記録する。

- Bluetooth adapter open。
- Switch pairing。
- HID advertising / connectable state。
- periodic input report loop。
- IPC input。
- cleanup confirmation。

実行条件:

- 専用 USB Bluetooth dongle を使う。内蔵 Bluetooth と常用 dongle は使わない。
- Windows native では対象 dongle の WinUSB driver assignment を記録する。
- `SWBT_DAEMON_BACKEND=production`。
- `SWBT_RUN_HARDWARE=1`。
- `SWBT_HARDWARE_APPROVED=1`。
- 主検証では `SWBT_DEVICE_INFO_PROFILE` を未指定にする。
- 補助確認を行う場合だけ `SWBT_DEVICE_INFO_PROFILE=swbt-pro` を明示する。
- `SWBT_DIAGNOSTIC_TRACE_PATH` と `SWBT_HCI_DUMP_TRACE_PATH` を設定する。
- `SWBT_REPORT_PERIOD_US` は既存 bring-up と同じ `8000` から開始する。別 period を試す場合は `docs/hardware-test-log.md` に別 entry として記録する。
- cleanup は daemon stop、HCI power-off、report timer stop、HCI dump close、IPC stop、runtime stop completion、neutral fail-safe の観測結果を記録する。

実行済み条件:

- `2026-06-22` の `tmp/hardware/local_049/20260622-202545-8000us-swbt-pro-default` run で実行済み。
- 承認範囲は CSR8510 A10、adapter open、HID advertising / connectable、Switch pairing、L2CAP 接続、`8000 us` report loop、`SWBT_DEVICE_INFO_PROFILE` 未指定の `swbt-pro` default、L+R 3 秒入力、Button A 3 秒入力、HCI dump / diagnostic trace 保存、cleanup 確認。
- Switch firmware は過去の pass run と同じ Switch2 `22.1.0`。ユーザ補足により、今回も同一 firmware 条件での観測として扱う。

停止条件:

- 対象 adapter が曖昧である。
- 対象 adapter が内蔵 Bluetooth または常用 device である。
- WinUSB driver assignment が確認できない。
- 承認範囲が adapter open、pairing、advertising、report loop、IPC input、cleanup のどれを含むか不明である。
- 証跡 path を設定せずに実機へ進もうとしている。

## 12. 先送り事項

- 別 adapter、Linux libusb、bonded reconnect、長時間安定性、report jitter / latency / drop-rate はこの work unit の対象外であり、必要になった時点で別 work unit または `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md` の source として扱う。

## 13. チェックリスト

- [x] preflight を記録した。
- [x] `just windows-cross` を実行し結果を記録した。
- [x] 実機承認範囲を記録した。
- [x] `SWBT_DEVICE_INFO_PROFILE` 未指定の production run を実行し、`swbt-pro` default の実機結果を記録した。
- [x] Switch pairing / HID L2CAP / subcommand reply / input report / IPC input の観測を記録した。
- [x] cleanup と neutral fail-safe の観測を記録した。
- [x] `docs/hardware-test-log.md` を更新した。
- [x] 検証結果と未実行理由をこの record に反映した。
