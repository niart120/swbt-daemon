# Production Runner Decomposition Plan

## 1. 概要

`swbt/daemon/production_runner.*` に集まった production 起動責務を、owner module ごとに段階分割する。

この record は全体計画と完了条件を固定する親 record である。個別の実装は `local_085` から `local_093` の各 work unit で扱う。各 work unit は対象外を再掲して完了条件を閉じ、既に別 work unit として起票済みの内容を先送り事項に重複記録しない。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-28: `production_runner` が複数責務を背負っているため、apps、daemon、BTstack bridge の境界を見直したい。
- user request, 2026-06-28: ファイル分割、validator、再配置先、全体計画、マイルストーン、終着点を確認したい。
- `work-units/complete/local_083/MODULE_RENAME_AND_PLACEMENT_CLEANUP.md`: rename だけでは意味が通らない箇所は責務移動候補として先送りされた。
- 現状分析, 2026-06-28: `production_runner.c` には ports validation、address parse / format、IPC pump、HID packet handler、report timer、active reconnect、shutdown、runner lifecycle が同居している。

use case:

- actor: production path の後続 refactor を行う開発者。
- 入力または状態: `production_runner.c` を読むと、production lifecycle の順序と BTstack / IPC / config / OS process の細部が混在している。
- 期待する観測結果: 各責務の owner、完了条件、検証が分かれ、後続 work unit が未完了事項を無制限に持ち越さない。
- 制約: IPC JSON、public C ABI、Switch-facing bytes、report period、BTstack source selection、shutdown neutral ordering は変更しない。

source から use case への変換:

`production_runner` を一度に解体すると、HID event、reconnect、shutdown の境界変更が混ざる。まず work unit record をマイルストーン単位で起票し、各 work unit の対象外と完了条件を閉じる。

## 3. 対象範囲

- 全体の終着点を定義する。
- `local_085` から `local_093` の実装順序と依存を記録する。
- 各 work unit に、対象外と先送り事項の扱いを明記する。
- 実機実行条件と根拠監査の扱いを共通方針として固定する。

## 4. 対象外

- この record 自体で code を変更しない。
- stable spec の更新。個別 work unit で spec 更新が必要になった場合だけ `spec-page` を使う。
- 実機検証。
- `vendor/btstack` の編集。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `docs/status.md`
- `work-units/complete/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md`
- `work-units/complete/local_079/DEVICE_LIFECYCLE_API.md`
- `work-units/complete/local_083/MODULE_RENAME_AND_PLACEMENT_CLEANUP.md`
- `tests/cmake/include_boundaries_test.cmake`
- `tests/cmake/compile_include_boundaries_test.cmake`

## 6. 根拠監査

not applicable.

この record は作業分割の管理記録であり、Switch HID report bytes、BTstack source selection、report timing、WinUSB/libusb behavior を追加または変更しない。

## 7. 設計メモ

Tidy status:

- classification: structure change
- decision: tidy first
- reason: production path の次の振る舞い変更を小さくするため、責務の owner と work unit 境界を先に固定する。
- verification: record 作成後、必須セクションと相互リンクを `rg` で確認する。code verification は個別 work unit で実行する。

終着点:

- `apps/swbt-daemon` は executable composition root と OS process support を持つ。
- `swbt/daemon` は daemon process、production lifecycle、IPC / runtime / config との接続を持つ。
- `swbt/btstack_bridge` は BTstack-facing ports、device lifecycle、BTstack concrete implementation を持つ。
- `production_runner` は `init -> process start -> power on -> reconnect -> run loop -> cleanup` の orchestration に縮小する。

マイルストーン:

| order | work unit | 完了後の状態 |
|---:|---|---|
| 1 | `local_085` Entry / platform split | `main.c` が production 起動詳細と Windows process support を直接持たない |
| 2 | `local_086` production ports validation boundary | ports validator が `btstack_bridge/production_ports.*` に移る |
| 3 | `local_087` address / reconnect boundary | address text / bytes 変換と active reconnect が runner から分離される |
| 4 | `local_088` IPC pump boundary | IPC runner から BTstack run loop pump への adapter が runner から分離される |
| 5 | `local_089` report timer bridge boundary | report timer send / metrics bridge が runner から分離される |
| 6 | `local_090` HID session boundary | context-less HID packet handler と HID event dispatch が runner から分離される |
| 7 | `local_091` process backend table boundary | production process backend table が接続だけを読む file へ分離される |
| 8 | `local_092` shutdown boundary | shutdown scheduling と pending neutral cleanup が runner から分離される |
| 9 | `local_093` runner header finalization | runner header の公開 surface を縮小し、obsolete API を処理する |

## 8. 対象ファイル

- `work-units/wip/local_085/PRODUCTION_ENTRYPOINT_PLATFORM_SPLIT.md`
- `work-units/wip/local_086/PRODUCTION_PORTS_VALIDATION_BOUNDARY.md`
- `work-units/wip/local_087/PRODUCTION_ADDRESS_RECONNECT_BOUNDARY.md`
- `work-units/wip/local_088/PRODUCTION_IPC_PUMP_BOUNDARY.md`
- `work-units/wip/local_089/PRODUCTION_REPORT_TIMER_BOUNDARY.md`
- `work-units/wip/local_090/PRODUCTION_HID_SESSION_BOUNDARY.md`
- `work-units/wip/local_091/PRODUCTION_PROCESS_BACKEND_TABLE_BOUNDARY.md`
- `work-units/wip/local_092/PRODUCTION_SHUTDOWN_BOUNDARY.md`
- `work-units/wip/local_093/PRODUCTION_RUNNER_HEADER_FINALIZATION.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | milestone work unit records define non-overlapping completion criteria for production runner decomposition | characterization | docs/review | no |
| todo | each milestone record states that already-created later milestones are not repeated as deferred work | characterization | docs/review | no |
| todo | overall endpoint preserves CLI, IPC JSON, HID bytes, report period, BTstack source selection, and shutdown neutral ordering | regression | docs/review | no |
| todo | decomposition series closes only after `local_085` through `local_093` complete without unassigned deferred items | regression | docs/review | no |

## 10. 検証

not run yet.

record 作成時:

- placeholder-pattern scan across `work-units\wip\local_084` through `work-units\wip\local_093`
  - result: first check found two vague references. They were replaced with concrete completed work unit paths.
  - result: final check found no matches.
- `rg -n "^## (1|2|3|4|5|6|7|8|9|10|11|12|13)\." work-units\wip\local_084 ... work-units\wip\local_093`
  - result: all 10 records contain sections 1 through 13.
- `Get-ChildItem -File work-units\wip\local_084 ... work-units\wip\local_093`
  - result: all 10 expected record files exist.
- trailing whitespace scan across all 10 new Markdown files
  - result: no matches.

## 11. 実機実行条件

実機実行は不要。

この record は作業分割だけを扱い、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop に触れない。個別 work unit で BTstack call order、WinUSB/libusb behavior、Switch-facing bytes を変える必要が出た場合は、その work unit で `source-audit` と `hardware-harness` の要否を再判断する。

## 12. 先送り事項

none.

後続作業は `local_085` から `local_093` として起票済みである。これらはこの record の先送り事項ではなく、独立した work unit の source として扱う。

## 13. チェックリスト

- [x] `local_085` から `local_093` の record が存在する。
- [x] 各 record の対象範囲、対象外、先送り事項が重複していない。
- [x] 各 record の TDD Test List が use case に結び付いている。
- [x] 全体の終着点と milestone order が記録されている。
- [x] record 作成後の `rg` 確認結果を記録した。
- [ ] `local_085` から `local_093` の完了後、この parent record を complete へ移すか判断した。
