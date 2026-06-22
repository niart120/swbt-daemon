# Rearchitecture Cutover Acceptance And Cleanup

## 1. 概要

rearchitecture の cutover、受入試験、互換層削除を完了する work unit。

この record は、移行の最後に旧 glue や compatibility wrapper が残置されることを防ぐための完了条件を定義する。実装作業の最後に作る checklist ではなく、最初から出口を固定する record である。

## 2. 起点 / ユースケース

source:

- `spec/architecture/daemon-application-boundary-rearchitecture.md` の最終状態と互換層方針。
- `work-units/wip/local_050` から `local_054` までの後続完了条件。
- 一時 roadmap note の acceptance harness と hardware smoke 方針。

use case:

- actor: maintainer、reviewer、CI、hardware operator。
- 入力または状態: rearchitecture 後の production executable、debug client journey、fake HID / timer journey、hardware smoke candidate、remaining compatibility wrappers。
- 期待する観測結果: 自動試験と必要最小限の実機 smoke で cutover を判定できる。旧 runtime backend、旧 IPC application logic、state mailbox、production backend glue、aggregate target が理由なく残らない。
- 制約: 実機は必要条件に該当した場合だけ実行する。raw artifact を長文で恒久文書へ転記しない。
- 対象外: bonded reconnect、release packaging、Windows native CI の追加。

source から use case への変換:

「最後に片付ける」ではなく、互換層を導入した各 work unit からこの record へ削除条件を渡す。cutover 完了時に、残った互換層が current spec を持つか、削除対象として追跡されているかを確認する。

## 3. 対象範囲

- `local_050` から `local_054` で導入した compatibility wrapper / glue / aggregate target を棚卸しする。
- 残すもの、削除するもの、別 work unit へ送るものを分類する。
- synthetic journey を自動試験として固定する。
- full verification gate を実行する。
- 必要条件に該当する場合だけ hardware smoke を実行し、manifest と `docs/hardware-test-log.md` に要約を残す。
- `spec/architecture/daemon-application-boundary-rearchitecture.md` の状態を current へ昇格するか、未解決事項を明記して draft のまま残すか判断する。

## 4. 対象外

- bonded reconnect の新規実装。
- parser fuzz と Windows native CI。
- release artifact 作成。
- 複数 controller、Joy-Con、NFC/IR semantic support。

## 5. 関連 spec / docs

- `spec/architecture/daemon-application-boundary-rearchitecture.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `work-units/wip/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`
- `work-units/wip/local_051/DAEMON_APPLICATION_COMMAND_API.md`
- `work-units/wip/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md`
- `work-units/wip/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`
- `work-units/wip/local_054/DAEMON_HOST_AND_BUILD_BOUNDARIES.md`

## 6. 根拠監査

条件付き。

cleanup と acceptance が Switch-facing bytes、BTstack callback registration、report period、WinUSB/libusb facts を変更しない場合、根拠監査は not applicable とする。hardware smoke を実行する場合は、実機観測として `docs/hardware-test-log.md` に記録する。

## 7. 設計メモ

- compatibility wrapper は `kept`, `removed`, `deferred` のいずれかに分類する。
- `kept` は current spec に責務がある場合だけ許可する。
- `deferred` は後続 work unit record の source として残す。
- raw HCI dump は pass 時には要約と artifact path だけを残す。fail 時は原因調査に必要な artifact を保存する。
- hardware smoke は Button A、owner disconnect neutral、shutdown trailing neutral を一 session で見る最小構成を優先する。

## 8. 対象ファイル

- `swbt/**`
- `apps/swbt-daemon/*`
- `apps/swbt-debug-client/*`
- `tests/*`
- `CMakeLists.txt`
- `spec/architecture/daemon-application-boundary-rearchitecture.md`
- `docs/status.md`
- `docs/hardware-test-log.md`（実機実行時のみ）
- `work-units/wip/local_050` から `local_055`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | synthetic journey covers start, connect, acquire, Button A, report send, owner disconnect neutral, reacquire, shutdown neutral | new | integration | no |
| todo | compatibility wrapper inventory has no unclassified remaining item | verification | docs | no |
| todo | forbidden include check passes after old aggregate dependencies are removed or justified | regression | build | no |
| todo | full software verification passes after cutover | verification | build | no |
| deferred | production cutover hardware smoke records pass manifest when hardware gate is required | characterization | hardware | yes |

## 10. 検証

未実行。

## 11. 実機実行条件

実機 smoke は次の変更条件に該当した場合にだけ実行する。

- production executable の composition を変えた。
- BTstack initialization または callback registration を変えた。
- report bytes、timer / send scheduling、shutdown 順序を変えた。

実行する場合は、専用 USB Bluetooth ドングル、WinUSB driver assignment、`SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、`docs/hardware-test-log.md` への記録を必須にする。

## 12. 先送り事項

- 観測: bonded reconnect、parser fuzz、Windows native CI、release / license boundary は rearchitecture 後の重要な roadmap 候補である。
  先送り理由: cutover の完了条件と混ぜると、構造移行の完了判定が曖昧になる。
  次の置き場: docs/status の未確認項目、`local_039`、または新規 work unit record。

## 13. チェックリスト

- [ ] compatibility wrapper / glue / aggregate target を棚卸しした。
- [ ] 残すものに current spec 上の責務がある。
- [ ] 削除するものを削除した。
- [ ] deferred item は後続 work unit source へ渡した。
- [ ] synthetic journey を実行した。
- [ ] full software verification を実行した。
- [ ] hardware gate の要否を判定した。
- [ ] hardware smoke を実行した場合、manifest と hardware log を記録した。
