# IPC Adapter Command Codec Boundary

## 1. 概要

IPC JSON codec と socket adapter を application command API へ接続し、JSON decode / encode を副作用のない境界へ寄せる work unit。

この work unit の完了後、IPC path は transport、line framing、client connection ID、command / response encode decode を担当する。owner policy、neutral 化、controller state update は application command API が担当する。

完了時点では `swbt/ipc/ipc_json.*` が typed command / response codec、`swbt/ipc/ipc_adapter.*` が codec と `swbt_ipc_session_t` / application command API の接続点になる。`swbt_ipc_session_t` は lock、mailbox publish、rumble status を持つ互換 context として残すが、JSON codec からは参照しない。

## 2. 起点 / ユースケース

source:

- `spec/architecture/daemon-application-boundary-rearchitecture.md` の IPC adapter 方針。
- `work-units/complete/local_051/DAEMON_APPLICATION_COMMAND_API.md` の後続 work unit。
- `spec/protocols/daemon-ipc-v1.md` の existing wire contract。

use case:

- actor: local IPC client、debug client、IPC adapter。
- 入力または状態: JSON Lines の `hello`、`acquire`、`release`、`set_state`、`get_status`、malformed input、partial read、client disconnect。
- 期待する観測結果: codec は typed command / response を parse / serialize し、application state を直接変更しない。既存 wire response は互換である。
- 制約: wire protocol v1 を破壊しない。status protocol v1 の拡張は `local_039` に分ける。
- 対象外: BTstack adapter、daemon host lifecycle、実機検証。

source から use case への変換:

roadmap の IPC hardening 全体ではなく、まず codec / adapter の責務分離に限定する。fuzz や Windows native CI は後続の IPC / platform hardening work unit の source として残す。

## 3. 対象範囲

- JSON parse を typed command 生成に分離する。
- JSON serialize を typed response 生成に分離する。
- malformed input が application state を変更しないことを test で固定する。
- IPC server / runner が `ipc_adapter` 経由で command / disconnect / heartbeat / shutdown event を扱う形へ移す。
- existing `daemon-ipc-v1` response 互換性を test で固定する。
- `ipc_session` compatibility wrapper の残り責務を lock、mailbox publish、rumble status に縮小した状態として記録する。

## 4. 対象外

- status protocol v1 の新 field。
- subscription / event stream。
- authentication token。
- fuzz infrastructure。
- Windows native CI。
- 実機検証。

## 5. 関連 spec / docs

- `spec/architecture/daemon-application-boundary-rearchitecture.md`
- `spec/protocols/daemon-ipc-v1.md`
- `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`
- `work-units/complete/local_051/DAEMON_APPLICATION_COMMAND_API.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`
- `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`

## 6. 根拠監査

not applicable。

この work unit は IPC JSON と local socket boundary を扱う。Switch HID report bytes、BTstack source selection、report period、WinUSB/libusb facts を追加しない。

## 7. 設計メモ

- codec function は application state pointer を受け取らない。
- client ID と owner ID の関係は command metadata と application authorization で扱う。
- `get_status` は application snapshot を response serializer に渡す。
- existing debug client は互換性確認の対象にする。
- `swbt_ipc_json_decode_command` は `swbt_ipc_command_t` または typed error response を返す。session、application、mailbox は受け取らない。
- `swbt_ipc_json_encode_response` は `swbt_ipc_response_t` だけから wire JSON を生成する。session、application、socket は受け取らない。
- `swbt_ipc_adapter_handle_line` が decode、application/session dispatch、encode を接続する。
- disconnect、heartbeat timeout、shutdown は `ipc_adapter` event entrypoint から session wrapper へ渡す。socket transport は owner policy を直接選ばない。

## 8. 対象ファイル

- `swbt/ipc/ipc_json.*`
- `swbt/ipc/ipc_adapter.*`
- `swbt/ipc/ipc_server.*`
- `swbt/daemon/ipc_runner.*`
- `tests/ipc_json_test.c`
- `tests/ipc_server_test.c`
- `tests/daemon_ipc_runner_test.c`
- `tests/debug_ipc_client_test.c`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | JSON parse returns typed set-state command without mutating application state | new | unit | no |
| refactor-done | malformed JSON returns typed error response and leaves application state unchanged | regression | unit | no |
| green | `state` object scope is enforced and top-level controller fields are rejected | regression | unit | no |
| green | existing acquire / set_state / get_status / release responses remain wire-compatible | regression | unit | no |
| refactor-done | client disconnect emits adapter event instead of calling owner policy in IPC transport | new | integration | no |
| green | debug client journey works through IPC adapter and application command API | regression | integration | no |

## 10. 検証

TDD red:

```text
just build-debug
# failed as expected: swbt_ipc_command_t / swbt_ipc_response_t / swbt_ipc_json_decode_command 未定義

just build-debug
# failed as expected: swbt_ipc_json_encode_response 未定義

just test-debug
# failed as expected: ipc_json_test が state object scope regression で失敗
```

TDD green / refactor:

```text
just build-debug
# pass

just test-debug
# pass, 34/34 tests

just format
# pass

just verify
# pass: format-check, clang-tidy, debug build/test, asan build/test, Windows MinGW cross build
```

補足:

- PowerShell から `$env:CTEST_ARGS='-R ipc_json_test --output-on-failure'; just test-debug` を実行した 2 回は、テスト実行前の Dev Container CLI container lookup で失敗した。同じ状態で `just test-debug` は通っており、テスト失敗として扱わない。
- 初回 `just verify` は `snprintf` に対する clang-tidy `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling` で失敗した。固定長文字列コピーを手書きの境界付きコピーに置き換え、再実行した `just verify` は通過した。

## 11. 実機実行条件

実機は不要である。loopback IPC と fake application / runtime で閉じる。

## 12. 先送り事項

- 観測: parser fuzz、slow client、Windows native CI は有用である。
  先送り理由: codec boundary を先に固定しないと、fuzz target と CI smoke の入口が安定しない。
  次の置き場: rearchitecture 後の IPC / platform hardening work unit。
- 観測: `swbt_ipc_session_t` は lock、mailbox publish、rumble status の compatibility context として残る。
  先送り理由: `local_052` で JSON codec と adapter の責務は分離したが、session context の完全削除は runtime / BTstack / CMake boundary と同時に扱う必要がある。
  次の置き場: `work-units/wip/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`。

## 13. チェックリスト

- [x] command / response 型を確認した。
- [x] red test を追加した。
- [x] JSON parse / serialize を副作用なしにした。
- [x] IPC adapter が application command API を呼ぶ。
- [x] existing wire compatibility を確認した。
- [x] targeted CTest を実行した。
- [x] 残った IPC compatibility wrapper の削除条件を更新した。
