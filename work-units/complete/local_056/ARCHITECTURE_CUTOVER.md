# Architecture Cutover

## 1. 概要

外部レビューで提示された破壊的 architecture cutover を current 方針として採用し、旧 compatibility layer を削除する work unit。

完了後は、daemon logical state の authoritative owner を application に一本化し、IPC、BTstack、host、platform の責務を target と include boundary で分ける。旧 `swbt_ipc_session_t`、`state_mailbox`、`swbt_daemon_runtime_t`、`swbt_daemon_runtime_backend_t`、production backend ops table、`swbt_core` aggregate target は残さない。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-23: `tmp/swbt-daemon-architecture-cutover-handoff.md` を spec として承認し、本採用する。既存指針と相反する部分は提示文書を優先する。
- `spec/architecture/daemon-architecture-cutover.md`: 外部レビュー結果を current architecture policy として採用した spec。
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`: 旧 compatibility layer を current responsibility 付きで残した直前の完了 record。この work unit ではその判断を上書きする。

use case:

- actor: maintainer、reviewer、CI、hardware operator。
- 入力または状態: 旧 session / mailbox / runtime / backend table / aggregate target が残る current implementation。
- 期待する観測結果: software gate で旧 symbol、旧 source、旧 target、旧 test が存在しない。JSON IPC から fake HID までの architecture journey が新しい host / application / adapter 境界で通る。
- 制約: IPC wire format、Switch-facing bytes、ユーザが実行する CLI / environment variable は明示判断なしに破壊しない。実機 H1 は人間の承認なしに実行しない。
- 対象外: bonded reconnect、adapter recovery、parser fuzz、Windows native CI、release packaging、複数 controller、Joy-Con、NFC / IR semantic support。

source から use case への変換:

外部レビュー文書を単なる参考ではなく current spec として採用する。`local_055` の「kept」分類は history として残すが、この work unit の完了条件には使わない。旧構造を残す場合は外部契約保護に必要な例外だけとし、現時点では例外を作らない。

## 3. 対象範囲

- `tmp/swbt-daemon-architecture-cutover-handoff.md` を `spec/architecture/daemon-architecture-cutover.md` として採用する。
- 旧段階移行 spec を archive へ移し、current spec の衝突をなくす。
- `swbt_app_t` を opaque にし、daemon logical state と status counters を application boundary へ寄せる。
- IPC server / runner / adapter を `swbt_ipc_session_t` ではなく application context へ接続する。
- report tick が application snapshot から state を読むようにし、`state_mailbox` を削除する。
- `swbt_daemon_host_t` を導入し、startup / shutdown / cleanup ordering を host へ移す。
- production composition から `swbt_daemon_runtime_t`、runtime backend table、production backend ops table を削除する。
- CMake target を `swbt_application`、`swbt_ipc`、`swbt_btstack_adapter`、`swbt_daemon_host` へ分け、`swbt_core` を削除する。
- old cutover acceptance を absence / boundary acceptance へ置き換える。
- README / docs/status / spec を実装後の current state に合わせる。

不要になった資材群の扱い:

- `swbt/ipc/ipc_session.*` と `tests/ipc_session_test.c` は削除した。
- `swbt/core/state_mailbox.*` と `tests/state_mailbox_test.c` は削除した。
- `swbt/daemon/runtime.*` と `tests/daemon_runtime_test.c` は `swbt/daemon/host.*` と `tests/daemon_host_test.c` へ置換した。
- `swbt/daemon/production_backend_ops.h` は削除し、BTstack 側 adapter 境界の `swbt/btstack_bridge/production_adapter.h` へ移した。
- `tests/daemon_cutover_journey_test.c` は旧 compatibility journey として残さず、`tests/architecture_journey_test.c` へ置換した。
- `tests/cmake/cutover_acceptance_test.cmake` は旧 kept / removed / deferred acceptance として残さず、`tests/cmake/architecture_absence_test.cmake` へ置換した。
- 旧 architecture spec は削除ではなく `spec/archive/` へ移し、history と conflict source として残した。
- `tmp/hardware/local_057/...` の H1 artifact は不要資材ではなく実機証跡であるため、repository に全文転記せず `docs/hardware-test-log.md` から artifact path を参照する。

## 4. 対象外

- Switch HID report byte、subcommand byte、SPI data、rumble packet の新規値。
- BTstack source selection の追加または変更。
- 実機 pairing、HID advertising、report loop の実行。
- IPC protocol version bump。
- release artifact 作成。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `spec/archive/daemon-runtime-boundaries-before-architecture-cutover.md`
- `spec/archive/daemon-application-boundary-rearchitecture-before-cutover.md`
- `spec/operations/work-unit-spec-tdd-flow.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`

## 6. 根拠監査

条件付き。

この work unit の主対象は architecture boundary、状態所有、target / include 境界である。Switch-facing bytes、BTstack source selection、report period、WinUSB/libusb fact を変更しない限り、根拠監査は not applicable とする。

BTstack callback registration、timer / send scheduling、shutdown 時の HCI power-off と neutral send の順序を実装上変更した場合は、source-audit または実機 H1 の根拠として記録する。

## 7. 設計メモ

- `swbt_app_t` は application 内の opaque type とする。adapter は field を直接参照しない。
- IPC status の wire compatibility は `ipc_json` / `ipc_adapter` の責務とし、旧 `ipc_session` wrapper は残さない。
- report scheduler は application snapshot から state を取得する。別 thread writer の確認なしに lock-based mailbox を残さない。
- production backend の能力は host / BTstack adapter の境界へ直接渡す。広い ops table を runtime rollback 手段として残さない。
- H1 は cutover 完了後の 2026-06-23 に `local_057` で一回実施し、pass として記録した。

## 8. 対象ファイル

- `spec/architecture/daemon-architecture-cutover.md`
- `spec/archive/*architecture-cutover*.md`
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`
- `swbt/application/*`
- `swbt/ipc/*`
- `swbt/daemon/*`
- `swbt/btstack_bridge/*`
- `api/*`
- `apps/swbt-daemon/*`
- `apps/swbt-debug-client/*`
- `tests/*`
- `tests/cmake/*`
- `CMakeLists.txt`
- `docs/status.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | application snapshot exposes owner, sequence, controller state, rumble, daemon status, hardware status, and metrics without exposing struct fields | new | unit | no |
| done | IPC JSON acquire / set_state / get_status keeps the v1 wire behavior while using application context instead of session | regression | integration | no |
| done | owner disconnect and heartbeat timeout neutralize through application revoke and do not rely on state mailbox | regression | integration | no |
| done | host startup / shutdown journey sends neutral before transport stop and power-off in fake backend | new | integration | no |
| done | CMake absence acceptance rejects old session, mailbox, runtime, backend table, production ops table, cutover acceptance, and swbt_core references in source / tests / build graph | verification | build | no |
| done | module tests link target-specific libraries rather than aggregate target | regression | build | no |
| done | Hardware Gate H1 confirms Button A, owner disconnect neutral, shutdown trailing neutral, and zero current connection transport anomalies on dedicated adapter | hardware-gated | hardware | yes |

TDD status:

- source: user request and `spec/architecture/daemon-architecture-cutover.md`
- use case: old compatibility path is absent while IPC-visible behavior and fake host journey still pass
- item: architecture cutover software suite
- state: done
- commands:
  - `just debug`: pass。38/38 tests。
  - `just format`: pass。
  - `just verify`: pass。format check、clang-tidy、debug CTest、ASan CTest、Windows MinGW cross build を含む。
  - Hardware Gate H1: pass。`work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md` と `docs/hardware-test-log.md` に記録。
  - `rg -n "ipc_session|swbt_ipc_session_t|swbt_state_mailbox|state_mailbox|daemon_runtime|swbt_daemon_runtime|SWBT_DAEMON_RUNTIME|production_backend_ops|swbt_daemon_production_backend_ops_t|swbt_core|cutover_acceptance|daemon_cutover" swbt apps api tests CMakeLists.txt`: no matches。
- notes: TDD Test List 作成後に software cutover をまとめて実装した。個別 item の red log は保持していない。最終 gate は、新 architecture journey と absence / boundary acceptance で旧経路の不存在を確認する。

## 10. 検証

- `just debug`: pass。38/38 tests。
- `just format`: pass。
- `just verify`: pass。`scripts/check-format.sh`、`linux-clang-tidy` configure / build、`linux-debug` CTest、`linux-asan` CTest、`windows-mingw-debug` cross build を含む。
- 旧経路 absence check: source / tests / build graph 対象の `rg` で `ipc_session`、`state_mailbox`、`daemon_runtime`、production backend ops、`swbt_core`、旧 cutover test 名が no matches。
- Hardware Gate H1: pass。artifact `tmp/hardware/local_057/20260623-105416-architecture-cutover-h1`。HCI dump は line `953` Button A、line `954` trailing neutral、line `955` `hci_power_control: 0` の順を記録した。current connection の `invalid size` と `non-registered handle` は `0` 件。

## 11. 実機実行条件

Hardware Gate H1 は `local_057` で実行済みである。

実行条件:

- 人間の明示承認あり。
- 専用 USB Bluetooth dongle: CSR8510 A10、VID/PID `0A12:0001`。
- Windows native、WinUSB driver assignment 記録済み。
- `SWBT_DAEMON_BACKEND=production`。
- `SWBT_RUN_HARDWARE=1`。
- `SWBT_HARDWARE_APPROVED=1`。
- OS、adapter VID/PID、driver、BTstack commit、swbt source snapshot、Switch firmware baseline、report period、結果は `docs/hardware-test-log.md` に記録済み。

raw HCI dump と daemon trace は artifact `tmp/hardware/local_057/20260623-105416-architecture-cutover-h1` に保持する。

## 12. 先送り事項

- none。Hardware Gate H1 は `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md` で pass として閉じた。

## 13. チェックリスト

- [x] current architecture cutover spec を採用した。
- [x] 旧 conflict spec を archive へ移した。
- [x] work unit record の source / use case / TDD Test List を更新した。
- [x] `swbt_ipc_session_t` が source / tests / build graph から消えた。
- [x] `state_mailbox` が source / tests / build graph から消えた。
- [x] `swbt_daemon_runtime_t` が source / tests / build graph から消えた。
- [x] `swbt_daemon_runtime_backend_t` が source / tests / build graph から消えた。
- [x] production backend ops table が source / tests / build graph から消えた。
- [x] `swbt_core` が source / tests / build graph から消えた。
- [x] architecture journey が新しい host / application / adapter 境界で通る。
- [x] `just debug` を実行して結果を記録した。
- [x] `just verify` または未実行理由を記録した。
- [x] Hardware Gate H1 の実行結果または未実行理由を記録した。
