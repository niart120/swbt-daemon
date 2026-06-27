# Production Address Reconnect Boundary

## 1. 概要

Switch address text / bytes 変換と active reconnect / learned address 保存を `production_runner` から分離する。

完了後、address 形式の検証・正規化は daemon config と共有できる helper に寄せ、active reconnect request の組み立てと learned address 保存は production reconnect 境界で読む。

## 2. 起点 / ユースケース

source:

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- user discussion, 2026-06-28: runner 冒頭の hex parse / format も分割候補ではないか。
- 現状分析, 2026-06-28: `config.c` に address 文字列検証・正規化があり、`production_runner.c` に byte parse / format が別実装としてある。

use case:

- actor: active reconnect と config file 境界を変更する開発者。
- 入力または状態: effective reconnect address は config では text、BTstack connect request では byte array で扱われる。
- 期待する観測結果: text validation / formatting と BTstack request 変換が runner lifecycle から分離される。
- 制約: `AA:BB:CC:DD:EE:FF` 正規化、explicit address 優先、learned address 保存 behavior は変えない。

source から use case への変換:

address helper は reconnect 専用に閉じると config 側の重複が残る。daemon config 表現に紐づく `swbt/daemon/switch_address.*` 相当を作り、config と production reconnect の両方から使える形にする。

## 3. 対象範囲

- `swbt/daemon/switch_address.*` 相当を追加し、text validation / normalization / bytes parse / bytes format を集約する。
- `config.c` の address 文字列 validation を新 helper へ差し替える。
- `production_reconnect.*` を追加し、active reconnect request と learned address 保存を runner から移す。
- invalid address failure status と existing run loop continuation behavior を維持する。
- config file tests と production runner tests を更新する。

## 4. 対象外

- TOML schema の変更。
- active reconnect 成功条件、pairing-free reconnect policy、link key DB policy の変更。
- BTstack `connect` port の behavior 変更。
- HID event dispatch の分離。
- 実機検証。

## 5. 関連 spec / docs

- `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md`
- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/complete/local_072/ACTIVE_SWITCH_RECONNECT.md`
- `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`
- `docs/status.md`

## 6. 根拠監査

not applicable unless BTstack connect semantics or Switch-facing behavior changes.

This work unit should only move text / bytes conversion and existing request construction. If BTstack PSM constants, reconnect timing, or pairing behavior changes, use `source-audit` and re-scope the work.

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: address conversion の重複を減らし、active reconnect の production-specific logic を runner lifecycle から外す。
- verification: config file address tests と production reconnect tests を同じ値で比較する。

配置方針:

- address text format は daemon config representation なので `swbt/daemon` に置く。
- BTstack bridge には config text format を持ち込まない。
- production reconnect は config text から `swbt_btstack_device_connect_request_t` への adapter として扱う。

## 8. 対象ファイル

- `swbt/daemon/switch_address.*`
- `swbt/daemon/config.c`
- `swbt/daemon/config.h`
- `swbt/daemon/production_reconnect.*`
- `swbt/daemon/production_runner.c`
- `tests/daemon_switch_address_test.c`
- `tests/daemon_config_file_test.c`
- `tests/daemon_production_reconnect_test.c`
- `tests/daemon_production_runner_test.c`
- `CMakeLists.txt`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | config address setters still normalize lowercase input to uppercase colon-separated text | regression | unit | no |
| green | invalid reconnect address still rejects config without partial update | regression | unit | no |
| green | production active reconnect still converts effective text address into BTstack byte request with HID PSM values unchanged | regression | integration | no |
| green | learned address save after HID connection opened still writes uppercase text address to the configured target | regression | integration | no |
| green | active reconnect request failure still records failed hardware status without stopping the run loop | regression | integration | no |

## 10. 検証

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: daemon config representation の address text は lowercase input を受けても
  uppercase colon-separated text として保存される。
- item: config address setters still normalize lowercase input to uppercase colon-separated text.
- state: green.
- red:
  - command: `just build-debug`
  - result: fail as expected. `daemon/config.h` が `daemon/switch_address.h` を include し、
    helper が未実装のため compile failure。
- green:
  - command: `just format`
  - result: pass.
  - command: `$env:CTEST_ARGS='-R "daemon_switch_address_test|daemon_config_file_test" --output-on-failure'; just debug`
  - result: pass, 2/2 tests passed.
- notes: `swbt/daemon/switch_address.*` を追加し、config address setters は
  `swbt_daemon_switch_address_normalize()` 経由で text を正規化する。既存 config file
  regression と focused helper test の両方で lowercase to uppercase を確認した。

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: invalid reconnect address を受けた場合、config file apply は失敗し、既存
  config value を部分更新しない。
- item: invalid reconnect address still rejects config without partial update.
- state: green.
- commands:
  - `just format`
  - `$env:CTEST_ARGS='-R "daemon_switch_address_test|daemon_config_file_test" --output-on-failure'; just debug`
- result:
  - `just format`: pass.
  - focused CTest: pass, 2/2 tests passed.
- notes: `daemon_switch_address_test` に invalid input が destination を部分更新しない
  regression を追加し、既存 `daemon_config_file_test` の invalid active reconnect
  preservation checks と合わせて確認した。

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: production active reconnect は config の effective text address を BTstack
  connect request の byte array へ変換し、HID PSM 値を維持する。
- item: production active reconnect still converts effective text address into BTstack byte
  request with HID PSM values unchanged.
- state: green.
- red:
  - command: `just build-debug`
  - result: fail as expected. `daemon_production_reconnect_test` が
    `daemon/production_reconnect.h` を要求し、header 未実装で compile failure。
- green:
  - command: `just format`
  - result: pass.
  - command: `$env:CTEST_ARGS='-R "daemon_production_reconnect_test|daemon_production_runner_test" --output-on-failure'; just debug`
  - result: pass, 2/2 tests passed.
- notes: `swbt/daemon/production_reconnect.*` を追加し、request build と active reconnect
  execution を runner から分離した。runner integration test で既存 startup sequence、
  PSM、address bytes を維持していることを確認した。

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: HID connection opened event で得た address bytes は configured target へ
  uppercase text address として保存される。
- item: learned address save after HID connection opened still writes uppercase text address to
  the configured target.
- state: green.
- red:
  - command: `just build-debug`
  - result: fail as expected. `daemon_production_reconnect_test` が
    `swbt_daemon_production_reconnect_save_learned_address` を要求し、関数未宣言で
    compile failure。
- green:
  - command: `just format`
  - result: pass.
  - command: `$env:CTEST_ARGS='-R "daemon_production_reconnect_test|daemon_production_runner_test" --output-on-failure'; just debug`
  - result: pass, 2/2 tests passed.
- notes: `swbt_daemon_switch_address_format_bytes()` と
  `swbt_daemon_production_reconnect_save_learned_address()` を追加し、runner の
  HID connection opened path から保存処理を分離した。

TDD status:

- source: `work-units/wip/local_084/PRODUCTION_RUNNER_DECOMPOSITION_PLAN.md` and this
  work unit.
- use case: active reconnect request が失敗しても failed hardware status を記録し、
  production run loop は停止しない。
- item: active reconnect request failure still records failed hardware status without stopping
  the run loop.
- state: green.
- command: `$env:CTEST_ARGS='-R "daemon_production_runner_test" --output-on-failure'; just debug`
- result: pass, 1/1 test passed.
- notes: `active_reconnect_failure_reports_failed_state_without_stopping_run_loop` を含む
  existing runner integration で、分離後も failed status と run loop continuation を確認した。

Expected checks:

- `just build-debug`
- `$env:CTEST_ARGS='-R "daemon_config_file_test|daemon_launch_options_test|daemon_cli_test|daemon_production_runner_test" --output-on-failure'; just test-debug`
- `just windows-cross`

## 11. 実機実行条件

実機実行は不要。

この work unit は software fake と config file behavior で固定済みの address conversion と request construction を移す。Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop の実行は不要。

## 12. 先送り事項

none.

pairing-free reconnect、link key DB policy、実機 reconnect success の確認は既存 docs / completed work unit の領域であり、この structure change の未完了事項として扱わない。

## 13. チェックリスト

- [ ] address helper の owner を daemon config representation として記録した。
- [ ] config address validation と production byte conversion を helper 経由へ寄せた。
- [ ] active reconnect と learned address 保存を runner から分離した。
- [ ] TDD Test List の検証を実行し、結果を記録した。
- [ ] 実機未実行理由を維持した。
