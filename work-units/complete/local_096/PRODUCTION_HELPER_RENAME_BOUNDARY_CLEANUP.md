# Production Helper Rename Boundary Cleanup

## 1. 概要

`production_` prefix が広く付いた helper 群を、実際の責務が分かる名前へ整理し、apps / daemon / BTstack bridge の境界をまたぐ include や allocation を見直す。

完了後、`production` は CLI / status の backend 名または production runner の orchestration 名としてだけ残り、helper file / symbol は `ipc pump adapter`、`report timer bridge`、`HID session`、`active reconnect`、`shutdown sequence` などの責務で読める。`apps/swbt-daemon` が daemon 内部 header を直接 include する必要も削減または明確化する。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-28: `production_` prefix の意味が不明瞭で、daemon 配下に IPC 資材があることも気になる。
- user request, 2026-06-28: helper 群の rename と境界跨ぎの対処を 1 work unit として立ち上げる。
- 現状確認, 2026-06-28: `swbt/daemon/production_ipc_pump.*` は daemon IPC runner と BTstack run loop pump port の adapter である。
- 現状確認, 2026-06-28: `apps/swbt-daemon/production_entrypoint.c` は `daemon/production_runner_internal.h` と `btstack_bridge/production_btstack_impl.h` を include し、opaque runner の内部型を app 側で stack allocate している。
- `work-units/complete/local_088/PRODUCTION_IPC_PUMP_BOUNDARY.md`: `production_ipc_pump.*` は daemon 配下に置く判断だが、BTstack bridge へ daemon IPC runner knowledge を入れない。
- `work-units/complete/local_093/PRODUCTION_RUNNER_HEADER_FINALIZATION.md`: runner は public header で opaque 化され、internal header は generated public include root から除外済み。

use case:

- actor: production path の helper を読む、または boundary を変更する開発者。
- 入力または状態: helper 名が `production_*` に寄り、何が production orchestration で何が BTstack adapter glue なのか名前から分かりにくい。
- 期待する観測結果: helper 名と include path から責務が分かり、app layer が daemon internal state に触れる箇所が削減または例外として明示される。
- 制約: rename と boundary cleanup は構造変更に限定し、CLI、IPC JSON、public C ABI、Switch-facing bytes、report period、BTstack source selection、shutdown neutral ordering を変えない。

source から use case への変換:

`production_` を一括で別語に置換すると、CLI backend 名や production runner orchestration まで曖昧になる。まず target boundary cleanup 後の構造を前提に、helper ごとに責務名へ寄せる。

## 3. 対象範囲

- `production_` helper files と symbols を棚卸しし、backend 名として残すものと責務名へ rename するものを分ける。
- `production_ipc_pump.*` の名前を、daemon IPC runner と BTstack run loop pump port の adapter であることが分かる名前へ寄せる。
- `production_report_timer.*`、`production_hid_session.*`、`production_reconnect.*`、`production_shutdown.*`、`production_process_backend.*` の rename 要否を判断し、実施または残存理由を record に残す。
- `apps/swbt-daemon/production_entrypoint.c` が `daemon/production_runner_internal.h` を include している境界を見直す。
- app layer から runner internal allocation をなくす、または explicit boundary test / record で例外理由を固定する。
- CMake source list、tests、include boundary tests、docs/status、architecture spec の current references を rename 後に合わせる。
- old helper names の current reference が残っていないことを `rg` で確認する。

## 4. 対象外

- CMake target 分割そのもの。これは `local_095` で扱う。
- CLI backend name `production` の変更。
- daemon protocol、public C ABI、Switch-facing HID report bytes、report period の変更。
- BTstack source selection、WinUSB/libusb transport behavior、timer scheduling cadence の変更。
- 実機検証。

## 5. 関連 spec / docs

- `apps/swbt-daemon/production_entrypoint.c`
- `swbt/daemon/active_reconnect.*`
- `swbt/daemon/btstack_hid_session.*`
- `swbt/daemon/btstack_ipc_pump_adapter.*`
- `swbt/daemon/btstack_process_backend.*`
- `swbt/daemon/btstack_report_timer_bridge.*`
- `swbt/daemon/shutdown_sequence.*`
- `swbt/daemon/production_runner.*`
- `swbt/btstack_bridge/production_ports.*`
- `swbt/btstack_bridge/production_btstack_impl.*`
- `tests/cmake/production_entrypoint_boundary_test.cmake`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/compile_include_boundaries_test.cmake`
- `spec/architecture/daemon-architecture-cutover.md`
- `docs/status.md`
- `work-units/complete/local_088/PRODUCTION_IPC_PUMP_BOUNDARY.md`
- `work-units/complete/local_093/PRODUCTION_RUNNER_HEADER_FINALIZATION.md`
- `work-units/complete/local_095/DAEMON_TARGET_BOUNDARY_CLEANUP.md`

## 6. 根拠監査

not applicable.

この work unit は helper rename と include / allocation boundary の構造変更であり、Switch HID report bytes、BTstack source selection、report timing、WinUSB/libusb behavior を追加または変更しない。BTstack callback order または timer scheduling cadence を変える必要が出た場合は、この work unit から外し、`source-audit` の要否を再判断する。

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy after
- reason: target boundary が整理された後で helper 名と apps / daemon boundary を合わせる方が、rename の置換先を誤りにくい。
- verification: rename absence scan、boundary CMake tests、`just debug`、`just asan`、`just windows-cross`。

rename 方針:

- `production` は executable option / domain status / runner orchestration の語として残してよい。
- helper 名では、どの port や lifecycle を接続するかを優先する。
- `swbt/btstack_bridge` は daemon IPC runner 型を参照しない境界を維持する。
- `apps/swbt-daemon` は OS process support と concrete BTstack impl selection に寄せ、daemon internal state へ直接依存しない方向を優先する。

実施結果:

- backend 名としての `production` は CLI / status / `swbt_btstack_production_*` / `production_runner` に残した。これは production backend と runner orchestration を表す語であり、helper の責務名ではない。
- `production_ipc_pump.*` は `btstack_ipc_pump_adapter.*` へ rename した。daemon IPC runner と BTstack run loop pump port の adapter であることを名前に出した。
- `production_report_timer.*` は `btstack_report_timer_bridge.*` へ rename した。daemon process の report timer callback と BTstack report timer port をつなぐ bridge であることを名前に出した。
- `production_hid_session.*` は `btstack_hid_session.*` へ rename した。BTstack HID service registration と HID event handling を持つ session であることを名前に出した。
- `production_reconnect.*` は `active_reconnect.*` へ rename した。learned Switch address への active reconnect request を扱う責務に寄せた。
- `production_shutdown.*` は `shutdown_sequence.*` へ rename した。neutral send、run loop exit、process shutdown listener の sequence を扱う責務に寄せた。
- `production_process_backend.*` は `btstack_process_backend.*` へ rename した。daemon process backend table の BTstack-backed 実装であることを名前に出した。
- `apps/swbt-daemon/production_entrypoint.c` は `daemon/production_runner_internal.h` を include しない。`swbt_daemon_production_run_config_t` と `swbt_daemon_production_run` を public runner header に追加し、runner internal allocation は daemon 側へ戻した。
- `swbt/btstack_bridge` は `swbt_daemon_ipc_runner` と `production_runner_internal` を参照しない境界を維持した。

## 8. 対象ファイル

- `apps/swbt-daemon/production_entrypoint.c`
- `apps/swbt-daemon/production_entrypoint.h`
- `swbt/daemon/active_reconnect.*`
- `swbt/daemon/btstack_hid_session.*`
- `swbt/daemon/btstack_ipc_pump_adapter.*`
- `swbt/daemon/btstack_process_backend.*`
- `swbt/daemon/btstack_report_timer_bridge.*`
- `swbt/daemon/shutdown_sequence.*`
- `swbt/daemon/production_runner.*`
- `tests/daemon_active_reconnect_test.c`
- `tests/daemon_btstack_hid_session_test.c`
- `tests/daemon_btstack_ipc_pump_adapter_test.c`
- `tests/daemon_btstack_process_backend_test.c`
- `tests/daemon_btstack_report_timer_bridge_test.c`
- `tests/daemon_shutdown_sequence_test.c`
- `tests/daemon_production_runner_test.c`
- `tests/cmake/production_entrypoint_boundary_test.cmake`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/compile_include_boundaries_test.cmake`
- `CMakeLists.txt`

reviewed but not changed:

- `docs/status.md`: renamed helper names の current reference はなかった。
- `spec/architecture/daemon-architecture-cutover.md`: renamed helper names の current reference はなかった。

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | renamed helpers preserve production startup, IPC pump, HID session, report timer, reconnect, and shutdown software behavior | regression | unit/integration | no |
| green | app production entrypoint no longer includes daemon runner internal header, or the remaining boundary exception is explicit and tested | regression | architecture | no |
| green | BTstack bridge remains free of daemon IPC runner and daemon runner internal type references | regression | architecture | no |
| green | old helper file names and symbols are absent from current source, tests, and current docs except historical records | regression | review | no |
| green | CLI backend status still reports `production` and `noop` with unchanged IPC JSON output | regression | integration | no |

## 10. 検証

- `rg -n 'swbt_daemon_production_(ipc_pump|report_timer|hid_session|reconnect|shutdown|process_backend)|daemon_production_(ipc_pump|report_timer|hid_session|reconnect|shutdown|process_backend)|#include "daemon/production_(ipc_pump|report_timer|hid_session|reconnect|shutdown|process_backend)\.h"|production_(ipc_pump|report_timer|hid_session|reconnect|shutdown|process_backend)\.(c|h)' CMakeLists.txt apps swbt tests tests/cmake docs/status.md spec/architecture spec/references`: pass. no matches.
- `rg -n 'production_runner_internal' apps/swbt-daemon`: pass. no matches.
- `rg -n 'swbt_daemon_ipc_runner|production_runner_internal' swbt/btstack_bridge`: pass. no matches.
- `git diff --check`: pass.
- `just format`: pass.
- `just format-check`: pass.
- `just debug`: pass after adding BTstack include dirs to `compile_include_boundaries_cmake_test`. `linux-debug` configure/build、CTest 59/59 passed.
- `just asan`: pass. `linux-asan` configure/build、CTest 59/59 passed.
- `just windows-cross`: pass. `windows-mingw-debug` configure/build passed.

## 11. 実機実行条件

実機実行は不要。

この work unit は rename と boundary cleanup であり、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop を実行しない。BTstack call order、timer cadence、HID registration config を変える必要が出た場合は、別 work unit として実機条件を再定義する。

## 12. 先送り事項

none.

この work unit 内で rename だけでは意味が通らない責務移動が見つかった場合は、無理に含めず、具体的な follow-up record または `spec/dev-journal.md` へ送る。

## 13. チェックリスト

- [x] `production_` helper 群を棚卸しした。
- [x] backend 名として残す `production` と rename 対象を分けた。
- [x] helper file / symbol / test 名を責務名へ寄せた。
- [x] apps / daemon internal boundary crossing を解消または例外として固定した。
- [x] CMake source list と boundary tests を更新した。
- [x] old helper name absence scan と targeted tests の結果を記録した。
- [x] 実機未実行理由を維持した。
