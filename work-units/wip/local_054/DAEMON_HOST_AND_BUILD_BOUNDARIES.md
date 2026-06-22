# Daemon Host And Build Boundaries

## 1. 概要

daemon host / composition root と CMake target / include boundary を整理する work unit。

この work unit の目的は、application、IPC adapter、BTstack adapter、platform adapter の依存方向をビルド構成で検出できるようにすることである。

## 2. 起点 / ユースケース

source:

- `spec/architecture/daemon-application-boundary-rearchitecture.md` の target boundary 方針。
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md` の後続 work unit。
- 現行 `swbt_core` が core、daemon、IPC、BTstack bridge、Switch protocol をまとめる implementation fact。
- `local_053` 完了後も `swbt/btstack_bridge/production_btstack.c` が `daemon/ipc_runner.h` と production backend ops table 接続を持つ implementation fact。

use case:

- actor: maintainer、reviewer、CI。
- 入力または状態: application / adapter / protocol の include、test link target、production executable composition。
- 期待する観測結果: 禁止依存が compile error または explicit check で検出される。unit test は不要な adapter library を link しない。
- 制約: target 分割は behavior change と混ぜない。production composition を変える場合は実機 gate を判定する。
- 対象外: BTstack 本体変更、protocol byte 変更、release packaging。

source から use case への変換:

CMake target 分割を最初に行うのではなく、source の所有者が狭まった後に検出手段として導入する。旧 aggregate target を残す場合は互換用と明記し、削除条件を持たせる。

## 3. 対象範囲

- daemon host / composition root の責務を文書化または実装する。
- application、protocol、IPC adapter、BTstack adapter、host の target を分ける。
- include directory を target ごとに狭める。
- forbidden include check を追加する。
- unit test の link target を対象 module へ寄せる。
- startup failure と cleanup ordering を host test で固定する。

## 4. 対象外

- owner policy や command API の新規挙動。
- Switch-facing bytes。
- BTstack source selection の変更。
- Windows native CI 拡張。
- release packaging。

## 5. 関連 spec / docs

- `spec/architecture/daemon-application-boundary-rearchitecture.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md`
- `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`
- `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md`

## 6. 根拠監査

not applicable。ただし production composition、BTstack initialization、shutdown order を変える場合は `local_053` と同じ実機 gate 判定に従う。

## 7. 設計メモ

- `swbt_core` を残す場合は compatibility aggregate target とし、使用箇所と削除条件を記録する。
- application target は BTstack include dirs を持たない。
- BTstack adapter target は IPC session / daemon runtime 内部 header を include しない。
- `production_btstack.c` の IPC pump は、host / composition root 側へ移すか、削除しない責務を current spec に定義する。
- protocol target は socket、JSON、BTstack run loop を知らない。
- host は composition と lifecycle ordering を担当する。

## 8. 対象ファイル

- `CMakeLists.txt`
- `cmake/*`
- `swbt/daemon/*`
- `swbt/ipc/*`
- `swbt/btstack_bridge/*`
- `swbt/switch/*`
- `tests/*`
- forbidden include check script または CMake check。

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | application target does not expose BTstack include directories | regression | build | no |
| todo | BTstack adapter target cannot include IPC session internal header | new | build | no |
| todo | `production_btstack.c` no longer includes `daemon/ipc_runner.h` after host composition owns IPC pump | new | build | no |
| todo | protocol tests link without daemon or BTstack adapter target | regression | build | no |
| todo | startup failure cleanup order is covered by host test | regression | unit | no |
| deferred | production composition cutover passes integrated hardware smoke when composition changes | characterization | hardware | yes |

## 10. 検証

未実行。

## 11. 実機実行条件

CMake target 分割と include check だけなら実機不要である。production executable の composition、BTstack initialization、shutdown order を変更する場合は `local_053` と統合した hardware gate を実行する。

## 12. 先送り事項

- 観測: Windows native CI は target 分割後の有用な gate である。
  先送り理由: CI 拡張は build boundary が固まってから行う。
  次の置き場: IPC / platform hardening work unit。

## 13. チェックリスト

- [ ] target 分割方針を確認した。
- [ ] forbidden include check を追加した。
- [ ] tests の link target を見直した。
- [ ] host lifecycle test を追加または更新した。
- [ ] build / CTest を実行した。
- [ ] 実機 gate の要否を判定した。
- [ ] aggregate target の残置理由と削除条件を記録した。
