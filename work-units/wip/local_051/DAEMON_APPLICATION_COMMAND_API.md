# Daemon Application Command API

## 1. 概要

daemon の owner、sequence、controller state、neutral 化の authoritative owner を application command API へ移す work unit。

`local_050` では controller state と command metadata を分け、`control_lease` を IPC 非依存に抽出する。この work unit では、その内側に `swbt_app_t` 相当の application boundary を置き、IPC session が直接 owner policy と neutral 化を所有しない状態へ進める。

## 2. 起点 / ユースケース

source:

- `spec/architecture/daemon-application-boundary-rearchitecture.md` の roadmap。
- `work-units/wip/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md` の後続 work unit。
- 現行 `ipc_session` が owner、state、rumble、mailbox、neutral 化を持つ implementation fact。

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
- owner、last accepted sequence、controller state、status counter の所有者を application に寄せる。
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
- `work-units/wip/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`
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

## 8. 対象ファイル

- `swbt/daemon/` または新設する application boundary 配下。
- `swbt/ipc/ipc_session.*`
- `swbt/ipc/ipc_json.*`
- `swbt/daemon/runtime.*`
- `tests/ipc_session_test.c`
- `tests/ipc_json_test.c`
- `tests/daemon_runtime_test.c`
- 新設する application tests。

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | application command API acquires and rejects owners without IPC session headers | new | unit | no |
| todo | stale sequence does not update controller state but preserves status compatibility | regression | unit | no |
| todo | release, disconnect, heartbeat timeout, and shutdown request reach one revoke policy | new | unit | no |
| todo | revoked control clears owner and leaves controller state neutral | regression | unit | no |
| todo | IPC session wrapper forwards existing commands without becoming authoritative owner | regression | integration | no |

## 10. 検証

未実行。

## 11. 実機実行条件

実機は不要である。Switch-facing bytes、BTstack callback registration、HID advertising、report loop を変更しないためである。

## 12. 先送り事項

- 観測: IPC JSON codec はまだ session と結び付いている。
  先送り理由: application command API を先に固定しないと、pure codec の入出力型が安定しない。
  次の置き場: `work-units/wip/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md`。

## 13. チェックリスト

- [ ] application command API の source / use case を確認した。
- [ ] red test を追加した。
- [ ] application revoke policy を実装した。
- [ ] `ipc_session` wrapper の責務を forwarding に限定した。
- [ ] IPC JSON 互換性を確認した。
- [ ] targeted CTest を実行した。
- [ ] 互換 wrapper の削除条件を更新した。
