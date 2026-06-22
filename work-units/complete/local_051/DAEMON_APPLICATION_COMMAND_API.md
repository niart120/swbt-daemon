# Daemon Application Command API

## 1. 概要

daemon の owner、sequence、controller state、neutral 化の authoritative owner を application command API へ移す work unit。

`local_050` では controller state と command metadata を分け、owner policy と latest sequence を IPC 非依存の `control_lease` へ抽出した。この work unit では、その内側に `swbt_app_t` 相当の application boundary を置き、set-state command handling と neutral 化の authoritative owner を application 側へ進める。

完了後は `swbt_app_t` が owner、last accepted sequence、controller state、revoke policy を所有する。`ipc_session` は lock、mailbox publish、rumble status、IPC 互換 result mapping を残す forwarding wrapper である。

## 2. 起点 / ユースケース

source:

- `spec/architecture/daemon-application-boundary-rearchitecture.md` の roadmap。
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md` の後続 work unit。
- 作業開始時の `ipc_session` が owner、state、rumble、mailbox、neutral 化を持つ implementation fact。

use case:

- actor: IPC adapter、future input adapter、daemon runtime。
- 入力または状態: acquire、release、set-state、heartbeat、client disconnect、shutdown request。
- 期待する観測結果: application command API が owner policy、sequence validation、controller state update、revoke policy を一貫して処理する。IPC transport は同じ API を呼ぶだけである。
- 制約: IPC wire response の互換性を維持する。Switch report bytes、BTstack callback registration、timer behavior は変更しない。
- 対象外: BTstack port / typed event、CMake target 分割、bond store。

source から use case への変換:

`ipc_session` から直接 runtime 全体を置き換えるのではなく、application command API を先に作り、既存 IPC path から呼ぶ。互換 wrapper は一時的な forwarding 層として扱う。

## 3. 対象範囲

- `swbt_app_t` 相当の application state を追加する。
- acquire / release / set-state / heartbeat の command handler を追加する。
- release、owner disconnect、heartbeat timeout、shutdown request を同じ revoke policy に通す。
- owner、last accepted sequence、controller state、status snapshot の所有者を application に寄せる。
- `ipc_session` から application API を呼ぶ compatibility wrapper を用意する。
- revoke invariant を table-driven unit test で固定する。

## 4. 対象外

- IPC JSON codec の純化。
- BTstack callback から typed event への変換。
- production backend ops table の分割。
- CMake target 分割。
- 実機検証。

## 5. 関連 spec / docs

- `spec/architecture/daemon-application-boundary-rearchitecture.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/protocols/daemon-ipc-v1.md`
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`
- `work-units/complete/local_008/IPC_SESSION_CORE.md`
- `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md`

## 6. 根拠監査

not applicable。

この work unit は application の状態所有権を移す。Switch HID report bytes、subcommand bytes、BTstack source selection、report period、WinUSB/libusb facts は追加しない。

## 7. 設計メモ

- application command API は socket、JSON、BTstack vendor header を公開しない。
- `ipc_session` compatibility wrapper は forwarding と response compatibility だけを担当する。
- 新旧両方が owner や controller state を authoritative に書く期間を作らない。
- shutdown revoke は HCI power-off 前の neutral send 順序を壊さない形で扱う。
- stale sequence は `swbt_app_set_state` では `SWBT_APP_ERROR_STALE_SEQUENCE` として観測できる。IPC wire では互換性のため `state_accepted` を返し、status の `last_seq` と controller state は更新しない。

## 8. 対象ファイル

- `swbt/application/app.*`
- `swbt/ipc/ipc_session.*`
- `swbt/ipc/ipc_json.*`
- `CMakeLists.txt`
- `tests/application_command_test.c`
- `tests/ipc_session_test.c`
- `tests/ipc_json_test.c`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | application command API acquires and rejects owners without IPC session headers | new | unit | no |
| refactor-done | stale sequence does not update controller state but preserves status compatibility | regression | unit | no |
| refactor-done | release, disconnect, heartbeat timeout, and shutdown request reach one revoke policy | new | unit | no |
| refactor-done | revoked control clears owner and leaves controller state neutral | regression | unit | no |
| refactor-done | IPC session wrapper forwards existing commands without becoming authoritative owner | regression | integration | no |

## 10. 検証

- red: 2026-06-22 `just build-debug` は `tests/application_command_test.c` の `#include "application/app.h"` が未実装で失敗した。これは application command API 未作成を示す期待どおりの失敗である。
- red: 2026-06-22 `just build-debug` は `swbt_app_set_state` 未定義で失敗した。これは stale sequence item の期待どおりの失敗である。
- red: 2026-06-22 `just build-debug` は `swbt_app_revoke_reason_t` と `swbt_app_revoke` 未定義で失敗した。これは revoke policy item の期待どおりの失敗である。
- red: 2026-06-22 `just build-debug` 後の `just test-debug` は `ipc_session_test` で失敗した。既存 `ipc_session` が stale sequence でも state を更新していたためであり、wrapper forwarding item の期待どおりの失敗である。
- green: 2026-06-22 `just build-debug` pass。
- green: 2026-06-22 `just test-debug` pass。34/34 tests passed。
- refactor-done: 2026-06-22 `ipc_session` の publish helper 名を application state publish に合わせた。同じ `just build-debug` と `just test-debug` は pass。
- verification fix: 2026-06-22 `just verify` は `swbt_app_revoke` と `tests/application_command_test.c` の helper に対する `bugprone-easily-swappable-parameters` で失敗した。`swbt_app_revoke` は狭い `NOLINT`、test helper は期待値 struct へ変更した。
- review red: 2026-06-22 `just build-debug` 後の `just test-debug` は `state_mailbox_test` で失敗した。非 owner の disconnect / heartbeat timeout が no-op ではなく mailbox generation を進めていたためである。
- review green: 2026-06-22 `swbt_ipc_revoke_owner_event_unlocked` で revoke 前 owner を確認し、active owner を revoke した場合だけ mailbox publish するよう修正した。`just build-debug` と `just test-debug` は pass。
- final: 2026-06-22 `just format` pass。
- final: 2026-06-22 `just verify` pass。format-check、clang-tidy、debug build/test、asan build/test、Windows MinGW cross build を通過した。

## 11. 実機実行条件

実機は不要である。Switch-facing bytes、BTstack callback registration、HID advertising、report loop を変更しないためである。

## 12. 先送り事項

- 観測: IPC JSON codec はまだ session と結び付いている。
  先送り理由: application command API を先に固定しないと、pure codec の入出力型が安定しない。
  次の置き場: `work-units/complete/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md`。

- 観測: `ipc_session` は forwarding wrapper として残る。
  先送り理由: JSON codec と IPC server がまだ `swbt_ipc_session_t *` を入口にしているため、この work unit で削除すると codec boundary の変更まで混ざる。
  次の置き場: `work-units/complete/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md` と `work-units/wip/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`。

## 13. チェックリスト

- [x] application command API の source / use case を確認した。
- [x] red test を追加した。
- [x] application revoke policy を実装した。
- [x] `ipc_session` wrapper の責務を forwarding に限定した。
- [x] IPC JSON 互換性を確認した。
- [x] targeted CTest を実行した。
- [x] 互換 wrapper の削除条件を更新した。
