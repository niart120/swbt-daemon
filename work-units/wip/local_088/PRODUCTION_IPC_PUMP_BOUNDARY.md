# Production IPC Pump Boundary

## 1. 概要

`swbt_daemon_ipc_runner_t` を BTstack run loop pump port へ接続する adapter を `production_runner` から分離する。

完了後、IPC runner start / stop と `swbt_btstack_production_ipc_pump_t` の作成は `production_ipc_pump.*` 相当で読める。BTstack bridge は daemon IPC runner 型を参照しない。

## 2. 起点 / ユースケース

source:

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- 現状分析, 2026-06-28: runner の process backend callback 群に IPC runner start / stop と BTstack IPC pump callback 作成が同居している。
- `tests/cmake/include_boundaries_test.cmake`: BTstack bridge は daemon IPC runner 型を参照しない境界を持つ。

use case:

- actor: IPC transport または BTstack run loop pump を変更する開発者。
- 入力または状態: daemon IPC runner の poll function を BTstack run loop へ渡す glue が production runner 内にある。
- 期待する観測結果: IPC pump glue が独立し、runner lifecycle と process backend table から読み分けられる。
- 制約: IPC JSON wire format、IPC listen config、poll timing semantics は変えない。

source から use case への変換:

IPC pump glue は daemon IPC runner と BTstack production ports の接続部であり、BTstack bridge に移せない。daemon 配下の small adapter として分ける。

## 3. 対象範囲

- `production_ipc_pump.*` 相当を追加する。
- IPC runner start / stop と BTstack IPC pump start / stop の順序を維持する。
- pump `is_running` と `poll_once_at` callback behavior を維持する。
- process backend callback は新 helper を呼ぶだけにする。

## 4. 対象外

- IPC server、IPC JSON codec、IPC command semantics の変更。
- BTstack run loop implementation の変更。
- report timer、HID session、shutdown の分離。
- 実機検証。

## 5. 関連 spec / docs

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- `work-units/complete/local_054/DAEMON_HOST_AND_BUILD_BOUNDARIES.md`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/daemon_ipc_runner_test.c`
- `tests/daemon_production_runner_test.c`

## 6. 根拠監査

not applicable.

This work unit keeps existing BTstack run loop pump callback shape and IPC runner behavior. If BTstack scheduling semantics or polling cadence changes, reclassify that part as behavior change.

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: IPC runner glue は lifecycle orchestration ではなく daemon IPC と BTstack run loop の adapter である。
- verification: production runner startup order tests and IPC runner tests should pass unchanged.

配置方針:

- `production_ipc_pump.*` は `swbt/daemon` に置く。
- It may include `daemon/ipc_runner.h` and `btstack_bridge/production_ports.h`.
- It must not move daemon IPC runner knowledge into `swbt/btstack_bridge`.

## 8. 対象ファイル

- `swbt/daemon/production_ipc_pump.*`
- `swbt/daemon/production_runner.c`
- `swbt/daemon/production_process_backend.*` if already introduced
- `tests/daemon_production_runner_test.c`
- `tests/daemon_ipc_runner_test.c`
- `CMakeLists.txt`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | production IPC start still starts daemon IPC runner before BTstack IPC pump start | regression | integration | no |
| todo | BTstack IPC pump start failure still stops daemon IPC runner | regression | integration | no |
| todo | production IPC stop still stops BTstack IPC pump before daemon IPC runner stop | regression | integration | no |
| todo | pump callbacks still report running state and poll the same IPC runner instance | regression | unit/integration | no |
| todo | BTstack bridge remains free of daemon IPC runner include and type references | regression | architecture | no |

## 10. 検証

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: daemon IPC runner の start 後に、同じ runner instance を BTstack IPC pump
  port へ渡す glue を production runner から分離する。
- item: production IPC start still starts daemon IPC runner before BTstack IPC pump start.
- state: green.
- red:
  - command: `just build-debug`
  - result: fail as expected. `daemon_production_ipc_pump_test` が
    `daemon/production_ipc_pump.h` を要求し、header 未実装で compile failure。
- green:
  - command: `just format`
  - result: pass.
  - command: `just build-debug`
  - result: pass.
  - command: `$env:CTEST_ARGS='-R "daemon_production_ipc_pump_test|daemon_production_runner_test|daemon_ipc_runner_test" --output-on-failure'; just test-debug`
  - result: pass, 3/3 tests passed.
- notes: `swbt/daemon/production_ipc_pump.*` を追加し、daemon IPC runner start と
  BTstack IPC pump port への adapter 作成を runner から分離した。new adapter test は
  pump start 時に runner が running で、pump context が同じ runner instance であることを
  確認する。既存 production runner integration でも startup order を確認した。

## 11. 実機実行条件

実機実行は不要。

This work unit moves software glue covered by fake ports and IPC runner tests. It does not open a Bluetooth adapter or run HID advertising / report loop.

## 12. 先送り事項

none.

IPC protocol or transport changes are outside this work unit and are not created as follow-up from this structure change.

## 13. チェックリスト

- [ ] IPC pump adapter を runner から分離した。
- [ ] start failure cleanup order を維持した。
- [ ] BTstack bridge include boundary を確認した。
- [ ] TDD Test List の検証を実行し、結果を記録した。
- [ ] 実機未実行理由を維持した。
