# Daemon Target Boundary Cleanup

## 1. 概要

`swbt_daemon_process` target に generic daemon process と production 専用 helper が同居している状態を棚卸しし、target 境界を production path の責務に合わせて整理する。

完了後、CMake target 名と source list から、daemon process composition、production runner / BTstack-backed backend、BTstack concrete implementation の差が読める。不要な aggregate target や互換 target を残さず、必要な target だけを executable と tests が明示的に link する。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-28: target 分割・整理、不要な target の削除を 1 work unit として立ち上げる。
- 現状確認, 2026-06-28: `CMakeLists.txt` の `swbt_daemon_process` target は `process.c`、`ipc_runner.c`、config 系に加えて `production_hid_session.c`、`production_ipc_pump.c`、`production_process_backend.c`、`production_report_timer.c`、`production_reconnect.c`、`production_shutdown.c`、`production_runner.c` を含む。
- 現状確認, 2026-06-28: `swbt-daemon` executable は `swbt_daemon_process` と `swbt_btstack_production_impl` を link する。
- `work-units/complete/local_083/MODULE_RENAME_AND_PLACEMENT_CLEANUP.md`: rename は済んだが、責務移動を必要とする target topology は対象外にした。
- `work-units/complete/local_088/PRODUCTION_IPC_PUMP_BOUNDARY.md`: production IPC pump glue は daemon 配下に置くが、BTstack bridge へ daemon IPC runner knowledge を入れない。

use case:

- actor: CMake target または daemon module を変更する開発者。
- 入力または状態: `swbt_daemon_process` target を見ると、generic daemon process と production-specific helper の境界が source list から分からない。
- 期待する観測結果: target を見れば、noop / generic daemon process tests と production runner / BTstack-backed tests の依存差が分かる。
- 制約: target 整理は構造変更に限定し、runtime behavior、IPC JSON、public C ABI、Switch-facing bytes、report period、BTstack source selection、shutdown neutral ordering を変えない。

source から use case への変換:

今の違和感は、ファイルの配置より CMake target の粒度が広いことから生じている。target を増やす場合も削る場合も、差分後に読み取り負荷と link 境界が下がったことを確認する。

## 3. 対象範囲

- `swbt_daemon_process` の source list と link dependencies を棚卸しする。
- production 専用 source を別 target へ分けるか、target 名をより正確にするかを判断する。
- 新 target を作る場合は、それが置き換える source grouping と link 境界を record に書く。
- 不要な aggregate target、互換 target、名前だけの wrapper target があれば削除する。
- executable と tests の link target を、新しい責務境界に合わせて更新する。
- generated public include root と include boundary probes を target 境界に合わせて更新する。
- target 数、source grouping、test link target の pre/post を記録し、整理が scaffolding 増加だけで終わっていないことを確認する。

## 4. 対象外

- `production_` helper file / symbol の rename。これは `local_096` で扱う。
- `apps/swbt-daemon/production_entrypoint.c` の boundary-crossing cleanup。これは `local_096` で扱う。
- daemon process backend contract の振る舞い変更。
- IPC JSON wire format、public C ABI、Switch-facing bytes、report period、BTstack source selection の変更。
- 実機検証。

## 5. 関連 spec / docs

- `CMakeLists.txt`
- `cmake/module_public_includes.cmake`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/compile_include_boundaries_test.cmake`
- `tests/cmake/architecture_absence_test.cmake`
- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/complete/local_083/MODULE_RENAME_AND_PLACEMENT_CLEANUP.md`
- `work-units/complete/local_088/PRODUCTION_IPC_PUMP_BOUNDARY.md`
- `work-units/complete/local_093/PRODUCTION_RUNNER_HEADER_FINALIZATION.md`

## 6. 根拠監査

not applicable.

この work unit は CMake target topology と include boundary の構造変更であり、Switch HID report bytes、BTstack source selection、report timing、WinUSB/libusb behavior を追加または変更しない。

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: helper rename に入る前に、target が generic daemon process と production path をどう分けるかを固定する。
- verification: CMake boundary tests、`just debug`、`just asan`、`just windows-cross`。

guardrail:

- target を増やすだけでは完了にしない。新 target は既存 target の責務を狭めるか、不要 target を削除する必要がある。
- `swbt_daemon_process` が production-specific source を持ち続ける判断をする場合は、なぜ target 分割より読みやすいかを record に残す。
- `swbt` public C ABI target が `swbt_ipc` や production implementation へ link しない境界は維持する。

実施結果:

- before: `swbt_daemon_process` は config、CLI、IPC runner、launch options、process、switch address に加えて `production_*.c` と `production_shutdown.c` を同じ source list に持っていた。
- after: `swbt_daemon_process` は generic daemon process / config / CLI / IPC runner / launch options / switch address の target とした。
- after: `swbt_daemon_production_runner` を追加し、production runner、production IPC pump、production process backend、HID session、report timer、reconnect、shutdown をこの target に寄せた。
- after: `swbt-daemon` executable と production helper tests は `swbt_daemon_production_runner` を明示的に link する。generic daemon tests は `swbt_daemon_process` のままにした。
- after: `swbt_daemon_process` の generated public include root から `daemon/production_*.h` と `daemon/shutdown_listener.h` を除外し、production 側 include root にだけ公開した。
- 不要な aggregate / compatibility target は存在しなかったため削除対象はなかった。代わりに、既存 target の責務を狭め、名前だけの wrapper target は追加していない。

## 8. 対象ファイル

- `CMakeLists.txt`
- `cmake/module_public_includes.cmake`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/compile_include_boundaries_test.cmake`
- `tests/cmake/architecture_absence_test.cmake`

reviewed but not changed:

- `tests/*daemon*_test.c`: test source は変更せず、link target だけを CMake で切り替えた。
- `docs/status.md`: current target names を説明する記述はなかった。
- `spec/architecture/daemon-architecture-cutover.md`: architecture spec の責務文を変える必要はなかった。

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | generic daemon process tests link only the target needed for daemon process behavior | regression | build | no |
| green | production runner / production helper tests link the production-specific target explicitly | regression | build | no |
| green | public C ABI target remains free of `swbt_ipc` and production implementation link dependencies | regression | build/architecture | no |
| green | obsolete aggregate or compatibility targets are absent from CMake after cleanup | regression | build/review | no |
| green | CMake include boundary probes still pass with the new target include roots | regression | architecture | no |

## 10. 検証

- `rg -n "add_library\\(swbt_daemon_process STATIC|add_library\\(swbt_daemon_production_runner STATIC|target_link_libraries\\(daemon_production|swbt_daemon_production_runner|swbt_daemon_process" CMakeLists.txt tests/cmake`: pass. `swbt_daemon_process` と `swbt_daemon_production_runner` の source grouping と test link target を確認した。
- `git diff --check`: pass.
- `just debug`: pass. `linux-debug` configure/build、CTest 59/59 passed.
- `just asan`: pass. `linux-asan` configure/build、CTest 59/59 passed.
- `just windows-cross`: pass. `windows-mingw-debug` configure/build passed.

## 11. 実機実行条件

実機実行は不要。

この work unit は build graph と include boundary の構造変更であり、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop を実行しない。

## 12. 先送り事項

none.

helper rename と app / daemon boundary cleanup は `local_096` で扱う。

## 13. チェックリスト

- [x] `swbt_daemon_process` target の責務を棚卸しした。
- [x] production-specific source の target boundary を整理した。
- [x] 不要な aggregate / compatibility target が残っていないことを確認した。
- [x] executable と tests の link target を更新した。
- [x] CMake / include boundary 検証結果を記録した。
- [x] 実機未実行理由を維持した。
