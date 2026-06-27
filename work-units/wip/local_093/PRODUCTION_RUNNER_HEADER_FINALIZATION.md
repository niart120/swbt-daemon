# Production Runner Header Finalization

## 1. 概要

production runner 分割後に、`production_runner.h` の公開 surface と obsolete API を整理する。

完了後、`swbt_daemon_production_runner_t` の内部 field は必要最小限の header exposure になる。test-only accessor や現行経路で使われない `hardware_approval` API は削除または明確な残存理由を record に残す。

## 2. 起点 / ユースケース

source:

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- 現状分析, 2026-06-28: `production_runner.h` は config、BTstack ports、process、IPC runner、device、report timer、atomic fields を直接公開している。
- 現状分析, 2026-06-28: `swbt_daemon_hardware_approval_*` は current implementation の runtime branch ではなく、tests からのみ参照されている可能性がある。

use case:

- actor: production runner を利用または変更する開発者。
- 入力または状態: runner header を include すると daemon process と BTstack bridge の詳細型が広く見える。
- 期待する観測結果: runner API は initialization, configuration, lifecycle entrypoints に絞られ、内部 state の変更先が限定される。
- 制約: internal API compatibility は保証しないが、observable daemon behavior は変えない。

source から use case への変換:

ファイル分割だけでは header が広いまま残る。最後に公開 surface を縮小し、不要 API を削除することで decomposition の完了条件を閉じる。

## 3. 対象範囲

- `production_runner.h` の include を削減する。
- runner state を opaque 化できるか判断し、可能なら opaque 化する。
- Opaque 化しない場合は、公開 field が必要な理由を record に残す。
- `swbt_daemon_production_process_backend()` の公開要否を再判定する。
- `swbt_daemon_hardware_approval_*` と `approval` 引数の current path 上の扱いを再判定し、削除または残存理由を記録する。
- test helpers を新しい public surface に合わせる。

## 4. 対象外

- 先行 work unit の実装再分割。
- CLI surface、IPC JSON、Switch-facing bytes、report period の変更。
- production startup semantics の変更。
- 実機検証。

## 5. 関連 spec / docs

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- `work-units/wip/local_085/PRODUCTION_ENTRYPOINT_PLATFORM_SPLIT.md`
- `work-units/wip/local_086/PRODUCTION_PORTS_VALIDATION_BOUNDARY.md`
- `work-units/wip/local_087/PRODUCTION_ADDRESS_RECONNECT_BOUNDARY.md`
- `work-units/wip/local_088/PRODUCTION_IPC_PUMP_BOUNDARY.md`
- `work-units/wip/local_089/PRODUCTION_REPORT_TIMER_BOUNDARY.md`
- `work-units/wip/local_090/PRODUCTION_HID_SESSION_BOUNDARY.md`
- `work-units/wip/local_091/PRODUCTION_PROCESS_BACKEND_TABLE_BOUNDARY.md`
- `work-units/wip/local_092/PRODUCTION_SHUTDOWN_BOUNDARY.md`
- `docs/status.md`
- `tests/cmake/compile_include_boundaries_test.cmake`

## 6. 根拠監査

not applicable.

This work unit is header/API surface cleanup for internal daemon code. It must not change protocol bytes, BTstack source selection, report timing, or WinUSB/libusb behavior.

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy after
- reason: header shrinking is safest after the production concerns have moved to their owner modules.
- verification: include boundary probes, production runner tests, architecture journey, and build gates should pass.

Completion principle:

- Do not keep test-only API as production API.
- Do not keep obsolete compatibility wrappers unless a current caller requires them.
- If full opaque runner would create more complexity than it removes, leave the state visible only with an explicit reason and a narrower follow-up decision.

## 8. 対象ファイル

- `swbt/daemon/production_runner.h`
- `swbt/daemon/production_runner.c`
- `swbt/daemon/production_*.h`
- `tests/daemon_production_runner_test.c`
- `tests/cmake/compile_include_boundaries_test.cmake`
- `tests/cmake/include_boundaries_test.cmake`
- `CMakeLists.txt`
- `docs/status.md` if public current-state wording changes

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | production runner public header no longer exposes implementation-only includes that moved to helper modules | regression | architecture | no |
| todo | production runner tests compile against the narrowed internal API without test-only production wrappers | regression | build/unit | no |
| todo | obsolete hardware approval helper is either removed from source/tests or recorded with a current caller and reason | characterization | review/unit | no |
| todo | daemon process, CLI, and architecture journey tests still pass after header finalization | regression | integration | no |

## 10. 検証

not run yet.

Expected checks:

- `just build-debug`
- `$env:CTEST_ARGS='-R "daemon_production_runner_test|daemon_process_test|daemon_cli_test|architecture_journey_test|include_boundaries_test|compile_include_boundaries_test" --output-on-failure'; just test-debug`
- `just windows-cross`
- `just verify` before moving the decomposition series to complete, if time and environment allow.

## 11. 実機実行条件

実機実行は不要.

This work unit narrows internal header exposure and removes obsolete API only. It must not change hardware-facing behavior.

## 12. 先送り事項

none at creation.

If full opaque state is rejected during implementation, record the reason in this work unit and create a separate follow-up only when there is a concrete blocker. Do not leave a vague "opaque later" item.

## 13. チェックリスト

- [ ] production runner header surface を再評価した。
- [ ] obsolete API の削除または残存理由を記録した。
- [ ] include boundary tests を更新または確認した。
- [ ] TDD Test List の検証を実行し、結果を記録した。
- [ ] 実機未実行理由を維持した。
