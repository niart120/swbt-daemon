# Windows Native CI Gate

## 1. 概要

Windows native 経路の CI gate を設計し、必要なら追加する work unit。

Windows native PowerShell 経路は `local_032` で検証済みだが、architecture cutover 後の deferred item では Windows native CI が複数回残っている。この work unit では、実機を使わない Windows native gate をどこまで CI に入れるかを決める。

対象は hardware gate ではなく、PowerShell entrypoint、Dev Container 委譲、CMake preset 読み取り、Windows-specific wrapper failure の検出である。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md` の先送り事項: Windows native CI。
- `work-units/complete/local_054/DAEMON_HOST_AND_BUILD_BOUNDARIES.md` の先送り事項: Windows native CI。
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md` の先送り事項: Windows native CI。
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md` の対象外: Windows native CI。
- `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md` の検証済み Windows native just / Dev Container flow。

use case:

- actor: maintainer、CI、Windows contributor。
- 入力または状態: Windows runner、PowerShell、`just`、Dev Container CLI、Docker Desktop 前提の有無、CMake presets。
- 期待する観測結果: Windows native entrypoint が壊れた場合に CI または明確な local gate で検出できる。実機承認を必要とする処理は CI に入らない。
- 制約: host build は既定で止める。hardware test は CI に入れない。CI cost と Docker availability を明示する。
- 対象外: Switch pairing、WinUSB driver assignment、hardware H1、release publish。

source から use case への変換:

deferred item の Windows native CI は、実機 smoke ではなく host entrypoint の再現性を指す。まず gate の目的を定義し、GitHub Actions job にするか docs / script gate に留めるかを判断する。

## 3. 対象範囲

- 現行 CI workflow と local `just` entrypoint を確認する。
- Windows native CI で確認する最小 command を決める。
- Docker / Dev Container が runner で使えない場合の代替 gate を決める。
- hardware environment variables が CI で有効にならないことを確認する。
- 必要なら `.github/workflows/*`、docs、work unit record を更新する。

## 4. 対象外

- 実機 H1、pairing、report loop。
- Windows host build の既定許可。
- release publish。
- parser fuzz の実装。
- CMake target include boundary の再設計。

## 5. 関連 spec / docs

- `spec/operations/development-tooling.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md`
- `work-units/complete/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md`
- `work-units/complete/local_054/DAEMON_HOST_AND_BUILD_BOUNDARIES.md`
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`

## 6. 根拠監査

not applicable。

この work unit は CI / tooling gate を扱う。Switch protocol byte、BTstack source selection、report period、WinUSB/libusb facts を追加または変更しない。WinUSB や実機 adapter state を CI で扱う場合は範囲外として切り直す。

## 7. 設計メモ

- Windows native CI は hardware gate ではない。
- `SWBT_RUN_HARDWARE` と `SWBT_HARDWARE_APPROVED` は CI で有効化しない。
- CI job は `SWBT_RUN_HARDWARE=0` と `SWBT_HARDWARE_APPROVED=0` を明示し、実機承認を必要とする処理を既定で開始しない。
- host build opt-in を CI の都合で既定化しない。
- Dev Container を CI で起動する場合は、Docker availability と cache failure を明示する。
- 2026-06-24 の判断: 独立した Windows native CI job は追加しない。現在の CI gate は Ubuntu runner 上の Dev Container で `just verify-ci` を実行する。
- Windows native PowerShell entrypoint は local gate として扱い、Windows filesystem checkout で `just list-presets` を実行して Dev Container 委譲と CMake preset 読み取りを最小確認する。広い確認が必要な場合は、同じ PowerShell 入口で `just verify` を実行する。
- Windows native PowerShell entrypoint の失敗再現コマンドは `just list-presets` とする。Dev Container CLI がない場合は `devcontainer CLI was not found. Install the Dev Containers CLI or open this repository in the Dev Container.` を前提条件不足として扱う。host build への退避は対象外であり、この失敗を `SWBT_ALLOW_HOST_BUILD=1` で迂回しない。

## 8. 対象ファイル

- `.github/workflows/*`
- `justfile`
- `spec/operations/development-tooling.md`
- `spec/operations/windows-native-preflight.md`
- `work-units/wip/local_063/WINDOWS_NATIVE_CI_GATE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | Windows native gate documents whether it uses Dev Container, host preset read, or docs-only verification | characterization | docs | no |
| green | CI or local script refuses hardware execution by default | regression | integration | no |
| green | Windows native entrypoint failure is represented by a reproducible command or explicit non-goal | characterization | build | no |
| todo | existing Linux / ASan / Windows cross gates remain the primary software verification path | regression | docs | no |

## 10. 検証

- red: `rg -n "Windows native CI decision|Windows native CI job is not added|Windows native gate mode|Windows native CI job は追加しない" spec\operations\development-tooling.md` -> no match。
- green: `spec/operations/development-tooling.md` に Windows native CI job を追加しない判断、CI の Dev Container gate、Windows native PowerShell local gate を記録する。
- red: `rg -n 'SWBT_RUN_HARDWARE:\s*"0"|SWBT_HARDWARE_APPROVED:\s*"0"|CI job explicitly disables hardware approval' .github\workflows\ci.yml work-units\wip\local_063\WINDOWS_NATIVE_CI_GATE.md` -> no match。
- green: `.github/workflows/ci.yml` の `verify` job に `SWBT_RUN_HARDWARE=0` と `SWBT_HARDWARE_APPROVED=0` を明示し、`spec/operations/development-tooling.md` とこの record に非実機 CI の既定状態を記録する。
- red: `rg -n '失敗再現コマンドは|host build への退避は対象外' spec\operations\development-tooling.md work-units\wip\local_063\WINDOWS_NATIVE_CI_GATE.md` -> no match。
- green: `spec/operations/development-tooling.md` とこの record に `just list-presets` を Windows native PowerShell entrypoint の失敗再現コマンドとして記録し、Dev Container CLI 不足と host build への退避の扱いを明示する。

## 11. 実機実行条件

実機不要。Windows native CI gate は hardware test を含まない。

実機を扱う場合は別 work unit とし、`hardware-harness`、専用 USB Bluetooth dongle、WinUSB、明示承認、`docs/hardware-test-log.md` 記録を必須にする。

## 12. 先送り事項

none。起票時点の先送り事項は、この record の source として取り込んだ。

## 13. チェックリスト

- [x] source を `local_052`、`local_054`、`local_055`、`local_057` から特定した。
- [x] use case を Windows native non-hardware gate として定義した。
- [ ] 現行 CI workflow を確認した。
- [ ] gate の追加または非追加判断を記録した。
- [ ] 必要な docs / workflow を更新した。
- [ ] CI または local verification を実行した。
