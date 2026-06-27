# Production HID Session Boundary

## 1. 概要

BTstack の context-less HID packet handler と HID connection event dispatch を `production_runner` から分離する。

完了後、`g_active_backend` 相当、HID device open / close、connection opened / closed、CAN_SEND_NOW dispatch は `production_hid_session.*` で読む。runner は HID session の詳細を持たない。

## 2. 起点 / ユースケース

source:

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- `spec/references/btstack-daemon-entrypoint.md`: BTstack HID packet handler は user context を持たないため、現行 scope では単一 active backend pointer を持つ。
- 現状分析, 2026-06-28: `production_runner.c` は `g_active_backend`、packet handler、HID open / close、connection events を同じ file に持つ。

use case:

- actor: HID event dispatch または BTstack callback integration を変更する開発者。
- 入力または状態: BTstack callback は user context を渡さず、production runner が global active backend を持っている。
- 期待する観測結果: context-less callback 制約が HID session module に閉じ、runner lifecycle と読み分けられる。
- 制約: HID registration config、event decode path、timer start / stop order、SSP confirmation behavior は変えない。

source から use case への変換:

HID packet handling は BTstack-facing constraint と daemon state update の接点であり、runner lifecycle とは別責務である。既存 fake packet handler tests で order を固定したまま分離する。

## 3. 対象範囲

- `production_hid_session.*` 相当を追加する。
- global active backend pointer 相当を HID session module に閉じる。
- HID register / stop callback を移す。
- connection opened success で report timer start と learned address save を呼ぶ behavior を維持する。
- CAN_SEND_NOW で report timer on-can-send と shutdown pending completion を呼ぶ behavior を維持する。
- connection closed で report timer stop と pending shutdown completion を呼ぶ behavior を維持する。
- SSP confirmation request behavior を維持する。

## 4. 対象外

- BTstack HID event decoder の変更。
- HID registration descriptor / config values の変更。
- active reconnect address conversion の変更。
- shutdown policy の変更。
- 実機検証。

## 5. 関連 spec / docs

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- `spec/references/btstack-daemon-entrypoint.md`
- `spec/references/btstack-hid-event-port-boundary.md`
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`
- `work-units/complete/local_079/DEVICE_LIFECYCLE_API.md`
- `tests/daemon_production_runner_test.c`

## 6. 根拠監査

not applicable if event constants, offsets, HID registration config, and BTstack source selection remain unchanged.

If event decode constants, HID registration values, or Switch-facing behavior change, use `source-audit`.

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: BTstack callback constraint is cohesive and currently hides runner lifecycle.
- verification: HID packet handler tests should pass unchanged, including timer start / send / stop and SSP confirmation.

配置方針:

- `production_hid_session.*` belongs under `swbt/daemon` because it coordinates daemon production state, report timer, reconnect persistence, and shutdown pending completion.
- It may use `btstack_bridge/device.h` and `btstack_bridge/hid_event.h`.
- It should not call real BTstack implementation directly.

## 8. 対象ファイル

- `swbt/daemon/production_hid_session.*`
- `swbt/daemon/production_runner.c`
- `swbt/daemon/production_reconnect.*`
- `swbt/daemon/production_report_timer.*`
- `swbt/daemon/production_shutdown.*`
- `tests/daemon_production_runner_test.c`
- `CMakeLists.txt`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | HID packet handler still starts timer after connection opened success and preserves captured HID CID | regression | integration | no |
| todo | HID CAN_SEND_NOW still calls report timer can-send and completes pending shutdown on success or failure | regression | integration | no |
| todo | HID connection closed still stops report timer and completes pending shutdown if needed | regression | integration | no |
| todo | SSP user confirmation event still calls the controller confirmation port with decoded address | regression | integration | no |
| todo | HID device open / close still preserve platform / HID registration cleanup behavior through device API | regression | integration | no |

## 10. 検証

not run yet.

Expected checks:

- `just build-debug`
- `$env:CTEST_ARGS='-R "daemon_production_runner_test|btstack_hid_event_test|btstack_device_test" --output-on-failure'; just test-debug`
- `just windows-cross`

## 11. 実機実行条件

実機実行は不要 if callback order and HID values remain unchanged.

If this work changes HID registration config, event decode constants, or BTstack source selection, stop and re-scope with `source-audit` and `hardware-harness`.

## 12. 先送り事項

none.

Future listener registration or queueing policy is not part of this structure change and is not created as follow-up here.

## 13. チェックリスト

- [ ] HID session module を追加した。
- [ ] global active backend pointer 相当を runner から移した。
- [ ] HID event dispatch behavior を維持した。
- [ ] TDD Test List の検証を実行し、結果を記録した。
- [ ] 実機未実行理由を維持した。
