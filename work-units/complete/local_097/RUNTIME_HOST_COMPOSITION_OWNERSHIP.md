# Runtime Host Composition Ownership

## 1. 概要

`daemon process` が `domain`、`control`、`runtime host`、IPC runner、runtime backend wrapper を同時に構成している状態を整理する。

完了後は、`runtime host` が `domain + control + runtime resource` の composition owner になり、`daemon process` は IPC runner と runtime host の開始 / 停止順序を持つ process lifecycle 境界へ縮小する。これにより、`swbt_daemon_process_backend_t` が `swbt_runtime_host_backend_t` と同型の関数テーブルを作り直して委譲する構造を削除する。

この work unit は破壊的な内部構造変更を許容する。internal C API、CMake target、tests、architecture boundary checks、docs / spec reference は変更してよい。ただし IPC JSON wire format、public C ABI の外部形、Switch-facing report bytes、report period、BTstack source selection、shutdown neutral ordering は変えない。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-28: runtime host と daemon host の責務を整理し、現状が関数テーブルの委譲をそっくり行うだけか確認したい。
- 現状確認, 2026-06-28: `swbt_daemon_process_backend_t` は IPC start / stop と daemon backend status を除くと `swbt_runtime_host_backend_t` とかなり同型である。
- 現状確認, 2026-06-28: `swbt/daemon/process.c` の `swbt_daemon_process_runtime_*` wrapper 群は、ほぼ同名の backend callback を `host->backend_context` へ転送する。
- 現状確認, 2026-06-28: `swbt_daemon_process_t` は `swbt_domain_t *app`、`swbt_runtime_host_t runtime`、`swbt_runtime_host_backend_t runtime_backend`、`swbt_control_t control` を直接所有している。
- 現状確認, 2026-06-28: `swbt/control/control.h` は `runtime/host.h` を include し、`swbt_runtime_host_t *runtime` を直接持つため、runtime host が control を所有するには先に循環依存を断つ必要がある。
- `spec/architecture/daemon-architecture-cutover.md`: 現在は daemon process を IPC runner と runtime host の composition root として記録している。
- `work-units/complete/local_082/CONTROL_RUNTIME_BOUNDARY_IMPLEMENTATION.md`: `swbt/control` と `swbt/runtime` を追加し、runtime host は IPC start / stop を持たない境界として実装した。
- `work-units/complete/local_083/MODULE_RENAME_AND_PLACEMENT_CLEANUP.md`: composition root を runtime へ吸収することは当時の rename / placement cleanup の対象外だった。
- `work-units/complete/local_095/DAEMON_TARGET_BOUNDARY_CLEANUP.md` と `work-units/complete/local_096/PRODUCTION_HELPER_RENAME_BOUNDARY_CLEANUP.md`: daemon / production helper の target と naming は整理済みであり、次は composition ownership を扱える状態になっている。

use case:

- actor: daemon process lifecycle、runtime host、IPC runner、production runner、後続の architecture cleanup を行う開発者。
- 入力または状態:
  - `daemon process` が domain / control / runtime を構成し、runtime backend と同型の wrapper table を保持している。
  - `runtime host` は runtime resource lifecycle を持つが、domain / control lifetime は持たない。
  - `control` は runtime host の具象型に依存している。
- 期待する観測結果:
  - IPC command、public C ABI smoke、production / noop daemon startup、shutdown neutral の観測可能な結果が変わらない。
  - runtime host が domain と control を構成し、IPC runner は runtime host から得た control handle を使う。
  - daemon process は domain / control / runtime backend wrapper を直接所有しない。
  - runtime backend は daemon process backend を経由せず runtime host へ直接渡される。
  - old wrapper symbol と process-owned composition fields は source / tests / build graph から消える。
- 制約:
  - 旧 runtime path、旧 production backend ops table、旧 session / mailbox は復活させない。
  - 新旧の composition owner を併存させた compatibility layer を残さない。
  - production hardware-facing port group は能力別 port のまま維持する。

source から use case への変換:

現状の違和感は、runtime host の存在そのものではなく、`daemon process` が runtime host に渡す関数テーブルを薄い wrapper として再構成している点にある。これを小さい rename や wrapper 削減として処理すると、途中状態で二重 composition が残る可能性が高い。先に着地点を `runtime host` の composition ownership として固定し、spec、TDD Test List、absence checks を同じ work unit で更新する。

## 3. 対象範囲

- `spec/architecture/daemon-architecture-cutover.md` を更新し、composition ownership の新しい境界を記録する。
- `swbt/control` から `runtime/host.h` への具象依存を外す。
- `swbt/control` の runtime status 参照を callback または小さい port へ置き換え、runtime host が control を所有できる依存方向にする。
- `swbt/runtime/host.*` を `domain + control + runtime resource` の owner へ昇格する。
- runtime host が `swbt_control_t *` と `swbt_domain_t *` を提供する accessor を定義する。
- `daemon process` から `swbt_domain_t *app`、`swbt_control_t control`、`swbt_runtime_host_backend_t runtime_backend`、`swbt_daemon_process_runtime_*` wrapper 群を削除する。
- `swbt_daemon_process_backend_t` から runtime backend と同型の callback 群を削除し、IPC start / stop と daemon lifecycle に必要な項目だけへ縮小する。
- runtime backend を daemon process backend 経由ではなく runtime host の init path へ直接渡す。
- production runner / BTstack-backed runtime wiring を新しい境界へ付け替える。
- noop backend と production backend の tests を新しい runtime ownership に合わせて更新する。
- include boundary / architecture absence tests に、old wrapper と old process-owned fields の不在条件を追加する。
- `docs/status.md` と関連 work unit / spec reference を更新する。

## 4. 対象外

- IPC JSON wire format の変更。
- public C ABI の外部 shape の変更。
- Switch-facing HID report bytes、subcommand bytes、SPI address、rumble packet、HID descriptor の変更。
- report period、timer cadence、BTstack source selection、WinUSB/libusb transport behavior の変更。
- 実機 pairing、HID advertising、report loop の実行。
- 複数 controller、Joy-Con、NFC / IR semantic support。
- `vendor/btstack` の編集。
- production helper rename や daemon target 分割そのもの。これらは `local_095` と `local_096` で扱ったため、この work unit では残った composition ownership だけを扱う。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `docs/status.md`
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`
- `work-units/complete/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md`
- `work-units/complete/local_082/CONTROL_RUNTIME_BOUNDARY_IMPLEMENTATION.md`
- `work-units/complete/local_083/MODULE_RENAME_AND_PLACEMENT_CLEANUP.md`
- `work-units/complete/local_095/DAEMON_TARGET_BOUNDARY_CLEANUP.md`
- `work-units/complete/local_096/PRODUCTION_HELPER_RENAME_BOUNDARY_CLEANUP.md`
- `swbt/daemon/process.*`
- `swbt/runtime/host.*`
- `swbt/control/control.*`
- `swbt/daemon/btstack_process_backend.*`
- `swbt/daemon/production_runner.*`
- `tests/cmake/architecture_absence_test.cmake`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/compile_include_boundaries_test.cmake`

## 6. 根拠監査

not applicable for the planned composition ownership change.

この work unit は ownership、internal API、target topology、tests、spec reference の構造変更を対象にする。Switch HID report bytes、BTstack source selection、report timing、WinUSB/libusb behavior を追加または変更しない。

BTstack callback order、timer scheduling cadence、HID registration config、adapter selection、Switch-facing bytes を変える必要が出た場合は、この work unit の範囲から外し、`source-audit` と `hardware-harness` の要否を再判断する。

## 7. 設計メモ

Tidy status:

- classification: structure change, but not a small tidy.
- decision: dedicated design work unit.
- reason: composition owner、internal API、CMake target、tests、architecture spec を同時に更新する破壊的整理である。小さい tidy として切ると、新旧 owner の併存や wrapper rollback path が残る。
- verification: spec update、targeted unit / integration tests、absence checks、`just debug`、`just asan`、`just windows-cross`、完了前の `just verify`。

### 7.1 目標境界

```text
production runner
  BTstack production ports
  hardware power
  run loop
  shutdown listener
  runtime backend provider

daemon process
  process lifecycle
  IPC runner start / stop
  runtime host start / stop
  daemon lifecycle status update

runtime host
  domain lifetime
  control object
  runtime resource lifecycle
  HID registration
  output handler
  report timer
  neutral shutdown
  runtime status

control
  owner / sequence / neutral / status operation semantics
  app-owned state update
  runtime status read through callback or small port
```

### 7.2 マイルストーン

M0: spec and guardrails

- architecture spec を更新し、現在の `daemon process is composition root` 記述を置き換える。
- TDD Test List と absence checks の期待値を先に固定する。

M1: break control-to-runtime concrete dependency

- `swbt/control` が `runtime/host.h` を include しない状態にする。
- runtime status は callback または port 経由で読む。

M2: move domain and control ownership into runtime host

- runtime host が domain を生成 / 破棄する。
- runtime host が control を構成し、`swbt_runtime_host_control()` と `swbt_runtime_host_app()` のような accessor を提供する。
- control / runtime tests で owner、sequence、neutral、status 合成の意味論が変わらないことを固定する。

M3: shrink daemon process

- daemon process は runtime host と IPC runner の start / stop 順序だけを持つ。
- `swbt_daemon_process_backend_t` から runtime backend callback を削る。
- `swbt_daemon_process_runtime_*` wrapper 群を削除する。

M4: rewire production path

- `btstack_process_backend` または後継境界から runtime backend を runtime host へ直接渡す。
- production runner は IPC pump、hardware power、run loop、shutdown listener と runtime backend provider の owner に留める。

M5: delete old ownership traces and verify

- old wrapper symbol、process-owned app / control / runtime_backend fields、runtime-shaped daemon process backend callbacks の absence checks を追加する。
- targeted tests、`just debug`、ASan、Windows cross build、`just verify` を実行する。

### 7.3 破壊的変更の許容範囲

- internal header、internal struct field、internal function、CMake target link、test fixture は壊してよい。
- completed work unit record の historical text は原則として書き換えない。current reference が必要な docs / spec / status だけ更新する。
- external IPC JSON、public C ABI の外部 shape、Switch-facing behavior は壊さない。
- 途中互換の adapter を残す場合は、同じ work unit 内で削除する。

## 8. 対象ファイル

- `spec/architecture/daemon-architecture-cutover.md`
- `docs/status.md`
- `CMakeLists.txt`
- `swbt/control/control.*`
- `swbt/runtime/host.*`
- `swbt/daemon/process.*`
- `swbt/daemon/btstack_process_backend.*`
- `swbt/daemon/production_runner.*`
- `swbt/daemon/production_runner_internal.h`
- `swbt/daemon/ipc_runner.*`
- `api/swbt_c_api.c`
- `tests/control_test.c`
- `tests/runtime_host_test.c`
- `tests/daemon_process_test.c`
- `tests/daemon_btstack_process_backend_test.c`
- `tests/daemon_production_runner_test.c`
- `tests/daemon_ipc_runner_test.c`
- `tests/architecture_journey_test.c`
- `tests/swbt_c_api_test.c`
- `tests/cmake/architecture_absence_test.cmake`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/compile_include_boundaries_test.cmake`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | control reads runtime status through a callback or port without including `runtime/host.h` | regression | unit/build | no |
| green | runtime host owns domain and control while preserving client owner, sequence, direct submit, neutral, and status semantics | regression | unit | no |
| green | IPC runner starts with a control handle obtained from runtime host instead of daemon process-owned control | regression | integration | no |
| green | daemon process starts IPC and runtime host without owning domain, control, or a runtime backend wrapper table | regression | integration/build | no |
| green | daemon process backend no longer contains HID/output/report timer/subcommand runtime callbacks | regression | architecture | no |
| green | production runner wires BTstack-backed runtime backend directly to runtime host and preserves software startup behavior | regression | integration | no |
| green | shutdown neutral ordering and pending neutral cleanup remain unchanged through the new ownership boundary | regression | integration | no |
| green | public C ABI smoke remains IPC-free and preserves existing operation semantics | regression | unit/build | no |
| green | old wrapper symbols and process-owned composition fields are absent from source, tests, and build graph | regression | build/review | no |
| green | architecture spec and docs/status describe the new composition owner without reintroducing old runtime or production ops table language | regression | docs | no |

## 10. 検証

作成時点:

- `git branch --show-current`
  - result: `main`
- `git status --short`
  - result: clean

record 作成時点では実装、build、unit tests は未実行。

実行結果:

- `git branch --show-current`
  - result: `codex/local-097-runtime-host-composition`
- red: `$env:CTEST_ARGS='-R "^architecture_absence_cmake_test$" --output-on-failure'; just test-debug`
  - result: expected failure。`swbt/control/control.h still depends on runtime host concrete header`。
- `just build-tests-debug`
  - result: pass.
- targeted: `$env:CTEST_ARGS='-R "^(control_test|runtime_host_test|daemon_process_test|daemon_btstack_process_backend_test|daemon_production_runner_test|daemon_ipc_runner_test|architecture_journey_test|swbt_c_api_test|architecture_absence_cmake_test|include_boundaries_cmake_test|compile_include_boundaries_cmake_test)$" --output-on-failure'; just test-debug`
  - result: pass. 11/11 tests passed.
- `just format`
  - result: pass.
- `just format-check`
  - result: pass.
- `just debug`
  - result: pass. `linux-debug` configure/build、CTest 59/59 passed.
- `just asan`
  - result: pass. `linux-asan` configure/build、CTest 59/59 passed.
- `just windows-cross`
  - result: pass. `windows-mingw-debug` configure/build passed.
- `just verify`
  - result: first attempt failed at format check because host-side `scripts/format.sh` did not match Dev Container clang-format. After `just format`, pass. format-check、clang-tidy build、fresh debug CTest 59/59、ASan CTest 59/59、Windows cross build passed.
- `rg -n 'swbt_daemon_process_runtime_' swbt tests api CMakeLists.txt`
  - result: pass. no matches.
- `rg -n 'swbt_domain_t[ \t\r\n]*\*[ \t\r\n]*app|swbt_control_t[ \t\r\n]+control|swbt_runtime_host_backend_t[ \t\r\n]+runtime_backend|hid_register|hid_stop|output_handler_start|output_handler_stop|report_timer_start|report_timer_stop|report_timer_send_neutral_now|subcommand_reply_enqueue|read_device_info|time_ms' swbt\daemon\process.h`
  - result: pass. no matches.
- `rg -n '#include "runtime/host.h"' swbt\control`
  - result: pass. no matches.

## 11. 実機実行条件

実機実行は通常不要。

この work unit は ownership と internal wiring の変更であり、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop を実行しない。Switch-facing bytes、report period、BTstack source selection、timer scheduling cadence を変更しない範囲では software verification で完了判定する。

実機-facing call order、report cadence、HID registration config、adapter selection を変更する必要が出た場合は、この work unit から外すか、明示的に範囲変更する。その場合は人間の承認、専用 USB Bluetooth dongle、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、`hardware-harness`、`docs/hardware-test-log.md` の更新を必要条件にする。

## 12. 先送り事項

none.

production runner より外側の CLI backend naming、runtime status schema の外部公開、BTstack event queue、複数 controller 対応へ広がる要求は、この work unit では発生しなかった。

## 13. チェックリスト

- [x] source と use case を記録した。
- [x] 破壊的変更の許容範囲を記録した。
- [x] マイルストーンを記録した。
- [x] TDD Test List を作成した。
- [x] architecture spec を新しい composition ownership に更新した。
- [x] `control` から runtime host 具象依存を外した。
- [x] runtime host へ domain / control ownership を移した。
- [x] daemon process から domain / control / runtime backend wrapper ownership を削除した。
- [x] daemon process backend contract を縮小した。
- [x] production path を新しい runtime ownership へ付け替えた。
- [x] absence checks を追加または更新した。
- [x] relevant tests を追加または更新した。
- [x] 検証コマンドと結果を記録した。
- [x] 実機未実行理由を維持または実機実行条件を更新した。
