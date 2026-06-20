# TDD Refactor Guidance Skill

## 1. 概要

TDD one cycle の refactor phase が formatter / linter 実行だけで終わらないよう、green 後の構造変更を扱う project-local skill を追加した work unit。

完了後は、`tdd-one-cycle` が `refactor-after-green` へ導線を持ち、work unit record の状態値として `refactor-skipped` を使える。

## 2. 起点 / ユースケース

source:

- ユーザ要求: `TDD One Cycle` の refactor 記述が薄く、linter / formatter で終わりそうなので、refactor 用 skill または reference を追加したい。
- ユーザ合意: `refactor-skipped` の状態追加にも賛成。

use case:

- actor: swbt-daemon の TDD を進める Codex agent。
- 入力 / 状態: TDD Test List の 1 item が green になっている。
- 期待する観測結果: agent が green 後に重複、命名、責務、test の読みやすさ、次 item への効きを確認し、構造変更を行うか、行わないかを記録できる。
- 制約: Switch protocol、BTstack、実機未検証値は refactor として扱わない。
- 対象外: C code の実装変更、実機検証、BTstack source selection の変更。

## 3. 対象範囲

- `refactor-after-green` skill を追加する。
- swbt 固有の refactoring guidance を reference として追加する。
- `tdd-one-cycle` から `refactor-after-green` へ導線を張る。
- `refactor-skipped` を TDD status に追加する。
- `tdd-workflow`、`tdd-test-list`、`work-unit-record`、`AGENTS.md` の skill / status 記述を更新する。
- `spec/operations/work-unit-spec-tdd-flow.md` を `refactor-after-green` 追加後の skill split に合わせる。
- skill validation と docs / skill guidance 向けの契約検索を実行する。

## 4. 対象外

- C source の refactor 実施。
- test-desiderata の内容変更。
- `tidy-first` の判断モデル変更。
- 実機コマンド、pairing、HID advertising、report loop。

## 5. 関連 spec / docs

- `.agents/skills/tdd-one-cycle/SKILL.md`
- `.agents/skills/refactor-after-green/SKILL.md`
- `.agents/skills/refactor-after-green/references/swbt-refactoring-guidance.md`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/work-unit-record/SKILL.md`
- `AGENTS.md`

## 6. 根拠監査

not applicable。

この work unit は TDD workflow と project-local skill guidance だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の値を追加または変更しない。

## 7. 設計メモ

`tidy-first` は behavior change と structure change の分類、および tidy first / tidy after / tidy later / do not tidy の判断に残す。

`refactor-after-green` は green 後に実際の構造変更を行うか、`refactor-skipped` として記録するかを扱う。詳細な例は reference に分け、`SKILL.md` は短い手順にした。

formatter、linter、include 並べ替えだけでは `refactor-done` にしない。構造変更が不要、または行うべきでない場合は `refactor-skipped` と記録する。

## 8. 対象ファイル

- `.agents/skills/refactor-after-green/SKILL.md`
- `.agents/skills/refactor-after-green/references/swbt-refactoring-guidance.md`
- `.agents/skills/refactor-after-green/agents/openai.yaml`
- `.agents/skills/tdd-test-list/SKILL.md`
- `.agents/skills/tdd-one-cycle/SKILL.md`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/work-unit-record/SKILL.md`
- `spec/operations/work-unit-spec-tdd-flow.md`
- `AGENTS.md`
- `work-units/complete/local_040/TDD_REFACTOR_GUIDANCE_SKILL.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | `refactor-after-green` skill validates as a project-local skill | regression | workflow | no |
| green | `tdd-one-cycle` routes green-after structure changes to `refactor-after-green` | regression | docs | no |
| green | TDD status lists accept `refactor-skipped` consistently | regression | docs | no |
| green | `work-unit-spec-tdd-flow` lists `refactor-after-green` in the TDD skill split | regression | spec | no |

TDD status:

- source: ユーザ要求と `refactor-skipped` 追加合意。
- use case: green 後の構造変更判断を formatter / linter 実行だけにしない。
- item: `refactor-after-green` skill validates as a project-local skill。
- state: green。
- commands: `uv run --with pyyaml python -X utf8 C:\Users\train\.codex\skills\.system\skill-creator\scripts\quick_validate.py .agents\skills\refactor-after-green`。
- notes: read `skill-creator`, `spec-page`, `work-unit-record`。`tdd-one-cycle` から `refactor-after-green` へ導線を張り、`refactor-skipped` を状態値として追加した。follow-up で `tdd-test-list` と `work-unit-spec-tdd-flow` の反映漏れを修正した。

## 10. 検証

commands:

- `uv run --with pyyaml python -X utf8 C:\Users\train\.codex\skills\.system\skill-creator\scripts\quick_validate.py .agents\skills\refactor-after-green`: pass, `Skill is valid!`。
- `uv run --with pyyaml python -X utf8 C:\Users\train\.codex\skills\.system\skill-creator\scripts\quick_validate.py .agents\skills\tdd-one-cycle`: pass, `Skill is valid!`。
- `uv run --with pyyaml python -X utf8 C:\Users\train\.codex\skills\.system\skill-creator\scripts\quick_validate.py .agents\skills\tdd-test-list`: pass, `Skill is valid!`。
- `uv run --with pyyaml python -X utf8 C:\Users\train\.codex\skills\.system\skill-creator\scripts\quick_validate.py .agents\skills\tdd-workflow`: pass, `Skill is valid!`。
- `uv run --with pyyaml python -X utf8 C:\Users\train\.codex\skills\.system\skill-creator\scripts\quick_validate.py .agents\skills\work-unit-record`: pass, `Skill is valid!`。
- `rg -n "refactor-skipped|refactor-after-green" .agents\skills AGENTS.md work-units\complete\local_040`: pass, expected references were present。
- `rg -n "state: red \| green \| refactor-done \| deferred|todo`、`red`、`green`、`refactor-done`、`deferred" .agents AGENTS.md`: pass after follow-up, no old status-only list remained in active skill docs。
- `rg -n "tdd-test-list.*tdd-one-cycle.*tidy-first|one cycle -> tidy|分割 skill|refactor-after-green" spec\operations\work-unit-spec-tdd-flow.md .agents\skills`: pass, operations spec and skill docs refer to `refactor-after-green` consistently。
- placeholder-pattern search over the new skill, modified skill docs, `AGENTS.md`, and this work unit record: pass, no placeholder text remained。
- `git diff --check`: pass. Git reported Windows checkout line-ending warnings for existing tracked Markdown files, but no whitespace error。

## 11. 実機実行条件

実機検証は不要。

この work unit は project-local skill guidance だけを変更し、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を含まない。

## 12. 先送り事項

none。

## 13. チェックリスト

- [x] 新規 skill を追加した。
- [x] `tdd-one-cycle` に導線と状態値を反映した。
- [x] `tdd-test-list` と operations spec の反映漏れを修正した。
- [x] 関連する入口文書を更新した。
- [x] validation を実行した。
- [x] 検証結果と実機未実行理由を記録した。
