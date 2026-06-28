# Process Public Header Boundary

## 1. 概要

`daemon/process.h` の public include 面から `runtime/host.h` と BTstack output handler の具象依存を外す。

この work unit は、将来の public C ABI runtime handle 化を先取りして実装しない。今回の完了条件は、daemon process の公開 header が runtime host と BTstack bridge の具象型を要求しないこと、既存の daemon process / production runner の観測結果が変わらないことである。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-28: 将来 public C ABI を runtime を持つ handle に育てられるよう、今は下準備に留める。別件として `process.h` の具象型露出は直す。
- review finding: `process.h` が `runtime/host.h` と `btstack_bridge/output_report_handler.h` を public に露出している。
- `spec/architecture/daemon-architecture-cutover.md`: daemon process は process lifecycle 境界であり、runtime host の start / stop と IPC runner の順序付けを持つ。

use case:

- actor: daemon process / production runner の境界を変更する開発者。
- 入力または状態: `daemon/process.h` を include すると runtime host と BTstack output handler の public include root が必要になる。
- 期待する観測結果: `daemon/process.h` は runtime / BTstack bridge public include root なしで compile でき、内部実装と tests は必要な internal header から具象型へアクセスする。
- 制約: IPC JSON wire format、public C ABI の外部形、Switch-facing report bytes、report period、BTstack source selection、shutdown neutral ordering は変えない。

source から use case への判断:

- 今回は status 合成の移動や public C ABI runtime handle 化を実装しない。`process.h` の具象依存を切る構造変更として扱う。

## 3. 対象範囲

- `daemon/process.h` を runtime host と BTstack output handler の具象型を含まない公開 header にする。
- daemon process の実体定義と internal accessor を `*_internal.h` へ移す。
- production runner、daemon process tests、architecture journey test を internal header 経由へ更新する。
- include boundary / architecture absence tests に、`process.h` の runtime / output handler 具象依存不在を固定する。

## 4. 対象外

- `swbt/control` の status 合成責務の移動。
- public C ABI handle を runtime host 所有へ変更すること。
- daemon process backend contract から runtime backend pointer を完全に撤去すること。
- IPC JSON、Switch HID report、BTstack source selection、report timing の変更。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/complete/local_097/RUNTIME_HOST_COMPOSITION_OWNERSHIP.md`

## 6. 根拠監査

not applicable. Switch HID report bytes、BTstack source selection、report period、subcommand / SPI / rumble / descriptor data、WinUSB/libusb behavior は変更しない。

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: `process.h` の具象依存を切ると、将来の public C ABI runtime handle 化と status 合成整理の前に include 境界を明確にできる。観測可能な daemon behavior は変えない。
- verification: targeted CMake boundary tests、daemon process / production runner 関連 tests、必要に応じて `just debug`。

`process.h` は opaque な process / backend pointer と lifecycle API だけを持つ。stack allocation、backend fixture、runtime/output handler accessor が必要な内部実装と tests は internal header を include する。

## 8. 対象ファイル

- `swbt/daemon/process.h`
- `swbt/daemon/process_internal.h`
- `swbt/daemon/process.c`
- daemon production / shutdown / report timer bridge headers and tests that allocate or inspect `swbt_daemon_process_t`
- `tests/cmake/compile_include_boundaries_test.cmake`
- `tests/cmake/architecture_absence_test.cmake`
- `CMakeLists.txt`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | `daemon/process.h` compiles without runtime or BTstack bridge public include roots | regression | build | no |
| green | architecture absence test rejects runtime host / output handler concrete exposure in `process.h` | regression | build | no |
| green | daemon process and production runner behavior remains unchanged after internal header split | regression | integration | no |

## 10. 検証

- red: `$env:CTEST_ARGS='-R "^(compile_include_boundaries_cmake_test|architecture_absence_cmake_test)$" --output-on-failure'; just test-debug`
  - result: expected failure. `compile_include_boundaries_cmake_test` failed because `daemon/process.h` still required `btstack_bridge/output_report_handler.h`. `architecture_absence_cmake_test` failed because `process.h` still exposed `swbt_runtime_host_backend_t *runtime_backend`.
- targeted: `$env:CTEST_ARGS='-R "^(compile_include_boundaries_cmake_test|architecture_absence_cmake_test|daemon_process_test|architecture_journey_test|daemon_btstack_process_backend_test|daemon_btstack_report_timer_bridge_test|daemon_production_runner_test)$" --output-on-failure'; just debug`
  - result: pass. 7/7 tests passed after configure/build.
- full debug: `Remove-Item Env:CTEST_ARGS -ErrorAction SilentlyContinue; just debug`
  - result: pass. 59/59 tests passed.
- `scripts/check-format.sh`
  - result: pass.
- `git diff --check`
  - result: pass. CRLF replacement warnings only for tracked text files; no whitespace errors.
- `rg -n '#include "runtime/host\.h"|#include "btstack_bridge/output_report_handler\.h"|swbt_runtime_host_backend_t|swbt_btstack_output_report_handler_t|swbt_runtime_host_t' swbt\daemon\process.h`
  - result: pass. no matches.

## 11. 実機実行条件

実機実行は不要。Bluetooth adapter、Switch pairing、HID advertising、report loop、WinUSB/libusb behavior に触れない構造変更である。

## 12. 先送り事項

- 観測: `swbt/control` は runtime status reader を持ち、将来の public C ABI runtime handle 化と status 合成責務に関係する。
  先送り理由: 今回は `process.h` 公開面の具象依存を切る work unit であり、status 合成の配置変更は別の設計判断を含む。
  次の置き場: 後続 work unit record または `spec/architecture/daemon-architecture-cutover.md` の current-state 更新。

## 13. チェックリスト

- [x] `process.h` から runtime host / output handler の具象 include を外した。
- [x] internal header に daemon process の実体定義と internal accessor を移した。
- [x] include boundary / absence tests を更新した。
- [x] 既存 daemon process / production runner behavior の検証結果を記録した。
- [x] 実機未実行理由を記録した。
