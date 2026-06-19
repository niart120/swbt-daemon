# TDD Workflow Skill Group Reimplementation

## 1. 概要

`tdd-workflow` skill と、その元になった TDD 系 skill 群を swbt-daemon 向けに再実装した work unit。

`tmp/swbt_agent_skill_adoption_policy.md` では、`ponkan-python` 由来の `tdd-workflow`、`tdd-test-list`、`tdd-one-cycle`、`tidy-first` を swbt 向けに導入する方針だった。後続の spec では `test-desiderata-review` も含めた分割候補が未解決事項として残っていた。

この work unit では、`tdd-workflow` を入口 skill に戻し、実作業を `tdd-test-list`、`tdd-one-cycle`、`tidy-first`、`test-desiderata-review` に分割した。各 skill は `ponkan-python` の記述密度を参考にしつつ、CMake / CTest / `just`、work unit record、根拠監査、実機実行条件へ置き換えた。

## 2. 起点 / ユースケース

source:

- ユーザ要求。`tdd-workflow` skill の分割検討を work unit record として新規作成する。
- ユーザ補足。これは単なる分割ではなく、元々引用していた skill 群の再実装でもある。
- `tmp/swbt_agent_skill_adoption_policy.md` の採用候補 5。`ponkan-python` の `tdd-workflow`、`tdd-test-list`、`tdd-one-cycle`、`tidy-first` は有用だが、初回導入では分割しすぎず `swbt-tdd-workflow` として軽量統合する方針だった。
- `tmp/swbt_agent_skill_adoption_policy.md` の実装状況。2026-06-18 時点では `.agents/skills/swbt-tdd-workflow/SKILL.md` を含む Phase 2 skill が導入済みと記録されているが、現在の worktree では `.agents/skills/tdd-workflow/` が該当する TDD skill である。
- `work-units/complete/local_034/WORK_UNIT_RECORD_SECTION_GUIDE.md` の先送り事項。`tdd-workflow` skill の分割検討を別 work unit として立てる候補がある。
- `spec/operations/work-unit-spec-tdd-flow.md` の未解決事項。`tdd-workflow` を複数 skill に分割するかどうかは未決定であり、候補は `tdd-test-list`、`tdd-one-cycle`、`tidy-first`、`test-desiderata-review` である。
- GitHub connector で確認した `niart120/ponkan-python` の `.agents/skills/tdd-workflow/SKILL.md`、`.agents/skills/tdd-test-list/SKILL.md`、`.agents/skills/tdd-one-cycle/SKILL.md`、`.agents/skills/tidy-first/SKILL.md`、`.agents/skills/test-desiderata-review/SKILL.md`。

use case:

- actor: agent または maintainer。
- 入力または状態: `tmp/swbt_agent_skill_adoption_policy.md` の TDD 系 skill 導入方針、`tdd-workflow` skill の現行記述、work unit record の source / use case / TDD Test List 方針、分割候補名、既存 work unit record の TDD status 記録。
- 期待する観測結果: 元々参照していた TDD skill 群が swbt 用の project-local skill として再実装され、`tdd-workflow` が入口、`tdd-test-list` / `tdd-one-cycle` / `tidy-first` / `test-desiderata-review` が分割 skill として使える。分割境界、呼び出し順序、検証方法が skill と operations spec に記録される。
- 制約: source から use case を作ってから TDD Test List を作る順序を壊さない。work unit record と spec の責務を混ぜない。protocol、BTstack、実機挙動の値は扱わない。
- 対象外: この work unit の開始時点では、Switch protocol 実装、C unit test、daemon 挙動、実機検証は扱わない。
- source から use case へ変換した判断: `tmp` の導入方針は元 skill 群の採用判断であり、後続 spec の未解決事項は分割候補名の列挙である。完了条件は、候補名の記録ではなく、agent が実際に読める分割 skill、swbt 固有の入出力、validation 結果として観測できる状態にする。

## 3. 対象範囲

- `tmp/swbt_agent_skill_adoption_policy.md` に記録された TDD 系 skill 導入方針を確認する。
- `ponkan-python` の TDD 系 skill 5 件を確認する。
- `.agents/skills/tdd-workflow/SKILL.md` を入口 skill として更新する。
- `.agents/skills/tdd-test-list/SKILL.md` を追加する。
- `.agents/skills/tdd-one-cycle/SKILL.md` を追加する。
- `.agents/skills/tidy-first/SKILL.md` を追加する。
- `.agents/skills/test-desiderata-review/SKILL.md` を追加する。
- 各 skill の `agents/openai.yaml` を追加または更新する。
- `spec/operations/work-unit-spec-tdd-flow.md` に分割後の方針を記録する。
- `AGENTS.md` の Agent Skills 一覧に分割 skill を追加する。

## 4. 対象外

- Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の値の追加または変更。
- `vendor/btstack` の変更。
- daemon runtime、IPC、scheduler、controller report packing の実装変更。
- 実機 pairing、HID advertising、report loop の実行。
- 既存 wip work unit record への source / use case 一括追記。

## 5. 関連 spec / docs

- `spec/operations/work-unit-spec-tdd-flow.md`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/tdd-test-list/SKILL.md`
- `.agents/skills/tdd-one-cycle/SKILL.md`
- `.agents/skills/tidy-first/SKILL.md`
- `.agents/skills/test-desiderata-review/SKILL.md`
- `.agents/skills/work-unit-record/SKILL.md`
- `.agents/skills/work-unit-record/references/section-guide.md`
- `tmp/swbt_agent_skill_adoption_policy.md`
- `work-units/complete/local_033/WORK_UNIT_SPEC_TDD_FLOW.md`
- `work-units/complete/local_034/WORK_UNIT_RECORD_SECTION_GUIDE.md`

## 6. 根拠監査

not applicable。

この work unit は project-local skill と operations policy の責務分割を扱う。Switch HID report bytes、BTstack source selection、report period、subcommand、SPI、rumble、descriptor data、WinUSB/libusb の実装値は追加または変更しない。

## 7. 設計メモ

再実装では、`tdd-workflow` を TDD 全体の入口にし、責務を分割 skill へ逃がす。これは、各 skill が単独で読まれたときに必要な入力、手順、出力、品質 gate を持てるようにするためである。

特に次の境界を分けて考える。

- `tdd-test-list`: source / use case から観測可能な test item を作る責務。
- `tdd-one-cycle`: TDD Test List の 1 item を red / green / refactor で進める責務。
- `tidy-first`: behavior change と structure change を分け、構造変更のタイミングを判断する責務。
- `test-desiderata-review`: test の価値、脆さ、trade-off、実機 / characterization の扱いを点検する責務。

`ponkan-python` 固有の Python / `uv` / pytest / package release / N3DSXL / cc3dsfs 前提は持ち込まない。swbt では CMake / CTest / `just`、work unit record、根拠監査、Nintendo Switch / Bluetooth 実機境界へ置き換える。

## 8. 対象ファイル

- `work-units/complete/local_035/TDD_WORKFLOW_SKILL_GROUP_REIMPLEMENTATION.md`
- `AGENTS.md`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/tdd-workflow/agents/openai.yaml`
- `.agents/skills/tdd-test-list/SKILL.md`
- `.agents/skills/tdd-test-list/agents/openai.yaml`
- `.agents/skills/tdd-one-cycle/SKILL.md`
- `.agents/skills/tdd-one-cycle/agents/openai.yaml`
- `.agents/skills/tidy-first/SKILL.md`
- `.agents/skills/tidy-first/agents/openai.yaml`
- `.agents/skills/test-desiderata-review/SKILL.md`
- `.agents/skills/test-desiderata-review/agents/openai.yaml`
- `.agents/skills/work-unit-record/SKILL.md`
- `.agents/skills/work-unit-record/references/section-guide.md`
- `spec/operations/work-unit-spec-tdd-flow.md`
- `tmp/swbt_agent_skill_adoption_policy.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | tmp adoption memo is used as source for the original TDD skill group reimplementation scope | characterization | docs | no |
| refactor-done | current tdd-workflow responsibilities are mapped to the originally referenced tdd-workflow, tdd-test-list, tdd-one-cycle, tidy-first, and test-desiderata-review skills | characterization | docs | no |
| refactor-done | tdd-workflow becomes an orchestration entrypoint and delegates to split skills | new | docs | no |
| refactor-done | split skills define source/input, process, output, validation or quality gate with swbt-specific replacements | new | docs | no |
| refactor-done | proposed skill boundaries preserve the source -> use case -> work unit -> TDD Test List -> TDD cycle order | regression | docs | no |
| refactor-done | related skill guidance remains consistent with work-unit-record section requirements | regression | docs | no |
| refactor-done | updated project-local skills pass skill validation | verification | workflow | no |

TDD status:

- source: ユーザ補足、`tmp/swbt_agent_skill_adoption_policy.md`、`local_034` の先送り事項、`spec/operations/work-unit-spec-tdd-flow.md` の未解決事項。
- use case: agent が元々参照していた TDD skill 群を swbt 向けに再実装する範囲を判断し、`tdd-workflow` の責務を誤用しない粒度へ分けるか判断し、分割または統合維持の境界と検証を記録する。
- item: split skills define source/input, process, output, validation or quality gate with swbt-specific replacements。
- state: refactor-done。
- commands:
  - `rg -n "test-desiderata|desiderata|tidy-first|tdd-test-list|tdd-one-cycle|ponkan|uv|pytest|Python|cc3dsfs|new 3DS" .agents/skills/tdd-workflow/SKILL.md .agents/skills/tdd-workflow/agents/openai.yaml spec/operations/work-unit-spec-tdd-flow.md`
  - `rg -n "TDD Test List 作成|1 cycle|tidy-first|desiderata review|Test Desiderata" .agents/skills/tdd-workflow/SKILL.md`
  - `rg -n "../tdd-test-list/SKILL.md|../tdd-one-cycle/SKILL.md|../tidy-first/SKILL.md|../test-desiderata-review/SKILL.md" .agents/skills/tdd-workflow/SKILL.md`
  - `rg -n "name: (tdd-test-list|tdd-one-cycle|tidy-first|test-desiderata-review)" .agents/skills/tdd-test-list/SKILL.md .agents/skills/tdd-one-cycle/SKILL.md .agents/skills/tidy-first/SKILL.md .agents/skills/test-desiderata-review/SKILL.md`
  - `rg -n "分割するかどうかは未決定|分割する場合の候補" spec/operations/work-unit-spec-tdd-flow.md`
  - `python3 /home/train/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/tdd-workflow`
  - `python3 /home/train/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/tdd-test-list`
  - `python3 /home/train/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/tdd-one-cycle`
  - `python3 /home/train/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/tidy-first`
  - `python3 /home/train/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/test-desiderata-review`
- notes: red では現行 `tdd-workflow` が分割 skill や `test-desiderata-review` を実質的に参照していないことを確認した。green / refactor では 4 つの分割 skill を追加し、`tdd-workflow` を入口に戻し、`ponkan-python` 固有語を swbt 固有の CMake / CTest / `just` / work unit record / 根拠監査 / 実機境界へ置き換えた。

## 10. 検証

- GitHub connector: `niart120/ponkan-python` の TDD 系 skill 5 件を確認した。
- `rg -n "test-desiderata|desiderata|tidy-first|tdd-test-list|tdd-one-cycle|ponkan|uv|pytest|Python|cc3dsfs|new 3DS" .agents/skills/tdd-workflow/SKILL.md .agents/skills/tdd-workflow/agents/openai.yaml spec/operations/work-unit-spec-tdd-flow.md`: red。旧状態では分割 skill への実質参照が `spec/operations/work-unit-spec-tdd-flow.md` の未解決事項だけだった。
- `rg -n "TDD Test List 作成|1 cycle|tidy-first|desiderata review|Test Desiderata" .agents/skills/tdd-workflow/SKILL.md`: red。旧状態では分割責務の境界が入口 skill に明示されていなかった。
- `rg -n "../tdd-test-list/SKILL.md|../tdd-one-cycle/SKILL.md|../tidy-first/SKILL.md|../test-desiderata-review/SKILL.md" .agents/skills/tdd-workflow/SKILL.md`: pass。
- `rg -n "name: (tdd-test-list|tdd-one-cycle|tidy-first|test-desiderata-review)" .agents/skills/tdd-test-list/SKILL.md .agents/skills/tdd-one-cycle/SKILL.md .agents/skills/tidy-first/SKILL.md .agents/skills/test-desiderata-review/SKILL.md`: pass。
- `rg -n "分割するかどうかは未決定|分割する場合の候補" spec/operations/work-unit-spec-tdd-flow.md`: pass。該当なし。
- `python3 /home/train/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/tdd-workflow`: pass。
- `python3 /home/train/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/tdd-test-list`: pass。
- `python3 /home/train/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/tdd-one-cycle`: pass。
- `python3 /home/train/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/tidy-first`: pass。
- `python3 /home/train/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/test-desiderata-review`: pass。
- `wc -l .agents/skills/tdd-workflow/SKILL.md .agents/skills/tdd-test-list/SKILL.md .agents/skills/tdd-one-cycle/SKILL.md .agents/skills/tidy-first/SKILL.md .agents/skills/test-desiderata-review/SKILL.md`: 73、73、86、64、72 行。ponkan-python の短く焦点を絞った記述密度に合わせつつ、swbt 固有の判断例と output / quality gate を追加した。

## 11. 実機実行条件

実機検証は不要。

この work unit は project-local skill と operations policy の整理だけを扱い、Bluetooth adapter、Switch pairing、HID advertising、report loop を実行しない。

## 12. 先送り事項

none。

`spec/operations/work-unit-spec-tdd-flow.md` に残っていた `tdd-workflow` 分割未決定事項は、この work unit で解消した。既存 wip work unit record へ source と use case を一括追記するかどうかは同 spec の別未解決事項として残っているが、この work unit の対象外である。

## 13. チェックリスト

- [x] `tdd-workflow` skill の現行責務を確認した。
- [x] `tmp/swbt_agent_skill_adoption_policy.md` の TDD 系 skill 導入方針を確認した。
- [x] 元々参照していた TDD skill 群と現行 `tdd-workflow` の対応を整理した。
- [x] 分割候補ごとの入力、出力、呼び出し条件を整理した。
- [x] 分割する判断を記録した。
- [x] `work-unit-record` skill と operations spec との整合性を確認した。
- [x] 必要な skill と spec 更新を実施した。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] 検証コマンドと結果を記録した。
