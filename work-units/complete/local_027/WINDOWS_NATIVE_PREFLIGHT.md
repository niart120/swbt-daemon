# Windows Native Preflight

## 1. 概要

Phase 5 の Windows 実機確認の前に、Windows native、WinUSB、専用 USB Bluetooth ドングルの preflight を切り出す work unit。

この work unit は `swbt-daemon.exe` の Windows native 起動、Switch pairing、report period comparison を始める前の承認条件と記録項目を `spec/operations/windows-native-preflight.md` に固定する。

実機は未実行であり、Windows native runtime、WinUSB driver assignment、pairing、report loop の成功をこの record では断定しない。

## 2. 起点 / ユースケース

source:

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md` Phase 5 は、専用 USB Bluetooth dongle、Zadig / WinUSB、Windows native 起動、Switch pairing、report period comparison、hardware log 記録を未完了項目としている。
- `AGENTS.md` と `hardware-harness` skill は、実機コマンド、pairing、HID advertising、report loop を人間の明示承認なしに実行しない方針を定めている。
- `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md` は Windows native PowerShell からの `just` 経路を確認済みだが、Windows native daemon 実行や WinUSB runtime behavior は証明しない。

use case:

- actor: 実機検証を開始する開発者。
- 入力または状態: Windows native host、専用 USB Bluetooth ドングル、WinUSB driver assignment、未実行の Switch pairing / report loop。
- 期待する観測結果: 実機コマンドを実行する前に承認範囲、対象アダプター、環境変数、記録項目、cleanup plan が文書で確認できる。
- 制約: 実機はこの work unit では動かさない。report period 候補は比較入力であり、source-audited default ではない。
- 対象外: daemon runtime 実装、BTstack backend 実装、実機成功判定。

source から use case への変換:

Phase 5 の未完了項目を、そのまま実機作業に進めるのではなく、実行前 gate と hardware log schema に変換する。これにより後続の実機 work unit は、承認と記録条件を満たした状態から開始できる。

## 3. 対象範囲

- 専用 USB Bluetooth ドングルを使う前提を確認し、内蔵 Bluetooth と常用ドングルを対象外にする手順を固める。
- Zadig による WinUSB driver assignment の記録項目を固定する。
- Windows native で `swbt-daemon.exe` を起動する前の承認、環境変数、log target を整理する。
- Switch pairing、HID advertising、report loop、report period comparison を実行する条件を明記する。
- `docs/hardware-test-log.md` に残す項目を Phase 5 TODO と合わせる。

## 4. 対象外

- Switch pairing の実行。
- HID advertising と periodic report loop の実行。
- report period `8000 / 8333 / 15000 / 16667 us` の実測。
- BTstack source selection の変更。
- `vendor/btstack` の変更。
- rumble callback と status 挙動の実機確認。

## 5. 関連 spec / docs

- `spec/operations/windows-native-preflight.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/initial/BTSTACK_SWITCH_DEVELOPMENT_PLAN.md`
- `spec/references/switch-hid-initial-source-audit.md`
- `spec/references/btstack-backend-build-matrix.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_016/BTSTACK_BACKEND_BUILD_VERIFICATION.md`

## 6. 根拠監査

Windows native、WinUSB、専用 USB Bluetooth ドングル、Switch pairing、report period comparison は実機検証または根拠監査が必要である。

既存の `spec/references/btstack-backend-build-matrix.md` は cross build の成功だけを記録しており、Windows native runtime 挙動は証明しない。

BTstack source selection または WinUSB backend facts を追加変更する場合は、`source-audit` を先に使う。

| 項目 | 状態 | 扱い |
|---|---|---|
| Windows MinGW cross build | recorded | `work-units/complete/local_016/BTSTACK_BACKEND_BUILD_VERIFICATION.md` に記録済みである。 |
| Windows native execution | pending | 実機未実行であり、`docs/hardware-test-log.md` への記録が必要である。 |
| WinUSB driver assignment | pending | Zadig の割り当て状態、USB VID/PID、driver を記録する必要がある。 |
| Switch pairing | pending | 人間の明示承認なしに実行しない。 |
| report period comparison | pending | 実測値は未記録であり、comparison 候補を default または fallback value として扱わない。 |

## 7. 設計メモ

- preflight は実機手順の gate であり、hardware result ではない。
- 実機へ触れる command は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を満たす場合だけ実行する。
- pairing、advertising、report loop、report period comparison は承認範囲を分けて扱う。
- 対象 adapter が曖昧な場合は、daemon 起動前に停止する。
- cleanup では daemon 停止、adapter 状態、neutral fail-safe の確認結果を記録する。

## 8. 対象ファイル

- `spec/operations/windows-native-preflight.md`
- `spec/operations/README.md`
- `docs/hardware-test-log.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/references/btstack-backend-build-matrix.md`
- `work-units/complete/local_027/WINDOWS_NATIVE_PREFLIGHT.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | Windows native preflight docs list approval, dedicated dongle, WinUSB assignment, environment variables, and hardware log target | new | docs | no |
| green | `swbt-daemon.exe` native start checklist does not claim pairing or report-loop success | regression | docs | no |
| green | hardware procedure is blocked unless `SWBT_RUN_HARDWARE=1` and `SWBT_HARDWARE_APPROVED=1` are both set | edge | docs | no |
| green | report period comparison records each candidate period separately in `docs/hardware-test-log.md` | characterization | docs | no |

TDD status:

- source: `spec/initial/REPOSITORY_INITIALIZATION_TODO.md` Phase 5 と実機安全境界。
- use case: 実機検証を開始する前に、承認、専用ドングル、WinUSB、環境変数、記録項目、cleanup plan を確認できる。
- item: Windows native preflight docs list approval, dedicated dongle, WinUSB assignment, environment variables, and hardware log target。
- state: green。
- commands:
  - red: `if (Test-Path spec\operations\windows-native-preflight.md) { exit 0 } else { Write-Error "missing spec/operations/windows-native-preflight.md"; exit 1 }` は preflight spec が存在しないため exit 1。
  - green: `rg -n "SWBT_RUN_HARDWARE=1|SWBT_HARDWARE_APPROVED=1|専用 USB Bluetooth|WinUSB|USB VID/PID|docs/hardware-test-log.md|8000 us|8333 us|15000 us|16667 us|cleanup|承認範囲" spec\operations\windows-native-preflight.md docs\hardware-test-log.md work-units\complete\local_027\WINDOWS_NATIVE_PREFLIGHT.md` は必要項目を検出した。
  - green: `rg -n "成功を開始前に主張しない|success|成功判定|未検証|未実行" spec\operations\windows-native-preflight.md work-units\complete\local_027\WINDOWS_NATIVE_PREFLIGHT.md` は未検証状態と成功主張禁止を検出した。
  - green: `git diff --check` は whitespace error なし。PowerShell の Git 設定に由来する CRLF 警告だけを出した。
  - green: unresolved placeholder check は、追加した spec、hardware log、record に未解決 placeholder が残っていないことを確認した。
  - green: `rg -n "Windows Native Preflight|SWBT_RUN_HARDWARE=1|SWBT_HARDWARE_APPROVED=1|report period comparison|source-audited default|未検証|hardware log" spec\operations\README.md spec\operations\windows-native-preflight.md docs\hardware-test-log.md` は operations index、preflight spec、hardware log のリンクと gate を検出した。
- notes: `tdd-workflow`、`tdd-test-list`、`tdd-one-cycle`、`work-unit-record`、`spec-page`、`hardware-harness`、`source-audit` を読んだ。C source は変更していないため CMake / CTest は実行していない。

## 10. 検証

実行済み:

- `rg -n "SWBT_RUN_HARDWARE=1|SWBT_HARDWARE_APPROVED=1|専用 USB Bluetooth|WinUSB|USB VID/PID|docs/hardware-test-log.md|8000 us|8333 us|15000 us|16667 us|cleanup|承認範囲" spec\operations\windows-native-preflight.md docs\hardware-test-log.md work-units\complete\local_027\WINDOWS_NATIVE_PREFLIGHT.md`: pass。
- `rg -n "成功を開始前に主張しない|success|成功判定|未検証|未実行" spec\operations\windows-native-preflight.md work-units\complete\local_027\WINDOWS_NATIVE_PREFLIGHT.md`: pass。
- `git diff --check`: pass。CRLF 警告あり、whitespace error なし。
- unresolved placeholder check: pass。
- `rg -n "Windows Native Preflight|SWBT_RUN_HARDWARE=1|SWBT_HARDWARE_APPROVED=1|report period comparison|source-audited default|未検証|hardware log" spec\operations\README.md spec\operations\windows-native-preflight.md docs\hardware-test-log.md`: pass。

未実行:

- CMake / CTest / sanitizer / cross build。理由は docs と work unit record の変更だけで、C source、CMake、test source、BTstack source selection を変更していないためである。
- Windows native preflight と実機コマンド。理由は、この work unit が preflight 文書と hardware log schema の固定であり、pairing、HID advertising、report loop の承認範囲を含まないためである。

## 11. 実機実行条件

実機検証は人間が実行範囲を明示承認した場合だけ実行する。

専用 USB Bluetooth ドングルを使い、内蔵 Bluetooth と常用ドングルは使わない。

Windows native では Zadig による WinUSB assignment、USB VID/PID、driver state を記録する。

実行時は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。

Switch pairing、HID advertising、report loop、report period comparison は承認範囲に含まれる場合だけ実行する。

結果は `docs/hardware-test-log.md` に OS、dongle、driver、backend、BTstack commit、swbt commit、Switch firmware、report period、result、cleanup を記録する。

この work unit では実機コマンドを実行しない。理由は、対象が preflight 文書と log schema の固定であり、pairing、HID advertising、report loop の承認範囲がこの依頼に含まれていないためである。

## 12. 先送り事項

- 観測: Windows native execution、WinUSB driver assignment、Bluetooth dongle recognition、Switch pairing、report period comparison は未実行である。
  先送り理由: この work unit は preflight gate の文書化だけを扱い、実機承認を伴う操作は対象外にしている。
  次の置き場: Phase 5 実機 work unit と `docs/hardware-test-log.md`。
- 観測: report period `8000 / 8333 / 15000 / 16667 us` は comparison 候補であり、source-audited default または fallback value ではない。
  先送り理由: 実測値と採用値は実機検証後に判断する必要がある。
  次の置き場: Phase 5 実機 work unit、必要なら `spec/references/` または protocol / operations spec。

## 13. チェックリスト

- [x] work unit record を作成した。
- [x] work unit record を新形式へ更新した。
- [x] Windows native preflight docs を追加した。
- [x] TDD テストまたは docs verification を実行した。
- [x] 検証結果を記録した。
- [x] 根拠監査状態を記録した。
- [x] 実機状態または未実行理由を記録した。
