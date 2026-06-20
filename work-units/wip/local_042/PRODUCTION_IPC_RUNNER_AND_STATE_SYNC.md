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
- `work-units/wip/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`

## 6. 根拠監査

not applicable。

この work unit は IPC runner lifecycle と state handoff の software boundary を扱う。Switch HID report bytes、BTstack source selection、report period、WinUSB/libusb facts を追加しない。

ただし、BTstack main-thread dispatch や OS-specific wake mechanism を採用する場合は、採用前に `source-audit` を使う。

## 7. 設計メモ

- 最初の実装候補は mutex 付き mailbox である。理由は、NyXpy bring-up に必要なのは latest snapshot の一貫した copy であり、lock-free queue の必要性は未証明だからである。
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
- `work-units/wip/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | IPC runner binds only `127.0.0.1` and exposes the bound endpoint before accepting clients | new | integration | no |
| todo | IPC runner serves `hello` / `acquire` / `set_state` / `get_status` and publishes the latest state through the daemon state boundary | new | integration | no |
| todo | state mailbox or equivalent boundary prevents concurrent store / load from reading partial state | edge | unit | no |
| todo | runner stop closes listener and active connection, clears owner, and stores neutral state once | edge | integration | no |
| todo | NyXpy-compatible JSON Lines sequence works through loopback without Bluetooth hardware | characterization | integration | no |
| deferred | NyXpy macro connects to production daemon during Switch-facing hardware run | characterization | hardware | yes |

## 10. 検証

未実行。

この record は work unit の範囲と TDD Test List を作成しただけであり、code、CTest、実機コマンドは実行していない。

## 11. 実機実行条件

この work unit 自体に実機検証は不要である。

対象は loopback IPC と state synchronization の software integration であり、Bluetooth adapter、Switch pairing、HID advertising、report loop を開始しない。

この runner を production BTstack backend と組み合わせて実機 daemon に使う場合は、`work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `spec/operations/windows-native-preflight.md` の実機実行条件に従う。

## 12. 先送り事項

- 観測: 実 NyXpy macro を production daemon に接続する確認は、この work unit では行わない。
  先送り理由: 実機 daemon run、artifact root、Switch capture、daemon log との対応付けが必要である。
  次の置き場: `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `docs/hardware-test-log.md`。
- 観測: stable IPC metrics / status schema は、この runner の最低要件ではない。
  先送り理由: first bring-up では endpoint と state input が主目的であり、公開 diagnostics contract は別 work unit の責務である。
  次の置き場: `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`。

## 13. チェックリスト

- [ ] source / use case を実装前に再確認した。
- [ ] red test を追加した。
- [ ] IPC runner を実装した。
- [ ] state handoff の同期方式を記録した。
- [ ] targeted CTest を実行した。
- [ ] `just debug` を実行した。
- [ ] sanitizer または Windows cross build の必要性を判断した。
- [ ] 実機状態または未実行理由を記録した。
