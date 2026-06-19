# BTstack Input Report Timer Adapter

## 1. 概要

periodic input report scheduler core を BTstack timer と HID interrupt send 境界へ接続するための計画 record。

既存の scheduler core は BTstack header に依存しないため、この work unit では production adapter を追加する。

## 2. 対象範囲

- BTstack run loop timer から periodic input report scheduler を起動する adapter を追加する。
- BTstack HID interrupt send API へ report を渡す adapter 境界を追加する。
- can-send event と timer callback の接続順序を fake backend unit test で固定する。
- scheduler stop 時に timer と送信待ち状態を止める。
- BTstack timer と can-send API の根拠監査を実装前に更新する。

## 3. 対象外

- periodic input report scheduler core の挙動変更。
- `0x30` input report layout の変更。
- Subcommand `0x21` priority queue。
- daemon lifecycle への組み込み。
- Switch pairing、HID advertising、report loop の実機実行。
- report period の実機最適化。

## 4. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/btstack-periodic-input-report-core.md`
- `work-units/complete/local_012/BTSTACK_HID_DEVICE_REGISTRATION.md`
- `work-units/complete/local_013/BTSTACK_OUTPUT_REPORT_PARSER_BRIDGE.md`
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md`
- `docs/hardware-test-log.md`

## 5. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| BTstack run loop timer API | 未確定 | 既存 source fact の再確認が必要 | `spec/references/btstack-periodic-input-report-core.md`, `vendor/btstack/src/btstack_run_loop.c` | pending |
| BTstack HID can-send / interrupt send API | 未確定 | 既存 source fact の再確認が必要 | `spec/references/btstack-periodic-input-report-core.md`, `vendor/btstack/src/classic/hid_device.h` | pending |
| report period | configurable default | 既存監査付き design policy | `spec/references/btstack-periodic-input-report-core.md` | not hardware-proven |
| Switch report loop acceptability | 未記録 | 実機観測が必要 | `docs/hardware-test-log.md` | not run |

BTstack timer callback と can-send event の exact ordering は未監査である。

実機 Switch での report period acceptability と jitter tolerance は未検証である。

未監査の BTstack timer 値や backend 挙動を実装の安定事実として扱わない。

## 6. 設計メモ

- scheduler core は引き続き BTstack header に直接依存しない。
- production adapter は BTstack timer source、HID CID、send-ready 状態を所有する。
- timer callback では due 判定と can-send request の責務を分ける案を unit test で固定する。
- can-send callback では最新 state を使って 1 report だけ送る。
- send failure は adapter の戻り値または metrics hook に記録し、挙動は別 work unit で daemon lifecycle に接続する。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/btstack_bridge/input_report_scheduler.h`
- `swbt/btstack_bridge/input_report_scheduler.c`
- `swbt/btstack_bridge/input_report_timer_adapter.h`
- `swbt/btstack_bridge/input_report_timer_adapter.c`
- `tests/btstack_input_report_timer_adapter_test.c`
- `spec/references/btstack-periodic-input-report-core.md`
- `work-units/wip/local_023/BTSTACK_INPUT_REPORT_TIMER_ADAPTER.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | adapter registers a timer and schedules the first due tick with fake BTstack backend | new | unit | no |
| todo | timer callback requests can-send instead of sending through an unavailable channel | new | unit | no |
| todo | can-send callback sends one scheduler report and schedules the next timer | new | unit | no |
| todo | stop cancels pending timer state and prevents later sends | edge | unit | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、実装、build、CTest、実機検証は実行していない。

実装後は `make debug CTEST_ARGS="-R btstack_input_report_timer_adapter_test"` を実行する。

BTstack backend 境界に compile 影響がある場合は `make windows-cross` を実行する。

## 10. 実機実行条件

通常の unit test では実機検証は不要である。

Switch-facing daemon run、Bluetooth adapter open、HID advertising、pairing、report loop は実機作業として扱う。

実機作業はユーザの明示承認を必要とする。

実機作業は専用 USB Bluetooth ドングルを使う。

実機作業は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定して実行する。

実機結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、driver、BTstack commit、swbt commit、Switch firmware、report period、結果、cleanup を記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] red を確認した。
- [ ] BTstack input report timer adapter を追加した。
- [ ] unit test を追加した。
- [ ] 根拠監査を更新した。
- [ ] `make debug` を実行した。
- [ ] sanitizer または cross build の必要性を判断した。
- [ ] 実機状態を記録した。
