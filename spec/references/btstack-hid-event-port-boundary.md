# BTstack HID Event Port Boundary Source Audit

## 1. 状態

recorded。

この reference は `local_053` で導入した BTstack HID event decoder、HID send port、timer port の根拠監査である。

この文書は Switch-facing report bytes、descriptor、GAP / SSP identity、report period の新規値を追加しない。既存 BTstack API と event layout を swbt の typed event / port wrapper へ写すための根拠を記録する。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| BTstack pinned submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | `vendor/btstack/src/bluetooth.h`, `vendor/btstack/src/btstack_defines.h`, `vendor/btstack/src/btstack_event.h`, `vendor/btstack/src/classic/hid_device.h`, `vendor/btstack/src/btstack_run_loop.h`, `vendor/btstack/src/btstack_run_loop.c` |
| swbt periodic report audit | current repository | `spec/references/btstack-periodic-input-report-core.md` |
| swbt runtime boundary draft | current repository | `spec/architecture/daemon-application-boundary-rearchitecture.md` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| HCI event packet type | `0x04` | source fact | `vendor/btstack/src/bluetooth.h:183` | stable BTstack constant |
| user confirmation event | `0x33` | source fact | `vendor/btstack/src/btstack_defines.h:536` | stable BTstack constant |
| HID meta event | `0xEF` | source fact | `vendor/btstack/src/btstack_defines.h:2123` | stable BTstack constant |
| HID connection opened subevent | `0x02` | source fact | `vendor/btstack/src/btstack_defines.h:4203` | stable BTstack constant |
| HID connection closed subevent | `0x03` | source fact | `vendor/btstack/src/btstack_defines.h:4210` | stable BTstack constant |
| HID can-send subevent | `0x04` | source fact | `vendor/btstack/src/btstack_defines.h:4217` | stable BTstack constant |
| user confirmation address offset | `event[2..7]`, reversed into `bd_addr` | source fact | `vendor/btstack/src/btstack_event.h:1307-1308` | decoder copies the same bytes without linking BTstack helper objects |
| opened HID CID and status offsets | CID at `event[3..4]`, status at `event[5]` | source fact | `vendor/btstack/src/btstack_event.h:15077-15087` | decoder keeps prior production behavior and requires full opened layout through `event[14]` |
| opened event full layout reaches incoming field | incoming at `event[14]` | source fact | `vendor/btstack/src/btstack_event.h:15113-15114` | short opened packets are ignored |
| closed HID CID offset | CID at `event[3..4]` | source fact | `vendor/btstack/src/btstack_event.h:15123-15124` | typed event payload stores HID CID only |
| can-send HID CID offset | CID at `event[3..4]` | source fact | `vendor/btstack/src/btstack_event.h:15133-15134` | typed event payload stores HID CID only |
| HID can-send request API | `hid_device_request_can_send_now_event(hid_cid)` | source fact | `vendor/btstack/src/classic/hid_device.h:133-137` | wrapped by `swbt_btstack_hid_port_request_can_send_now` |
| HID interrupt send API | `hid_device_send_interrupt_message(hid_cid, message, message_len)` | source fact | `vendor/btstack/src/classic/hid_device.h:139-143` | wrapped by `swbt_btstack_hid_port_send_report` |
| run loop timer API | set handler, set context, set timeout, add timer, remove timer, get time ms | source fact | `vendor/btstack/src/btstack_run_loop.h:228-259`, `vendor/btstack/src/btstack_run_loop.c:193-296` | wrapped by `swbt_btstack_timer_port_*` |

## 4. 実装反映

- `swbt/btstack_bridge/hid_event.*` は BTstack raw packet を swbt の typed event へ変換する。public header は BTstack vendor header を公開しない。
- `swbt/btstack_bridge/hid_port.*` は HID can-send request と interrupt send を薄く包む。report bytes は変更しない。
- `swbt/btstack_bridge/timer_port.*` は BTstack run loop timer API を薄く包む。`btstack_timer_source_t` を扱うため `timer_port.h` は BTstack header を含むが、これは `btstack_bridge` 内の port であり application public API ではない。
- `swbt/btstack_bridge/input_report_timer_adapter.c` は HID send と timer の直接 BTstack call を port 経由にした。
- `swbt/daemon/production_backend.c` は raw packet offset 判定を `swbt_btstack_hid_event_decode` へ移した。

## 5. 未解決事項

- `swbt/btstack_bridge/production_btstack.c` はまだ BTstack run loop 上の IPC pump と `swbt_daemon_ipc_runner_t` を直接扱う。これは `local_054` の host / build boundary と `local_055` の cutover cleanup で棚卸しする。
- production 実機経路の callback registration、report scheduling、shutdown order は今回の software gate では再実行していない。production composition を切り替える PR で hardware gate を判定する。
