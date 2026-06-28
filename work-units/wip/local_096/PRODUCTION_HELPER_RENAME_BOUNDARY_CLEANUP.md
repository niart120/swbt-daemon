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
- `swbt/daemon/production_*.c`
- `swbt/daemon/production_*.h`
- `swbt/btstack_bridge/production_ports.*`
- `swbt/btstack_bridge/production_btstack_impl.*`
- `tests/cmake/production_entrypoint_boundary_test.cmake`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/compile_include_boundaries_test.cmake`
- `spec/architecture/daemon-architecture-cutover.md`
- `docs/status.md`
- `work-units/complete/local_088/PRODUCTION_IPC_PUMP_BOUNDARY.md`
- `work-units/complete/local_093/PRODUCTION_RUNNER_HEADER_FINALIZATION.md`
- `work-units/wip/local_095/DAEMON_TARGET_BOUNDARY_CLEANUP.md`

## 6. 根拠監査

not applicable.

この work unit は helper rename と include / allocation boundary の構造変更であり、Switch HID report bytes、BTstack source selection、report timing、WinUSB/libusb behavior を追加または変更しない。BTstack callback order または timer scheduling cadence を変える必要が出た場合は、この work unit から外し、`source-audit` の要否を再判断する。

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy after
- reason: target boundary が整理された後で helper 名と apps / daemon boundary を合わせる方が、rename の置換先を誤りにくい。
- verification: rename absence scan、boundary CMake tests、targeted daemon production tests、必要に応じて `just debug`。

rename 方針:

- `production` は executable option / domain status / runner orchestration の語として残してよい。
- helper 名では、どの port や lifecycle を接続するかを優先する。
- `swbt/btstack_bridge` は daemon IPC runner 型を参照しない境界を維持する。
- `apps/swbt-daemon` は OS process support と concrete BTstack impl selection に寄せ、daemon internal state へ直接依存しない方向を優先する。

## 8. 対象ファイル

- `apps/swbt-daemon/production_entrypoint.c`
- `apps/swbt-daemon/production_entrypoint.h`
- `swbt/daemon/production_ipc_pump.*`
- `swbt/daemon/production_report_timer.*`
- `swbt/daemon/production_hid_session.*`
- `swbt/daemon/production_reconnect.*`
- `swbt/daemon/production_shutdown.*`
- `swbt/daemon/production_process_backend.*`
- `swbt/daemon/production_runner.*`
- `tests/daemon_production_*_test.c`
- `tests/cmake/production_entrypoint_boundary_test.cmake`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/compile_include_boundaries_test.cmake`
- `CMakeLists.txt`
- `docs/status.md`
- `spec/architecture/daemon-architecture-cutover.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | renamed helpers preserve production startup, IPC pump, HID session, report timer, reconnect, and shutdown software behavior | regression | unit/integration | no |
| todo | app production entrypoint no longer includes daemon runner internal header, or the remaining boundary exception is explicit and tested | regression | architecture | no |
| todo | BTstack bridge remains free of daemon IPC runner and daemon runner internal type references | regression | architecture | no |
| todo | old helper file names and symbols are absent from current source, tests, and current docs except historical records | regression | review | no |
| todo | CLI backend status still reports `production` and `noop` with unchanged IPC JSON output | regression | integration | no |

## 10. 検証

not run yet.

予定:

- `rg -n "production_ipc_pump|production_report_timer|production_hid_session|production_reconnect|production_shutdown|production_process_backend" CMakeLists.txt apps swbt tests docs spec`
- `$env:CTEST_ARGS='-R "production_entrypoint_boundary_cmake_test|include_boundaries_cmake_test|compile_include_boundaries_cmake_test|daemon_production_runner_test|daemon_production_ipc_pump_test|daemon_production_report_timer_test|daemon_production_hid_session_test|daemon_production_shutdown_test|daemon_production_reconnect_test|daemon_production_process_backend_test" --output-on-failure'; just test-debug`
- `just debug` if rename touches broad CMake source lists or generated include roots
- `just windows-cross` if executable source lists or app boundary change

## 11. 実機実行条件

実機実行は不要。

この work unit は rename と boundary cleanup であり、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop を実行しない。BTstack call order、timer cadence、HID registration config を変える必要が出た場合は、別 work unit として実機条件を再定義する。

## 12. 先送り事項

none.

この work unit 内で rename だけでは意味が通らない責務移動が見つかった場合は、無理に含めず、具体的な follow-up record または `spec/dev-journal.md` へ送る。

## 13. チェックリスト

- [ ] `production_` helper 群を棚卸しした。
- [ ] backend 名として残す `production` と rename 対象を分けた。
- [ ] helper file / symbol / test 名を責務名へ寄せた。
- [ ] apps / daemon internal boundary crossing を解消または例外として固定した。
- [ ] CMake source list と boundary tests を更新した。
- [ ] old helper name absence scan と targeted tests の結果を記録した。
- [ ] 実機未実行理由を維持した。
