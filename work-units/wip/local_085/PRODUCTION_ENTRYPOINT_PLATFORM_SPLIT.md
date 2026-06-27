# Production Entrypoint Platform Split

## 1. 概要

`apps/swbt-daemon/main.c` を実行ファイル入口に戻し、real production 起動準備と OS process support を app-local file へ分ける。

完了後、`main.c` は stdio 設定、CLI dispatch、launch options parse、diagnostic path 設定、backend 選択を読む file になる。real BTstack implementation の configure、runner 初期化、shutdown listener 接続は `apps/swbt-daemon/production_entrypoint.*` が持つ。

## 2. 起点 / ユースケース

source:

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- user discussion, 2026-06-28: apps 側に持つべきものと daemon 側に持つべきものを分けたい。
- 現状分析, 2026-06-28: `main.c` が crash dump、console control handler、config env、real BTstack setup、production runner init を同居させている。

use case:

- actor: daemon executable の起動経路を読む開発者。
- 入力または状態: `apps/swbt-daemon/main.c` が production の concrete setup まで直接持っている。
- 期待する観測結果: app executable が real BTstack implementation を接続し、daemon library は abstract ports のまま維持される。
- 制約: CLI surface、production / noop backend selection、diagnostic path semantics は変えない。

source から use case への変換:

まず app と daemon library の境界を固定する。これにより、後続の runner 分割が real BTstack implementation や Windows console handler に引きずられない。

## 3. 対象範囲

- `apps/swbt-daemon/production_entrypoint.*` を追加し、real BTstack setup と production runner 呼び出しを移す。
- Windows crash dump と console shutdown listener を `apps/swbt-daemon/platform_*.*` など app-local support へ移す。
- `main.c` から production setup の詳細を削る。
- `CMakeLists.txt` の `swbt-daemon` executable sources を更新する。
- 既存 CLI behavior と backend selection を維持する。

## 4. 対象外

- `swbt/daemon/production_runner.*` 内部の HID、reconnect、IPC pump、report timer、shutdown 分割。
- `swbt/btstack_bridge/production_ports.*` の validator 移動。
- CLI command の追加または出力形式変更。
- 実機検証。

## 5. 関連 spec / docs

- `docs/status.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- `work-units/complete/local_078/DAEMON_CLI_MANAGEMENT_SURFACE.md`
- `work-units/complete/local_083/MODULE_RENAME_AND_PLACEMENT_CLEANUP.md`

## 6. 根拠監査

not applicable.

この work unit は executable composition の構造変更に限定する。BTstack source selection、WinUSB/libusb behavior、Switch-facing bytes、report period は変更しない。

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: app / daemon / BTstack bridge の owner 境界を先に固定し、後続の runner 分割を daemon library 内の構造変更に限定する。
- verification: `main.c` 分割前後で CLI tests、launch options tests、production runner tests、Windows cross build を比較する。

配置方針:

- real BTstack implementation を知る code は `apps/swbt-daemon` と `swbt/btstack_bridge/production_btstack_impl.*` に限定する。
- `swbt_daemon_process` target は `swbt_btstack_production_impl` に依存させない。
- Windows console handler は daemon library ではなく executable process support として扱う。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `apps/swbt-daemon/production_entrypoint.*`
- `apps/swbt-daemon/platform_*.*`
- `CMakeLists.txt`
- `tests/daemon_cli_test.c`
- `tests/daemon_launch_options_test.c`
- `tests/daemon_production_runner_test.c`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | management commands still return before production startup composition is invoked | regression | unit/integration | no |
| green | noop backend startup still avoids real BTstack production implementation | regression | unit/integration | no |
| green | production startup still configures link key DB, HCI dump, adapter location, learned address target, and shutdown listener before runner main | regression | integration | no |
| todo | executable target links after moving app-local sources out of `main.c` | regression | build | no |
| refactor-skipped | `main.c` no longer owns production BTstack wiring or platform process support | regression | source/build | no |

## 10. 検証

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: daemon executable の起動経路を読む開発者が、`main.c` から production
  concrete setup と platform process support を追わなくてよい。
- item: `main.c` no longer owns production BTstack wiring or platform process support.
- state: refactor-skipped.
- red:
  - command: `$env:CTEST_ARGS='-R production_entrypoint_boundary_cmake_test --output-on-failure'; just debug`
  - result: fail as expected. `apps/swbt-daemon/production_entrypoint.c is missing`.
- green:
  - command: `$env:CTEST_ARGS='-R "production_entrypoint_boundary_cmake_test|daemon_cli_test|daemon_launch_options_test|daemon_production_runner_test|btstack_production_hci_dump_test" --output-on-failure'; just debug`
  - result: pass, 5/5 tests passed.
  - command: `just format`
  - result: pass.
  - command: same targeted `just debug` after formatting.
  - result: pass, 5/5 tests passed.
- notes: green 後の追加構造変更は行っていない。formatter のみで、refactor 本体は
  skipped とした。

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: management command は daemon runtime startup を行わず、production
  entrypoint 分割後も CLI 結果だけを返す。
- item: management commands still return before production startup composition is invoked.
- state: green.
- command: `$env:CTEST_ARGS='-R "daemon_cli_test|swbt_daemon_help_test|swbt_daemon_config_smoke_test" --output-on-failure'; just test-debug`
- result: pass, 3/3 tests passed.
- notes: この item は既存の CLI behavior の regression check。追加実装は不要。

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: `--backend noop` は production entrypoint 分割後も real BTstack setup を通らず、
  daemon process の noop backend で起動する。
- item: noop backend startup still avoids real BTstack production implementation.
- state: green.
- command: `$env:CTEST_ARGS='-R "swbt_daemon_noop_smoke_test|daemon_launch_options_test" --output-on-failure'; just test-debug`
- result: pass, 2/2 tests passed.
- notes: source boundary test と合わせ、`main.c` の noop branch が production
  entrypoint を経由しないことを確認した。

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: production backend は分割後も link key DB、HCI dump、adapter location、
  learned address target、shutdown listener を runner main 前に構成する。
- item: production startup still configures link key DB, HCI dump, adapter location, learned
  address target, and shutdown listener before runner main.
- state: green.
- command: `$env:CTEST_ARGS='-R "daemon_production_runner_test|btstack_production_hci_dump_test" --output-on-failure'; just test-debug`
- result: pass, 2/2 tests passed.
- notes: BTstack source selection、report timing、Switch-facing bytes は変更していない。

Expected checks:

- `just build-debug`
- `$env:CTEST_ARGS='-R "daemon_cli_test|daemon_launch_options_test|daemon_production_runner_test|btstack_production_hci_dump_test" --output-on-failure'; just test-debug`
- `just windows-cross`

## 11. 実機実行条件

実機実行は不要。

この work unit は startup composition の配置を変えるだけで、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop の意味を変更しない。real hardware run が必要になった場合は、この work unit の範囲から外す。

## 12. 先送り事項

none.

後続の runner 内部分割は `local_086` から `local_093` として起票済みであり、この record の未完了事項として扱わない。

## 13. チェックリスト

- [ ] `main.c` から real production setup と Windows process support を分離した。
- [ ] `swbt-daemon` executable source list を更新した。
- [ ] `swbt_daemon_process` target が real BTstack implementation に依存していないことを確認した。
- [ ] TDD Test List の検証を実行し、結果を記録した。
- [ ] 実機未実行理由を維持した。
