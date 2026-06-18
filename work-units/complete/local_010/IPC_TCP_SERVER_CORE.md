# IPC TCP Server Core

## 1. 概要

Phase 3: daemon IPC の local TCP JSON Lines server core を実装する work unit。

`127.0.0.1` だけに bind する TCP listener を用意し、1 client connection から JSON Lines message を読み、`IPC_JSON_PROTOCOL_CORE` を通して response を返す。

## 2. 対象範囲

- TCP loopback listener を初期化する。
- `0.0.0.0` bind を拒否する。
- client connection に内部 `client_id` を割り当てる。
- 1 行 JSON message を読み、IPC JSON protocol core に渡す。
- response がある場合は client socket へ返す。
- owner client disconnect で neutral state に戻す。
- Linux と Windows MinGW cross build で compile できる socket 境界にする。

## 3. 対象外

- 多 client event broadcast。
- non-blocking event loop。
- authentication token。
- heartbeat timeout。
- daemon CLI option との接続。
- BTstack thread との queue / mailbox。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `work-units/complete/local_008/IPC_SESSION_CORE.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`

## 5. 根拠監査

not applicable。

この work unit は local IPC transport と internal owner/state 管理であり、Switch HID protocol、BTstack source selection、report timing、backend facts を追加しない。

## 6. 設計メモ

- v0 は blocking helper として実装し、daemon event loop への統合は後続 work unit に分ける。
- TCP bind は loopback only とし、`0.0.0.0` は explicit error にする。
- accepted connection が owner の場合、read EOF / socket error で `swbt_ipc_disconnect` を適用する。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/ipc/ipc_server.h`
- `swbt/ipc/ipc_server.c`
- `tests/ipc_server_test.c`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | loopback server rejects wildcard bind and accepts loopback bind | new | integration | no |
| refactor-done | client can send hello over TCP JSON Lines and receive response | new | integration | no |
| refactor-done | client can acquire owner and set_state over TCP JSON Lines | new | integration | no |
| refactor-done | get_status over TCP reflects latest owner state | new | integration | no |
| refactor-done | owner disconnect returns neutral state | edge | integration | no |

## 9. 検証

- red: `make debug CTEST_ARGS="-R ipc_server_test"` は `swbt/ipc/ipc_server.c` 未追加のため CMake configure で失敗した。
- green: `make debug CTEST_ARGS="-R ipc_server_test"` は 1/1 passed。
- standard verification: `make verify` は pass。
  - format-check pass。
  - clang-tidy preset build pass。
  - linux-debug CTest 9/9 passed。
  - linux-asan CTest 9/9 passed。
  - windows-mingw-debug cross build pass。

## 10. 実機実行条件

実機検証は不要。

この work unit は loopback TCP socket と IPC JSON command/response の integration test のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 11. チェックリスト

- [x] red を確認した。
- [x] IPC TCP server core を追加した。
- [x] IPC TCP server integration test を追加した。
- [x] `make debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] 実機状態を記録した。
