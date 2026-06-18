# IPC Heartbeat Core

## 1. 概要

Phase 3: daemon IPC の heartbeat / timeout neutral を TCP connection に紐づける work unit。

client connection ごとに heartbeat deadline を管理し、timeout 時に owner の latest state を neutral に戻す。

## 2. 対象範囲

- connection に heartbeat timeout 設定を保持する。
- message 受信時に heartbeat を更新する。
- timeout check API で期限切れを検出する。
- owner connection の heartbeat timeout で neutral state に戻す。
- Phase 3 TODO の heartbeat / timeout neutral を完了状態にする。

## 3. 対象外

- real timer thread。
- ping command の JSON protocol 追加。
- multi-client event broadcast。
- daemon CLI option との接続。
- BTstack thread との queue / mailbox。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `work-units/complete/local_008/IPC_SESSION_CORE.md`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`

## 5. 根拠監査

not applicable。

この work unit は IPC connection health と internal owner/state 管理であり、Switch HID protocol、BTstack source selection、report timing、backend facts を追加しない。

## 6. 設計メモ

- timeout 判定は caller から渡される monotonic milliseconds に基づく。
- heartbeat enabled connection は message 受信時に `last_heartbeat_ms` を更新する。
- timeout 時は `swbt_ipc_heartbeat_timeout` を適用し、connection は閉じずに error を返す。

## 7. 対象ファイル

- `swbt/ipc/ipc_server.h`
- `swbt/ipc/ipc_server.c`
- `tests/ipc_server_test.c`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | heartbeat before deadline keeps owner state | new | integration | no |
| refactor-done | heartbeat timeout clears owner and returns neutral state | edge | integration | no |

## 9. 検証

- red: `make debug CTEST_ARGS="-R ipc_server_test"` は missing heartbeat API のため compile で失敗した。
- green: `make debug CTEST_ARGS="-R ipc_server_test"` は 1/1 passed。
- standard verification: `make verify` は pass。
  - format-check pass。
  - clang-tidy preset build pass。
  - linux-debug CTest 9/9 passed。
  - linux-asan CTest 9/9 passed。
  - windows-mingw-debug cross build pass。

## 10. 実機実行条件

実機検証は不要。

この work unit は IPC connection health の integration test のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 11. チェックリスト

- [x] red を確認した。
- [x] IPC heartbeat core を追加した。
- [x] IPC heartbeat integration test を追加した。
- [x] `make debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] 実機状態を記録した。
