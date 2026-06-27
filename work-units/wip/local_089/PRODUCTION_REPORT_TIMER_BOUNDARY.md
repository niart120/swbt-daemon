# Production Report Timer Boundary

## 1. 概要

production report timer bridge を `production_runner` から分離する。対象は HID send callback、timer adapter config、report tick metrics observer、neutral send / subcommand reply enqueue の process backend callback である。

完了後、runner は report timer の設定や metrics result mapping を直接持たない。

## 2. 起点 / ユースケース

source:

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- 現状分析, 2026-06-28: runner は report timer send、timer config、metrics mapping、report tick observer、neutral send, reply enqueue を同じ file に持つ。
- `spec/protocols/daemon-ipc-v1.md`: status metrics field names and units are protocol contract / implementation fact.

use case:

- actor: report timer または status metrics を変更する開発者。
- 入力または状態: production report timer adapter と domain metrics update が runner lifecycle 内に埋まっている。
- 期待する観測結果: report timer bridge と metrics observer が独立し、runner は lifecycle の順序だけを扱う。
- 制約: report period、send result mapping、status metrics JSON field semantics は変えない。

source から use case への変換:

report timer bridge は Switch-facing report generation に近いが、今回の目的は配置変更である。既存 tests で send success / failure metrics を固定したまま helper へ分ける。

## 3. 対象範囲

- `production_report_timer.*` 相当を追加する。
- HID sender callback が `swbt_btstack_device_send` を使う behavior を維持する。
- report tick observer が `swbt_domain_record_report_tick` へ送る mapping を維持する。
- neutral send immediate / pending / error trace と return behavior を維持する。
- subcommand reply enqueue behavior を維持する。

## 4. 対象外

- report scheduler policy、report period、subcommand reply packet bytes の変更。
- metrics JSON field name / unit の変更。
- HID session packet handling の分離。
- 実機検証。

## 5. 関連 spec / docs

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- `spec/protocols/daemon-ipc-v1.md`
- `work-units/complete/local_064/PRODUCTION_STATUS_METRICS_CONNECTION.md`
- `work-units/complete/local_080/DEVICE_API_PRODUCTION_PATH.md`
- `tests/report_metrics_test.c`
- `tests/daemon_production_runner_test.c`

## 6. 根拠監査

not applicable if packet bytes and report period remain unchanged.

If this work changes report scheduler timing, report bytes, or BTstack send-ready behavior, use `source-audit` and re-scope the behavior change separately.

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: report timer / metrics bridge is a cohesive production adapter concern and currently obscures runner lifecycle.
- verification: status metrics and fake HID send tests should pass unchanged.

配置方針:

- `production_report_timer.*` is in `swbt/daemon` because it reads daemon process state and domain metrics.
- It may depend on `btstack_bridge/input_report_timer_adapter.h` and `btstack_bridge/device.h`.
- It should not own HID event dispatch or run loop shutdown.

## 8. 対象ファイル

- `swbt/daemon/production_report_timer.*`
- `swbt/daemon/production_runner.c`
- `swbt/daemon/production_process_backend.*` if already introduced
- `tests/daemon_production_runner_test.c`
- `tests/report_metrics_test.c`
- `CMakeLists.txt`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | production report timer sender still uses `swbt_btstack_device_send` and preserves fake device send observations | regression | unit/integration | no |
| green | successful report send still updates status metrics without hardware measurement values | regression | integration | no |
| green | failed report send still updates failure metrics and cleans up through existing lifecycle order | regression | integration | no |
| todo | neutral send immediate / pending / error return behavior remains unchanged | regression | integration | no |
| todo | subcommand reply enqueue still routes through the production report timer port | regression | integration | no |

## 10. 検証

Partial.

TDD status:

- use case: report timer HID sender callback を `production_runner` から分離しても、
  production send path は `swbt_btstack_device_send` を使う。
- item: production report timer sender still uses `swbt_btstack_device_send` and preserves fake device send observations.
- red:
  - command: `just build-debug`
  - result: fail as expected. `tests/daemon_production_report_timer_test.c` が
    `daemon/production_report_timer.h` を要求し、header 未実装で compile failure。
- green:
  - command: `just build-debug`
  - result: pass。
  - command: `$env:CTEST_ARGS='-R "daemon_production_report_timer_test|daemon_production_runner_test|report_metrics_test" --output-on-failure'; just test-debug`
  - result: pass、3/3。
- notes: `swbt/daemon/production_report_timer.*` を追加し、timer config、HID sender
  callback、report tick observer を runner から分離した。runner は stable な
  `report_timer_bridge` context を持ち、timer callback へ一時 object を渡さない。
- item: successful report send still updates status metrics without hardware measurement values.
- item: failed report send still updates failure metrics and cleans up through existing lifecycle order.
- green:
  - command: `just build-debug`
  - result: pass。
  - command: `$env:CTEST_ARGS='-R "daemon_production_report_timer_test|daemon_production_runner_test|report_metrics_test" --output-on-failure'; just test-debug`
  - result: pass、3/3。
- notes: `daemon_production_report_timer_test` に report tick observer の success /
  failure mapping を追加した。成功時は `report_ticks=1`, `report_send_ok=1`、
  失敗時は `report_ticks=1`, `report_send_failed=1` を確認した。既存
  `daemon_production_runner_test` は failure path の cleanup order も維持している。

Expected checks:

- `just build-debug`
- `$env:CTEST_ARGS='-R "daemon_production_runner_test|report_metrics_test" --output-on-failure'; just test-debug`

## 11. 実機実行条件

実機実行は不要 if this remains a structure change.

Do not change report period, report bytes, BTstack send-ready scheduling, or Switch-facing behavior in this work unit.

## 12. 先送り事項

none.

Report timing or packet behavior changes are explicitly outside this work unit and should become a new behavior work unit only if requested.

## 13. チェックリスト

- [ ] report timer bridge を runner から分離した。
- [ ] metrics observer behavior を維持した。
- [ ] neutral send and reply enqueue behavior を維持した。
- [ ] TDD Test List の検証を実行し、結果を記録した。
- [ ] 実機未実行理由を維持した。
