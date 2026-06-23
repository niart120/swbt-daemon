# Architecture Journey Integration Hardening

## 1. 概要

architecture cutover 後の journey test を強化する work unit。

`local_056` は旧 session、mailbox、runtime、aggregate target の不存在と新境界の基本 journey を固定した。`local_058` では shutdown neutral retry failure を閉じたが、現行 journey が JSON IPC から fake HID、trailing neutral、power-off までを一つの production-like 経路で縦断していない点を先送り事項として残した。

この work unit では、実機を使わず fake adapter で production shutdown まで通す integration test を作り、cutover 後の境界が分断されていないことを software gate で確認できるようにする。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md` の先送り事項: architecture journey が JSON から fake HID trailing neutral と power-off まで一経路で通る。
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md` の architecture journey と absence acceptance。
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md` の実機 H1 pass。H1 は hardware evidence であり、日常の software gate ではない。
- `spec/architecture/daemon-architecture-cutover.md` の architecture journey 方針。

use case:

- actor: maintainer、reviewer、CI。
- 入力または状態: JSON IPC command、fake IPC connection、fake HID / timer adapter、daemon host shutdown。
- 期待する観測結果: acquire、set_state、report tick、owner disconnect neutral、reacquire、shutdown trailing neutral、power-off / run-loop exit が一つの fake production journey で観測できる。
- 制約: Switch-facing bytes、BTstack source selection、report period、実機 pairing は変更しない。
- 対象外: hardware H1 の再実行、production adapter table 分割、status metrics schema 再設計。

source から use case への変換:

`local_058` の failure cleanup は個別 regression で閉じた。この work unit は、その修正を含む production-like path を縦断で観測する gate として切る。

## 3. 対象範囲

- 現行 `tests/architecture_journey_test.c` と `tests/daemon_production_backend_test.c` の責務を棚卸しする。
- JSON IPC command から `swbt_app_t`、daemon host、fake HID send、shutdown power-off まで通る journey を追加する。
- shutdown neutral が fake HID send と power-off の順序で観測できるようにする。
- failure cleanup item と正常 shutdown item を同じ journey に混ぜず、test 名で意図を分ける。
- architecture journey が aggregate target や旧 runtime symbol に依存しないことを維持する。

## 4. 対象外

- 実機 H1 の再実行。
- BTstack callback registration、HID descriptor、report period の変更。
- `swbt_btstack_production_adapter_t` の分割。
- include boundary の compile-time 強制。
- status metrics の field 追加。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`
- `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md`
- `docs/status.md`

## 6. 根拠監査

not applicable。

この work unit は fake adapter による integration journey を扱う。Switch protocol byte、BTstack source selection、WinUSB/libusb fact は追加または変更しない。実装中に BTstack callback registration、timer scheduling、Switch-facing bytes を変更する必要が出た場合は範囲を切り直す。

## 7. 設計メモ

- journey は旧 compatibility symbol の不在を再確認するためではなく、新境界を一経路で通すために置く。
- hardware H1 は pass 済みの証跡として参照し、日常 CI の代替にしない。
- fake adapter は HCI dump の再現ではなく、host が送る意図と順序を観測する test double として扱う。
- shutdown failure cleanup の regression は残しつつ、正常系 journey では failure injection を行わない。

## 8. 対象ファイル

- `tests/architecture_journey_test.c`
- `tests/daemon_production_backend_test.c`
- `tests/cmake/architecture_absence_test.cmake`
- `CMakeLists.txt`
- `spec/architecture/daemon-architecture-cutover.md`
- `docs/status.md`
- `work-units/wip/local_059/ARCHITECTURE_JOURNEY_INTEGRATION_HARDENING.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | JSON IPC set_state reaches fake HID report send through daemon host without old runtime symbols | regression | integration | no |
| refactor-done | owner disconnect in the journey emits neutral before reacquire | regression | integration | no |
| refactor-skipped | daemon shutdown in the journey emits trailing neutral before fake power-off / run-loop exit | regression | integration | no |
| green | architecture journey target links only cutover-era module targets | regression | build | no |
| todo | failure-cleanup regression remains separate from normal journey naming | regression | unit | no |

## 10. 検証

TDD status:

- source: `local_058` の先送り事項。
- use case: production backend の run loop 中に JSON IPC `acquire` / `set_state` を処理し、daemon host の state provider から fake HID send へ状態が届く。
- item: JSON IPC set_state reaches fake HID report send through daemon host without old runtime symbols。
- state: refactor-skipped。
- commands:
  - red: `just build-debug` pass。`CTEST_ARGS="-R daemon_production_backend_test --output-on-failure" just test-debug` は `hid send calls: expected 1, got 0` で fail。
  - green: `just build-debug` pass。`CTEST_ARGS="-R daemon_production_backend_test --output-on-failure" just test-debug` pass。
  - format: `just format` pass。
- notes: `tdd-one-cycle` と `refactor-after-green` に従った。green 後の見直しでは、今回の item で必要な構造変更は fake timer の観測点追加だけだったため `refactor-skipped` とした。Switch-facing byte、BTstack source selection、report period は変更していない。

TDD status:

- source: `local_058` の先送り事項。
- use case: production backend の run loop 中に owner disconnect を処理し、reacquire 前に neutral report が fake HID send として観測される。
- item: owner disconnect in the journey emits neutral before reacquire。
- state: refactor-done。
- commands:
  - red: `just build-debug` pass。`CTEST_ARGS="-R daemon_production_backend_test --output-on-failure" just test-debug` は `hid send calls: expected 3, got 0` で fail。
  - green: `just build-debug` pass。`CTEST_ARGS="-R daemon_production_backend_test --output-on-failure" just test-debug` pass。
  - refactor: `just format` pass。`just build-debug` pass。`CTEST_ARGS="-R daemon_production_backend_test --output-on-failure" just test-debug` pass。
- notes: green 後に fake run loop の JSON command 注入と HID event 注入を helper へ分離した。観測対象の順序は A report、disconnect neutral、reacquire 後 A report のまま。実機は不要。

TDD status:

- source: `local_058` の先送り事項。
- use case: JSON state が入った production-like journey の shutdown で、trailing neutral が fake HID send として power-off / run-loop exit より前に観測される。
- item: daemon shutdown in the journey emits trailing neutral before fake power-off / run-loop exit。
- state: refactor-skipped。
- commands:
  - red: `just build-debug` pass。`CTEST_ARGS="-R daemon_production_backend_test --output-on-failure" just test-debug` は `hid send calls: expected 2, got 1` で fail。
  - green: `just format` pass。`just build-debug` pass。`CTEST_ARGS="-R daemon_production_backend_test --output-on-failure" just test-debug` pass。
- notes: fake `report_timer_send_neutral_now` でも HIDP input message を記録し、既存の step order check と合わせて trailing neutral が `STEP_POWER_OFF` と `STEP_RUN_LOOP_TRIGGER_EXIT` より前に出ることを固定した。追加の refactor は不要。

TDD status:

- source: `local_056` の architecture cutover absence gate と `local_058` の follow-up。
- use case: `architecture_journey_test` が旧 aggregate target や旧 runtime ではなく cutover 後の daemon host target だけを link する。
- item: architecture journey target links only cutover-era module targets。
- state: green。
- commands:
  - green: `CTEST_ARGS="-R architecture_absence_cmake_test --output-on-failure" just test-debug` pass。
- notes: 既存の `tests/cmake/architecture_absence_test.cmake` が `target_link_libraries(architecture_journey_test PRIVATE swbt_daemon_host)` を要求している。現在の gate で item を満たしているため、新しい build scaffolding は追加しない。

開始時の確認:

- 対象 source は `local_058` の先送り事項である。
- `local_057` の H1 は pass 済みであり、この work unit は実機証跡の再取得を目的にしない。

## 11. 実機実行条件

通常は実機不要。fake adapter と CTest で閉じる。

実機が必要になるのは、BTstack initialization、callback registration、HID send scheduling、Switch-facing report bytes、shutdown の実機順序を変更した場合である。その場合は `hardware-harness` を使い、専用 USB Bluetooth dongle、WinUSB、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、`docs/hardware-test-log.md` への記録を条件にする。

## 12. 先送り事項

none。起票時点の先送り事項は、この record の source として取り込んだ。

## 13. チェックリスト

- [x] source を `local_058` の先送り事項から特定した。
- [x] use case を integration journey として定義した。
- [x] red test を追加した。
- [x] green 実装を行った。
- [x] `just debug` または targeted CTest を実行した。
- [ ] full verification の要否を判定した。
- [x] 実機未実行理由または実機結果を記録した。
