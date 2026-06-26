# Daemon Launch Mode Flags

## 1. 概要

daemon の起動モードと一時診断出力を、環境変数中心の指定から CLI flag へ移す work unit。

`local_073` では、実機 reconnect 検証に必要だった `--config` と `--link-key-db` を先に実装した。この record は、そこで分離した残タスクである production / noop 起動モード、実機承認 env の扱い、diagnostic path の CLI flag 化を扱う。

## 2. 起点 / ユースケース

source:

- user discussion, 2026-06-26: 実機承認 env はそろそろ外せるのではないか。診断出力 path は環境変数で有効化すると意図しない場所へファイルが作られ得るため、実行時引数の flag に寄せる案が出た。
- user discussion, 2026-06-26: backend 指定は何かを確認した結果、現行の noop は test / smoke 用であり、daemon の通常起動は production を既定にして、test 時だけ明示的に noop を与える方針が妥当と判断した。
- `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`: `--config`、learned Switch address target、`--link-key-db`、pairing-free active reconnect までを先に実装した。production 既定化、`--backend noop`、diagnostic CLI flag 化、hardware approval env 整理はこの work unit に分離する。
- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`: 設定ファイル schema は `ipc`、`report`、`device.profile` に絞り、backend 起動モード、実機承認、診断出力 path は別 work unit へ切り出した。
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`: 環境変数を backend selection、hardware safety gate、runtime override、diagnostic sink に分類した。

use case:

- actor: maintainer、hardware operator、unit / integration test。
- 入力または状態: daemon を引数なしで起動する通常運用、Bluetooth adapter を開かず host lifecycle だけを通したい test / smoke、実機検証時だけ trace / HCI dump / crash dump を保存したい起動。
- 期待する観測結果:
  - `swbt-daemon` は既定で production backend を選ぶ。
  - `swbt-daemon --backend noop` は Bluetooth adapter を開かず、現行 noop host 経路を選ぶ。
  - 不正な `--backend` 値は adapter open 前に失敗する。
  - 診断出力 path は CLI flag を指定した起動だけで有効になる。
  - 診断出力 path が不正な場合は、adapter open 前に失敗する。
  - 実機承認 env を code-level gate として残すか、互換期間を置いて撤去するかを決める。
- 制約: エージェント運用上の実機コマンド承認は残す。Bluetooth adapter open、Switch pairing、HID advertising、report loop は `hardware-harness` の承認境界に従う。

source から use case への変換:

`local_071` は永続設定の対象を runtime 値へ絞った。backend selection と診断出力 path は永続設定よりも起動コマンドに現れている方が事故を減らせるため、CLI parser の起動モードとして扱う。

## 3. 対象範囲

- `--backend production|noop` を設計し、既定を production にする。
- `SWBT_DAEMON_BACKEND` の削除または互換期間を決める。
- `SWBT_RUN_HARDWARE` / `SWBT_HARDWARE_APPROVED` を production backend の code gate から削除するか、互換期間を決める。
- `--trace-path`、`--hci-dump-path`、`--crash-dump-path` を設計する。
- `SWBT_DIAGNOSTIC_TRACE_PATH`、`SWBT_HCI_DUMP_TRACE_PATH`、`SWBT_CRASH_DUMP_PATH` の削除または互換期間を決める。
- 診断出力 path の open failure policy を決める。
- test / CI / smoke entrypoint が Bluetooth adapter を開かない場合は `--backend noop` を明示するよう更新する。
- `docs/status.md` と関連 operations docs に、新しい起動モードと実機承認境界を記録する。

## 4. 対象外

- `--config`、TOML config file 読み込み、learned Switch address target。`local_073` で扱う。
- `--link-key-db` と TLV-backed Classic link key DB。`local_073` で扱う。
- `local_071` の設定ファイル schema、TOML parser / serializer、環境変数 runtime override precedence。
- active reconnect、Switch address の取得 / 保存 / 削除。
- adapter selector、VID/PID selector、複数 Bluetooth adapter の選択 policy。
- Switch protocol byte、device info payload、report period の既定値変更。
- service manager、installer、Windows registry、binary release。

## 5. 関連 spec / docs

- `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`
- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`
- `docs/status.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `spec/architecture/daemon-architecture-cutover.md`

## 6. 根拠監査

not applicable for the initial record。Switch protocol bytes、BTstack source selection、report timing、WinUSB/libusb の採用値はこの work unit の主対象ではない。

production 既定化は Bluetooth adapter open に直結するため、実機実行や docs の表現では `hardware-harness` を使う。

## 7. 設計メモ

- `backend` は adapter 種別ではなく daemon host backend の選択である。`production` は BTstack USB transport、HID registration、discoverable 化、HCI power on、run loop を使う。`noop` は Bluetooth adapter を開かない test / smoke 用である。
- CLI flag は永続設定より優先する。ただし `backend` と診断出力 path は `local_071` の設定ファイル schema には入れない。
- `SWBT_RUN_HARDWARE` / `SWBT_HARDWARE_APPROVED` は code-level double gate としては削除候補である。エージェント運用上の実機承認は別物として残す。
- 診断出力 path は、明示 flag がある起動だけで有効化する。設定ファイルや残留環境変数から暗黙にファイルを書き始める設計にはしない。
- 診断出力 path の open failure は失敗として扱う方向で検討する。operator が明示した観測対象が作れないまま実機へ進むと、失敗時に根拠が残らないためである。
- 親ディレクトリの自動作成は初期案では行わない。意図しない path への作成範囲を広げないためである。
- crash dump は Windows 専用挙動を含むため、非 Windows では flag の扱いを no-op にするか、unsupported として失敗させるかを test で固定する。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `swbt/daemon/launch_options.*`
- `swbt/daemon/production_backend.*`
- `swbt/daemon/host.*`
- `swbt/core/diagnostics.*`
- `swbt/btstack_bridge/production_btstack.*`
- `tests/daemon_*`
- `tests/diagnostics_test.c`
- `docs/status.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `work-units/wip/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | CLI parser defaults to production backend when no backend flag is supplied | new | unit | no |
| refactor-skipped | CLI parser accepts `--backend noop` and `--backend=noop` | new | unit | no |
| green | `--backend noop` selects noop backend and does not require production hardware approval state | new | integration | no |
| green | invalid backend value fails before adapter open | edge | unit/integration | no |
| green | production backend no longer requires `SWBT_RUN_HARDWARE` and `SWBT_HARDWARE_APPROVED` as code-level gates, while hardware execution remains documented as human-approved | behavior | unit/integration | no |
| green | diagnostic trace path is enabled only by CLI flag and not by persistent config file | regression | unit/integration | no |
| green | HCI dump path is enabled only by CLI flag and fails before production run loop when the path cannot be opened | edge | unit/integration | no |
| green | crash dump path CLI behavior is fixed for Windows and non-Windows builds | edge | unit/build | no |
| green | test / smoke entrypoints that need no hardware pass `--backend noop` explicitly | regression | integration | no |
| deferred | adapter selector or dedicated dongle identity guard is designed if production default leaves adapter selection ambiguous | behavior | design/hardware | yes |

## 10. 検証

TDD status:
- source: `local_073` から分離した backend 起動モード整理。
- use case: daemon を引数なしで起動すると通常運用として production backend を選ぶ。
- item: CLI parser defaults to production backend when no backend flag is supplied。
- state: refactor-skipped。
- commands:
  - red: `just build-debug`。`tests/daemon_launch_options_test.c` が `options.backend` と `SWBT_DAEMON_LAUNCH_BACKEND_PRODUCTION` 未定義で fail。
  - green: `just build-debug` pass。
  - green: `CTEST_ARGS='-R daemon_launch_options_test --output-on-failure' just test-debug` pass。sandboxed run は Dev Container CLI の Docker lookup failure で not run。権限付き再実行で対象 test pass。
- notes: `tdd-workflow`、`tdd-one-cycle`、`refactor-after-green`、`work-unit-record` を読んだ。`swbt_daemon_launch_options_t` に backend enum を追加し、zero-initialized default を production として固定した。今回の item では重複や責務混在がなく、refactor-skipped とした。

TDD status:
- source: `local_073` から分離した backend 起動モード整理。
- use case: test / smoke が Bluetooth adapter を開かない起動を明示できる。
- item: CLI parser accepts `--backend noop` and `--backend=noop`。
- state: refactor-skipped。
- commands:
  - red: `just build-debug` pass 後、`CTEST_ARGS='-R daemon_launch_options_test --output-on-failure' just test-debug` fail。`--backend` が unknown option 扱いで対象 test が fail。最初の CTest は build 前の古い binary を実行したため red として扱わない。
  - green: `just build-debug` pass。
  - green: `CTEST_ARGS='-R daemon_launch_options_test --output-on-failure' just test-debug` pass。
- notes: parser に `--backend` と `--backend=` を追加した。main の noop 分岐接続と hardware approval 非依存の integration 確認は別 item として残す。今回の item では構造変更不要のため refactor-skipped とした。

TDD status:
- source: `local_073` から分離した backend 起動モード整理。
- use case: 不正な backend 名は adapter open 前に CLI validation で失敗する。
- item: invalid backend value fails before adapter open。
- state: green。
- commands:
  - characterization: `just build-debug` pass。
  - characterization: `CTEST_ARGS='-R daemon_launch_options_test --output-on-failure' just test-debug` pass。
- notes: 前 cycle の `swbt_daemon_launch_options_parse_backend` が invalid value と missing value をすでに reject していたため、追加 test は red にならなかった。adapter open 前の前提は parser unit test で確認した。main 分岐で adapter を開かないことは後続 integration item で確認する。

TDD status:
- source: `local_073` から分離した backend 起動モード整理。
- use case: `swbt-daemon --backend noop` は Bluetooth adapter と production hardware approval gate を通らず、noop host backend を選ぶ。
- item: `--backend noop` selects noop backend and does not require production hardware approval state。
- state: green。
- commands:
  - red: `rg -n "SWBT_DAEMON_BACKEND" apps/swbt-daemon/main.c` found `getenv("SWBT_DAEMON_BACKEND")`。最初の複合 regex は構文誤りのため red として扱わない。
  - green: `rg -n "SWBT_DAEMON_BACKEND" apps/swbt-daemon/main.c` no match。
  - green: `scripts/check-format.sh` pass。
  - green: `just build-debug` pass。
- notes: `main.c` の backend 分岐を process env から `launch_options.backend` の switch へ移した。`SWBT_DAEMON_LAUNCH_BACKEND_NOOP` case は noop host backend を直接呼ぶため、production hardware approval env を参照しない。実機コマンド、pairing、advertising、report loop は実行していない。

TDD status:
- source: user discussion, 2026-06-26。実機承認 env は code-level gate ではなく、エージェント運用上の承認境界として扱う。
- use case: production backend は CLI で選ばれたら起動経路へ入り、`SWBT_RUN_HARDWARE` / `SWBT_HARDWARE_APPROVED` の有無では分岐しない。
- item: production backend no longer requires `SWBT_RUN_HARDWARE` and `SWBT_HARDWARE_APPROVED` as code-level gates, while hardware execution remains documented as human-approved。
- state: green。
- commands:
  - red: `just build-debug` pass 後、`CTEST_ARGS='-R daemon_production_backend_test --output-on-failure' just test-debug` fail。`main result: expected 0, got -3`、`step count: expected 13, got 0`。
  - green: `just build-debug` pass。
  - green: `CTEST_ARGS='-R daemon_production_backend_test --output-on-failure' just test-debug` pass。
  - green: `rg -n "SWBT_RUN_HARDWARE|SWBT_HARDWARE_APPROVED|hardware_approval_from_process_env" apps/swbt-daemon/main.c` no match。
  - green: `scripts/check-format.sh` pass。
- notes: `hardware-harness` を読んだ。実機向けコマンド、Bluetooth adapter open、pairing、HID advertising、report loop は実行していない。実機実行は引き続き人間の明示承認と専用 USB Bluetooth dongle の確認を必要とする。

TDD status:
- source: user discussion, 2026-06-26。診断出力 path は環境変数ではなく、その起動の CLI flag として明示する。
- use case: `--trace-path` を指定した起動だけ startup / cleanup trace を書き、`SWBT_DIAGNOSTIC_TRACE_PATH` だけでは trace を書かない。
- item: diagnostic trace path is enabled only by CLI flag and not by persistent config file。
- state: green。
- commands:
  - red: `just build-debug` fail。`swbt_diagnostic_trace_set_path` と `options.trace_path` が未定義。
  - green: `just build-debug` pass。
  - green: `CTEST_ARGS='-R "(daemon_launch_options_test|diagnostics_test)" --output-on-failure' just test-debug` pass。
  - green: `rg -n "SWBT_DIAGNOSTIC_TRACE_PATH" apps swbt tests` は implementation 側 no match、test helper の env ignored 確認だけ match。最初の複合 regex は構文誤りのため判定に使わない。
  - green: `just format-check` pass。
- notes: `swbt_diagnostic_trace` は process-global に設定された path だけを見る。`main.c` は CLI parse 後に `launch_options.trace_path` を設定する。TOML config file schema には trace path を追加していない。

TDD status:
- source: user discussion, 2026-06-26。HCI dump path は環境変数ではなく、その起動の CLI flag として明示する。
- use case: `--hci-dump-path` を指定した production 起動だけ HCI dump を開き、path が開けない場合は production platform start が fail する。`SWBT_HCI_DUMP_TRACE_PATH` だけでは HCI dump を開かない。
- item: HCI dump path is enabled only by CLI flag and fails before production run loop when the path cannot be opened。
- state: green。
- commands:
  - red: `just build-debug` fail。`options.hci_dump_path` と `swbt_btstack_production_hci_dump_configure` が未定義。
  - green: `just build-debug` pass。
  - green: `CTEST_ARGS='-R "(daemon_launch_options_test|btstack_production_hci_dump_test)" --output-on-failure' just test-debug` pass。
  - green: `rg -n "SWBT_HCI_DUMP_TRACE_PATH" apps swbt tests` は implementation 側 no match、test helper の env ignored 確認だけ match。
  - green: `just format-check` pass。
- notes: `main.c` は production 起動前に `launch_config.hci_dump_path` を `swbt_btstack_production_hci_dump_configure` へ渡す。BTstack platform start は configured path だけを使う。実 Bluetooth adapter は開いていない。

TDD status:
- source: user discussion, 2026-06-26。crash dump path は環境変数ではなく、その起動の CLI flag として明示する。
- use case: `--crash-dump-path` を指定した起動だけ Windows minidump handler の出力先を設定し、非 Windows build では flag を受け付けた上で no-op とする。`SWBT_CRASH_DUMP_PATH` だけでは crash dump path を設定しない。
- item: crash dump path CLI behavior is fixed for Windows and non-Windows builds。
- state: green。
- commands:
  - red: `just build-debug` fail。`swbt_daemon_launch_options_t` に `crash_dump_path` member がなく、parser test の build が fail。
  - green: `just build-debug` pass。
  - green: `CTEST_ARGS='-R daemon_launch_options_test --output-on-failure' just test-debug` pass。
  - green: `just windows-cross` pass。
  - green: `rg -n "SWBT_CRASH_DUMP_PATH" apps swbt tests` no match。
  - green: `just format-check` pass。
- notes: `main.c` は CLI parse 後に `launch_options.crash_dump_path` を crash dump handler へ渡す。Windows では minidump handler の出力先として使い、非 Windows では flag を受け付けて破棄する。実 crash は発生させていないため、minidump ファイル生成は未検証である。

TDD status:
- source: user discussion, 2026-06-26。test / smoke で Bluetooth adapter を開かない場合は noop backend を CLI flag で明示する。
- use case: CTest の smoke entrypoint は `swbt-daemon` を直接起動しても production backend の adapter open へ進まず、`--backend noop` を明示して即時終了する。
- item: test / smoke entrypoints that need no hardware pass `--backend noop` explicitly。
- state: green。
- commands:
  - characterization: `rg -n "swbt-daemon(\\.exe)?|SWBT_DAEMON_BACKEND|--backend|SWBT_RUN_HARDWARE|SWBT_HARDWARE_APPROVED|SWBT_DIAGNOSTIC_TRACE_PATH|SWBT_HCI_DUMP_TRACE_PATH|SWBT_CRASH_DUMP_PATH" .github scripts tests docs spec work-units apps swbt`。current implementation と docs の更新対象を確認した。
  - characterization: `rg -n "add_test|swbt-daemon|daemon_.*smoke|smoke" CMakeLists.txt tests cmake .github scripts`。CTest は `swbt-daemon` executable を直接呼んでいなかった。
  - green: `just build-tests-debug` pass。sandboxed run は Dev Container CLI の Docker lookup failure で not run。権限付き再実行で pass。
  - green: `CTEST_ARGS='-R swbt_daemon_noop_smoke_test --output-on-failure' just test-debug` pass。
- notes: `CMakeLists.txt` に `swbt_daemon_noop_smoke_test` を追加し、`COMMAND swbt-daemon --backend noop` とした。`swbt_unit_tests` に `swbt-daemon` dependency を追加し、test target build だけでも smoke executable が生成されるようにした。実 Bluetooth adapter は開いていない。

TDD status:
- source: `docs/status.md`、`spec/operations/windows-native-preflight.md`、`spec/operations/windows-hardware-bringup-sequence.md` の current state 更新。
- use case: current docs は production 既定、`--backend noop`、diagnostic CLI flag、実機承認の運用 gate を implementation と同じ意味で説明する。
- item: docs / status reflect daemon launch mode flags。
- state: green。
- commands:
  - green: `rg -n "SWBT_DAEMON_BACKEND|SWBT_RUN_HARDWARE|SWBT_HARDWARE_APPROVED|SWBT_DIAGNOSTIC_TRACE_PATH|SWBT_HCI_DUMP_TRACE_PATH|SWBT_CRASH_DUMP_PATH|--trace-path|--hci-dump-path|--crash-dump-path|--backend" docs/status.md spec/operations/windows-native-preflight.md spec/operations/windows-hardware-bringup-sequence.md spec/architecture/daemon-architecture-cutover.md`。current docs の更新対象を確認した。
  - green: `rg -n "SWBT_DAEMON_BACKEND|SWBT_RUN_HARDWARE|SWBT_HARDWARE_APPROVED|SWBT_DIAGNOSTIC_TRACE_PATH|SWBT_HCI_DUMP_TRACE_PATH|SWBT_CRASH_DUMP_PATH|no-op backend|noop backend" docs/status.md spec/operations/windows-native-preflight.md spec/operations/windows-hardware-bringup-sequence.md`。残った match は current implementation で selector ではないこと、または noop backend の明示指定を説明する記述だけ。
- notes: 実機ログの過去 entry は履歴証跡として変更していない。operations spec では `SWBT_RUN_HARDWARE` / `SWBT_HARDWARE_APPROVED` を current implementation の分岐条件ではなく、人間承認 gate と切り分けた。

## 11. 実機実行条件

この work unit の最終確認は software-only とする。Bluetooth adapter open、Switch pairing、HID advertising、report loop は実行していない。

実機実行前条件:

- `hardware-harness` を読む。
- 人間の明示承認を得る。
- 専用 USB Bluetooth dongle と WinUSB driver assignment を確認する。
- 実行範囲を adapter open、HID advertising、pairing、report loop、diagnostic output のどこまで含むか明示する。
- `docs/hardware-test-log.md` へ OS、dongle VID/PID、driver、BTstack commit、swbt commit、Switch firmware、結果、cleanup を記録する。

software-only で確認できる範囲:

- CLI parser の既定値と validation。
- noop 明示指定が adapter port を呼ばないこと。
- invalid backend / invalid diagnostic path が production run loop 前に失敗すること。
- 環境変数依存削除または互換期間の挙動。
- current docs が implementation と同じ起動 mode / 診断 path selector を説明していること。

## 12. 先送り事項

- 観測: production 既定化後も、BTstack USB transport がどの adapter を開くかは swbt 側で明示 selector を持たない。
  先送り理由: backend 起動モードと CLI parser の整理とは別の問題であり、VID/PID selector や driver state 確認を含めると scope が広がる。
  次の置き場: 後続 work unit。必要なら `spec/operations/windows-native-preflight.md` と接続する。
- 観測: `show-config` / `validate-config` / reconnect state cleanup などの CLI subcommand は有用である。
  先送り理由: この work unit は daemon 起動引数に絞る。管理 command surface は active reconnect と設定ファイル運用が固まった後に扱う。
  次の置き場: 後続 work unit。

## 13. チェックリスト

- [x] source を user discussion、`local_073`、`local_071`、`local_045` から特定した。
- [x] use case を production default、explicit noop、CLI diagnostic flag として定義した。
- [x] 設定ファイル schema と link key DB reconnect とは別 work unit に分離した。
- [x] CLI parser の test list を実装前に再確認した。
- [x] red test または characterization test を追加した。
- [x] production default / noop explicit behavior を実装した。
- [x] diagnostic CLI flag を実装した。
- [x] hardware approval env の削除または互換期間を決めた。
- [x] docs / status を更新した。
- [x] software verification と実機未実行理由または実機結果を記録した。
