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
| done | application target does not expose BTstack include directories | regression | build | no |
| done | BTstack adapter target cannot include IPC session internal header | new | build | no |
| done | `production_btstack.c` no longer includes `daemon/ipc_runner.h` after host composition owns IPC pump | new | build | no |
| done | protocol tests link without daemon or BTstack adapter target | regression | build | no |
| done | startup failure cleanup order is covered by host test | regression | unit | no |
| deferred | production composition cutover passes integrated hardware smoke when composition changes | characterization | hardware | yes |

## 10. 検証

- red:
  - `just test-debug` は sandbox 内の Docker 検出で CTest 前に停止したため、TDD red には数えない。
  - `just debug` を Dev Container 経由で実行し、`include_boundaries_cmake_test` が `swbt/btstack_bridge/production_btstack.c` の `#include "daemon/ipc_runner.h"` を検出して失敗した。
- green:
  - `swbt_switch_protocol` と `swbt_application` target を追加した。
  - protocol / application tests は `swbt_core` ではなく対象 module target に link する。
  - `production_backend` が IPC runner の start / stop と poll callback 作成を持ち、`production_btstack` は `swbt_daemon_production_ipc_pump_t` を BTstack run loop timer に登録する。
  - `tests/cmake/include_boundaries_test.cmake` は application target の BTstack include / link 禁止、BTstack bridge の IPC 内部 header 禁止、`production_btstack` の `daemon/production_backend.h` 禁止、protocol test link target を確認する。
  - `just debug`: 38/38 passed。
  - 境界チェックを `daemon/` 全体禁止へ広げた試行では `production_backend_ops.h` まで禁止して失敗した。これは今回導入した最小 port contract まで禁止する過剰な check だったため、禁止対象を `ipc/`、`daemon/ipc_runner.h`、`daemon/runtime.h` に絞り直した。
  - `just test-debug`: 38/38 passed。
- full verification:
  - `just verify`: passed。format check、clang-tidy build、debug build / CTest、ASan build / CTest、Windows MinGW cross build を実行した。

## 11. 実機実行条件

今回の変更では実機を実行していない。

理由:

- Switch-facing report bytes、HID descriptor、subcommand、SPI、rumble packet を変更していない。
- BTstack HID registration、HID send / can-send、report timer adapter の scheduling は変更していない。
- IPC pump の runner 所有を `production_backend` へ寄せたが、production start / cleanup ordering は既存の fake backend unit test で固定している。

production executable の composition、BTstack initialization、shutdown order をさらに変更する場合は、`local_055` または後続 work unit で `local_053` と統合した hardware gate を実行する。

## 12. 先送り事項

- 観測: Windows native CI は target 分割後の有用な gate である。
  先送り理由: CI 拡張は build boundary が固まってから行う。
  次の置き場: IPC / platform hardening work unit。
- 観測: `swbt_core` は互換用 aggregate target として残る。
  先送り理由: daemon / IPC / BTstack bridge の target 分割を同時に進めると production composition の変更と混ざる。
  次の置き場: `work-units/wip/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`。
- 観測: `production_btstack` は `swbt_daemon_production_backend_ops_t` を返す。
  先送り理由: IPC runner 直接参照は外したが、ops table 接続の削除は composition root の最終整理と同時に行う必要がある。
  次の置き場: `work-units/wip/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`。

## 13. チェックリスト

- [x] target 分割方針を確認した。
- [x] forbidden include check を追加した。
- [x] tests の link target を見直した。
- [x] host lifecycle test を追加または更新した。
- [x] build / CTest を実行した。
- [x] 実機 gate の要否を判定した。
- [x] aggregate target の残置理由と削除条件を記録した。
