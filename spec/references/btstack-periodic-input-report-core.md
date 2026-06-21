# BTstack Periodic Input Report Core Source Audit

## 1. 状態

recorded。

この reference は Phase 4 の periodic input report scheduler core と BTstack input report timer adapter で使う `0x30` input report、report period、BTstack send/timer 境界の根拠監査である。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| swbt Switch Report Core audit | current repository | `spec/references/switch-report-core.md` |
| swbt daemon IPC design | current repository | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` |
| BTstack pinned submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | `vendor/btstack/src/btstack_run_loop.c`, `vendor/btstack/src/classic/hid_device.h`, `vendor/btstack/example/hid_keyboard_demo.c` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| periodic input report ID | `0x30` | source fact via existing audit | `spec/references/switch-report-core.md` | stable for implementation |
| standard full report size | `49` bytes, report ID included | implementation contract via existing audit | `spec/references/switch-report-core.md` | stable for implementation |
| default report period | `8000us` | design policy with upstream background; hardware observation in later bring-up | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md:145-180`, `CMakeLists.txt` `SWBT_REPORT_PERIOD_US`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | configurable default; coarse acceptance observed, not optimized |
| scheduler drift handling | `next_deadline += period`, reset from now when multiple deadlines were missed | design policy | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md:661-708` | stable for scheduler core |
| BTstack run loop timer API | set handler, set context, set timeout, add timer, remove timer, get time ms | source fact | `vendor/btstack/src/btstack_run_loop.h:101-259`; `vendor/btstack/src/btstack_run_loop.c:193-294` | stable BTstack API at pinned submodule |
| BTstack HID interrupt send API | request can-send event, send interrupt message | source fact | `vendor/btstack/src/classic/hid_device.h:130-143`; `vendor/btstack/example/hid_keyboard_demo.c:246-251,408-414` | stable BTstack API at pinned submodule |
| input report timer adapter order | timer callback requests can-send; can-send callback sends one scheduler report | implementation contract | `swbt/btstack_bridge/input_report_timer_adapter.c`; `tests/btstack_input_report_timer_adapter_test.c` | unit-tested with fake BTstack backend |

## 4. 未解決事項

- daemon lifecycle への組み込み、connection open / close event からの start / stop は `local_043` と `local_037` で実装・観測済みである。
- `local_037` では `8000 / 8333 / 15000 / 16667 us` の各 run で画面遷移までの粗い受理を観測した。report jitter、入力遅延、取りこぼし率、別 adapter / firmware の tolerance は未検証である。
