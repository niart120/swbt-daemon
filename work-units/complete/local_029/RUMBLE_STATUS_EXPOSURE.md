# Rumble Status Exposure

## 1. 概要

local_007 で raw rumble state まで実装した内容を daemon status へ露出する work unit。

Switch output report から得た raw rumble payload を controller input state と分離したまま保持し、IPC `get_status` で確認できるようにする。

この work unit は rumble effect interpretation を扱わず、実機での rumble effect と callback path は未検証として扱う。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_007/SWITCH_RUMBLE_CORE.md` は raw rumble state と neutral payload を追加した。
- `work-units/complete/local_013/BTSTACK_OUTPUT_REPORT_PARSER_BRIDGE.md` は BTstack output report callback から `swbt_switch_output_report_t` を得る bridge を追加した。
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md` は `get_status` response を追加した。
- `spec/references/switch-rumble-core.md` は raw rumble payload size、neutral payload、input state との分離を記録している。

use case:

- actor: daemon runtime と IPC client。
- 入力または状態: parsed output report の 8-byte raw rumble payload、caller-provided monotonic milliseconds。
- 期待する観測結果: IPC status は controller input state と別に rumble `updated`、`last_update_ms`、raw hex を返す。owner disconnect は input state を neutral に戻すが、last observed rumble raw status は上書きしない。
- 制約: rumble effect decode、実機 callback path、actual actuator behavior は断定しない。
- 対象外: amplitude / frequency decode、subscribe event delivery、実機 rumble effect 確認。

source から use case への変換:

既存 parser が返す raw rumble bytes を daemon-owned IPC session state に保存し、JSON status へ serialize する。BTstack DATA / SET_REPORT の実機経路は未検証のままにし、software test では output report handler callback から明示的に status 更新関数を呼ぶ。

## 3. 対象範囲

- IPC session または daemon status state に `swbt_switch_rumble_state_t` を保持する。
- parsed output report の rumble payload を status 用 raw rumble state に反映する software path を追加する。
- `get_status` response に `updated`、`last_update_ms`、raw hex を含む rumble status を追加する。
- owner input state と rumble state を混ぜないことを unit test で固定する。
- neutral rumble payload の受信を raw status として表現する。

## 4. 対象外

- rumble frequency と amplitude の semantic decode。
- actuator-safe amplitude conversion。
- rumble output scheduling。
- subscribe event delivery。
- 実機での rumble effect 確認。
- 実機 Switch が DATA callback と SET_REPORT callback のどちらで output report を渡すかの断定。
- `vendor/btstack` の変更。

## 5. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-rumble-core.md`
- `spec/references/switch-rumble-status-exposure.md`
- `spec/references/btstack-output-report-parser-bridge.md`
- `work-units/complete/local_007/SWITCH_RUMBLE_CORE.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_013/BTSTACK_OUTPUT_REPORT_PARSER_BRIDGE.md`

## 6. 根拠監査

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

## 7. 設計メモ

- rumble status は daemon-observed output report state であり、controller input state ではない。
- status は raw bytes と update timestamp だけを露出する。
- timestamp は caller-provided monotonic milliseconds とし、hardware arrival latency の事実として扱わない。
- owner disconnect の neutral input fallback は rumble raw facts を上書きしない。
- BTstack thread から IPC client callback を直接呼ばず、daemon-owned state へ反映する境界を保つ。

## 8. 対象ファイル

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
- `work-units/complete/local_029/RUMBLE_STATUS_EXPOSURE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | IPC status initializes rumble as `updated=false` with neutral raw payload | new | unit | no |
| done | recording active raw rumble payload updates raw bytes and timestamp separately from controller state | new | unit | no |
| done | `get_status` serializes rumble updated flag, timestamp, and raw hex without effect decode | new | unit | no |
| done | parsed `0x01` and `0x10` output reports can feed rumble status in software tests | new | integration | no |
| done | invalid or short output report does not change rumble status | edge | unit | no |

TDD status:

- source: raw rumble state と IPC status / output report handler callback。
- use case: parsed output report の rumble bytes を daemon-owned status と JSON に露出する。
- item: invalid or short output report does not change rumble status。
- state: done。
- commands:
  - red: `just build-debug` は `swbt_ipc_status_t.rumble` と `swbt_ipc_record_rumble` 未実装のため compile 失敗。
  - green: `just debug` pass。CTest 23/23。
  - final: `just verify` pass。format check、clang-tidy、linux debug tests、ASan/UBSan tests、Windows MinGW cross build が通った。

Test desiderata:

- purpose: raw rumble bytes と timestamp を input state と分けて IPC status / JSON に露出し、output report parser callback から software path で記録できることを固定する。
- key trade-offs: deterministic / fast / behavioral を優先した。実機 callback path は predictive ではないため、この work unit では test に入れていない。
- risks: 実機 Switch が DATA callback と SET_REPORT callback のどちらで output report を渡すか、実 rumble effect がどのように観測されるかは未検証である。
- action: 実機由来の callback path と effect は hardware gate 付きの後続 work unit で記録する。

## 10. 検証

実行済み:

- red: `just build-debug` は `swbt_ipc_status_t.rumble` と `swbt_ipc_record_rumble` 未実装のため compile 失敗。
- green: `just debug` pass、CTest 23/23。
- final verify: `just verify` pass。format check、clang-tidy、linux debug tests、ASan/UBSan tests、Windows MinGW cross build が通った。

実機で rumble callback が status に反映されることは、この software verification だけでは証明しない。

## 11. 実機実行条件

software unit test だけなら実機検証は不要である。

実機で rumble callback と status 反映を確認する場合は、人間が pairing、HID advertising、report loop、rumble output observation の範囲を明示承認する。

実機確認では専用 USB Bluetooth ドングルを使い、内蔵 Bluetooth と常用ドングルは使わない。

Windows native では Zadig による WinUSB assignment、USB VID/PID、driver state を記録する。

実行時は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。

結果は `docs/hardware-test-log.md` に OS、dongle、driver、backend、BTstack commit、swbt commit、Switch firmware、report period、rumble result、cleanup を記録する。

この work unit では実機コマンドを実行していない。理由は、software unit test だけで IPC status、JSON serialization、output report parser callback からの記録 helper を検証し、Switch pairing、HID advertising、report loop、rumble output observation を開始していないためである。

## 12. 先送り事項

- 観測: 実機 Switch が DATA callback と SET_REPORT callback のどちらで output report を渡すかは、この work unit では証明しない。
  先送り理由: Switch pairing、HID advertising、report loop、rumble output observation が必要である。
  次の置き場: `docs/hardware-test-log.md` または実機 bring-up work unit。
- 観測: rumble frequency / amplitude decode は raw status exposure と別問題である。
  先送り理由: actuator-safe conversion と実機確認が必要である。
  次の置き場: 後続 rumble semantics work unit。

## 13. チェックリスト

- [x] work unit record を作成した。
- [x] work unit record を新形式へ更新した。
- [x] red を確認した。
- [x] rumble status exposure を実装した。
- [x] TDD テストを追加した。
- [x] 根拠監査を完了した。
- [x] `just` 経由の検証を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] 実機状態または未実行理由を記録した。
