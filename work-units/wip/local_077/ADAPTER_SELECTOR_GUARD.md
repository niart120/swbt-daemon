# Adapter Selector Guard

## 1. 概要

production 既定化後に、BTstack USB transport がどの Bluetooth アダプターを開くかを swbt 側で明示できない問題を扱う work unit。

この record は、専用 USB Bluetooth ドングルだけを実機対象にする運用方針と、実装上の adapter selector / identity guard の境界を整理する。`local_074` では起動 mode と診断 path に限定したため、adapter selector はここへ分離する。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`: production 既定化後も、BTstack USB transport がどの adapter を開くかは swbt 側で明示 selector を持たない、という先送り事項。
- `spec/operations/windows-native-preflight.md`: 実機実行では専用 USB Bluetooth ドングルと WinUSB driver assignment を記録し、内蔵 Bluetooth と常用ドングルを使わない。
- `docs/hardware-test-log.md`: CSR8510 A10 + WinUSB を既知の実機構成として記録している。

use case:

- actor: hardware operator、maintainer。
- 入力または状態: Windows native production run、複数 Bluetooth adapter が存在し得る host、専用 USB Bluetooth ドングルの VID/PID または driver state。
- 期待する観測結果:
  - production run の前に、対象 adapter を人間が確認できる。
  - swbt 側で selector または identity guard を実装するか、BTstack / WinUSB 側の制約により運用 preflight に留めるかを決める。
  - 実装する場合、対象 adapter が不明な状態で実機 run に進まない。
- 制約: Bluetooth adapter open、Switch pairing、HID advertising、report loop は人間の明示承認なしに実行しない。

## 3. 対象範囲

- BTstack Windows USB transport で adapter identity を選択または検査できる境界を調べる。
- `--adapter`、`--adapter-vid-pid`、`--require-adapter` などの CLI 形を採用するか判断する。
- 実装する場合は adapter mismatch が production run loop 前に失敗することを test で固定する。
- 実装しない場合は、その理由と preflight 手順で担保する範囲を spec / docs に記録する。

## 4. 対象外

- 複数 controller 同時接続。
- Bluetooth adapter driver の自動切替。
- Zadig / WinUSB の設定変更自動化。
- Linux / macOS の adapter selector 実装。必要なら別 work unit へ分離する。
- Switch protocol、HID report、pairing sequence の変更。

## 5. 関連 spec / docs

- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `docs/status.md`
- `docs/hardware-test-log.md`

## 6. 根拠監査

required if this work unit changes BTstack source selection, WinUSB/libusb behavior assumptions, or adapter identity values used by production code.

初期 record 作成時点では実装していないため、根拠監査は未開始である。

## 7. 設計メモ

- 起動 mode の `production` / `noop` は adapter selector ではない。
- 実機承認の運用 gate は残す。selector を実装しても、人間承認なしに adapter open へ進まない。
- adapter identity guard は、誤った adapter を開かないことが目的であり、driver setup tool ではない。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `swbt/daemon/launch_options.*`
- `swbt/btstack_bridge/production_btstack.*`
- `spec/operations/windows-native-preflight.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `tests/daemon_*`
- `tests/btstack_*`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | adapter selector syntax is accepted or explicitly rejected with a documented reason | new | unit/docs | no |
| todo | adapter mismatch fails before production run loop when identity guard is configured | edge | unit/integration | no |
| todo | Windows preflight records adapter identity and driver state required by the chosen policy | regression | docs | no |
| hardware-gated | selected adapter policy is checked against a dedicated USB Bluetooth dongle | behavior | hardware | yes |

## 10. 検証

not run. This record only captures a deferred work unit source from `local_074`.

## 11. 実機実行条件

この work unit の実装時に adapter open、pairing、advertising、report loop を含む場合は `hardware-harness` を読む。実機実行には人間の明示承認、専用 USB Bluetooth ドングル、WinUSB driver assignment、`docs/hardware-test-log.md` への記録が必要である。

## 12. 先送り事項

none.

## 13. チェックリスト

- [x] source を `local_074` の先送り事項から特定した。
- [x] use case を adapter selector / identity guard として定義した。
- [x] `local_074` の範囲外として分離した。
- [ ] BTstack / WinUSB の adapter selection 境界を調査した。
- [ ] TDD Test List を実装前に再確認した。
- [ ] software verification または実機未実行理由を記録した。
