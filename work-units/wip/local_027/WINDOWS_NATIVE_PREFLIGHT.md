# Windows Native Preflight

## 1. 概要

Phase 5 の Windows 実機確認の前に、Windows native、WinUSB、専用 USB Bluetooth ドングルの preflight を切り出す計画 record。

この work unit は `swbt-daemon.exe` の Windows native 起動、Switch pairing、report period comparison を始める前の承認条件と記録項目を固定する。

実機は未実行であり、Windows native runtime、WinUSB driver assignment、pairing、report loop の成功をこの record では断定しない。

## 2. 対象範囲

- 専用 USB Bluetooth ドングルを使う前提を確認し、内蔵 Bluetooth と常用ドングルを対象外にする手順を固める。
- Zadig による WinUSB driver assignment の記録項目を固定する。
- Windows native で `swbt-daemon.exe` を起動する前の承認、環境変数、log target を整理する。
- Switch pairing、HID advertising、report loop、report period comparison を実行する条件を明記する。
- `docs/hardware-test-log.md` に残す項目を Phase 5 TODO と合わせる。

## 3. 対象外

- Switch pairing の実行。
- HID advertising と periodic report loop の実行。
- report period `8000 / 8333 / 15000 / 16667 us` の実測。
- BTstack source selection の変更。
- `vendor/btstack` の変更。
- rumble callback と status behavior の実機確認。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/initial/BTSTACK_SWITCH_DEVELOPMENT_PLAN.md`
- `spec/references/switch-hid-initial-source-audit.md`
- `spec/references/btstack-backend-build-matrix.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_016/BTSTACK_BACKEND_BUILD_VERIFICATION.md`

## 5. 根拠監査

Windows native、WinUSB、専用 USB Bluetooth ドングル、Switch pairing、report period comparison は実機検証または根拠監査が必要である。

既存の `spec/references/btstack-backend-build-matrix.md` は cross build の成功だけを記録しており、Windows native runtime behavior は証明しない。

BTstack source selection または WinUSB backend facts を追加変更する場合は、`source-audit` を先に使う。

| 項目 | 状態 | 扱い |
|---|---|---|
| Windows MinGW cross build | recorded | `work-units/complete/local_016/BTSTACK_BACKEND_BUILD_VERIFICATION.md` に記録済みである。 |
| Windows native execution | pending | 実機未実行であり、`docs/hardware-test-log.md` への記録が必要である。 |
| WinUSB driver assignment | pending | Zadig の割り当て状態、USB VID/PID、driver を記録する必要がある。 |
| Switch pairing | pending | 人間の明示承認なしに実行しない。 |
| report period comparison | pending | 実測値は未記録であり、既定値を hardware fact として扱わない。 |

## 6. 設計メモ

- preflight は実機手順の gate であり、hardware result ではない。
- 実機へ触れる command は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を満たす場合だけ実行する。
- pairing、advertising、report loop、report period comparison は承認範囲を分けて扱う。
- 対象 adapter が曖昧な場合は、daemon 起動前に停止する。
- cleanup では daemon 停止、adapter 状態、neutral fail-safe の確認結果を記録する。

## 7. 対象ファイル

- `spec/operations/windows-native-preflight.md`
- `docs/hardware-test-log.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/references/btstack-backend-build-matrix.md`
- `apps/swbt-daemon/main.c`
- `work-units/wip/local_027/WINDOWS_NATIVE_PREFLIGHT.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | Windows native preflight docs list approval, dedicated dongle, WinUSB assignment, environment variables, and hardware log target | new | docs | no |
| todo | `swbt-daemon.exe` native start checklist does not claim pairing or report-loop success | regression | docs | no |
| todo | hardware procedure is blocked unless `SWBT_RUN_HARDWARE=1` and `SWBT_HARDWARE_APPROVED=1` are both set | edge | hardware | yes |
| todo | report period comparison records each candidate period separately in `docs/hardware-test-log.md` | characterization | hardware | yes |

## 9. 検証

未実行。

この record では計画を作成しただけで、`make debug`、`make windows-cross`、`make verify`、実機コマンドは実行していない。

完了時には影響範囲に応じた build、test、docs verification の結果を記録する。

Windows native preflight を実行する場合は、実機条件を満たしたうえで `docs/hardware-test-log.md` に結果を記録する。

## 10. 実機実行条件

実機検証は人間が実行範囲を明示承認した場合だけ実行する。

専用 USB Bluetooth ドングルを使い、内蔵 Bluetooth と常用ドングルは使わない。

Windows native では Zadig による WinUSB assignment、USB VID/PID、driver state を記録する。

実行時は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。

Switch pairing、HID advertising、report loop、report period comparison は承認範囲に含まれる場合だけ実行する。

結果は `docs/hardware-test-log.md` に OS、dongle、driver、backend、BTstack commit、swbt commit、Switch firmware、report period、result、cleanup を記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] Windows native preflight の実装または docs を追加した。
- [ ] TDD テストまたは docs verification を実行した。
- [ ] 検証結果を記録した。
- [ ] 根拠監査を完了した。
- [ ] 実機検証を実行し `docs/hardware-test-log.md` に記録した。
