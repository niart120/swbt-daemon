# Swbt Device Info Profile Definition

## 1. 概要

`SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` は Switch 2 bring-up の実験条件として使われたが、`swbt-daemon` の規定 profile として扱うには名前も根拠も弱い。この work unit では、Mizuyoukanao 由来の profile を流用した状態を解消し、`swbt-daemon` が所有する Pro Controller 相当の device info profile を定義する。

完了後は、daemon が返す request device info reply の規定値、profile 名、文書上の説明、既存実験 profile の削除方針が明確になる。Switch protocol bytes を変更または正規化するため、実装前に根拠監査を行う。

## 2. 起点 / ユースケース

source: ユーザ要求。Mizuyoukanao 氏の profile を流用している状態は望ましくなく、`swbt-daemon` の規定 profile を定義したい。

source: `docs/status.md` は `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` を Switch 2 bring-up の実験条件とし、正規 identity として固定した値ではないと記録している。

source: `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md` は `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` の source-audited default 化を、behavior または hardware 影響を伴う別 work unit として先送りしている。

use case: daemon maintainer が production 実行や文書を確認したとき、`swbt-daemon` の規定 device info profile と、Mizuyoukanao 由来の実験 profile を混同しない。規定 profile は、根拠監査済みの値、profile 名、daemon の既定値として適用されること、実機未検証範囲を明示する。

source から use case への変換: 既存の `mizuyoukanao-pro` は過去の動作確認用 profile としても残さず、互換期間を設けず削除する。新しい規定 profile は `swbt-daemon` が所有する名前と根拠監査済みの bytes を持ち、daemon の既定値になる。

## 3. 対象範囲

- `swbt-daemon` の規定 device info profile 名を定義する。
- request device info reply data の規定値を根拠監査する。
- `swbt/switch/switch_device_info.*` と daemon config の profile 選択を更新する。
- 規定 profile を daemon の既定値にし、`SWBT_DEVICE_INFO_PROFILE` 未指定時にも適用されることをテストで固定する。
- `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` を削除し、互換 selector としても残さないことをテストで固定する。
- `spec/protocols/switch-hid-core.md`、`README.md`、`docs/status.md` を更新し、Mizuyoukanao 由来の profile を正規 profile として読ませない。
- 実機確認が必要な場合は、専用 USB Bluetooth ドングルと明示承認の条件を満たしたうえで `docs/hardware-test-log.md` に記録する。

## 4. 対象外

- HID descriptor、input report packing、rumble、SPI data、report period default の変更。
- BTstack submodule の変更。
- production backend selection の既定化。
- daemon protocol への time-based macro、tap、duration_ms、sequence、at_ms の追加。
- 複数 controller 同時接続、Joy-Con、NFC/IR MCU、amiibo 対応。
- 実機承認なしの pairing、HID advertising、report loop 実行。

## 5. 関連 spec / docs

- `spec/protocols/switch-hid-core.md`
- `docs/status.md`
- `README.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`
- `work-units/complete/local_047/CURRENT_STATE_AND_SUPPORT_MATRIX.md`
- `spec/operations/windows-native-preflight.md`

## 6. 根拠監査

この work unit は Switch request device info reply bytes と identity policy を扱うため、`source-audit` が必須である。`mizuyoukanao-pro` の値は切り分け用の実験値であり、規定 profile として採用しない。

規定 profile 名は `swbt-pro` とする。`swbt-pro` は現行 default reply bytes を project-owned profile として名前付けし、daemon の既定値にする。Bluetooth address は固定値ではなく、既存実装どおり runtime / production backend が得た local BD_ADDR を使う。

### 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| request device info reply shape | ACK `0x82`, subcommand `0x02`, 12 byte data | source fact / implementation contract / hardware observation | `spec/references/switch-subcommand-dispatcher-core.md`, `spec/references/switch-subcommand-reply-core.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | stable for current implementation |
| `swbt-pro` profile reply | firmware `04 00`, controller type `03`, marker `02`, local BD_ADDR, tail `01 01` | implementation policy backed by existing implementation contract | `swbt/switch/switch_device_info.c`, `spec/protocols/switch-hid-core.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | daemon default; hardware rerun not performed |
| experimental profile reply | firmware `03 48`, controller type `03`, marker `02`, local BD_ADDR, tail `03 02` | implementation fact / hardware observation | `swbt/switch/switch_device_info.c`, `docs/hardware-test-log.md` | experimental only |
| `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` | experimental selector | implementation fact / hardware observation | `swbt/daemon/config.c`, `docs/status.md` | remove without compatibility period |

### 未解決事項

- `swbt-pro` の実機受理はこの work unit の自動テスト範囲外であり、実機承認後の hardware item に残す。

## 7. 設計メモ

この record では、`規定 profile` は `swbt-daemon` が定める project-owned profile を指し、`既定値` は daemon が設定未指定時に使う default selection を指す。規定 profile は daemon の既定値にする。

`mizuyoukanao-pro` という名前は、実験条件の出所を示すには有用だったが、project-owned identity を表す名前ではない。過去の動作確認用 profile としても残さず、互換期間を設けず削除する。規定 profile には `swbt` を含む名前を使う。

profile bytes は「Switch 2 bring-up で通ったから」だけでは stable としない。source fact、implementation fact、hardware observation、inference、unverified hypothesis を分け、必要なら `spec/references/` に根拠整理を置く。

## 8. 対象ファイル

- `spec/references/switch-subcommand-dispatcher-core.md`
- `swbt/switch/switch_device_info.h`
- `swbt/switch/switch_device_info.c`
- `swbt/daemon/config.h`
- `swbt/daemon/config.c`
- `tests/daemon_runtime_test.c`
- `tests/switch_subcommand_dispatcher_test.c`
- `spec/protocols/switch-hid-core.md`
- `README.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `work-units/wip/local_048/SWBT_DEVICE_INFO_PROFILE_DEFINITION.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | `SWBT_DEVICE_INFO_PROFILE` 未指定時に swbt-owned profile が daemon の既定 device info として適用される | new | unit | no |
| refactor-skipped | `SWBT_DEVICE_INFO_PROFILE` が swbt-owned profile 名を明示的に受理する | new | unit | no |
| green | unknown profile は reject され、既存 config を破壊しない | regression | unit | no |
| refactor-skipped | request device info reply が swbt-owned profile の firmware、controller type、marker、local BD_ADDR、tail を返す | new | unit | no |
| refactor-skipped | `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` は reject され、互換 selector としても残らない | regression | unit | no |
| todo | `spec/protocols/switch-hid-core.md` が規定 profile と削除済み実験 profile を分けて説明する | regression | docs | no |
| todo | `README.md` と `docs/status.md` が production 実行例で Mizuyoukanao 由来の profile を正規値として提示しない | regression | docs | no |
| todo | swbt-owned profile で Switch 2 pairing 画面の Button A 入力反映を確認する | characterization | hardware | yes |

## 10. 検証

Record 作成時点では実装と文書の変更は未実施。

- `git status --short`: clean before record creation。
- `git branch --show-current`: `main`。
- `rg -n "device_info|DEVICE_INFO|mizuyoukanao|SWBT_DEVICE_INFO_PROFILE" tests swbt api apps README.md docs/status.md spec/protocols/switch-hid-core.md work-units/complete/local_045 work-units/complete/local_047`: 既存の実装、テスト、docs、先送り事項を確認した。
- diff comment 反映後、`mizuyoukanao-pro` は互換期間なしで削除し、規定 profile を daemon の既定値にする方針へ更新した。
- 未決定のまま残す表現、互換期間を設ける表現、環境変数選択に留める表現の検索: no matches。
- `git diff --check -- work-units/wip/local_048/SWBT_DEVICE_INFO_PROFILE_DEFINITION.md`: pass。
- source audit: `spec/references/switch-subcommand-dispatcher-core.md` と `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` を確認し、`swbt-pro` profile 名、現行 default reply bytes、`mizuyoukanao-pro` 削除方針をこの record に反映した。
- TDD red: `just build-debug` fail。`tests/daemon_runtime_test.c` の `config_default_uses_swbt_pro_device_info_profile` が `swbt_switch_device_info_swbt_pro()` を要求し、現行実装では implicit declaration / invalid initializer で停止した。これは選択 item の期待どおりの red と扱う。
- TDD green: `swbt_switch_device_info_swbt_pro()` を追加し、`swbt_switch_device_info_default()` が同 profile を返すようにした。`just build-debug` pass。`CTEST_ARGS='-R daemon_runtime_test --output-on-failure' just test-debug` pass。共有 protocol code に触れたため `just test-debug` も実行し、32/32 pass。
- Refactor status: refactor-skipped。今回の item は named profile と既定値の接続だけで閉じており、追加の構造変更は不要と判断した。
- TDD red: `SWBT_DEVICE_INFO_PROFILE` が swbt-owned profile 名を明示的に受理する item。`just build-debug` pass。`CTEST_ARGS='-R daemon_runtime_test --output-on-failure' just test-debug` fail。追加した `config_applies_swbt_pro_device_info_profile` が現行 config では通らないため、期待どおりの red と扱う。
- TDD green: `swbt_daemon_config_apply_device_info_profile()` が `swbt-pro` を受理し、同 profile を適用するようにした。`just build-debug` pass。`CTEST_ARGS='-R daemon_runtime_test --output-on-failure' just test-debug` pass。`just test-debug` 32/32 pass。
- Refactor status: refactor-skipped。selector 追加だけで責務境界が閉じており、追加の構造変更は不要と判断した。
- TDD red: `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` rejection item。`just build-debug` pass。`CTEST_ARGS='-R daemon_runtime_test --output-on-failure' just test-debug` fail。現行 config が `mizuyoukanao-pro` を受理するため、期待どおりの red と扱う。
- TDD green: `mizuyoukanao-pro` selector、専用 device info function、専用 constants、dispatcher の実験 profile test を削除した。`just build-debug` pass。`CTEST_ARGS='-R "daemon_runtime_test|switch_subcommand_dispatcher_test" --output-on-failure' just test-debug` pass。`just test-debug` 32/32 pass。
- `rg -n "swbt_switch_device_info_mizuyoukanao_pro|MIZUYOUKANAO_PRO|SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro" swbt tests apps`: no matches。runtime code / tests / app に互換 selector と専用 profile 実装が残っていないことを確認した。
- Refactor status: refactor-skipped。削除対象は selector と実験 profile API に閉じており、追加の構造変更は不要と判断した。
- existing regression: unknown profile rejection は `config_env_unknown_device_info_profile_rejects_and_preserves_config` と `config_rejects_unknown_device_info_profile` で既に coverage があり、`just test-debug` 32/32 pass で確認済み。
- characterization green: request device info reply の test を `test_request_device_info_builds_swbt_pro_identity_reply` に更新し、`swbt_switch_device_info_swbt_pro()` 由来の firmware `04 00`、controller type `03`、marker `02`、local BD_ADDR、tail `01 01` を確認する形にした。`just build-debug` pass。`CTEST_ARGS='-R switch_subcommand_dispatcher_test --output-on-failure' just test-debug` pass。
- Refactor status: refactor-skipped。test 名と入力 profile を規定 profile に合わせただけで、追加の構造変更は不要と判断した。

## 11. 実機実行条件

Record 作成時点では実機を実行しない。

この work unit の実装後に実機確認を行う場合は、次を必要条件にする。

- ユーザの明示承認。
- 専用 USB Bluetooth ドングル。
- Windows native では対象ドングルの WinUSB driver assignment 記録。
- `SWBT_DAEMON_BACKEND=production`。
- `SWBT_RUN_HARDWARE=1`。
- `SWBT_HARDWARE_APPROVED=1`。
- `SWBT_DEVICE_INFO_PROFILE` は未指定で実行し、daemon の既定値として swbt-owned profile が使われたことを記録する。明示指定で確認する場合も swbt-owned profile 名だけを使い、`mizuyoukanao-pro` は使わない。
- `SWBT_DIAGNOSTIC_TRACE_PATH` と `SWBT_HCI_DUMP_TRACE_PATH` に証跡を残す。
- OS、dongle VID/PID、driver、BTstack commit、swbt commit、Switch firmware、report period、artifact path、結果を `docs/hardware-test-log.md` に記録する。

## 12. 先送り事項

- 規定 profile 以外の controller identity variation は、この work unit で必要になった場合だけ `spec/dev-journal.md` または後続 work unit の source として記録する。
- bonded reconnect、別ドングル、Linux 実機経路は `docs/status.md` の未確認項目であり、この work unit の profile 定義とは別に扱う。

## 13. チェックリスト

- [x] source audit で規定 profile の値と根拠を分類した。
- [x] swbt-owned profile 名を決めた。
- [ ] 規定 profile を daemon の既定値にした。
- [x] `mizuyoukanao-pro` を互換期間なしで削除した。
- [ ] daemon config と device info reply のテストを更新した。
- [ ] `spec/protocols/switch-hid-core.md`、`README.md`、`docs/status.md` を更新した。
- [ ] 実機実行の要否と結果または未実行理由を記録した。
- [ ] 検証コマンドと結果を記録した。
