# TDD Workflow Skill Group Reimplementation

## 1. 概要

`tdd-workflow` skill と、その元になった TDD 系 skill 群を swbt-daemon 向けにどう再実装するかを検討する work unit。

`tmp/swbt_agent_skill_adoption_policy.md` では、`ponkan-python` 由来の `tdd-workflow`、`tdd-test-list`、`tdd-one-cycle`、`tidy-first` を swbt 向けに軽量統合して導入する方針だった。現在の `.agents/skills/tdd-workflow` はその統合版に近いが、後続の spec では `test-desiderata-review` も含めた分割候補が未解決事項として残っている。

この work unit では、単に現行 skill を分割するかではなく、元々参照していた TDD skill 群を swbt 用に再実装する範囲、統合したままにする範囲、分割して戻す範囲を判断する。

この record は検討作業の入口である。分割の実装、skill validation、既存 record の更新は未着手である。

## 2. 起点 / ユースケース

source:

- ユーザ要求。`tdd-workflow` skill の分割検討を work unit record として新規作成する。
- ユーザ補足。これは単なる分割ではなく、元々引用していた skill 群の再実装でもある。
- `tmp/swbt_agent_skill_adoption_policy.md` の採用候補 5。`ponkan-python` の `tdd-workflow`、`tdd-test-list`、`tdd-one-cycle`、`tidy-first` は有用だが、初回導入では分割しすぎず `swbt-tdd-workflow` として軽量統合する方針だった。
- `tmp/swbt_agent_skill_adoption_policy.md` の実装状況。2026-06-18 時点では `.agents/skills/swbt-tdd-workflow/SKILL.md` を含む Phase 2 skill が導入済みと記録されているが、現在の worktree では `.agents/skills/tdd-workflow/` が該当する TDD skill である。
- `work-units/complete/local_034/WORK_UNIT_RECORD_SECTION_GUIDE.md` の先送り事項。`tdd-workflow` skill の分割検討を別 work unit として立てる候補がある。
- `spec/operations/work-unit-spec-tdd-flow.md` の未解決事項。`tdd-workflow` を複数 skill に分割するかどうかは未決定であり、候補は `tdd-test-list`、`tdd-one-cycle`、`tidy-first`、`test-desiderata-review` である。

use case:

- actor: agent または maintainer。
- 入力または状態: `tmp/swbt_agent_skill_adoption_policy.md` の TDD 系 skill 導入方針、`tdd-workflow` skill の現行記述、work unit record の source / use case / TDD Test List 方針、分割候補名、既存 work unit record の TDD status 記録。
- 期待する観測結果: 元々参照していた TDD skill 群を swbt 用にどう再実装するか、`tdd-workflow` を現状維持するか分割するか、分割する場合の skill 境界、呼び出し順序、移行対象、検証方法が work unit record または関連 docs に記録される。
- 制約: source から use case を作ってから TDD Test List を作る順序を壊さない。work unit record と spec の責務を混ぜない。protocol、BTstack、実機挙動の値は扱わない。
- 対象外: この work unit の開始時点では、Switch protocol 実装、C unit test、daemon 挙動、実機検証は扱わない。
- source から use case へ変換した判断: `tmp` の導入方針は元 skill 群の採用判断であり、後続 spec の未解決事項は分割候補名の列挙である。agent が実際に skill を使う場面で観測できる責務境界、再実装範囲、検証結果へ変換してから検討する。

## 3. 対象範囲

- `tmp/swbt_agent_skill_adoption_policy.md` に記録された TDD 系 skill 導入方針を確認する。
- 現行 `.agents/skills/tdd-workflow/SKILL.md` の責務を、元々参照していた `tdd-workflow`、`tdd-test-list`、`tdd-one-cycle`、`tidy-first` の責務と対応づける。
- `tdd-test-list`、`tdd-one-cycle`、`tidy-first`、`test-desiderata-review` の分割候補について、各 skill が扱う source、入力、出力、前提 skill を整理する。
- 統合したままにする場合の理由、分割する場合の最小移行単位、再実装する場合に元 skill から持ち込むべき要素を記録する。
- 分割判断が `work-unit-record` skill、`spec/operations/work-unit-spec-tdd-flow.md`、既存 work unit record の TDD status 記録と矛盾しないことを確認する。
- 必要になった場合だけ、関連 skill または operations spec の更新案を作る。

## 4. 対象外

- Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の値の追加または変更。
- `vendor/btstack` の変更。
- daemon runtime、IPC、scheduler、controller report packing の実装変更。
- 実機 pairing、HID advertising、report loop の実行。
- 既存 wip work unit record への source / use case 一括追記。

## 5. 関連 spec / docs

- `spec/operations/work-unit-spec-tdd-flow.md`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/work-unit-record/SKILL.md`
- `.agents/skills/work-unit-record/references/section-guide.md`
- `tmp/swbt_agent_skill_adoption_policy.md`
- `work-units/complete/local_033/WORK_UNIT_SPEC_TDD_FLOW.md`
- `work-units/complete/local_034/WORK_UNIT_RECORD_SECTION_GUIDE.md`

## 6. 根拠監査

not applicable。

この work unit は project-local skill と operations policy の責務分割を扱う。Switch HID report bytes、BTstack source selection、report period、subcommand、SPI、rumble、descriptor data、WinUSB/libusb の実装値は追加または変更しない。

## 7. 設計メモ

再実装検討では、skill 名ではなく呼び出し境界を先に確認する。`tmp` の採用方針では `tdd-workflow`、`tdd-test-list`、`tdd-one-cycle`、`tidy-first` を swbt 向けに軽量統合する前提だったため、現行 skill がその責務を十分に吸収しているかを確認する。

特に次の境界を分けて考える。

- TDD Test List 作成: source / use case から観測可能な test item を作る責務。
- 1 cycle 実行: TDD Test List の 1 item を red / green / refactor で進める責務。
- tidy-first: 挙動変更前に安全な構造整理を行う条件と検証を判断する責務。
- desiderata review: test item が実装都合や file list ではなく use case に対応しているかを点検する責務。

分割する場合でも、agent が最初に読む入口 skill と、各補助 skill を読む条件を明確にする必要がある。分割しない場合は、現行 skill に責務が集中していても運用上の誤用が少ない理由を残す。再実装する場合は、`ponkan-python` 固有の Python / `uv` / package release 前提を持ち込まず、CMake / CTest / work unit record 前提へ置き換える。

## 8. 対象ファイル

- `work-units/wip/local_035/TDD_WORKFLOW_SKILL_GROUP_REIMPLEMENTATION.md`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/tdd-workflow/agents/openai.yaml`
- `.agents/skills/work-unit-record/SKILL.md`
- `.agents/skills/work-unit-record/references/section-guide.md`
- `spec/operations/work-unit-spec-tdd-flow.md`
- `tmp/swbt_agent_skill_adoption_policy.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | tmp adoption memo is used as source for the original TDD skill group reimplementation scope | characterization | docs | no |
| todo | current tdd-workflow responsibilities are mapped to the originally referenced tdd-workflow, tdd-test-list, tdd-one-cycle, and tidy-first skills | characterization | docs | no |
| todo | current tdd-workflow responsibilities are categorized into test-list creation, one-cycle execution, tidy-first decision, and desiderata review | characterization | docs | no |
| todo | split decision records whether each candidate should become a separate skill, remain inside tdd-workflow, be reimplemented from the original skill group, or be deferred | new | docs | no |
| todo | proposed skill boundaries preserve the source -> use case -> work unit -> TDD Test List -> TDD cycle order | regression | docs | no |
| todo | related skill guidance remains consistent with work-unit-record section requirements | regression | docs | no |
| todo | any updated project-local skill passes skill validation | verification | workflow | no |

TDD status:

- source: ユーザ補足、`tmp/swbt_agent_skill_adoption_policy.md`、`local_034` の先送り事項、`spec/operations/work-unit-spec-tdd-flow.md` の未解決事項。
- use case: agent が元々参照していた TDD skill 群を swbt 向けに再実装する範囲を判断し、`tdd-workflow` の責務を誤用しない粒度へ分けるか判断し、分割または統合維持の境界と検証を記録する。
- item: tmp adoption memo is used as source for the original TDD skill group reimplementation scope。
- state: todo。
- commands: none yet。
- notes: この record は再実装 / 分割検討の入口であり、red / green / refactor は未開始である。

## 10. 検証

未実行。

この record の更新時点では、`tmp/swbt_agent_skill_adoption_policy.md` の TDD 系 skill 導入方針を source として確認した。`tdd-workflow` skill の再実装判断、分割判断、skill 更新、skill validation はまだ実行していない。検討または実装を進めた時点で、実行したコマンドと結果をここへ追記する。

## 11. 実機実行条件

実機検証は不要。

この work unit は project-local skill と operations policy の整理だけを扱い、Bluetooth adapter、Switch pairing、HID advertising、report loop を実行しない。

## 12. 先送り事項

none。

この record は先送り事項と `tmp` の導入メモから起こした work unit の入口であり、現時点で追加の follow-up はない。検討中に分割候補から外す項目、別 spec に移す項目、既存 record の移行候補が見つかった場合は、観測、先送り理由、次の置き場を追記する。

## 13. チェックリスト

- [ ] `tdd-workflow` skill の現行責務を確認した。
- [ ] `tmp/swbt_agent_skill_adoption_policy.md` の TDD 系 skill 導入方針を確認した。
- [ ] 元々参照していた TDD skill 群と現行 `tdd-workflow` の対応を整理した。
- [ ] 分割候補ごとの入力、出力、呼び出し条件を整理した。
- [ ] 分割するか現状維持するかの判断を記録した。
- [ ] `work-unit-record` skill と operations spec との整合性を確認した。
- [ ] 必要な skill または spec 更新を実施した、または不要理由を記録した。
- [ ] 根拠監査の状態を記録した。
- [ ] 実機状態を記録した。
- [ ] 検証コマンドと結果を記録した。
