# Production Ports Validation Boundary

## 1. 概要

`production_runner.c` 冒頭にある `swbt_btstack_production_ports_t` validator 群を、port table を所有する `swbt/btstack_bridge/production_ports.*` へ移す。

完了後、runner は `swbt_btstack_production_ports_has_ipc_pump()` と `swbt_btstack_production_ports_is_valid()` 相当を呼ぶだけになる。BTstack bridge は daemon internal type を include しない。

## 2. 起点 / ユースケース

source:

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- user discussion, 2026-06-28: `production_runner` 冒頭に validator があり、さらに割れるのではないか。
- 現状分析, 2026-06-28: `*_port_is_valid` と `ports_are_valid` が runner 本体の前に並んでいる。

use case:

- actor: BTstack production ports を変更する開発者。
- 入力または状態: port table の required callback 定義が runner 側 static helper に埋まっている。
- 期待する観測結果: port table の validation rule は `production_ports.*` で確認でき、runner は lifecycle に集中する。
- 制約: callback 必須条件、error code、startup order は変えない。

source から use case への変換:

validator は daemon lifecycle ではなく port table contract である。BTstack bridge 側へ移しても daemon internal include を増やさなければ、既存 include boundary と整合する。

## 3. 対象範囲

- `swbt_btstack_production_ports_has_ipc_pump()` を追加する。
- `swbt_btstack_production_ports_is_valid()` を追加する。
- runner init と runner main の validation 呼び出しを新 API に差し替える。
- validator tests を追加または既存 production runner tests で固定する。
- include boundary tests を確認する。

## 4. 対象外

- `swbt_btstack_production_ports_t` の field 追加、削除、group 再編。
- real BTstack implementation の callback 実装変更。
- IPC pump、report timer、HID session の分離。
- 実機検証。

## 5. 関連 spec / docs

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- `work-units/complete/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md`
- `work-units/complete/local_083/MODULE_RENAME_AND_PLACEMENT_CLEANUP.md`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/compile_include_boundaries_test.cmake`

## 6. 根拠監査

not applicable.

この work unit は required callback validation の配置変更であり、BTstack source selection、WinUSB/libusb behavior、Switch-facing bytes を変更しない。

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: runner 冒頭の port contract noise を消し、BTstack-facing table の owner を明確にする。
- verification: invalid ports / missing IPC pump の既存 behavior と include boundary を確認する。

配置方針:

- `production_ports.*` は BTstack-facing table の shape と validation を所有する。
- `production_ports.*` は `daemon/process.h`、`daemon/ipc_runner.h`、`daemon/production_runner.h` を include しない。
- `production_runner` は validation の詳細を知らない。

## 8. 対象ファイル

- `swbt/btstack_bridge/production_ports.h`
- `swbt/btstack_bridge/production_ports.c`
- `swbt/daemon/production_runner.c`
- `tests/daemon_production_runner_test.c`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/compile_include_boundaries_test.cmake`
- `CMakeLists.txt` if a new focused test is added

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | runner init still accepts ports with only IPC pump for initialization-time validation | regression | unit | no |
| green | runner main still rejects ports missing required production callbacks before startup side effects | regression | unit | no |
| green | BTstack bridge production ports validation remains free of daemon internal includes | regression | architecture | no |
| todo | full fake production ports still pass validation and preserve existing startup sequence tests | regression | integration | no |

## 10. 検証

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: runner init は startup-time callback 一式を要求せず、IPC pump callback だけを
  initialization-time validation として受け付ける。
- item: runner init still accepts ports with only IPC pump for initialization-time validation.
- state: green.
- red:
  - command: `just build-debug`
  - result: fail as expected. `swbt_btstack_production_ports_has_ipc_pump` の prototype がなく、
    `btstack_production_ports_test` が implicit declaration で compile failure。
- green:
  - command: `just format`
  - result: pass.
  - command: `$env:CTEST_ARGS='-R "btstack_production_ports_test|daemon_production_runner_test" --output-on-failure'; just debug`
  - result: pass, 2/2 tests passed.
- notes: `swbt_btstack_production_ports_has_ipc_pump()` を `production_ports.*` に追加し、
  runner init と runner main の IPC pump check をこの API 経由に差し替えた。

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: runner main は startup side effects の前に required production callbacks を
  検証し、callback 欠落時は既存の invalid argument path を維持する。
- item: runner main still rejects ports missing required production callbacks before startup side
  effects.
- state: green.
- red:
  - command: `just build-debug`
  - result: fail as expected. `swbt_btstack_production_ports_is_valid` の prototype がなく、
    `btstack_production_ports_test` が implicit declaration で compile failure。
- green:
  - command: `just format`
  - result: pass.
  - command: `$env:CTEST_ARGS='-R "btstack_production_ports_test|daemon_production_runner_test" --output-on-failure'; just debug`
  - result: pass, 2/2 tests passed.
- notes: startup-time callback validator 群を `production_ports.c` へ移し、runner main は
  `swbt_btstack_production_ports_is_valid()` を呼ぶだけにした。

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: BTstack bridge の production ports validation は daemon internal type を
  include せず、bridge 側の table contract として維持される。
- item: BTstack bridge production ports validation remains free of daemon internal includes.
- state: green.
- commands:
  - `rg -n "#include \"daemon/" swbt\btstack_bridge\production_ports.h swbt\btstack_bridge\production_ports.c`
  - `$env:CTEST_ARGS='-R "include_boundaries_cmake_test|compile_include_boundaries_cmake_test" --output-on-failure'; just test-debug`
- result:
  - `rg` は no matches。daemon internal include はない。
  - CTest boundary checks pass, 2/2 tests passed.
- notes: `production_ports.*` は `daemon/process.h`、`daemon/ipc_runner.h`、
  `daemon/production_runner.h` を include していない。

Expected checks:

- `just build-debug`
- `$env:CTEST_ARGS='-R "daemon_production_runner_test|include_boundaries_test|compile_include_boundaries_test" --output-on-failure'; just test-debug`
- `rg -n "#include \"daemon/" swbt/btstack_bridge/production_ports.*`

## 11. 実機実行条件

実機実行は不要。

この work unit は validation helper の配置変更であり、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop に触れない。

## 12. 先送り事項

none.

`swbt_btstack_production_ports_t` の field 再編は既存の対象外であり、この work unit の後続事項として起票しない。

## 13. チェックリスト

- [ ] production ports validation API を追加した。
- [ ] runner から static validator 群を削除した。
- [ ] BTstack bridge から daemon internal type を参照していないことを確認した。
- [ ] TDD Test List の検証を実行し、結果を記録した。
- [ ] 実機未実行理由を維持した。
