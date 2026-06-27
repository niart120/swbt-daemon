# Switch Rumble Status Exposure Source Audit

## 1. 状態

recorded。

この reference は raw rumble payload を IPC status に露出する software path の根拠監査である。

## 2. 参照元

| source | path |
|---|---|
| Switch rumble core audit | `spec/references/switch-rumble-core.md` |
| BTstack output report parser bridge audit | `spec/references/btstack-output-report-parser-bridge.md` |
| swbt IPC status implementation | `swbt/domain/domain.*`, `swbt/ipc/ipc_adapter.*`, `swbt/ipc/ipc_json.c` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| raw rumble payload size | `8` bytes | recorded source fact | `spec/references/switch-rumble-core.md` | reused |
| neutral rumble payload | `00 01 40 40 00 01 40 40` | recorded source fact | `spec/references/switch-rumble-core.md` | reused |
| rumble payload position in output report | after output report id and packet counter | recorded source fact | `spec/references/switch-rumble-core.md` | reused |
| BTstack DATA report callback shape | callback with report type, report ID, and payload | recorded BTstack fact | `spec/references/btstack-output-report-parser-bridge.md` | reused |
| IPC rumble status fields | `updated`, `last_update_ms`, raw hex | swbt implementation fact | `tests/domain_command_test.c`, `tests/ipc_json_test.c`, `tests/btstack_output_report_handler_test.c` | tested |

## 4. 未解決事項

- 実機 Switch が DATA callback と SET_REPORT callback のどちらで output report を渡すかは未検証である。
- 実機での rumble effect は未検証である。
- Rumble frequency / amplitude semantic decode は未実装である。
