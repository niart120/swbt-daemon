# Daemon CLI Launch Mode

## 1. 概要

daemon の起動モードと一時診断出力を、環境変数中心の指定から CLI flag へ移す work unit。

現行の `SWBT_DAEMON_BACKEND=production` は、daemon host の noop 経路と production BTstack 経路を切り替える。noop は Bluetooth adapter を開かない test / smoke 用の退避経路であり、production が実際の daemon 経路である。この work unit では、その意味に合わせて既定を production に寄せ、test / smoke 時だけ `--backend noop` のように明示する設計へ移行する。

診断出力 path は永続設定ではなく、その起動だけの観測指定として扱う。`SWBT_DIAGNOSTIC_TRACE_PATH`、`SWBT_HCI_DUMP_TRACE_PATH`、`SWBT_CRASH_DUMP_PATH` は CLI flag への移行対象にする。

## 2. 起点 / ユースケース

source:

- user discussion, 2026-06-26: 実機承認 env はそろそろ外せるのではないか。診断出力 path は環境変数で有効化すると意図しない場所へファイルが作られ得るため、実行時引数の flag に寄せる案が出た。
- user discussion, 2026-06-26: backend 指定は何かを確認した結果、現行の noop は test / smoke 用であり、daemon の通常起動は production を既定にして、test 時だけ明示的に noop を与える方針が妥当と判断した。
- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`: 設定ファイル schema は `ipc`、`report`、`device.profile` に絞り、backend 起動モード、実機承認、診断出力 path はこの work unit へ切り出す。
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`: 環境変数を backend selection、hardware safety gate、runtime override、diagnostic sink に分類した。
- `docs/status.md`: 現行状態として、未指定では noop backend、`SWBT_DAEMON_BACKEND=production` で production backend、production には `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` が必要と記録している。

use case:

- actor: hardware operator、maintainer、unit / integration test。
- 入力または状態: daemon を引数なしで起動する通常運用、Bluetooth adapter を開かず host lifecycle だけを通したい test / smoke、実機検証時だけ trace / HCI dump / crash dump を保存したい起動。
- 期待する観測結果:
  - `swbt-daemon` は production backend を選ぶ。
  - `swbt-daemon --backend noop` は Bluetooth adapter を開かず、現行 noop host 経路を選ぶ。
  - 不正な `--backend` 値は adapter open 前に失敗する。
  - 診断出力 path は CLI flag を指定した起動だけで有効になる。
  - 診断出力 path が不正な場合は、可能な限り adapter open 前に失敗し、意図しない場所へ黙って書き込まない。
  - test / CI / smoke は必要に応じて `--backend noop` を明示する。
- 制約: エージェント運用上の実機コマンド承認は残す。Bluetooth adapter open、Switch pairing、HID advertising、report loop は `hardware-harness` の承認境界に従う。

source から use case への変換:

`local_071` は永続設定の対象を runtime 値へ絞る。backend selection と診断出力 path は永続設定よりも起動コマンドに現れている方が事故を減らせるため、CLI parser の導入と同時に扱う。

## 3. 対象範囲

- `swbt-daemon` 用の testable CLI parser を追加する。
- `--backend production|noop` を設計し、既定を production にする。
- `SWBT_DAEMON_BACKEND` の削除または互換期間を決める。
- `SWBT_RUN_HARDWARE` / `SWBT_HARDWARE_APPROVED` を production backend の code gate から削除するか、互換期間を決める。
- `--trace-path`、`--hci-dump-path`、`--crash-dump-path` を設計する。
- `SWBT_DIAGNOSTIC_TRACE_PATH`、`SWBT_HCI_DUMP_TRACE_PATH`、`SWBT_CRASH_DUMP_PATH` の削除または互換期間を決める。
- 診断出力 path の open failure policy を決める。
- test / CI / smoke entrypoint が Bluetooth adapter を開かない場合は `--backend noop` を明示するよう更新する。
- `docs/status.md` と関連 operations docs に、新しい起動モードと実機承認境界を記録する。

## 4. 対象外

- `local_071` の設定ファイル schema、TOML parser / serializer、環境変数 runtime override precedence。
- active reconnect、Switch address の取得 / 保存 / 削除。
- adapter selector、VID/PID selector、複数 Bluetooth adapter の選択 policy。
- Switch protocol byte、device info payload、report period の既定値変更。
- service manager、installer、Windows registry、binary release。

## 5. 関連 spec / docs

- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `spec/architecture/daemon-architecture-cutover.md`

## 6. 根拠監査

not applicable for source-audit。

この work unit は CLI 起動指定、環境変数削除、診断出力 path の扱いを変更する。Switch HID report bytes、subcommand、SPI、rumble、report period 採用値、BTstack source selection は追加または変更しない。

ただし production 既定化は Bluetooth adapter open に直結するため、実機実行や docs の表現では `hardware-harness` を使う。

## 7. 設計メモ

- `backend` は adapter 種別ではなく daemon host backend の選択である。`production` は BTstack USB transport、HID registration、discoverable 化、HCI power on、run loop を使う。`noop` は Bluetooth adapter を開かない test / smoke 用である。
- CLI flag は永続設定より優先する。ただし `backend` と診断出力 path は `local_071` の設定ファイル schema には入れない。
- `swbt-daemon` 引数なしを production にする場合、unit / smoke / CI で daemon binary を直接起動する箇所は `--backend noop` へ移す。
- `SWBT_RUN_HARDWARE` / `SWBT_HARDWARE_APPROVED` は code-level double gate としては削除候補である。エージェント運用上の実機承認は別物として残す。
- 診断出力 path は、明示 flag がある起動だけで有効化する。設定ファイルや残留環境変数から暗黙にファイルを書き始める設計にはしない。
- 診断出力 path の open failure は失敗として扱う方向で検討する。operator が明示した観測対象が作れないまま実機へ進むと、失敗時に根拠が残らないためである。
- 親ディレクトリの自動作成は初期案では行わない。意図しない path への作成範囲を広げないためである。
- crash dump は Windows 専用挙動を含むため、非 Windows では flag の扱いを no-op にするか、unsupported として失敗させるかを test で固定する。
- adapter selector はこの work unit の対象外だが、production 既定化後に残る安全上の違和感である。専用 WinUSB ドングル運用は docs で維持し、selector が必要なら後続 work unit に切り出す。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `swbt/daemon/production_backend.*`
- `swbt/daemon/host.*`
- `swbt/core/diagnostics.*`
- `swbt/btstack_bridge/production_btstack.*`
- `tests/daemon_*`
- `tests/diagnostics_test.c`
- `tests/btstack_production_hci_dump_test.c`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `work-units/wip/local_073/DAEMON_CLI_LAUNCH_MODE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | CLI parser defaults to production backend when no backend flag is supplied | new | unit | no |
| todo | `--backend noop` selects noop backend and does not require production hardware approval state | new | unit/integration | no |
| todo | invalid backend value fails before adapter open | edge | unit/integration | no |
| todo | production backend no longer requires `SWBT_RUN_HARDWARE` and `SWBT_HARDWARE_APPROVED` as code-level gates, while hardware execution remains documented as human-approved | behavior | unit/integration | no |
| todo | diagnostic trace path is enabled only by CLI flag and not by persistent config file | regression | unit/integration | no |
| todo | HCI dump path is enabled only by CLI flag and fails before production run loop when the path cannot be opened | edge | unit/integration | no |
| todo | crash dump path CLI behavior is fixed for Windows and non-Windows builds | edge | unit/build | no |
| todo | test / smoke entrypoints that need no hardware pass `--backend noop` explicitly | regression | integration | no |
| deferred | adapter selector or dedicated dongle identity guard is designed if production default leaves adapter selection ambiguous | behavior | design/hardware | yes |

## 10. 検証

起票のみ。実装、software test、実機検証はまだ実行していない。

起票時確認:

- `apps/swbt-daemon/main.c` は `main(void)` で CLI 引数を受け取らず、`SWBT_DAEMON_BACKEND=production` のときだけ production backend を選ぶ。
- `swbt/daemon/host.c` の noop backend は Bluetooth adapter を開かない dummy port set である。
- `swbt/daemon/production_backend.c` は `SWBT_RUN_HARDWARE` と `SWBT_HARDWARE_APPROVED` 由来の approval がない場合に production main を拒否する。
- `swbt/core/diagnostics.c`、`swbt/btstack_bridge/production_btstack.c`、`apps/swbt-daemon/main.c` は診断出力 path を環境変数から読む。

## 11. 実機実行条件

この work unit は production 既定化と hardware approval env の削除候補を含むため、最終確認では実機が必要になる可能性が高い。

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

## 12. 先送り事項

- 観測: production 既定化後も、BTstack USB transport がどの adapter を開くかは swbt 側で明示 selector を持たない。
  先送り理由: backend 起動モードと CLI parser の整理とは別の問題であり、VID/PID selector や driver state 確認を含めると scope が広がる。
  次の置き場: 後続 work unit。必要なら `spec/operations/windows-native-preflight.md` と接続する。
- 観測: `show-config` / `validate-config` / reconnect state cleanup などの CLI subcommand は有用である。
  先送り理由: この work unit は daemon 起動引数に絞る。管理 command surface は active reconnect と設定ファイル運用が固まった後に扱う。
  次の置き場: 後続 work unit。

## 13. チェックリスト

- [x] source を user discussion、`local_071`、`local_045`、`docs/status.md` から特定した。
- [x] use case を production default、explicit noop、CLI diagnostic flag として定義した。
- [x] 設定ファイル schema とは別 work unit に分離した。
- [ ] CLI parser の test list を実装前に再確認した。
- [ ] red test または characterization test を追加した。
- [ ] production default / noop explicit behavior を実装した。
- [ ] diagnostic CLI flag を実装した。
- [ ] hardware approval env の削除または互換期間を決めた。
- [ ] docs / status を更新した。
- [ ] software verification と実機未実行理由または実機結果を記録した。
