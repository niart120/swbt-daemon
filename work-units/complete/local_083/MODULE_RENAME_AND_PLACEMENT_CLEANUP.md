# Module Rename And Placement Cleanup

## 1. 概要

module 名、directory 名、CMake target 名の粒度を揃え、現行 tree の読み取り負荷を下げる。

この work unit は rename / placement cleanup に閉じる。IPC JSON wire format、public C ABI の外部形、Switch-facing report bytes、report period、BTstack source selection、実機 pairing / report loop の挙動は変えない。

完了後は、狭い責務を持つ directory と広い composition directory の差が名前から分かる状態にする。BTstack callback order の変更、device event listener の導入は扱わない。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-28: `apps / app`、`daemon / runtime`、`production_backend / production_adapter / production_btstack`、`application`、`core` の名前と配置が似ており、不要な経路や過剰な抽象に見える部分を整理したい。
- 現状分析, 2026-06-28: 旧 `swbt_ipc_session_t`、`state_mailbox`、旧 `swbt_daemon_runtime_t`、旧 production backend ops、`swbt_core` は削除済み。現在の違和感は旧経路の残存ではなく、新しい境界の名前と粒度の不揃いが主因である。
- `spec/architecture/daemon-architecture-cutover.md`: JSON Lines IPC と public C ABI は `swbt/control` を通り、`swbt/runtime` は IPC を含まない runtime resource lifecycle に限定する。daemon process は IPC runner と runtime host の composition root である。
- `work-units/complete/local_082/CONTROL_RUNTIME_BOUNDARY_IMPLEMENTATION.md`: `swbt/control` と `swbt/runtime` は旧 runtime 復活ではなく、IPC と public C ABI の共通操作境界、IPC-free runtime 境界として追加された。

use case:

- actor: swbt-daemon の後続 refactor を行う開発者。
- 入力または状態:
  - 現行 tree に `apps` と `application/app`、`daemon/host` と `runtime/host`、production 系の複数 module、広い `core` が混在している。
  - CMake target と directory 名に `swbt_support` / `swbt/core` のような名前ずれがある。
- 期待する観測結果:
  - rename 後も production / noop 起動、IPC command、public C ABI smoke、既存 unit tests の観測可能な結果が変わらない。
  - `runtime` が IPC start / stop を持たない境界は維持される。
  - broad composition と narrow domain/runtime/control/support が名前から判別できる。
  - 旧 architecture token absence checks は新しい名前に合わせて維持される。
- 制約:
  - rename-only の構造変更として扱う。
  - C symbol、file path、target 名の rename は internal API 破壊を許容するが、外部 IPC / public C ABI / Switch-facing behavior は変えない。
  - rename が責務移動を必要とする場合は、この work unit では実装せず先送り事項にする。

source から use case への変換:

手広い module と狭い module の差が名前に出ていないため、後続の責務整理を始める前に rename / placement cleanup で読み取り境界を揃える。まず名前と配置だけを直し、統合や責務移動は rename 後に残る違和感として判断する。

## 3. 対象範囲

- rename map を確定し、record に追記してから file / directory / target rename を行う。
- `swbt/application` の名前を、daemon の論理状態 model であることが分かる名前へ寄せる。
- `swbt/core` と `swbt_support` target の名前ずれを解消する。
- `swbt/daemon/host.*` の名前を、runtime host ではなく daemon process composition であることが分かる名前へ寄せる。
- `production_backend`、`production_adapter`、`production_btstack` の名前を、daemon 側 orchestration、BTstack-facing port table、pinned BTstack concrete implementation の差が分かる名前へ寄せる。
- CMake target、include path、tests、architecture boundary checks、docs/status、関連 work unit / spec の参照を新しい名前に合わせる。
- rename 後に旧 path / target / include 名が残っていないことを `rg` と CMake boundary tests で確認する。

## 4. 対象外

- `swbt/runtime` に IPC start / stop を入れること。
- `swbt/control` と `swbt/application` 相当の統合。
- `core` 内の機能分割や未使用 logging の削除。
- `swbt/daemon/host` 相当の composition root を削除して `swbt/runtime` へ吸収すること。
- `btstack_bridge/device_recv` の rename / delete、device event listener registration。
- IPC JSON wire format、public C ABI の外部 shape、Switch-facing report bytes、report period、BTstack source selection の変更。
- 実機 pairing、HID advertising、report loop の再検証。
- `vendor/btstack` の編集。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `docs/status.md`
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`
- `work-units/complete/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md`
- `work-units/complete/local_080/DEVICE_API_PRODUCTION_PATH.md`
- `work-units/complete/local_082/CONTROL_RUNTIME_BOUNDARY_IMPLEMENTATION.md`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/architecture_absence_test.cmake`

## 6. 根拠監査

not applicable.

この work unit は名前、配置、CMake target、include path、docs reference の構造変更に限定する。Switch HID report bytes、subcommand bytes、SPI address、rumble packet、HID descriptor、BTstack source selection、report period、WinUSB/libusb facts を追加または変更しない。

BTstack callback order、timer scheduling、HID registration config、adapter selection、実機 link state 名を変更する必要が出た場合は、この work unit の範囲から外し、`source-audit` と `hardware-harness` の要否を再判断する。

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: 後続の責務削減に入る前に、名前と配置の不揃いを解消することで、実際に残る過剰抽象を判別しやすくする。
- verification: rename 前後で targeted `rg`、CMake boundary tests、unit tests、必要に応じて `just debug` を比較する。

初期 rename 方針:

- まず directory / target / internal type / docs reference の対応表を作る。
- 対応表に載せた rename だけを実施する。
- rename だけでは意味が通らない箇所は責務移動候補として先送りし、この work unit で無理に統合しない。
- `runtime` は IPC-free boundary として維持する。`daemon` 側の composition 名だけを調整する。

実施 rename map:

| 旧名 | 新名 | 意図 |
|---|---|---|
| `swbt/application/` | `swbt/domain/` | daemon logical state / policy を表す module として `apps/` と区別する。 |
| `swbt/application/app.*` | `swbt/domain/domain.*` | `app` を実行ファイルではなく domain model として読む。 |
| `swbt/application/control_lease.*` | `swbt/domain/lease.*` | domain 内の owner lease として短くする。 |
| `swbt/application/status.h` | `swbt/domain/status.h` | status model を domain 所有に寄せる。 |
| `swbt/core/` | `swbt/support/` | `swbt_support` target と directory 名を一致させる。 |
| `swbt/daemon/host.*` | `swbt/daemon/process.*` | `runtime/host.*` ではなく daemon process composition root であることを名前に出す。 |
| `swbt/daemon/production_backend.*` | `swbt/daemon/production_runner.*` | production lifecycle orchestration として読む。CLI の `--backend` 概念は維持する。 |
| `swbt/btstack_bridge/production_adapter.*` | `swbt/btstack_bridge/production_ports.*` | BTstack-facing port table であり daemon 側 runner と区別する。 |
| `swbt/btstack_bridge/production_btstack.*` | `swbt/btstack_bridge/production_btstack_impl.*` | pinned BTstack concrete implementation であることを名前に出す。 |
| `swbt_application` | `swbt_domain` | CMake target と directory 名を合わせる。 |
| `swbt_daemon_host` | `swbt_daemon_process` | daemon process composition target として読む。 |
| `swbt_btstack_adapter` | `swbt_btstack_bridge` | directory 名と target 名を合わせる。 |
| `swbt_btstack_production_adapter` | `swbt_btstack_production_impl` | concrete production implementation target として読む。 |
| `application_*_test`, `daemon_host_test`, `daemon_production_backend_test` | `domain_*_test`, `daemon_process_test`, `daemon_production_runner_test` | test 名を対象 module 名に合わせる。 |

## 8. 対象ファイル

- `CMakeLists.txt`
- `swbt/domain/*`
- `swbt/support/*`
- `swbt/daemon/process.*`
- `swbt/daemon/production_runner.*`
- `swbt/btstack_bridge/production_ports.*`
- `swbt/btstack_bridge/production_btstack_impl.*`
- `apps/swbt-daemon/main.c`
- `api/swbt_c_api.c`
- `tests/*`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/architecture_absence_test.cmake`
- `tests/cmake/compile_include_boundaries_test.cmake`
- `docs/status.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `spec/protocols/daemon-ipc-v1.md`
- `spec/references/*`
- related completed work unit records only when references would otherwise point at removed paths.

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | rename map に載せた old include path、old file path、old CMake target 名が source / tests / docs の current reference から消える | regression | build/review | no |
| done | `runtime` boundary が IPC start / stop を含まないまま維持される | regression | architecture | no |
| done | `swbt` shared target が `swbt_ipc` に link しない状態を維持する | regression | build | no |
| done | production / noop daemon startup tests が rename 後も同じ経路を通る | regression | unit/integration | no |
| done | public C ABI smoke が rename 後も `swbt_control` 相当の境界を通り、IPC に依存しない | regression | unit/build | no |
| deferred | `support` 内の logging / diagnostics / metrics / version / spin_lock の分割可否を判断する | deferred | design | no |

## 10. 検証

software checks:

作業開始時点:

- `git branch --show-current`
  - result: `main`
- `git status --short`
  - result: clean

実施:

- `scripts/format.sh`
  - result: pass。
- `$env:CTEST_ARGS='-R \"^(architecture_absence_test|include_boundaries_test|compile_include_boundaries_test|domain_lease_test|domain_command_test|daemon_process_test|runtime_host_test|control_test|swbt_c_api_test|daemon_production_runner_test|btstack_production_hci_dump_test)$\" --output-on-failure'; just test-debug`
  - result: first sandboxed attempt failed before test execution at Dev Container `docker ps` setup.
  - result: escalated rerun reached the existing build tree, but only 4 stale CTest entries matched. This run is not used as rename coverage.
- `just debug`
  - result: pass。fresh configure、unit test target build、CTest 50/50 passed。
- `rg -n "production_adapter\\.[ch]|production_backend\\.[ch]|daemon/host\\.[ch]|daemon_host_test|daemon_production_backend_test|application_command_test|application_control_lease_test|swbt/application/|swbt/core/|swbt_daemon_host|swbt_application|swbt_btstack_adapter|swbt_btstack_production_adapter_t|btstack-production-adapter" CMakeLists.txt swbt apps api tests docs/status.md spec/protocols spec/operations spec/references`
  - result: no matches。
- `rg -n "#include \\\"(application|core|daemon/host|btstack_bridge/production_adapter|btstack_bridge/production_btstack)" swbt apps api tests`
  - result: no matches。
- `rg -n "add_library\\(swbt_(application|core|daemon_host|btstack_adapter|btstack_production_adapter)|add_executable\\((application_|daemon_host|daemon_production_backend)" CMakeLists.txt`
  - result: no matches。

PR 前または完了前:

- `just verify`
  - result: first attempt failed at format-check. Windows-side `scripts/format.sh` と Dev Container 側 clang-format の判定がずれていたため、`just format` を実行した。
- `just format`
  - result: pass。Dev Container 側 clang-format を適用した。
- `just verify`
  - result: pass。format-check、clang-tidy、fresh debug configure / unit test target build / CTest、ASan / UBSan、Windows MinGW cross build が成功した。

## 11. 実機実行条件

実機実行は不要。

この work unit は Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop、Switch-facing bytes、report period を変更しない。実機経路に触れる変更が必要になった場合は、この work unit から外し、人間の明示承認、専用 USB Bluetooth dongle、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、`hardware-harness`、`docs/hardware-test-log.md` の更新条件を別 work unit で定義する。

## 12. 先送り事項

- 観測: `support` には diagnostics、metrics、logging、version、spin_lock が同居している。
  先送り理由: 分割は target topology と include boundary の変更を伴い、単純 rename より広い。
  次の置き場: rename 後も `support` の広さが問題として残る場合、`spec/dev-journal.md` または別 work unit record に起こす。

- 観測: `btstack_bridge/device_recv` と device event listener registration は packet handling 境界の整理候補である。
  先送り理由: packet dispatch、listener lifetime、同期処理時間、将来の queueing 判断へ広がるため、この work unit では扱わない。
  次の置き場: `spec/dev-journal.md` の device event listener 先送り記録、または別 work unit record。

## 13. チェックリスト

- [x] rename map を確定してこの record に追記した。
- [x] file / directory / CMake target / include path を rename map に従って更新した。
- [x] architecture / include boundary checks を新しい名前に合わせた。
- [x] docs/status と architecture spec の current reference を更新した。
- [x] 対象外と先送り事項を更新した。
- [x] 検証コマンドと結果を記録した。
- [x] 実機未実行理由を維持または更新した。
- [x] old path / target / include 名の current reference が残っていないことを最終確認した。
- [x] `just verify` を実行した。
