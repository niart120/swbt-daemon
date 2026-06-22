# BTstack Port Event Boundary

## 1. 概要

BTstack callback、HID send、timer、run loop integration を application port / typed event 境界へ分ける work unit。

この work unit では、BTstack adapter が daemon IPC runner や production backend 内部型を直接所有しない構造へ近づける。production 実機経路に触れるため、software gate と hardware gate の境界を明確にする。

## 2. 起点 / ユースケース

source:

- `spec/architecture/daemon-application-boundary-rearchitecture.md` の BTstack adapter 方針。
- `work-units/complete/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md` の後続 work unit。
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `docs/hardware-test-log.md` の実機観測。

use case:

- actor: BTstack callback、application reactor、daemon host。
- 入力または状態: HID connected / disconnected、output report、can-send、report tick、shutdown request。
- 期待する観測結果: BTstack packet は typed event へ変換され、application は port interface 経由で HID send / timer を要求する。BTstack adapter は IPC runner を直接 poll する構造から離れる。
- 制約: Switch-facing bytes と report scheduling を変えた場合は実機 gate が必要である。
- 対象外: bond store、target 分割の完了、release packaging。

source から use case への変換:

BTstack port interface を全能力に広げない。まず HID send / can-send、timer、clock、typed HID event に絞る。bond store は bonded reconnect work unit から起こす。

## 3. 対象範囲

- HID port を定義し、BTstack interrupt send へ接続する。
- timer port を定義し、report tick / heartbeat / reconnect 候補を扱える形にする。
- BTstack callback から typed event を生成する。
- application dispatch が typed event を受ける。
- BTstack adapter から daemon IPC runner / production backend 内部型への直接依存を減らす。
- fake port tests と BTstack adapter contract tests を追加する。
- production cutover 候補の実機 gate 条件を記録する。

## 4. 対象外

- BTstack 本体の変更。
- Switch report bytes の意図的変更。
- GAP / SSP / SDP identity の変更。
- bond store と bonded reconnect。
- CMake target 分割の完了。

## 5. 関連 spec / docs

- `spec/architecture/daemon-application-boundary-rearchitecture.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/protocols/switch-hid-core.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md`
- `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`

## 6. 根拠監査

必要。

BTstack callback registration、HID send、timer / run loop boundary に触れるため、BTstack source selection と callback / event layout の根拠を `spec/references/` と pinned `vendor/btstack` source へ辿れるようにする。Switch-facing bytes を変更する場合は `source-audit` を追加で使う。

## 7. 設計メモ

- application public API は BTstack vendor header を公開しない。
- typed event payload は application が意味を持つ値だけにする。
- BTstack run loop を system reactor として使ってよいが、application state の writer を一つに絞る。
- production 実機 gate は `local_054` の host cutover と統合して一度に実行する候補とする。

## 8. 対象ファイル

- `swbt/btstack_bridge/*`
- application command / event files。
- `swbt/daemon/production_backend.*`
- `swbt/daemon/runtime.*`
- `tests/btstack_*`
- `tests/daemon_*`
- `spec/references/btstack-*.md`
- `docs/hardware-test-log.md`（実機実行時のみ）

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | BTstack HID opened callback becomes typed application event | new | unit | no |
| todo | HID port send_report forwards to BTstack interrupt send with unchanged bytes | regression | unit | no |
| todo | report tick requests can-send without IPC runner ownership in BTstack adapter | new | unit | no |
| todo | output report event records rumble / subcommand path through application boundary | regression | integration | no |
| deferred | production cutover candidate passes hardware smoke after host composition is split | characterization | hardware | yes |

## 10. 検証

未実行。

## 11. 実機実行条件

実機は production cutover 候補で実行する。対象は専用 USB Bluetooth ドングル、WinUSB、`SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、`docs/hardware-test-log.md` への記録を必要条件とする。

この work unit 単体の software PR では、Switch-facing bytes と production composition を変えない範囲なら実機不要である。

## 12. 先送り事項

- 観測: bond store port は port interface 候補である。
  先送り理由: bonded reconnect の source と実機手順が必要であり、この work unit の HID / timer boundary と混ぜない。
  次の置き場: bonded reconnect persistence work unit。

## 13. チェックリスト

- [ ] BTstack source 根拠を確認した。
- [ ] red test を追加した。
- [ ] typed event と port interface を追加した。
- [ ] BTstack adapter の daemon 内部型依存を削減した。
- [ ] targeted CTest を実行した。
- [ ] hardware gate の要否を判定した。
- [ ] 互換 glue の削除条件を `local_055` に渡した。
