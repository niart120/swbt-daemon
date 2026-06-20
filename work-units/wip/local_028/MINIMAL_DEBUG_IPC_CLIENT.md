# Minimal Debug IPC Client

## 1. 概要

Phase 5 前後の手動確認に使う最小 debug IPC client を追加する work unit。

この client は local TCP JSON Lines IPC に接続し、`hello`、`acquire`、`set_state`、`get_status`、`release` を送る。実機 bring-up では daemon へ state snapshot を送るための手動操作入口になる。

この work unit は software IPC client だけを扱う。Switch 実機への入力反映は検証しない。

NyX handoff の macro は、`local_037` の実機 bring-up で使える一時的な外部 IPC client である。この work unit は、Project NyX に依存しない repo-local C client を追加する作業として残す。

## 2. 起点 / ユースケース

source:

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md` の Future client libraries にある CLI debug client。
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` の Windows 実機確認手順。debug IPC client から controller state を送信する手順がある。
- `spec/protocols/daemon-ipc-v1.md`。current IPC contract は `hello`、`acquire`、`release`、`set_state`、`get_status` を含む。
- `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md`。実機 bring-up では state snapshot を手動で送る入口が必要になる。
- NyX handoff。NyX macro を起動済み daemon の debug IPC client として使う手順を示すが、swbt repo-local client の実装完了を代替しない。

use case:

- actor: maintainer または hardware bring-up operator。
- 入力または状態: 起動済み daemon、loopback IPC endpoint、CLI で指定した button / stick / neutral state。
- 期待する観測結果: debug client は daemon IPC へ接続し、owner を取得して full state snapshot を送信し、status を表示し、終了時に release または neutral state を試みる。
- 制約: daemon protocol に timing macro を追加しない。Bluetooth adapter、Switch pairing、HID advertising、report loop はこの work unit で開始しない。
- 対象外: Python / C# / GUI client、Project NyX macro、macro executor、multi-client event subscribe、authentication token。
- source から use case へ変換した判断: 実機 bring-up の入力送信に必要なのは daemon protocol 拡張ではなく、既存 IPC contract を使う最小 CLI client である。

## 3. 対象範囲

- `127.0.0.1` の daemon IPC port へ接続する C CLI executable を追加する。
- `hello` と `acquire` の response から `client_id` と `owner_id` を扱う。
- button と stick の full state snapshot を `set_state` として送る。
- `get_status` response を表示し、latest state を確認できるようにする。
- 正常終了時に `release` または neutral state を送る cleanup path を用意する。
- unsupported timing macro arguments を送信前に拒否する。
- 既存 IPC JSON protocol と local TCP server core を再利用する。

## 4. 対象外

- `tap`、`duration_ms`、`sequence`、`at_ms` を daemon protocol として追加すること。
- macro executor と timed input scheduler。
- Python、C#、GUI client。
- multi-client event subscribe。
- authentication token。
- BTstack adapter access、Switch pairing、HID advertising、report loop。

## 5. 関連 spec / docs

- `spec/protocols/daemon-ipc-v1.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `work-units/complete/local_008/IPC_SESSION_CORE.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`
- `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md`
- `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md`

## 6. 根拠監査

not applicable。

この work unit は local IPC client と controller state snapshot serialization を扱うだけであり、Switch HID protocol、BTstack source selection、report timing、WinUSB facts を追加しない。

ただし、この client を実機接続済み daemon に対して使う行為は実機検証に含まれる。実機 daemon と組み合わせる場合は、`work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` の実機実行条件に従う。

## 7. 設計メモ

- client は daemon protocol の利用例であり、daemon の責務を増やさない。
- `set_state` は full snapshot だけを送る。
- timing macro は client-side helper の将来課題に残す。
- CLI argument の validation は送信前に行い、不正値では IPC message を送らない。
- owner 取得後の error path では release または neutral state の送信を試みる。
- NyX macro を `local_037` で使った実機結果は `local_037` と `docs/hardware-test-log.md` に記録する。NyX 経路で入力反映を確認しても、この C client の build、validation、cleanup path は未実装のまま扱う。

## 8. 対象ファイル

- `CMakeLists.txt`
- `apps/swbt-debug-client/main.c`
- `tests/debug_ipc_client_test.c`
- `swbt/ipc/ipc_server.h`
- `swbt/ipc/ipc_server.c`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/wip/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | debug client executable builds and rejects invalid CLI state values before connecting | new | unit | no |
| todo | debug client sends hello, acquire, set_state, get_status, and release over loopback IPC | new | integration | no |
| todo | unsupported timing macro arguments are rejected without extending daemon protocol | edge | unit | no |
| todo | connection failure and owner_busy return nonzero exit status without sending partial state | edge | integration | no |
| todo | owner acquired path attempts neutral or release cleanup before exit | edge | integration | no |

## 10. 検証

未実行。

この record update では計画を現行構成へ更新しただけで、`just debug`、targeted CTest、`just windows-cross`、実機コマンドは実行していない。

完了時には debug client の targeted test、`just debug`、影響範囲に応じた sanitizer または Windows cross build を記録する。

## 11. 実機実行条件

この work unit 自体に実機検証は不要である。

対象は local loopback IPC client の software integration であり、Bluetooth adapter 操作、Switch pairing、HID advertising、report loop を実行しない。

この client を Windows native daemon と Switch 実機に接続して使う場合は、人間の承認、専用 USB Bluetooth ドングル、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、`docs/hardware-test-log.md` への記録を必要条件にする。

## 12. 先送り事項

- 観測: debug client を実機接続済み daemon に使ったときの button / stick 反映は、この software client work unit では証明しない。
  先送り理由: Switch pairing、HID advertising、report loop、専用 USB Bluetooth dongle が必要である。
  次の置き場: `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `docs/hardware-test-log.md`。

## 13. チェックリスト

- [x] work unit record を現行構成へ更新した。
- [ ] debug IPC client を実装した。
- [ ] TDD テストを追加した。
- [ ] `just debug` を実行した。
- [ ] sanitizer または cross build の結果を記録した。
- [ ] 実機状態を完了判定用に更新した。
