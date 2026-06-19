# BTstack Input Report Timer Adapter

## 1. 概要

periodic input report scheduler core を BTstack timer と HID interrupt send 境界へ接続する adapter を追加した。

既存の scheduler core は BTstack header に依存しないまま維持し、この work unit では adapter 側で BTstack run loop timer、HID CID、can-send 待ち状態を所有する。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md` の未解決事項。
- `spec/references/btstack-periodic-input-report-core.md` の adapter deferred item。

use case:

- 境界: BTstack run loop timer / HID can-send event と scheduler core の間。
- 入力: HID CID、現在時刻、latest state provider。
- 期待する観測結果: timer callback は can-send を要求し、can-send callback は scheduler で 1 report だけ送信して次 timer を設定する。
- 制約: scheduler core の挙動と `0x30` report layout は変更しない。
- 対象外: daemon lifecycle への組み込み、実機 pairing、HID advertising、report loop。

## 3. 対象範囲

- BTstack run loop timer から periodic input report scheduler を起動する adapter を追加する。
- BTstack HID interrupt send API へ report を渡す adapter 境界を追加する。
- can-send event と timer callback の接続順序を fake BTstack unit test で固定する。
- scheduler stop 時に timer と送信待ち状態を止める。
- BTstack timer と can-send API の根拠監査を更新する。

## 4. 対象外

- periodic input report scheduler core の挙動変更。
- `0x30` input report layout の変更。
- Subcommand `0x21` priority queue。
- daemon lifecycle への組み込み。
- Switch pairing、HID advertising、report loop の実機実行。
- report period の実機最適化。

## 5. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/btstack-periodic-input-report-core.md`
- `work-units/complete/local_012/BTSTACK_HID_DEVICE_REGISTRATION.md`
- `work-units/complete/local_013/BTSTACK_OUTPUT_REPORT_PARSER_BRIDGE.md`
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md`
- `docs/hardware-test-log.md`

## 6. 根拠監査

`source-audit` を使い、`spec/references/btstack-periodic-input-report-core.md` を更新した。

BTstack run loop timer API は `set_timer_handler`、`set_timer_context`、`set_timer`、`add_timer`、`remove_timer`、`get_time_ms` を使う。
BTstack HID send 境界は `hid_device_request_can_send_now_event` と `hid_device_send_interrupt_message` を使う。

実機 Switch での report period acceptability と jitter tolerance は未検証である。

## 7. 設計メモ

- scheduler core は引き続き BTstack header に直接依存しない。
- adapter は BTstack timer source、HID CID、send-ready 状態を所有する。
- timer callback では送信せず、`hid_device_request_can_send_now_event` を呼ぶ。
- can-send callback では latest state provider から得た state で 1 report だけ送る。
- stop は pending timer を remove し、pending can-send が後から届いても送信しない。

## 8. 対象ファイル

- `CMakeLists.txt`
- `swbt/btstack_bridge/input_report_timer_adapter.h`
- `swbt/btstack_bridge/input_report_timer_adapter.c`
- `tests/btstack_input_report_timer_adapter_test.c`
- `spec/references/btstack-periodic-input-report-core.md`
- `work-units/complete/local_023/BTSTACK_INPUT_REPORT_TIMER_ADAPTER.md`

## 9. TDD Test List

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | adapter registers a timer and schedules the first due tick with fake BTstack backend | new | unit | no |
| done | timer callback requests can-send instead of sending through an unavailable channel | new | unit | no |
| done | can-send callback sends one scheduler report and schedules the next timer | new | unit | no |
| done | stop cancels pending timer state and prevents later sends | edge | unit | no |
| done | invalid arguments are rejected | edge | unit | no |

## 10. 検証

- red: `just debug` failed as expected because `tests/btstack_input_report_timer_adapter_test.c` included missing `btstack_bridge/input_report_timer_adapter.h`.
- green: `just debug` passed. 21 tests passed, including `btstack_input_report_timer_adapter_test`.
- final verify: `just verify` passed. Format check, clang-tidy, linux debug tests, ASan/UBSan tests, and Windows MinGW cross build passed.

## 11. 実機実行条件

通常の unit test では実機検証は不要である。
fake BTstack backend だけを使い、Bluetooth adapter、Switch pairing、HID advertising、report loop に触れていない。

Switch-facing daemon run、Bluetooth adapter open、HID advertising、pairing、report loop は実機作業として扱う。
実機作業はユーザの明示承認を必要とする。
実機作業は専用 USB Bluetooth ドングルを使う。
実機作業は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定して実行する。
実機結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、driver、BTstack commit、swbt commit、Switch firmware、report period、結果、cleanup を記録する。

## 12. 先送り事項

- daemon lifecycle への組み込み、connection open / close event からの start / stop は `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md` で扱った。実 adapter open と実機 report loop は未検証である。
- 実機 Switch での report period acceptability と jitter tolerance は未検証である。実機承認を得た work unit で `docs/hardware-test-log.md` に記録する。

## 13. チェックリスト

- [x] work unit record を新形式へ更新した。
- [x] red を確認した。
- [x] BTstack input report timer adapter を追加した。
- [x] unit test を追加した。
- [x] 根拠監査を更新した。
- [x] green を確認した。
- [x] `just verify` を実行した。
- [x] 実機未実行理由を記録した。
