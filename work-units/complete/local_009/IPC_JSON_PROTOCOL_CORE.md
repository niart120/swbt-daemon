# IPC JSON Protocol Core

## 1. 概要

Phase 3: daemon IPC の JSON Lines command/response core を実装する work unit。

TCP loopback server へ接続する前に、1 行 JSON object を IPC session core に適用し、`hello`、`acquire` / `release`、`set_state`、`get_status` の応答を unit test で固定する。

## 2. 対象範囲

- JSON Lines 1 message を command として処理する。
- protocol version `v:1` を受け付け、未対応 version を error response にする。
- `hello` へ client ID を含む応答を返す。
- `acquire` / `release` を `swbt_ipc_session_t` に適用する。
- owner のみ `set_state` を許可する。
- `get_status` で owner と latest state の JSON response を返す。
- malformed JSON と unsupported command を error response にする。

## 3. 対象外

- TCP loopback listen / accept / read / write。
- authentication token。
- subscribe / event delivery。
- daemon metrics、Switch connection status、rumble status の実データ。
- BTstack thread との queue / mailbox。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `work-units/complete/local_008/IPC_SESSION_CORE.md`

## 5. 根拠監査

not applicable。

この work unit は daemon IPC の JSON command/response と内部 owner/state 管理であり、Switch HID protocol、BTstack source selection、report timing、backend facts を追加しない。

## 6. 設計メモ

- `owner_id` は TCP connection adapter が割り当てる `client_id` を 8 桁 lowercase hex string として表現する。
- `set_state` は full snapshot のみ扱う。partial update や時間指定 command は扱わない。
- `set_state` の ack は `request_id` がある場合に返す。
- JSON parser は v0 command subset に閉じる。外部 dependency は追加しない。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/ipc/ipc_json.h`
- `swbt/ipc/ipc_json.c`
- `tests/ipc_json_test.c`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | hello returns protocol response with client ID | new | unit | no |
| refactor-done | acquire returns owner ID and owner_busy for second client | new | unit | no |
| refactor-done | only owner can set_state from JSON state snapshot | new | unit | no |
| refactor-done | invalid state returns error and keeps latest state unchanged | edge | unit | no |
| refactor-done | get_status returns owner and latest state | new | unit | no |
| refactor-done | release clears owner and neutral state | edge | unit | no |
| refactor-done | invalid version and malformed JSON return error response | edge | unit | no |

## 9. 検証

- red: `make debug CTEST_ARGS="-R ipc_json_test"` は `swbt/ipc/ipc_json.c` 未追加のため CMake configure で失敗した。
- green: `make debug CTEST_ARGS="-R ipc_json_test"` は 1/1 passed。
- refactor: invalid state regression test 追加後の `make debug CTEST_ARGS="-R ipc_json_test"` は 1/1 passed。
- standard verification: `make verify` は pass。
  - format-check pass。
  - clang-tidy preset build pass。
  - linux-debug CTest 8/8 passed。
  - linux-asan CTest 8/8 passed。
  - windows-mingw-debug cross build pass。

## 10. 実機実行条件

実機検証は不要。

この work unit は IPC JSON command/response の unit test のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 11. チェックリスト

- [x] red を確認した。
- [x] IPC JSON protocol core を追加した。
- [x] IPC JSON unit test を追加した。
- [x] `make debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] 実機状態を記録した。
