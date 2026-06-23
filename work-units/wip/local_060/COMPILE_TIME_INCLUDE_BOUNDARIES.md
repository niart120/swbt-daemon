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
| todo | BTstack adapter target cannot include IPC transport internals through public include paths | regression | build | no |
| todo | daemon host remains the composition owner for cross-module wiring | regression | build | no |
| todo | protocol tests link without IPC, daemon host, or BTstack adapter targets | regression | build | no |
| todo | old text-only boundary checks are either removed or justified as absence checks | characterization | build | no |
| todo | added boundary probes / targets are paired with removed or narrowed checks, or their retention condition is recorded | verification | docs/build | no |

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
- [ ] full verification の要否を判定した。
- [ ] 追加した build scaffolding と削除または縮小した check を対応付けた。
- [ ] diff の増加分が boundary enforcement に必要な範囲へ閉じているか確認した。
