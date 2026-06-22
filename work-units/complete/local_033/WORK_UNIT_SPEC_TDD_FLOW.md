# Work Unit, Spec, And TDD Flow

## 1. 概要

`work unit`、`work unit record`、`spec`、TDD Test List の作成順序を整理した work unit。

`spec/operations/work-unit-spec-tdd-flow.md` を追加し、source から use case を取り出してから work unit と TDD Test List を作る方針を記録した。関連する project skills も同じ方針へ更新した。

## 2. 起点 / ユースケース

source:

- ユーザ指摘。`TDD はユースケース無しにはできず、work unit を無から生成できないのではないか` という疑問。
- ユーザ指摘。`spec-page` skill も修正対象になるはず、という判断。
- ユーザ指摘。`dev-journal` もユースケースの source になり、skill の見直し対象になるのではないかという判断。
- 既存状態。`tdd-workflow` skill は `work unit record` の TDD Test List から始める記述になっていた。
- 既存状態。`work-unit-record` skill の必須セクションに source と use case がなかった。
- 既存状態。`dev-journal` skill は journal entry を後続 work unit / spec の source として扱うことを明記していなかった。

use case:

- agent が roadmap TODO、journal entry、deferred item、bug、実機観測、根拠監査 finding から作業を起こすとき、source を use case へ変換してから work unit record を作る。
- agent が TDD Test List を作るとき、roadmap TODO や file list ではなく use case から観測可能な test item を作る。
- agent が spec page を作るとき、spec を work queue として使わず、複数 work unit に効く安定判断だけを spec に置く。
- agent が既存 wip work unit を再開するとき、source と use case が不足していれば追記してから TDD cycle に入る。

## 3. 対象範囲

- `spec/operations/work-unit-spec-tdd-flow.md` を追加する。
- `spec/operations/README.md` と `spec/README.md` から新 spec を参照する。
- `spec-page` skill に、spec と work unit の関係、spec が work queue ではないことを記録する。
- `work-unit-record` skill に、source と use case の必須化を記録する。
- `tdd-workflow` skill に、use case から TDD Test List を作る順序を記録する。
- `dev-journal` skill に、journal entry が後続 work unit / spec の source になることを記録する。

## 4. 対象外

- `tdd-workflow` skill を複数 skill へ分割する実装。
- 既存 wip work unit record への source / use case 一括追記。
- Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の値の変更。
- 実機検証。

## 5. 関連 spec / docs

- `spec/operations/work-unit-spec-tdd-flow.md`
- `spec/operations/README.md`
- `spec/README.md`
- `.agents/skills/spec-page/SKILL.md`
- `.agents/skills/work-unit-record/SKILL.md`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/dev-journal/SKILL.md`

## 6. 根拠監査

not applicable。

この work unit は operations policy と project skill guidance を扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は追加しない。

## 7. 設計メモ

`spec/operations/` は、繰り返し使う開発運用方針を置く場所である。今回の方針は複数の work unit と skill に効くため、`spec/operations/` に置く。

`work unit record` は work unit の管理記録であり、要求そのものではない。要求、TODO、journal entry、deferred item、bug、実機観測、根拠監査 finding は source として扱う。source は use case または観測したい振る舞いへ変換してから TDD Test List にする。

既存 completed work unit record は historical evidence として残す。既存 wip work unit record は、再開時に source と use case を追記する方針にする。

## 8. 対象ファイル

- `spec/operations/work-unit-spec-tdd-flow.md`
- `spec/operations/README.md`
- `spec/README.md`
- `.agents/skills/spec-page/SKILL.md`
- `.agents/skills/spec-page/agents/openai.yaml`
- `.agents/skills/work-unit-record/SKILL.md`
- `.agents/skills/work-unit-record/agents/openai.yaml`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/tdd-workflow/agents/openai.yaml`
- `.agents/skills/dev-journal/SKILL.md`
- `.agents/skills/dev-journal/agents/openai.yaml`
- `work-units/complete/local_033/WORK_UNIT_SPEC_TDD_FLOW.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | operations spec defines source, use case, work unit, spec, and TDD Test List as separate concepts | new | docs | no |
| refactor-done | operations spec defines the order source -> use case -> work unit -> TDD Test List -> TDD cycle -> spec update | new | docs | no |
| refactor-done | spec-page skill states that spec pages are not work queues | regression | docs | no |
| refactor-done | work-unit-record skill requires source and use case before TDD Test List | regression | docs | no |
| refactor-done | tdd-workflow skill starts from source/use case instead of roadmap TODO or file list | regression | docs | no |
| refactor-done | dev-journal skill states that journal entries can become sources for later work units or specs | regression | docs | no |
| refactor-done | skill validation passes for updated skills | verification | workflow | no |

TDD status:

- source: user指摘と既存 skill guidance の不整合。
- use case: agent が source から use case を作り、そこから work unit と TDD Test List を作る。
- item: operations spec defines the order source -> use case -> work unit -> TDD Test List -> TDD cycle -> spec update。
- state: refactor-done。
- commands:
  - `skill-creator` validator for `.agents/skills/spec-page`
  - `skill-creator` validator for `.agents/skills/work-unit-record`
  - `skill-creator` validator for `.agents/skills/tdd-workflow`
  - `skill-creator` validator for `.agents/skills/dev-journal`
  - `git diff --check`
- notes: `tdd-workflow` の skill 分割は未実装であり、未解決事項として spec に残した。

## 10. 検証

- `skill-creator` validator for `.agents/skills/spec-page`: pass。
- `skill-creator` validator for `.agents/skills/work-unit-record`: pass。
- `skill-creator` validator for `.agents/skills/tdd-workflow`: pass。
- `skill-creator` validator for `.agents/skills/dev-journal`: pass。
- `git diff --check`: pass。

## 11. 実機実行条件

実機検証は不要。

この work unit は operations spec と project skill guidance だけを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 12. チェックリスト

- [x] source、use case、work unit、spec、TDD Test List の関係を operations spec に記録した。
- [x] spec と work unit の使い分けを spec-page skill に反映した。
- [x] source と use case を work-unit-record skill に反映した。
- [x] use case から TDD Test List を作る順序を tdd-workflow skill に反映した。
- [x] journal entry を source として扱う方針を dev-journal skill に反映した。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] skill validation を実行した。
- [x] whitespace check を実行した。
