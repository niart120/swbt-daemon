# Rumble Status Exposure

## 1. 概要

local_007 で raw rumble state まで実装した内容を daemon status へ露出するための計画 record。

Switch output report から得た raw rumble payload を controller input state と分離したまま保持し、IPC `get_status` で確認できるようにする。

この work unit は rumble effect interpretation を扱わず、実機での rumble effect と callback path は未検証として扱う。

## 2. 対象範囲

- IPC session または daemon status state に `swbt_switch_rumble_state_t` を保持する。
- parsed output report の rumble payload を status 用 raw rumble state に反映する software path を追加する。
- `get_status` response に `updated`、`last_update_ms`、raw hex を含む rumble status を追加する。
- owner input state と rumble state を混ぜないことを unit test で固定する。
- neutral rumble payload の受信を raw status として表現する。

## 3. 対象外

- rumble frequency と amplitude の semantic decode。
- actuator-safe amplitude conversion。
- rumble output scheduling。
- subscribe event delivery。
- 実機での rumble effect 確認。
- 実機 Switch が DATA callback と SET_REPORT callback のどちらで output report を渡すかの断定。
- `vendor/btstack` の変更。

## 4. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-rumble-core.md`
- `spec/references/btstack-output-report-parser-bridge.md`
- `work-units/complete/local_007/SWITCH_RUMBLE_CORE.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_013/BTSTACK_OUTPUT_REPORT_PARSER_BRIDGE.md`

## 5. 根拠監査

raw rumble payload size、neutral payload、raw state separation は `spec/references/switch-rumble-core.md` に記録済みである。

BTstack report data callback shape は `spec/references/btstack-output-report-parser-bridge.md` に記録済みである。

実機 Switch の output report transport path、rumble effect、callback-to-status runtime 挙動は未検証である。

SET_REPORT callback facts または hardware callback facts を追加する場合は、`source-audit` を先に使う。

| 項目 | 状態 | 扱い |
|---|---|---|
| raw rumble payload size | recorded | `spec/references/switch-rumble-core.md` の根拠を使う。 |
| neutral rumble payload | recorded | neutral 判定の実装根拠として扱う。 |
| BTstack report data callback shape | recorded | pinned BTstack source の callback shape として扱う。 |
| actual Switch callback path | pending | 実機未検証であり、DATA と SET_REPORT の経路を断定しない。 |
| rumble effect interpretation | out of scope | frequency、amplitude、actuator behavior は扱わない。 |

## 6. 設計メモ

- rumble status は daemon-observed output report state であり、controller input state ではない。
- status は raw bytes と update timestamp だけを露出する。
- timestamp は caller-provided monotonic milliseconds とし、hardware arrival latency の事実として扱わない。
- owner disconnect の neutral input fallback は rumble raw facts を上書きしない。
- BTstack thread から IPC client callback を直接呼ばず、daemon-owned state へ反映する境界を保つ。

## 7. 対象ファイル

- `swbt/switch/switch_rumble.h`
- `swbt/switch/switch_rumble.c`
- `swbt/ipc/ipc_session.h`
- `swbt/ipc/ipc_session.c`
- `swbt/ipc/ipc_json.c`
- `swbt/btstack_bridge/output_report_handler.h`
- `swbt/btstack_bridge/output_report_handler.c`
- `tests/switch_rumble_test.c`
- `tests/ipc_session_test.c`
- `tests/ipc_json_test.c`
- `tests/btstack_output_report_handler_test.c`
- `spec/references/switch-rumble-status-exposure.md`
- `work-units/wip/local_029/RUMBLE_STATUS_EXPOSURE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | IPC status initializes rumble as `updated=false` with neutral raw payload | new | unit | no |
| todo | recording active raw rumble payload updates raw bytes and timestamp separately from controller state | new | unit | no |
| todo | `get_status` serializes rumble updated flag, timestamp, and raw hex without effect decode | new | unit | no |
| todo | parsed `0x01` and `0x10` output reports can feed rumble status in software tests | new | integration | no |
| todo | invalid or short output report does not change rumble status | edge | unit | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、`make debug`、targeted CTest、`make asan`、`make windows-cross`、実機コマンドは実行していない。

完了時には rumble status の targeted unit test、IPC JSON regression、影響範囲に応じた sanitizer または cross build を記録する。

実機で rumble callback が status に反映されることは、この software verification だけでは証明しない。

## 10. 実機実行条件

software unit test だけなら実機検証は不要である。

実機で rumble callback と status 反映を確認する場合は、人間が pairing、HID advertising、report loop、rumble output observation の範囲を明示承認する。

実機確認では専用 USB Bluetooth ドングルを使い、内蔵 Bluetooth と常用ドングルは使わない。

Windows native では Zadig による WinUSB assignment、USB VID/PID、driver state を記録する。

実行時は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。

結果は `docs/hardware-test-log.md` に OS、dongle、driver、backend、BTstack commit、swbt commit、Switch firmware、report period、rumble result、cleanup を記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] rumble status exposure を実装した。
- [ ] TDD テストを追加した。
- [ ] 根拠監査を完了した。
- [ ] `make debug` を実行した。
- [ ] sanitizer または cross build の結果を記録した。
- [ ] 実機状態または未実行理由を記録した。
