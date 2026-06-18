# BTstack Periodic Input Report Core Source Audit

## 1. 状態

recorded。

この reference は Phase 4 の periodic input report scheduler core で使う `0x30` input report、report period、BTstack send/timer 境界の根拠監査である。

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
| default report period | `8000us` | design policy with upstream background | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md:145-180`, `CMakeLists.txt` `SWBT_REPORT_PERIOD_US` | configurable default; not hardware-proven |
| scheduler drift handling | `next_deadline += period`, reset from now when multiple deadlines were missed | design policy | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md:661-708` | stable for scheduler core |
| BTstack run loop timer API | set handler, set timeout, add timer | source fact | `vendor/btstack/src/btstack_run_loop.c:193-285` | stable BTstack API at pinned submodule |
| BTstack HID interrupt send API | request can-send event, send interrupt message | source fact | `vendor/btstack/src/classic/hid_device.h:130-143`; `vendor/btstack/example/hid_keyboard_demo.c:246-251,408-414` | stable BTstack API at pinned submodule |

## 4. 未解決事項

- この work unit は BTstack timer registration と `hid_device_request_can_send_now_event` の production adapter を実装しない。
- 実機 Switch での report period acceptability と jitter tolerance は未検証である。
