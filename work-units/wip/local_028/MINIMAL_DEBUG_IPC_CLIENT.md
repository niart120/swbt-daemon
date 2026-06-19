# Minimal Debug IPC Client

## 1. 概要

Phase 5 前後の手動確認に使う最小 debug IPC client を追加するための計画 record。

この client は local TCP JSON Lines IPC に接続し、`hello`、`acquire`、`set_state`、`get_status`、`release` を送る。

この work unit は software IPC client だけを扱い、Switch 実機への入力反映は検証しない。

## 2. 対象範囲

- `127.0.0.1` の daemon IPC port へ接続する C CLI executable を追加する。
- `hello` と `acquire` の response から `client_id` と `owner_id` を扱う。
- button と stick の full state snapshot を `set_state` として送る。
- `get_status` response を表示し、latest state を確認できるようにする。
- 正常終了時に `release` または neutral state を送る cleanup path を用意する。
- 既存 IPC JSON protocol と local TCP server core を再利用する。

## 3. 対象外

- `tap`、`duration_ms`、`sequence`、`at_ms` を daemon protocol として追加すること。
- macro executor と timed input scheduler。
- Python、C#、GUI client。
- multi-client event subscribe。
- authentication token。
- BTstack adapter access、Switch pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `work-units/complete/local_008/IPC_SESSION_CORE.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`
- `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md`

## 5. 根拠監査

not applicable。

この work unit は local IPC client と controller state snapshot serialization を扱うだけであり、Switch HID protocol、BTstack source selection、report timing、WinUSB facts を追加しない。

ただし、この client を実機接続済み daemon に対して使う行為は実機検証に含まれる。

実機 daemon と組み合わせる場合は、別 work unit の実機実行条件に従う。

## 6. 設計メモ

- client は daemon protocol の利用例であり、daemon の責務を増やさない。
- `set_state` は full snapshot だけを送る。
- timing macro は client-side helper の将来課題に残す。
- CLI argument の validation は送信前に行い、不正値では IPC message を送らない。
- owner 取得後の error path では release または neutral state の送信を試みる。

## 7. 対象ファイル

- `CMakeLists.txt`
- `apps/swbt-debug-client/main.c`
- `tests/debug_ipc_client_test.c`
- `swbt/ipc/ipc_server.h`
- `swbt/ipc/ipc_server.c`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/wip/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | debug client executable builds and rejects invalid CLI state values before connecting | new | unit | no |
| todo | debug client sends hello, acquire, set_state, get_status, and release over loopback IPC | new | integration | no |
| todo | unsupported timing macro arguments are rejected without extending daemon protocol | edge | unit | no |
| todo | connection failure and owner_busy return nonzero exit status without sending partial state | edge | integration | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、`make debug`、targeted CTest、`make windows-cross`、実機コマンドは実行していない。

完了時には debug client の targeted test、`make debug`、影響範囲に応じた sanitizer または Windows cross build を記録する。

## 10. 実機実行条件

この work unit 自体に実機検証は不要である。

対象は local loopback IPC client の software integration であり、Bluetooth adapter 操作、Switch pairing、HID advertising、report loop を実行しない。

この client を Windows native daemon と Switch 実機に接続して使う場合は、人間の承認、専用 USB Bluetooth ドングル、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、`docs/hardware-test-log.md` への記録を必要条件にする。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] debug IPC client を実装した。
- [ ] TDD テストを追加した。
- [ ] `make debug` を実行した。
- [ ] sanitizer または cross build の結果を記録した。
- [ ] 実機状態を完了判定用に更新した。
