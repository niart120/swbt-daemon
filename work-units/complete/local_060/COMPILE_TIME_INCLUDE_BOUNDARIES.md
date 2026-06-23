# Compile-Time Include Boundaries

## 1. 概要

architecture cutover 後の target / include boundary を、文字列検査だけでなく compile-time failure で検出できる形へ強める work unit。

`local_054` は module target と include boundary check を導入した。`local_058` では、現行 target が `swbt/` 全体を include path として公開し、禁止依存を CMake script の文字列検査に寄せている点が先送り事項になった。

この work unit では、behavior change を伴わず、application、IPC、BTstack adapter、daemon host の依存方向を build graph で検出しやすくする。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md` の先送り事項: include boundary の compile-time 強制。
- `work-units/complete/local_054/DAEMON_HOST_AND_BUILD_BOUNDARIES.md` の target 分割と forbidden include check。
- `spec/architecture/daemon-architecture-cutover.md` の module target 方針。
- user follow-up, 2026-06-23: リアーキテクチャ直後の follow-up でコードベースが増大し続けるリスクを避ける。

use case:

- actor: maintainer、reviewer、CI。
- 入力または状態: `swbt_application`、`swbt_ipc`、`swbt_btstack_adapter`、`swbt_daemon_host` の include directory と link target。
- 期待する観測結果: 禁止依存を追加すると compile error または CMake configure error で失敗する。単なる source text search に依存しない。追加した probe / target / helper は、置き換えた check または削除した依存と対応している。
- 制約: runtime behavior、Switch-facing bytes、production composition は変えない。
- 対象外: source tree の大規模移動、public C ABI 再設計、release packaging。

source から use case への変換:

`include_boundaries_test.cmake` の文字列検査は cutover の absence gate として有効だった。後続では、module ごとの公開 header と内部 header の境界を build system の構造で表す作業として切る。

## 3. 対象範囲

- 現行 target の include directory と link dependency を棚卸しする。
- module ごとに公開してよい header と internal header を分類する。
- 禁止 include を compile probe または CMake target property で検出する。
- 既存 `include_boundaries_cmake_test` を残すか、compile-time check へ置き換えるか判断する。
- protocol / application / IPC / BTstack adapter tests が余計な target を link しないことを維持する。
- 追加する probe、target、helper script、CMake logic と、削除または縮小する text-only check を対応付ける。
- 完了時に、追加行だけでなく削除した glue / check / include path と、残した理由を記録する。

## 4. 対象外

- daemon behavior の変更。
- Switch HID protocol、BTstack source selection、report period の変更。
- production adapter function table の分割。
- Windows native CI の追加。
- directory layout の全面再編。
- compile-time check のためだけに、実装 module と同じ広さの mirror target 群を作ること。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/complete/local_054/DAEMON_HOST_AND_BUILD_BOUNDARIES.md`
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`
- `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md`
- `tests/cmake/include_boundaries_test.cmake`

## 6. 根拠監査

not applicable。

この work unit は build boundary を扱う。Switch HID report bytes、BTstack source selection、report period、WinUSB/libusb facts を追加または変更しない。

## 7. 設計メモ

- compile-time enforcement は、禁止依存の発見精度を上げるための構造変更である。behavior change と混ぜない。
- `swbt/` 全体を include path に残す場合は、残す理由と削除条件を record に書く。
- header の移動が必要な場合は、公開 contract と内部 implementation の分離を先に定義する。
- CMake script の文字列検査は、compile-time check で拾えない absence gate だけに縮小する。
- 新しい build helper を増やす場合は、何を置き換えるための helper かを record に書く。置き換え対象がない helper はこの work unit の完了条件に含めない。
- 完了時の diff は `build / test / docs` に分けて記録し、build scaffolding が主目的から外れて増えていないか確認する。

## 8. 対象ファイル

- `CMakeLists.txt`
- `cmake/*`
- `tests/cmake/include_boundaries_test.cmake`
- `swbt/application/*`
- `swbt/ipc/*`
- `swbt/btstack_bridge/*`
- `swbt/daemon/*`
- `tests/*`
- `work-units/wip/local_060/COMPILE_TIME_INCLUDE_BOUNDARIES.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | application target cannot include BTstack adapter internals through public include paths | regression | build | no |
| refactor-skipped | BTstack adapter target cannot include IPC transport internals through public include paths | regression | build | no |
| refactor-skipped | daemon host remains the composition owner for cross-module wiring | regression | build | no |
| green | protocol tests link without IPC, daemon host, or BTstack adapter targets | regression | build | no |
| refactor-done | old text-only boundary checks are either removed or justified as absence checks | characterization | build | no |
| green | added boundary probes / targets are paired with removed or narrowed checks, or their retention condition is recorded | verification | docs/build | no |

## 10. 検証

TDD status:

- source: `local_058` の先送り事項。
- use case: `swbt_application` を link する利用側が、`swbt_application` 本体と公開依存の include path 経由で `btstack_bridge/` を include できない。
- item: application target cannot include BTstack adapter internals through public include paths。
- state: refactor-done。
- commands:
  - red: `just build-debug` pass。`CTEST_ARGS="-R compile_include_boundaries_cmake_test --output-on-failure" just test-debug` は `swbt_application public include root is missing` で fail。
  - green: `just build-debug` pass。`CTEST_ARGS="-R compile_include_boundaries_cmake_test --output-on-failure" just test-debug` pass。
  - format: `just format` pass。
  - affected checks: `CTEST_ARGS="-R \"(compile_include_boundaries_cmake_test|include_boundaries_cmake_test|switch_hid_descriptor_test|daemon_production_hid_sdp_record_test)\" --output-on-failure" just test-debug` pass。
- notes: `swbt_application` の公開依存である `swbt_support` と `swbt_switch_protocol` が `swbt/` 全体を公開したままだと、推移的 include path から `btstack_bridge/` が見える。そのため 3 target の公開 include root を build directory の `swbt_public_includes/` 配下に生成し、公開 module を `application`、`core`、`switch` に限定した。target 自身の source compile には source tree の `swbt/` を `PRIVATE` include path として残す。生成 include root の更新は `CMAKE_CURRENT_BINARY_DIR/swbt_public_includes/<target>` 配下だけに限定し、path guard を置いた。
- refactor: `switch_hid_descriptor_test` は protocol target だけに link する test なので、BTstack registration config への接続確認を `daemon_production_hid_sdp_record_test` へ移した。descriptor bytes / size / report ID の検査は protocol test に残した。

TDD status:

- source: `local_058` の先送り事項。
- use case: `swbt_btstack_adapter` を link する利用側が、BTstack adapter の公開 header とその公開依存を使える一方で、IPC transport header を include できない。
- item: BTstack adapter target cannot include IPC transport internals through public include paths。
- state: refactor-skipped。
- commands:
  - red: `just build-debug` pass。`CTEST_ARGS="-R compile_include_boundaries_cmake_test --output-on-failure" just test-debug` は `swbt_btstack_adapter public include root is missing` で fail。
  - green: `just build-debug` pass。`CTEST_ARGS="-R compile_include_boundaries_cmake_test --output-on-failure" just test-debug` pass。
  - affected checks: `CTEST_ARGS="-R \"(btstack_hid_device_registration_test|btstack_hid_device_btstack_adapter_test|btstack_output_report_handler_test|btstack_input_report_timer_adapter_test|btstack_input_report_scheduler_test|btstack_subcommand_reply_queue_test|btstack_output_report_callbacks_test)\" --output-on-failure" just test-debug` pass。
  - boundary checks: `CTEST_ARGS="-R include_boundaries_cmake_test --output-on-failure" just test-debug` pass。`compile_include_boundaries_cmake_test` も同じ実行で pass。
- notes: `swbt_btstack_adapter` の公開 include root は `btstack_bridge` に限定した。`btstack_run_loop.h` など BTstack upstream header は public header から参照されるため、`swbt_btstack_selected_include_dirs` は public include path として残した。IPC module は公開 root に含めない。
- refactor: green 後に追加の構造変更は行わなかった。今回の変更は `swbt_configure_module_public_includes(swbt_btstack_adapter MODULES btstack_bridge)` の適用に閉じる。

TDD status:

- source: `local_054` の host / composition root 方針と `local_058` の先送り事項。
- use case: daemon host が application、IPC、BTstack adapter を構成する owner として公開 header を compile できる。BTstack adapter 側から daemon host header は include できない。
- item: daemon host remains the composition owner for cross-module wiring。
- state: refactor-skipped。
- commands:
  - red: `just build-debug` pass。`CTEST_ARGS="-R compile_include_boundaries_cmake_test --output-on-failure" just test-debug` は `swbt_daemon_host public include root is missing` で fail。
  - green: `just build-debug` pass。`CTEST_ARGS="-R compile_include_boundaries_cmake_test --output-on-failure" just test-debug` pass。
  - affected checks: `CTEST_ARGS="-R \"(daemon_host_test|daemon_ipc_runner_test|daemon_production_backend_test|ipc_json_test|ipc_server_test|debug_ipc_client_test)\" --output-on-failure" just test-debug` pass。
  - boundary checks: `CTEST_ARGS="-R include_boundaries_cmake_test --output-on-failure" just test-debug` pass。`compile_include_boundaries_cmake_test` も同じ実行で pass。
- notes: daemon host の公開 header は `daemon/host.h` と `daemon/ipc_runner.h` を probe した。`daemon/ipc_runner.h` は `ipc/ipc_server.h` を必要とするため、`swbt_ipc` も `ipc` root へ狭めた。`swbt_daemon_host` は `daemon` root を公開し、`swbt_ipc`、`swbt_btstack_adapter`、`swbt_application`、`swbt_support` を link して cross-module wiring を担う。
- refactor: green 後に追加の構造変更は行わなかった。behavior、Switch-facing bytes、BTstack source selection は変更していない。

TDD status:

- source: `local_054` の protocol target link boundary。
- use case: protocol test は IPC、daemon host、BTstack adapter target を link せず、`swbt_switch_protocol` だけで compile / link する。
- item: protocol tests link without IPC, daemon host, or BTstack adapter targets。
- state: green。
- commands:
  - green: `rg -n "target_link_libraries\\((switch_report_test|switch_hid_descriptor_test|switch_subcommand_test|switch_subcommand_reply_test|switch_subcommand_dispatcher_test|switch_spi_test|switch_spi_seed_test|switch_rumble_test|switch_player_lights_test) PRIVATE" CMakeLists.txt` で protocol test が `swbt_switch_protocol` だけを link することを確認。
  - green: `rg -n "#include \"(ipc|daemon|btstack_bridge)/" tests/switch_* tests/swbt_smoke_test.c` は no matches。
  - green: `CTEST_ARGS="-R \"(switch_report_test|switch_hid_descriptor_test|switch_subcommand_test|switch_subcommand_reply_test|switch_subcommand_dispatcher_test|switch_spi_test|switch_spi_seed_test|switch_rumble_test|switch_player_lights_test|include_boundaries_cmake_test)\" --output-on-failure" just test-debug` pass。`compile_include_boundaries_cmake_test` も同じ実行で pass。
- notes: この item は既存 `include_boundaries_cmake_test` と、今回の `swbt_switch_protocol` 公開 root の縮小で満たせている。追加の build scaffolding は不要。

TDD status:

- source: `local_060` の完了条件。
- use case: 既存の text-only boundary check が、compile probe に置き換わった stale check と、private source include / target topology を見る absence check に分類されている。
- item: old text-only boundary checks are either removed or justified as absence checks。
- state: refactor-done。
- commands:
  - green: `CTEST_ARGS="-R \"(include_boundaries_cmake_test|compile_include_boundaries_cmake_test)\" --output-on-failure" just test-debug` pass。
- notes: `target_include_directories(swbt_application ... swbt_btstack)` の文字列 check は、公開 include root compile probe に置き換わったため削除した。source scan は target source の `PRIVATE` include path 経由の禁止 include を検出する absence check として残した。CMake target / unit-test link check は、target 名と link topology の absence / topology check として残した。
- refactor: `tests/cmake/include_boundaries_test.cmake` に残す check の分類コメントを追加した。runtime behavior は変更していない。

TDD status:

- source: `local_060` の完了条件。
- use case: 追加した boundary probe / helper が、削除または縮小した check と対応している。残す helper には削除条件がある。
- item: added boundary probes / targets are paired with removed or narrowed checks, or their retention condition is recorded。
- state: green。
- commands:
  - green: `git diff --stat main..HEAD` で全体差分を確認。
  - green: `git diff --numstat main..HEAD` で build / test / docs の増減を確認。
  - green: `git log --oneline main..HEAD` で Test List item ごとの commit を確認。
- notes:
  - `cmake/module_public_includes.cmake` は、`swbt_switch_protocol`、`swbt_support`、`swbt_application`、`swbt_ipc`、`swbt_btstack_adapter`、`swbt_daemon_host` の `swbt/` 全体公開を置き換えるために追加した。
  - `tests/cmake/compile_include_boundaries_test.cmake` は、公開 include root 経由の compile success / failure を見るために追加した。削除した stale check は `target_include_directories(swbt_application ... swbt_btstack)` の文字列 check。
  - `tests/cmake/include_boundaries_test.cmake` は、target source の `PRIVATE` include path からの禁止 include と、CMake target / unit-test link topology を見る absence check として残す。
  - `switch_hid_descriptor_test` から BTstack registration config 依存を外し、`daemon_production_hid_sdp_record_test` へ移した。protocol test link boundary と production config verification を分けるための移動である。
  - 生成 include root helper の保持条件: source tree が `swbt/<module>` layout で、公開 header が `application/...` のような prefix include を使う間だけ残す。将来 `include/` 配下の public/private header layout へ移した場合は、この helper を削除し、通常の `target_include_directories` に戻す。
- diff review: `main..HEAD` は CMake 16+/25-、helper 57+、compile CMake test 154+、旧 boundary test 4+/3-、C test 責務移動 16+/17-、record 73+/10-。`swbt/*.c` / `swbt/*.h` の runtime 実装は変更していない。

Full verification:

- `just verify` pass。format check、clang-tidy build、linux-debug build / CTest、linux-asan build / CTest、Windows MinGW cross build を実行した。
- 実機は未実行。build boundary の work unit であり、Bluetooth adapter、Switch pairing、HID advertising、report loop を実行していない。

## 11. 実機実行条件

実機不要。build graph と compile-time check の work unit であり、Bluetooth adapter、Switch pairing、HID advertising、report loop を実行しない。

## 12. 先送り事項

none。起票時点の先送り事項は、この record の source として取り込んだ。

## 13. チェックリスト

- [x] source を `local_054` と `local_058` から特定した。
- [x] use case を build boundary enforcement として定義した。
- [x] target include directory の現状を棚卸しした。
- [x] red build check を追加した。
- [x] green 実装を行った。
- [x] `just debug` または targeted configure/build を実行した。
- [x] full verification の要否を判定した。
- [x] 追加した build scaffolding と削除または縮小した check を対応付けた。
- [x] diff の増加分が boundary enforcement に必要な範囲へ閉じているか確認した。
