# IPC Session Core

## 1. 概要

Phase 3: daemon IPC のうち、JSON Lines server へ接続する前の owner / state session core を実装する work unit。

active owner、latest state、release、disconnect、heartbeat timeout neutral の挙動を unit test で固定する。

## 2. 対象範囲

- IPC session state を neutral で初期化する。
- client ID による exclusive acquire / release を実装する。
- active owner のみ `set_state` を許可する。
- `get_status` 用に owner と latest state を取得する。
- owner disconnect / heartbeat timeout で owner を解除し、neutral state に戻す。
- NULL argument に explicit error を返す。

## 3. 対象外

- JSON Lines parser。
- TCP loopback server。
- authentication token。
- request / response JSON serialization。
- BTstack thread との queue / mailbox。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`

## 5. 根拠監査

not applicable。

この work unit は daemon IPC の内部 owner/state 管理であり、Switch HID protocol、BTstack source selection、report timing、backend facts を追加しない。

## 6. 設計メモ

- `client_id` は TCP connection adapter が後で割り当てる内部 ID として扱う。
- `set_state` は full snapshot を受け取る。partial update や時間指定 command は扱わない。
- owner release / disconnect / heartbeat timeout は neutral state を適用する。
- state validation は JSON parser work unit で外部入力型と合わせて拡張する。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/ipc/ipc_session.h`
- `swbt/ipc/ipc_session.c`
- `tests/ipc_session_test.c`
- `work-units/complete/local_008/IPC_SESSION_CORE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | session init returns neutral state and no owner | new | unit | no |
| refactor-done | acquire succeeds for first owner and rejects second owner | new | unit | no |
| refactor-done | only owner can set_state | new | unit | no |
| refactor-done | release by owner clears owner and returns neutral state | edge | unit | no |
| refactor-done | owner disconnect returns neutral state | edge | unit | no |
| refactor-done | heartbeat timeout returns neutral state | edge | unit | no |

## 9. 検証

- red: `make debug CTEST_ARGS="-R ipc_session_test"` は `swbt/ipc/ipc_session.c` 未追加のため CMake configure で失敗した。
- green attempt: `make debug CTEST_ARGS="-R ipc_session_test"` は `<stddef.h>` 不足で compile 失敗した。
- green: `make debug CTEST_ARGS="-R ipc_session_test"` は 1/1 passed。
- refactor / standard verification: `make verify` は pass。
  - format-check pass。
  - clang-tidy preset build pass。
  - linux-debug CTest 7/7 passed。
  - linux-asan CTest 7/7 passed。
  - windows-mingw-debug cross build pass。

## 10. 実機実行条件

実機検証は不要。

この work unit は IPC session core の unit test のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 11. チェックリスト

- [x] red を確認した。
- [x] IPC session core を追加した。
- [x] IPC session unit test を追加した。
- [x] `make debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] 実機状態を記録した。
