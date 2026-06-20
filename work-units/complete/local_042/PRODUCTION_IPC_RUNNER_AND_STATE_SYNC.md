# Production IPC Runner And State Sync

## 1. 概要

NyXpy handoff と production daemon entrypoint の前提として、production daemon が loopback IPC endpoint を起動し、controller state snapshot を race なく BTstack-facing side へ渡せるようにする work unit。

この work unit は Bluetooth adapter、BTstack run loop、HID advertising、Switch pairing を開始しない。実機不要の software integration として、IPC runner lifecycle、endpoint reporting、state handoff の同期境界を固定する。

## 2. 起点 / ユースケース

source:

- ユーザ要求: NyXpy を使う Windows hardware bring-up へ進む前に、手順を spec 化し、必要な work unit record を作る。
- `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` の停止条件。現行 `swbt-daemon.exe` は no-op backend で実 IPC listener を起動しない。
- `spec/operations/windows-hardware-bringup-sequence.md` の gate 2。
- `spec/protocols/daemon-ipc-v1.md`。NyXpy と debug IPC client は JSON Lines over loopback IPC に接続する。
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`。state mailbox は copy boundary を持つが、現行実装は OS thread 間同期を固定していない。

use case:

- actor: production daemon process と IPC client。
- 入力または状態: daemon runtime の IPC session、`127.0.0.1` host、設定済み port、NyXpy または debug IPC client からの `hello` / `acquire` / `set_state` / `get_status`。
- 期待する観測結果: IPC runner は loopback endpoint を bind し、endpoint を記録できる形で公開し、client message を処理し、accepted state を synchronized mailbox または同等の境界へ保存する。stop では listener と connection を閉じ、owner state を neutral に戻す。
- 制約: non-loopback bind は許可しない。Bluetooth adapter、HID advertising、BTstack run loop はこの work unit で起動しない。
- 対象外: NyXpy macro 実行、Switch pairing、report period comparison、stable observability protocol。
- source から use case へ変換した判断: NyXpy 入力反映の前提は daemon protocol 拡張ではなく、既存 IPC v1 を production daemon process から起動できることと、IPC / report boundary の同期である。

## 3. 対象範囲

- production IPC runner を追加する。
- runner は `127.0.0.1` だけに bind し、bound port を取得できるようにする。
- daemon log または metadata へ IPC endpoint を記録できる API を用意する。
- IPC session と state mailbox の接続を production runner で保持する。
- state mailbox または同等境界を OS thread 間で安全に使えるようにする。
- runner stop で listener、active connection、owner state、mailbox neutral cleanup を実行する。
- NyXpy と同じ JSON Lines command sequence を debug IPC client または test client で確認する。

## 4. 対象外

- Bluetooth adapter を開くこと。
- BTstack run loop の起動。
- HID Device registration、HID advertising、Switch pairing。
- report period の採用判断。
- Project NyX repo の設定更新や `uv run nyxpy run swbt_hardware_bringup` の実行。
- stable IPC metrics / status schema。
- binary release。

## 5. 関連 spec / docs

- `spec/operations/windows-hardware-bringup-sequence.md`
- `spec/protocols/daemon-ipc-v1.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/operations/windows-native-preflight.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md`
- `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`
- `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`

## 6. 根拠監査

not applicable。

この work unit は IPC runner lifecycle と state handoff の software boundary を扱う。Switch HID report bytes、BTstack source selection、report period、WinUSB/libusb facts を追加しない。

ただし、BTstack main-thread dispatch や OS-specific wake mechanism を採用する場合は、採用前に `source-audit` を使う。

## 7. 設計メモ

- 実装は C11 `atomic_bool` の短い spin lock を `swbt_spin_lock_t` として使う。理由は、latest snapshot の copy 境界を守るだけであり、OS thread や lock-free queue の導入はこの段階では不要だからである。
- `swbt_ipc_server_t` は既定の内部 session を維持しつつ、production runner では runtime-owned `swbt_ipc_session_t` を外部 session として bind できるようにする。
- `swbt_daemon_ipc_runner_t` は start で bind し、endpoint を公開し、accept / serve / stop を明示的に呼ぶ API にする。BTstack run loop や background thread の採用は `local_043` の entrypoint composition で扱う。
- IPC endpoint は test では port `0` を許可し、実機 run では設定値または log から取得できるようにする。
- IPC runner は daemon protocol を拡張しない。NyXpy 側も `daemon-ipc-v1` の `hello`、`acquire`、`set_state`、`get_status`、`release` を使う。
- stop path は `release` 未送信の client disconnect でも neutral を保存する。

## 8. 対象ファイル

- `CMakeLists.txt`
- `swbt/core/state_mailbox.h`
- `swbt/core/state_mailbox.c`
- `swbt/daemon/config.h`
- `swbt/daemon/config.c`
- `swbt/daemon/ipc_runner.h`
- `swbt/daemon/ipc_runner.c`
- `swbt/daemon/runtime.h`
- `swbt/daemon/runtime.c`
- `tests/daemon_ipc_runner_test.c`
- `tests/state_mailbox_test.c`
- `swbt/core/spin_lock.h`
- `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | IPC runner binds only `127.0.0.1` and exposes the bound endpoint before accepting clients | new | integration | no |
| done | IPC runner serves `hello` / `acquire` / `set_state` / `get_status` and publishes the latest state through the daemon state boundary | new | integration | no |
| done | state mailbox or equivalent boundary prevents concurrent store / load from reading partial state | edge | unit | no |
| done | runner stop closes listener and active connection, clears owner, and stores neutral state once | edge | integration | no |
| done | NyXpy-compatible JSON Lines sequence works through loopback without Bluetooth hardware | characterization | integration | no |
| deferred | NyXpy macro connects to production daemon during Switch-facing hardware run | characterization | hardware | yes |

## 10. 検証

実行済み:

- red: `just build-debug` failed。`tests/daemon_ipc_runner_test.c` が未実装の `daemon/ipc_runner.h` を参照したため。
- green: `just build-debug` passed。
- targeted: `just test-debug` passed。CTest 26/26。
- format: `just format` passed。
- clean verification: `just debug` passed。clean configure、build、CTest 26/26。
- full verification: `just verify` passed。format check、clang-tidy、linux debug tests、ASan/UBSan tests、Windows MinGW cross build。

TDD status:

- red: 未実装 runner API と外部 session bind を test から要求した。
- green: `swbt_daemon_ipc_runner_t`、`swbt_ipc_server_bind_session`、state mailbox / IPC session lock を実装して pass させた。
- refactor: runner `stop` は、failed start 後に session を不要に neutral 化しないよう、running または active connection がある場合だけ cleanup する形に絞った。

Test Desiderata review:

- isolated: `daemon_ipc_runner_test` は loopback TCP と既存 JSON Lines client helper を使うが、Bluetooth adapter、BTstack run loop、HID advertising には触れない。
- behavior-focused: endpoint exposure、non-loopback rejection、JSON Lines command sequence、mailbox handoff、stop neutral cleanup を観測する。
- risk: background thread と BTstack run loop への組み込みは未実装であり、`local_043` の production entrypoint composition で扱う。

## 11. 実機実行条件

この work unit 自体に実機検証は不要である。

対象は loopback IPC と state synchronization の software integration であり、Bluetooth adapter、Switch pairing、HID advertising、report loop を開始していない。

この runner を production BTstack backend と組み合わせて実機 daemon に使う場合は、`work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `spec/operations/windows-native-preflight.md` の実機実行条件に従う。

## 12. 先送り事項

- 観測: 実 NyXpy macro を production daemon に接続する確認は、この work unit では行わない。
  先送り理由: 実機 daemon run、artifact root、Switch capture、daemon log との対応付けが必要である。
  次の置き場: `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `docs/hardware-test-log.md`。
- 観測: stable IPC metrics / status schema は、この runner の最低要件ではない。
  先送り理由: first bring-up では endpoint と state input が主目的であり、公開 diagnostics contract は別 work unit の責務である。
  次の置き場: `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`。

## 13. チェックリスト

- [x] source / use case を実装前に再確認した。
- [x] red test を追加した。
- [x] IPC runner を実装した。
- [x] state handoff の同期方式を記録した。
- [x] targeted CTest を実行した。
- [x] `just debug` を実行した。
- [x] sanitizer または Windows cross build の必要性を判断した。
- [x] 実機状態または未実行理由を記録した。
