# Minimal Debug IPC Client

## 1. 概要

Phase 5 前後の手動確認に使う最小 debug IPC client を追加する work unit。

この client は local TCP JSON Lines IPC に接続し、`hello`、`acquire`、`set_state`、`get_status`、`release` を送る。実機 bring-up では daemon へ state snapshot を送るための手動操作入口になる。

この work unit は software IPC client だけを扱う。Switch 実機への入力反映は検証しない。

NyX handoff の macro は、`local_037` の実機 bring-up で使える一時的な外部 IPC client である。この work unit では、Project NyX に依存しない repo-local C client を追加した。

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
- owner 取得後の error path では release の送信を試みる。
- NyX macro を `local_037` で使った実機結果は `local_037` と `docs/hardware-test-log.md` に記録する。NyX 経路で入力反映を確認しても、この C client の実機入力反映を代替しない。

## 8. 対象ファイル

- `CMakeLists.txt`
- `apps/swbt-debug-client/main.c`
- `tests/debug_ipc_client_test.c`
- `swbt/ipc/ipc_server.h`
- `swbt/ipc/ipc_server.c`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | debug client executable builds and rejects invalid CLI state values before connecting | new | unit | no |
| refactor-skipped | debug client sends hello, acquire, set_state, get_status, and release over loopback IPC | new | integration | no |
| refactor-skipped | unsupported timing macro arguments are rejected without extending daemon protocol | edge | unit | no |
| refactor-skipped | connection failure and owner_busy return nonzero exit status without sending partial state | edge | integration | no |
| refactor-skipped | owner acquired path attempts neutral or release cleanup before exit | edge | integration | no |

## 10. 検証

TDD status:

- source: `spec/initial/REPOSITORY_INITIALIZATION_TODO.md` の Future client libraries にある CLI debug client。
- use case: debug client は daemon IPC へ接続する前に CLI state 値を検証する。
- item: debug client executable builds and rejects invalid CLI state values before connecting。
- state: refactor-skipped。
- commands:
  - `just build-debug` -> pass。
  - `$env:CTEST_ARGS='-R debug_ipc_client_test'; just test-debug` -> red。`--lx 4096` を指定しても skeleton client が exit code `0` を返した。
  - `just build-debug` -> pass。CLI state validation 実装後。
  - `$env:CTEST_ARGS='-R debug_ipc_client_test'; just test-debug` -> pass。
- notes: `tdd-workflow`、`tdd-test-list`、`tdd-one-cycle`、`work-unit-record`、`refactor-after-green` を読んだ。初回の `just test-debug` は sandbox 内の Docker 状態確認で失敗したため、同じ command を承認付きで再実行した。

TDD status:

- source: `spec/protocols/daemon-ipc-v1.md` の `hello`、`acquire`、`set_state`、`get_status`、`release`。
- use case: debug client は loopback IPC server に対し、owner を取得して full state snapshot を送り、status を取得して release する。
- item: debug client sends hello, acquire, set_state, get_status, and release over loopback IPC。
- state: refactor-skipped。
- commands:
  - `just build-debug` -> pass。
  - `$env:CTEST_ARGS='-R debug_ipc_client_test'; just test-debug` -> red。loopback server 接続後、stub の `swbt_debug_client_send_hello` が失敗し、`hello` を送信できなかった。
  - `just build-debug` -> pass。protocol message 送受信実装後。
  - `$env:CTEST_ARGS='-R debug_ipc_client_test'; just test-debug` -> pass。
- notes: green 後の構造整理は、次 item の error handling test で cleanup fixture の形が変わるため `refactor-skipped` とした。

TDD status:

- source: `spec/protocols/daemon-ipc-v1.md` の `owner_busy` error response と TCP loopback transport。
- use case: connection failure または `owner_busy` では debug client は nonzero で終了し、`set_state` を送らない。
- item: connection failure and owner_busy return nonzero exit status without sending partial state。
- state: refactor-skipped。
- commands:
  - `just build-debug` -> pass。
  - `$env:CTEST_ARGS='-R debug_ipc_client_test'; just test-debug` -> red。valid CLI state で未使用 port に接続しても skeleton main が exit code `0` を返した。
  - `just build-debug` -> pass。connection failure と `owner_busy` handling 実装後。
  - `$env:CTEST_ARGS='-R debug_ipc_client_test'; just test-debug` -> pass。
- notes: `owner_busy` は callback I/O の scripted response で `hello` と `acquire` までの送信数を観測した。green 後の追加構造変更は行わず `refactor-skipped` とした。

TDD status:

- source: `spec/protocols/daemon-ipc-v1.md` の daemon protocol 対象外にある timing macro arguments。
- use case: debug client は `tap`、`duration_ms`、`sequence`、`at_ms` を daemon protocol へ送らない。
- item: unsupported timing macro arguments are rejected without extending daemon protocol。
- state: refactor-skipped。
- commands:
  - `$env:CTEST_ARGS='-R debug_ipc_client_test'; just test-debug` -> pass。
- notes: parser 実装時に unsupported macro rejection が入っていたため、red は個別に取り直していない。regression test では `--duration-ms` が connection 前の parse error exit code `2` で止まることを確認した。

TDD status:

- source: `spec/protocols/daemon-ipc-v1.md` の `release` と neutral fail-safe。
- use case: owner 取得後の error path では debug client が release cleanup を試みる。
- item: owner acquired path attempts neutral or release cleanup before exit。
- state: refactor-skipped。
- commands:
  - `$env:CTEST_ARGS='-R debug_ipc_client_test'; just test-debug` -> pass。
- notes: callback I/O の scripted `invalid_state` response 後に `release` request を送ることを確認した。green 後の追加構造変更は行わず `refactor-skipped` とした。

Test desiderata:

- purpose: debug IPC client の parser、loopback transport、error handling、cleanup path の new / edge regression。
- key trade-offs: loopback test は実 transport に近く predictive だが、blocking client main を直接並列実行しない。owner_busy と cleanup は callback I/O で fast / deterministic を優先した。
- risks: `tests/debug_ipc_client_test.c` は `apps/swbt-debug-client/main.c` を include しており、将来 client core を複数 file に分ける場合は test include 境界を見直す必要がある。
- action: 現段階では client 内部 API を公開 header に昇格しない。複数 client から再利用する時点で分割する。

検証結果:

- `just format-check` -> pass。
- `$env:CTEST_ARGS='-R debug_ipc_client_test'; just test-debug` -> pass。
- `just debug` -> pass。25/25 tests passed。
- `just asan` -> pass。25/25 tests passed。
- `just windows-cross` -> pass。
- 実機コマンドは未実行。理由: この work unit は local loopback IPC client の software integration であり、Bluetooth adapter 操作、Switch pairing、HID advertising、report loop を実行しない。

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
- [x] debug IPC client を実装した。
- [x] TDD テストを追加した。
- [x] `just debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] 実機状態を完了判定用に更新した。
