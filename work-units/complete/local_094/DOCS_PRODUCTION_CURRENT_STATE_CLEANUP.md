# Docs Production Current State Cleanup

## 1. 概要

README と current docs に残った production 起動条件、module 名、実機承認条件の古い記述を削除し、現行実装と同じ言葉に揃える。

完了後、README、`docs/status.md`、current spec / operations docs は、`swbt-daemon` の既定 production backend、`--backend noop`、`--adapter-location`、現行の `swbt_domain_t` / `swbt_daemon_process_t` / `swbt_btstack_production_ports_t` を矛盾なく説明する。実装上の環境変数 gate が残っているような説明は置かない。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-28: `production_` prefix の意味が不明瞭で、資材配置換えを検討したい。
- user request, 2026-06-28: 古い情報の削除を 1 work unit として立ち上げる。
- 現状確認, 2026-06-28: `README.md` は `SWBT_DAEMON_BACKEND`、`SWBT_RUN_HARDWARE`、`SWBT_HARDWARE_APPROVED` を production 起動条件として説明している。
- 現状確認, 2026-06-28: `docs/status.md` は、現行実装が既定で production backend を選び、Bluetooth adapter を開かない確認では `--backend noop` を明示すると説明している。
- `work-units/complete/local_083/MODULE_RENAME_AND_PLACEMENT_CLEANUP.md`: `swbt/domain`、`swbt/support`、`daemon/process`、`production_runner`、`production_ports`、`production_btstack_impl` へ rename 済み。

use case:

- actor: README または status docs を見て daemon を起動する開発者。
- 入力または状態: README と docs/status の production 起動条件が異なり、旧 module 名も残っている。
- 期待する観測結果: current docs から、現行 CLI、現行 module 名、実機運用 gate、実装上の起動分岐を誤解なく辿れる。
- 制約: 起動挙動、IPC JSON wire format、Switch-facing bytes、report period、BTstack source selection、実機承認運用を変更しない。

source から use case への変換:

この work unit は、コード整理の前に読者が参照する current-state surface を直す。`production` の意味を変えるのではなく、古い implementation fact と旧名を削除する。

## 3. 対象範囲

- `README.md` の旧 production 起動条件、旧 module 名、旧 architecture 名を現行実装へ合わせる。
- `README.md` の状態表を `docs/status.md` と重複しすぎない形へ整理する。
- `docs/status.md` に古い path、旧 target 名、旧環境変数 gate の記述が残っていないか確認し、必要なら修正する。
- current spec / operations docs で、実装上の事実として古い production 起動条件を述べている箇所を修正する。
- archived spec / historical work unit record の旧記述は、current guidance として読まれるリンクだけを修正対象にする。
- `rg` で旧語彙の残存を確認し、歴史的記録として残すものと current docs から削除するものを分ける。

## 4. 対象外

- C source、CMake target、test source の変更。
- production backend の意味や CLI surface の変更。
- `--backend`、`--adapter-location`、diagnostic path options の挙動変更。
- `spec/archive/` 内の歴史的本文を全面的に書き換えること。
- 実機検証。

## 5. 関連 spec / docs

- `README.md`
- `docs/status.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`
- `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md`
- `work-units/complete/local_083/MODULE_RENAME_AND_PLACEMENT_CLEANUP.md`

## 6. 根拠監査

not applicable.

この work unit は docs の current-state cleanup であり、Switch HID report bytes、BTstack source selection、report timing、WinUSB/libusb behavior を追加または変更しない。BTstack source fact を更新する必要が出た場合は、その変更だけ `source-audit` の要否を再判断する。

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: target 分割や helper rename の前に、読者が見る current-state docs から古い実装条件を消す。
- verification: docs-only scan、必要に応じて `git diff --check`。コード検証は不要な理由を記録する。

方針:

- README は短い入口にし、詳細な状態表は `docs/status.md` を正とする。
- `production` は現行 CLI / status の backend 名として説明する。release quality や実機承認済みという意味では使わない。
- 実機承認は運用 gate として説明し、実装上の必須環境変数分岐として書かない。

## 8. 対象ファイル

- `README.md`
- `spec/operations/development-tooling.md`
- `spec/references/btstack-daemon-entrypoint.md`

確認のみ:

- `docs/status.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `spec/operations/windows-native-preflight.md`
- `docs/hardware-test-log.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | README and docs/status describe the same current production / noop launch behavior | regression | docs/review | no |
| green | current docs no longer state that `SWBT_DAEMON_BACKEND`, `SWBT_RUN_HARDWARE`, or `SWBT_HARDWARE_APPROVED` are implementation launch gates | regression | docs/review | no |
| green | current docs use current module names instead of `swbt_app_t`, `swbt_daemon_host_t`, `production_backend.c`, or `swbt_btstack_production_adapter_t` as implementation facts | regression | docs/review | no |
| green | archived or historical references that intentionally keep old names are distinguishable from current guidance | characterization | docs/review | no |

## 10. 検証

docs-only verification completed.

実行:

- `rg -n "SWBT_DAEMON_BACKEND|SWBT_RUN_HARDWARE|SWBT_HARDWARE_APPROVED|swbt_app_t|swbt_daemon_host_t|production_backend\\.c|swbt_btstack_production_adapter_t" README.md docs/status.md spec/operations spec/references`
  - result: remaining matches are explicit "not current selector / not implementation gate" statements in README, `docs/status.md`, operations specs, and `spec/references/btstack-daemon-entrypoint.md`.
- `rg -n "SWBT_DAEMON_BACKEND=production.*だけ|SWBT_RUN_HARDWARE=1.*SWBT_HARDWARE_APPROVED=1.*(揃|必要条件)|明示的な環境変数を必要条件|production_backend\\.c|swbt_daemon_host_t|swbt_app_t|swbt_btstack_production_adapter_t" README.md docs/status.md spec/operations spec/references`
  - result: no matches. Current guidance no longer states old environment variable gates or old module names as implementation facts.
- `rg -n "SWBT_DAEMON_BACKEND|SWBT_RUN_HARDWARE|SWBT_HARDWARE_APPROVED|swbt_app_t|swbt_daemon_host_t|production_backend\\.c|swbt_btstack_production_adapter_t" README.md docs spec`
  - result: remaining matches outside the current guidance set are historical hardware log entries, `spec/archive/`, and the adopted external review body in `spec/architecture/daemon-architecture-cutover.md`.
- `git diff --check`
  - result: pass. No whitespace errors.

`just debug` と `just verify` は実行していない。この work unit は docs-only cleanup であり、C source、CMake target、test source、Switch-facing bytes、BTstack source selection、runtime behavior を変更していない。PR 作成時の pre-push hook で `just verify` が走る場合は、その結果を PR に記録する。

## 11. 実機実行条件

実機実行は不要。

この work unit は docs の事実整合だけを扱い、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop を実行しない。

## 12. 先送り事項

none.

target 分割と helper rename は `local_095` と `local_096` で別 work unit として扱う。

## 13. チェックリスト

- [x] README の current-state 表記を現行実装に合わせた。
- [x] docs/status と README の production / noop 起動条件が矛盾していない。
- [x] current docs から旧 module 名と旧 implementation gate を削除または歴史記録として区別した。
- [x] docs-only 検証結果を記録した。
- [x] 実機未実行理由を維持した。
