# Production Shutdown Boundary

## 1. 概要

production shutdown scheduling、pending neutral cleanup、power-off / run-loop exit を `production_runner` から分離する。

完了後、runner は shutdown policy の細部を持たず、shutdown helper が neutral send attempt、pending CAN_SEND_NOW completion、power-off、run-loop exit の既存順序を維持する。

## 2. 起点 / ユースケース

source:

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md`: pending neutral send failure cleanup is software-fixed.
- 現状分析, 2026-06-28: shutdown listener validation、main-thread scheduling、pending neutral、finish shutdown が runner lifecycle と同居している。

use case:

- actor: shutdown cleanup または fail-safe behavior を変更する開発者。
- 入力または状態: shutdown path が production runner 本体にあり、HID CAN_SEND_NOW dispatch と power-off cleanup にまたがっている。
- 期待する観測結果: shutdown logic が独立し、runner lifecycle から neutral pending state の詳細が外れる。
- 制約: shutdown neutral send attempt must remain before power-off.

source から use case への変換:

shutdown は lifecycle の一部だが、pending neutral state と main-thread callback scheduling は独立した failure-prone concern である。既存 regression を維持したまま helper へ分ける。

## 3. 対象範囲

- `production_shutdown.*` 相当を追加する。
- shutdown listener validation を移す。
- shutdown request scheduling を移す。
- main-thread shutdown handler と pending neutral state transitions を移す。
- HID session から pending shutdown completion を呼べる interface を定義する。
- power-off / run-loop trigger-exit order を維持する。

## 4. 対象外

- OS console handler implementation。これは `local_085` の apps platform support。
- shutdown neutral retry policy の変更。
- report timer behavior の変更。
- run loop implementation の変更。
- 実機検証。

## 5. 関連 spec / docs

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- `work-units/wip/local_090/PRODUCTION_HID_SESSION_BOUNDARY.md`
- `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md`
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`
- `docs/status.md`
- `tests/daemon_production_runner_test.c`

## 6. 根拠監査

not applicable if shutdown ordering and BTstack calls remain unchanged.

If this work changes shutdown neutral ordering, HCI power-off order, or BTstack run loop trigger semantics, reclassify that portion and evaluate `source-audit` plus hardware gate.

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: shutdown pending cleanup is a cohesive lifecycle concern with existing regression coverage and should not obscure runner main flow.
- verification: shutdown neutral tests must pass unchanged.

配置方針:

- `production_shutdown.*` belongs under `swbt/daemon`.
- It may know production runner state, run loop port, and daemon process neutral send.
- OS process signal / console handling remains in `apps/swbt-daemon`.

## 8. 対象ファイル

- `swbt/daemon/production_shutdown.*`
- `swbt/daemon/production_runner.c`
- `swbt/daemon/production_hid_session.*`
- `tests/daemon_production_runner_test.c`
- `CMakeLists.txt`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | stop request still sends neutral before power-off and run-loop exit | regression | integration | no |
| todo | shutdown after JSON state still sends trailing neutral before power-off | regression | integration | no |
| todo | pending stop request still finishes after CAN_SEND_NOW success | regression | integration | no |
| todo | pending stop request still finishes after CAN_SEND_NOW failure | regression | integration | no |
| todo | repeated stop request still does not power off twice | regression | integration | no |
| todo | shutdown listener install / uninstall order remains unchanged | regression | integration | no |

## 10. 検証

not run yet.

Expected checks:

- `just build-debug`
- `$env:CTEST_ARGS='-R "daemon_production_runner_test" --output-on-failure'; just test-debug`
- `just windows-cross`

## 11. 実機実行条件

実機実行は不要 if shutdown ordering is preserved.

If shutdown neutral ordering changes, this work unit must stop and become a behavior change requiring hardware gate review.

## 12. 先送り事項

none.

Any new shutdown policy change is outside this structure change and should be a separate source, not a deferred item here.

## 13. チェックリスト

- [ ] shutdown helper を runner から分離した。
- [ ] pending neutral state transitions を維持した。
- [ ] power-off / trigger-exit ordering を維持した。
- [ ] TDD Test List の検証を実行し、結果を記録した。
- [ ] 実機未実行理由を維持した。
