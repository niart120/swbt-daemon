# Periodic Input Report Core

## 1. 概要

Phase 4: BTstack bridge の Periodic `0x30` input report を扱う work unit。

BTstack timer / send-ready loop に載せる前段として、周期 tick ごとに最新 `swbt_state_t` から standard full input report `0x30` を生成し、send callback に渡す scheduler core を追加する。

## 2. 対象範囲

- BTstack bridge 配下に periodic input report scheduler core を追加する。
- `0x30` report builder を使って 49-byte input report を生成する。
- report period、次回 deadline、start/stop、late tick の catch-up drop を unit test で固定する。
- send callback failure を error として返し、periodic tick は次 deadline へ進める。
- Phase 4 TODO の Periodic `0x30` input report を完了状態にする。

## 3. 対象外

- BTstack timer source の production registration。
- `hid_device_request_can_send_now_event` / `hid_device_send_interrupt_message` の直接呼び出し。
- Subcommand `0x21` priority queue。
- Switch pairing、HID advertising、report loop 実機実行。
- report period の実機最適化。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/btstack-periodic-input-report-core.md`
- `spec/references/switch-report-core.md`
- `work-units/complete/local_014/SWITCH_SUBCOMMAND_REPLY_CORE.md`

## 5. 根拠監査

### 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| periodic input report ID | `0x30` | source fact via existing audit | `spec/references/switch-report-core.md` | stable for implementation |
| report size | `49` bytes | implementation contract via existing audit | `spec/references/switch-report-core.md` | stable for implementation |
| default report period | `8000us` | design policy with upstream background | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`, `CMakeLists.txt` | configurable default; not hardware-proven |
| drift handling | reset next deadline from `now_us` when late by more than one period | design policy | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md:661-708` | stable for scheduler core |
| BTstack send/timer boundary | timer and interrupt send APIs exist | source fact | `vendor/btstack/src/btstack_run_loop.c`; `vendor/btstack/src/classic/hid_device.h` | adapter deferred |

### 未解決事項

- 実機 Switch での report period acceptability は未検証である。
- BTstack send-ready event との exact integration は後続 work unit に残す。

## 6. 設計メモ

- scheduler core は BTstack header に直接依存しない。production adapter は callback で接続する。
- `start` は `now_us + period` を初回 deadline とし、state 変更時の immediate send はしない。
- late tick は 1 report だけ送信し、追いつくための連続送信はしない。
- `battery_connection` と `vibrator_report` は caller-provided options とし、scheduler では固定値にしない。

## 7. 対象ファイル

- `swbt/btstack_bridge/input_report_scheduler.h`
- `swbt/btstack_bridge/input_report_scheduler.c`
- `tests/btstack_input_report_scheduler_test.c`
- `spec/references/btstack-periodic-input-report-core.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | scheduler sends a standard full `0x30` report only when due | new | unit | no |
| refactor-done | late tick sends one report and resets next deadline without catch-up burst | new | unit | no |
| refactor-done | stopped scheduler, invalid arguments, and send failure are reported explicitly | edge | unit | no |

## 9. 検証

- red: `make debug CTEST_ARGS="-R btstack_input_report_scheduler_test"` は missing `btstack_bridge/input_report_scheduler.h` のため compile で失敗した。
- green: `make debug CTEST_ARGS="-R btstack_input_report_scheduler_test"` は 1/1 passed。
- full debug: `make debug` は 13/13 passed。
- standard verification: `make verify` は pass。
  - format-check pass。
  - clang-tidy preset build pass。
  - linux-debug CTest 13/13 passed。
  - linux-asan CTest 13/13 passed。
  - windows-mingw-debug cross build pass。

## 10. 実機実行条件

実機検証は不要。

この work unit は scheduler core と fake send callback unit test のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 11. チェックリスト

- [x] red を確認した。
- [x] Periodic `0x30` input report scheduler core を追加した。
- [x] unit test を追加した。
- [x] `make debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] 実機状態を記録した。
