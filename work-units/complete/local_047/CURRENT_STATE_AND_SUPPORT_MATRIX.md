# Current State And Support Matrix

## 1. 概要

README の古い状態説明を表形式へ置き換え、詳細な current state / support matrix を `docs/status.md` に分離する。巨大な `docs/hardware-test-log.md` は証跡としてリンクし、README、`apps/swbt-daemon/main.c`、実機ログの間で production 起動条件と対応構成が矛盾しない状態にする。

この work unit は文書更新に限定する。`main.c` や production backend の挙動は変更しない。

## 2. 起点 / ユースケース

source: ユーザ要求 `CURRENT_STATE_AND_SUPPORT_MATRIX`。PR は文書更新に限定し、README の状態説明、確認済み / 未確認 / 未実装の表、production 起動条件、既知の対応構成、未確認項目、`docs/status.md` または `ROADMAP.md` の追加を求めている。

use case: reviewer が README と `docs/status.md` を読めば、現時点で実機確認済みの構成、未確認の構成、未実装の機能、production 起動時の安全条件を判断できる。詳細な実機証跡は `docs/hardware-test-log.md` へ辿れる。

source から use case への変換: README は入口として短い表に留め、詳細な状態表は `docs/status.md` に置く。実機観測は CSR8510 A10 / WinUSB / Switch 2 firmware `22.1.0`（実機ログ表記は Switch2）の条件に限定し、他環境へ一般化しない。

## 3. 対象範囲

- README の状態説明を表形式へ更新する。
- `docs/status.md` を追加し、確認済み / 未確認 / 未実装を分けて記録する。
- production 起動に必要な環境変数と安全確認を記録する。
- `docs/hardware-test-log.md` を証跡としてリンクする。
- README、`apps/swbt-daemon/main.c`、実機ログの整合を確認する。
- 文書内に残っている host-local full path を repo-relative path または環境中立の表記へ改める。

## 4. 対象外

- C source の変更。
- production backend の既定化。
- report period default の変更。
- `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` の正規 identity 化。
- 追加の実機実行。
- 初代Switch、別ドングル、Linux実機、bonded reconnect、遅延 / jitter / 取りこぼし率測定。
- 既存の実機観測結果、過去の検証結果、実行コマンドの意味の変更。

## 5. 関連 spec / docs

- `README.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `apps/swbt-daemon/main.c`
- `swbt/daemon/production_backend.c`
- `spec/operations/windows-native-preflight.md`
- `spec/protocols/switch-hid-core.md`
- `spec/references/btstack-periodic-input-report-core.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`
- `work-units/complete/local_046/DOC_IMPLEMENTATION_STATE_ALIGNMENT.md`

## 6. 根拠監査

この work unit は新しい protocol 値、BTstack source selection、report period default、実機手順を追加しない。既存の implementation fact と hardware observation を documentation に反映する。

### 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| production backend selection | `SWBT_DAEMON_BACKEND=production` | implementation fact | `apps/swbt-daemon/main.c` | opt-in |
| hardware approval gate | `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1` | implementation fact | `swbt/daemon/production_backend.c` | required before adapter open |
| known hardware configuration | Windows native / CSR8510 A10 / WinUSB / Switch 2 firmware `22.1.0` | hardware observation | `docs/hardware-test-log.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | limited observation |
| report period comparison | `8000 / 8333 / 15000 / 16667 us` | hardware observation | `docs/hardware-test-log.md`, `spec/references/btstack-periodic-input-report-core.md` | coarse acceptance only |

### 未解決事項

- 初代Switch各モデル、他USBドングル、Linux実機経路、bonded reconnect、厳密な遅延 / jitter / 取りこぼし率は未確認のまま残す。

## 7. 設計メモ

`docs/status.md` を選んだ。`ROADMAP.md` は後続 work queue の性格が強く、今回の目的である current state / support matrix の置き場としては `docs/status.md` の方が直接的である。

README には入口としての短い表を置く。詳細な証跡や未確認範囲は `docs/status.md` へ分離し、`docs/hardware-test-log.md` の巨大なログ本文をREADMEへ再掲しない。

## 8. 対象ファイル

- `README.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_047/CURRENT_STATE_AND_SUPPORT_MATRIX.md`
- `work-units/complete/local_017/SWITCH_HID_DESCRIPTOR_CORE.md`
- `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md`
- `work-units/complete/local_033/WORK_UNIT_SPEC_TDD_FLOW.md`
- `work-units/complete/local_034/WORK_UNIT_RECORD_SECTION_GUIDE.md`
- `work-units/complete/local_035/TDD_WORKFLOW_SKILL_GROUP_REIMPLEMENTATION.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_040/TDD_REFACTOR_GUIDANCE_SKILL.md`
- `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | README が確認済み / 未確認 / 未実装を分けて説明する | regression | docs | no |
| green | `docs/status.md` が production 起動条件と安全確認を `main.c` / production backend と矛盾なく説明する | regression | docs | no |
| green | 実機観測が CSR8510 A10 / WinUSB / Switch 2 firmware `22.1.0` の限定条件として扱われる | regression | docs | no |
| green | hardware log は証跡リンクとして扱い、巨大なログ本文を重複転載しない | regression | docs | no |
| green | 文書内の host-local full path を repo-relative path または環境中立の表記に置き換える | regression | docs | no |

## 10. 検証

- `rg -n "[ \t]+$" README.md docs/status.md work-units/complete/local_047/CURRENT_STATE_AND_SUPPORT_MATRIX.md`: no matches。
- placeholder / 仮テキスト / 曖昧表現検索: no matches。
- `git diff --check -- README.md`: pass。Windows checkout 由来の LF to CRLF warning のみ。
- `rg -n "SWBT_DAEMON_BACKEND=production|SWBT_RUN_HARDWARE=1|SWBT_HARDWARE_APPROVED=1|CSR8510 A10|WinUSB|Switch2|Switch 2|22\.1\.0|bonded reconnect|取りこぼし率" README.md docs/status.md docs/hardware-test-log.md apps/swbt-daemon/main.c swbt/daemon/production_backend.c`: pass。production 起動条件、既知の対応構成、未確認項目が該当ファイル間で矛盾しないことを確認した。
- host-local / repository checkout full path search over `README.md docs work-units spec .github`: no matches。URL ではなく checkout 依存の filesystem full path だけを対象にした。
- `git diff --check`: pass。Windows checkout 由来の LF to CRLF warning のみ。
- CMake / CTest / 実機: not run。文書更新のみで C source と実機手順を変更していないため。

## 11. 実機実行条件

この work unit では実機を実行しない。Bluetooth adapter、Switch pairing、HID advertising、report loop へ触れる変更を行わず、既存ログを文書化するだけである。

実機を伴う後続作業では `SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、専用 USB Bluetooth ドングル、WinUSB driver assignment、`docs/hardware-test-log.md` への記録を必要条件にする。

## 12. 先送り事項

- 初代Switch各モデル、他USBドングル、Linux実機経路、daemon再起動後の bonded reconnect、厳密な遅延 / jitter / 取りこぼし率は `docs/status.md` の未確認項目として残す。これらは後続 work unit の source として扱う。

## 13. チェックリスト

- [x] README の古い状態説明を表形式へ更新した。
- [x] `docs/status.md` を追加し、実機ログを証跡としてリンクした。
- [x] production 起動条件と安全確認を記録した。
- [x] 文書内の host-local full path を repo-relative path または環境中立の表記へ更新した。
- [x] README、`main.c`、実機ログの状態矛盾がないことを確認した。
- [x] 文書のみの検証結果を記録した。
