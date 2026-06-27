# Production Process Backend Table Boundary

## 1. 概要

`swbt_daemon_process_backend_t` の production callback table を `production_runner` から分離し、各 helper module への接続だけを読む file にする。

完了後、process backend table は IPC pump、HID session、report timer、device info、clock を束ねる adapter として独立する。runner は `swbt_daemon_process_init` に backend と context を渡すだけになる。

## 2. 起点 / ユースケース

source:

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- 現状分析, 2026-06-28: `swbt_daemon_production_process_backend()` と callback 群が runner lifecycle と同じ file にある。
- `swbt/daemon/process.h`: daemon process は backend callback table を通して IPC、HID、output handler、report timer、device info、clock に依存する。

use case:

- actor: daemon process backend を変更する開発者。
- 入力または状態: production callback table と lifecycle orchestration が同じ file にあり、runner の責務が広く見える。
- 期待する観測結果: production process backend table が independent adapter として読める。
- 制約: callback names and behavior can change internally, but daemon process observable behavior must not.

source から use case への変換:

IPC pump、report timer、HID session を個別に分けた後、process backend table はそれらを束ねる thin adapter にできる。この work unit は connection layer だけを閉じる。

## 3. 対象範囲

- `production_process_backend.*` 相当を追加する。
- `swbt_daemon_production_process_backend()` を移す。
- output handler start / stop、read device info、time_ms callback を移す。
- IPC pump、report timer、HID session helper が存在する場合はそれらを呼ぶ。
- daemon process tests and production runner tests の behavior を維持する。

## 4. 対象外

- `swbt_daemon_process_t` の public shape 変更。
- process start / stop order の変更。
- IPC pump、report timer、HID session helper の内部 behavior 変更。
- shutdown lifecycle の変更。
- 実機検証。

## 5. 関連 spec / docs

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- `work-units/complete/local_088/PRODUCTION_IPC_PUMP_BOUNDARY.md`
- `work-units/complete/local_089/PRODUCTION_REPORT_TIMER_BOUNDARY.md`
- `work-units/complete/local_090/PRODUCTION_HID_SESSION_BOUNDARY.md`
- `swbt/daemon/process.h`
- `tests/daemon_process_test.c`
- `tests/daemon_production_runner_test.c`

## 6. 根拠監査

not applicable.

This is an internal adapter table placement change. It does not alter Switch protocol, BTstack source selection, report timing, or WinUSB/libusb behavior.

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: process backend table is the seam between daemon process and production adapters; isolating it leaves runner as lifecycle orchestration.
- verification: daemon process and production runner tests should pass unchanged.

配置方針:

- `production_process_backend.*` belongs under `swbt/daemon`.
- It may depend on daemon process types and production helper modules.
- It should not depend on `swbt/btstack_bridge/production_btstack_impl.h`.

## 8. 対象ファイル

- `swbt/daemon/production_process_backend.*`
- `swbt/daemon/production_runner.{c,h}`
- `tests/daemon_production_process_backend_test.c`
- `tests/daemon_process_test.c`
- `tests/daemon_production_runner_test.c`
- `CMakeLists.txt`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | production process backend table still exposes production daemon backend status | regression | integration | no |
| done | output handler start / stop still route through production ports | regression | integration | no |
| done | read device info still combines configured device info with controller address port | regression | integration | no |
| done | time_ms callback still delegates to production clock port | regression | unit/integration | no |
| done | daemon process startup and shutdown still call backend callbacks in the existing order | regression | integration | no |

## 10. 検証

Red:

- `just build-debug`: `tests/daemon_production_process_backend_test.c` 追加後、`daemon/production_process_backend.h` 不在で想定通り失敗した。

Green / 完了検証:

- `just build-debug`
- `$env:CTEST_ARGS='-R "daemon_production_process_backend_test|daemon_process_test|daemon_production_runner_test|architecture_journey_test" --output-on-failure'; just test-debug`

結果:

- `just build-debug`: pass.
- `$env:CTEST_ARGS='-R "daemon_production_process_backend_test|daemon_process_test|daemon_production_runner_test|architecture_journey_test" --output-on-failure'; just test-debug`: pass, 4/4.

## 11. 実機実行条件

実機実行は未実行。

理由: callback table と薄い adapter callback の配置だけを変更した。Bluetooth adapter open、HCI power、HID advertising、report loop、Switch-facing packet values は変更していない。

## 12. 先送り事項

なし。

daemon process backend interface 自体を縮小すべきだと分かった場合は、`local_093` の後に別途記録する。この work unit の未完了条件にはしない。

`swbt_daemon_production_runner_finish_shutdown` は HID session の pending shutdown completion から runner lifecycle を呼ぶための既存責務の公開である。shutdown boundary は既存の `local_092`、runner header cleanup は既存の `local_093` で扱うため、この work unit から新しい先送り事項は作らない。

## 13. チェックリスト

- [x] production process backend table を runner から分離した。
- [x] backend callbacks delegate to existing helper modules or ports.
- [x] runner lifecycle no longer contains callback table definition.
- [x] TDD Test List の検証を実行し、結果を記録した。
- [x] 実機未実行理由を維持した。
