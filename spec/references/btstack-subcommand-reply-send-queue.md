# BTstack Subcommand Reply Send Queue Source Audit

## 1. 状態

recorded。

この reference は、Subcommand reply report を periodic input report より優先して送るための bounded queue core の根拠監査である。
queue は already-built report bytes を保持し、report payload を解釈しない。

## 2. 参照元

| source | path |
|---|---|
| swbt daemon IPC design | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` |
| Switch subcommand reply core audit | `spec/references/switch-subcommand-reply-core.md` |
| Periodic input report core audit | `spec/references/btstack-periodic-input-report-core.md` |
| swbt subcommand reply builder | `swbt/switch/switch_subcommand_reply.h` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| subcommand reply report size | `50` bytes | implementation contract via existing audit | `spec/references/switch-subcommand-reply-core.md` | stable queue max size |
| subcommand reply priority | `0x21` reply should not be dropped behind periodic `0x30` | design policy | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md:688-698` | stable queue policy; hardware not proven |
| periodic report drop tolerance | periodic `0x30` may slip by one tick before dropping a reply | design policy | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md:697-708` | adapter policy deferred |
| send callback signature | `context`, `hid_cid`, `report`, `report_size` | implementation contract | `swbt/btstack_bridge/input_report_scheduler.h` | reused shape; no BTstack header dependency |

## 4. 実装上の扱い

`swbt_btstack_subcommand_reply_queue_enqueue` は report bytes を queue item に copy する。
caller buffer の lifetime には依存しない。

`swbt_btstack_subcommand_reply_queue_send_next` は head item だけを send callback に渡す。
send callback が失敗した場合、head item は queue に残し、次の呼び出しで同じ item を retry できる。

queue core は BTstack header に依存しない。
BTstack の `hid_device_request_can_send_now_event` と `hid_device_send_interrupt_message` への接続は、`work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md` で `swbt_btstack_input_report_timer_adapter` に統合した。
production wrapper は BTstack の void API を success として扱う。fake backend test は send failure を注入し、queue retry policy を検証する。

## 5. 未解決事項

- Switch 実機で `0x21` reply を優先したときの acceptability は未検証である。
- production daemon entrypoint から実 BTstack backend と IPC runner を起動する接続は `local_042` と `local_043` で扱う。
