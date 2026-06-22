# Rearchitecture Cutover Acceptance And Cleanup

## 1. 概要

rearchitecture の cutover、受入試験、互換層削除を完了する work unit。

この record は、移行の最後に旧 glue や compatibility wrapper が残置されることを防ぐための完了条件を定義する。実装作業の最後に作る checklist ではなく、最初から出口を固定する record である。

## 2. 起点 / ユースケース

source:

- `spec/architecture/daemon-application-boundary-rearchitecture.md` の最終状態と互換層方針。
- `work-units/complete/local_050`、`work-units/complete/local_051`、`work-units/complete/local_052` から `local_054` までの後続完了条件。
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md` で残した `production_btstack` IPC pump / production backend ops table 接続。
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
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`
- `work-units/complete/local_051/DAEMON_APPLICATION_COMMAND_API.md`
- `work-units/complete/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md`
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`
- `work-units/complete/local_054/DAEMON_HOST_AND_BUILD_BOUNDARIES.md`

## 6. 根拠監査

条件付き。

cleanup と acceptance が Switch-facing bytes、BTstack callback registration、report period、WinUSB/libusb facts を変更しない場合、根拠監査は not applicable とする。hardware smoke を実行する場合は、実機観測として `docs/hardware-test-log.md` に記録する。

## 7. 設計メモ

- compatibility wrapper は `kept`, `removed`, `deferred` のいずれかに分類する。
- `kept` は current spec に責務がある場合だけ許可する。
- `deferred` は後続 work unit record の source として残す。
- `production_btstack` IPC pump、production backend ops table、`swbt_ipc_session_t` forwarding wrapper、`swbt_core` aggregate target は最低限の棚卸し対象にする。
- raw HCI dump は pass 時には要約と artifact path だけを残す。fail 時は原因調査に必要な artifact を保存する。
- hardware smoke は Button A、owner disconnect neutral、shutdown trailing neutral を一 session で見る最小構成を優先する。

## 8. 互換層棚卸し

| item | classification | current responsibility / deletion condition |
|---|---|---|
| `production_btstack` IPC pump | kept | BTstack run loop 上で generic IPC pump callback を schedule する port adapter である。`daemon/ipc_runner.h` と `daemon/production_backend.h` は参照しない。BTstack run loop を system reactor として使わない composition へ移るか、別 reactor port を導入した時点で削除または置換する。 |
| production backend ops table | kept | production backend の hardware-facing 能力を束ねる current port table である。platform、HID、timer、SSP、clock、power、run loop、IPC pump を production composition root へ渡す。差し替え理由がある能力ごとに port を分ける後続 work unit が起きた時点で縮小する。 |
| `swbt_ipc_session_t` | kept | IPC wire 互換 status、rumble status、mailbox publish、application result mapping を束ねる current IPC/application facade である。owner、sequence、neutral 化の authoritative logic は `swbt_app_t` へ移動済みで、session は transport そのものではない。IPC server が application command handler を直接 bind でき、status / rumble / mailbox の移管先が揃った時点で縮小または削除する。 |
| `state_mailbox` | kept | IPC/application update と report scheduler の間で latest state を渡す current concurrency boundary である。report scheduler が runtime-owned application state を同等の coalescing guarantee 付きで読める構造へ移った時点で削除する。 |
| `swbt_core` aggregate target | kept | daemon executable、public C ABI、IPC、BTstack bridge を結合する current integration target である。protocol と application の単体 target は `local_054` で分離済みであり、aggregate だけに依存する protocol / application test は残さない。daemon、IPC、BTstack bridge の link target を分けても production executable と include boundary test が維持できる時点で分割する。 |

この PR で削除対象に分類した item はない。削除しない判断は、current spec の責務定義と削除条件がある場合だけ認める。

## 9. 対象ファイル

- `swbt/**`
- `apps/swbt-daemon/*`
- `apps/swbt-debug-client/*`
- `tests/*`
- `CMakeLists.txt`
- `spec/architecture/daemon-application-boundary-rearchitecture.md`
- `docs/status.md`
- `docs/hardware-test-log.md`（実機実行時のみ）
- `work-units/complete/local_050`、`work-units/complete/local_051`、`work-units/complete/local_052` から `local_055`

## 10. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | synthetic journey covers start, connect, acquire, Button A, report send, owner disconnect neutral, reacquire, shutdown neutral | new | integration | no |
| done | compatibility wrapper inventory has no unclassified remaining item | verification | docs | no |
| done | `production_btstack` IPC pump and production backend ops table are removed or justified in current spec | verification | docs/build | no |
| done | forbidden include check passes after old aggregate dependencies are removed or justified | regression | build | no |
| done | full software verification passes after cutover | verification | build | no |
| deferred | production cutover hardware smoke records pass manifest when hardware gate is required | characterization | hardware | yes |

## 11. 検証

- red: `just debug`
  - 期待失敗: `cutover_acceptance_cmake_test` が `daemon cutover synthetic journey test target is required` で失敗した。
- red: `just debug`
  - 期待失敗: `cutover_acceptance_cmake_test` が `cutover inventory must classify production_btstack IPC pump` で失敗した。
- green: `just debug`
  - 40/40 tests passed。`daemon_cutover_journey_test` と `cutover_acceptance_cmake_test` を含む。
- full verification: `just verify`
  - pass。format check、clang-tidy preset build、debug build / CTest、ASan build / CTest、Windows MinGW cross build を実行した。

## 12. 実機実行条件

実機 smoke は次の変更条件に該当した場合にだけ実行する。

- production executable の composition を変えた。
- BTstack initialization または callback registration を変えた。
- report bytes、timer / send scheduling、shutdown 順序を変えた。

実行する場合は、専用 USB Bluetooth ドングル、WinUSB driver assignment、`SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、`docs/hardware-test-log.md` への記録を必須にする。

今回の変更は、fake backend の synthetic journey、CMake 検査、spec / work unit record の更新に閉じる。production executable composition、BTstack initialization、callback registration、report bytes、timer / send scheduling、shutdown 順序は変更していないため、追加の実機 smoke は実行しない。

## 13. 先送り事項

- 観測: bonded reconnect、parser fuzz、Windows native CI、release / license boundary は rearchitecture 後の重要な roadmap 候補である。
  先送り理由: cutover の完了条件と混ぜると、構造移行の完了判定が曖昧になる。
  次の置き場: docs/status の未確認項目、`local_039`、または新規 work unit record。

## 14. セルフレビュー

- 要件充足: 合成 journey は start、acquire、Button A、report 生成、owner disconnect neutral、reacquire、shutdown neutral を 1 つの integration test で通す。
- 棚卸し: 最低限の対象 5 件を `kept` として分類し、削除条件を書いた。`removed` / `deferred` に該当する item は今回の実装断面ではない。
- 根拠監査: Switch-facing bytes、BTstack source selection、report period、WinUSB/libusb facts は変更していないため not applicable。
- 実機: hardware gate 条件に該当しないため未実行。
- 残リスク: `swbt_ipc_session_t` と `swbt_core` は責務定義済みの current boundary として残る。小さくする作業は後続 source から起こす。

## 15. チェックリスト

- [x] compatibility wrapper / glue / aggregate target を棚卸しした。
- [x] 残すものに current spec 上の責務がある。
- [x] 削除するものを削除した。今回の削除対象は該当なし。
- [x] deferred item は後続 work unit source へ渡した。今回の deferred item は hardware smoke だけである。
- [x] synthetic journey を実行した。
- [x] full software verification を実行した。
- [x] hardware gate の要否を判定した。
- [x] hardware smoke を実行した場合、manifest と hardware log を記録した。今回は hardware gate 非該当のため実行なし。
