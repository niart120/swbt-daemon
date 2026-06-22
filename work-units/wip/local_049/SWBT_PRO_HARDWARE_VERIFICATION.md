# Swbt Pro Hardware Verification

## 1. 概要

`local_048` で `swbt-daemon` の規定 device info profile を `swbt-pro` として定義し、daemon の既定値にした。この work unit では、その変更後の実機経路を Windows native + 専用 USB Bluetooth dongle + WinUSB + Switch2 で再確認する。

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

- `work-units/wip/local_049/SWBT_PRO_HARDWARE_VERIFICATION.md`
- `docs/hardware-test-log.md`
- `docs/status.md`
- `README.md`
- `spec/protocols/switch-hid-core.md`
- `spec/references/switch-subcommand-dispatcher-core.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | preflight が dedicated dongle identity、USB VID/PID、WinUSB driver state、approval scope を記録する | characterization | docs | yes |
| todo | `just windows-cross` で Windows native daemon artifact を作成できる | regression | build | no |
| todo | `SWBT_DEVICE_INFO_PROFILE` 未指定の production run が request device info で `swbt-pro` default reply を出す | characterization | hardware | yes |
| todo | `swbt-pro` default run が pairing、HID L2CAP open、known subcommand reply sequence、`0x30` input report loop まで進む | characterization | hardware | yes |
| todo | IPC から Button A または画面に合う probe input を送り、Switch UI 上の入力反映を確認する | characterization | hardware | yes |
| todo | daemon shutdown が cleanup trace と必要な neutral fail-safe を記録し、process exit が成功する | edge | hardware | yes |
| todo | `docs/hardware-test-log.md` が実行条件、artifact path、result、cleanup を条件付き観測として記録する | regression | docs | yes |
| deferred | 別 adapter、Linux libusb、bonded reconnect、長時間安定性、report jitter / latency / drop-rate を測る | characterization | hardware | yes |

## 10. 検証

Record 作成時点では実機を実行していない。

- `git branch --show-current`: `work/swbt-device-info-profile`。
- `git status --short`: clean before record creation。
- `Get-ChildItem -Name work-units\wip` / `Get-ChildItem -Name work-units\complete`: `local_049` が未使用であることを確認した。
- `work-units/complete/local_048/SWBT_DEVICE_INFO_PROFILE_DEFINITION.md`: deferred hardware item と実機実行条件を確認した。
- `spec/operations/windows-native-preflight.md`: 承認、専用ドングル、WinUSB state、hardware log 記録 gate を確認した。
- `spec/operations/windows-hardware-bringup-sequence.md`: Windows native bring-up の software gate が既存完了 work unit に閉じていることを確認した。
- `docs/hardware-test-log.md`: 既存の hardware log template と CSR8510 A10 / WinUSB / Switch2 記録形式を確認した。

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

停止条件:

- 対象 adapter が曖昧である。
- 対象 adapter が内蔵 Bluetooth または常用 device である。
- WinUSB driver assignment が確認できない。
- 承認範囲が adapter open、pairing、advertising、report loop、IPC input、cleanup のどれを含むか不明である。
- 証跡 path を設定せずに実機へ進もうとしている。

## 12. 先送り事項

- 別 adapter、Linux libusb、bonded reconnect、長時間安定性、report jitter / latency / drop-rate はこの work unit の対象外であり、必要になった時点で別 work unit または `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md` の source として扱う。
- `SWBT_DEVICE_INFO_PROFILE=swbt-pro` 明示指定 run は補助確認であり、未指定 default run が明確に通った場合は実行しない判断も許容する。その場合は理由を検証欄へ記録する。

## 13. チェックリスト

- [ ] preflight を記録した。
- [ ] `just windows-cross` を実行し結果を記録した。
- [ ] 実機承認範囲を記録した。
- [ ] `SWBT_DEVICE_INFO_PROFILE` 未指定の production run を実行し、`swbt-pro` default の実機結果を記録した。
- [ ] Switch pairing / HID L2CAP / subcommand reply / input report / IPC input の観測を記録した。
- [ ] cleanup と neutral fail-safe の観測を記録した。
- [ ] `docs/hardware-test-log.md` を更新した。
- [ ] 検証結果と未実行理由をこの record に反映した。
