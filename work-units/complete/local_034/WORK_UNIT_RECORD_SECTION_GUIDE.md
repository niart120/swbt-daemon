# Work Unit Record Section Guide

## 1. 概要

`work-unit-record` skill に、必須セクションの記載粒度と先送り事項の扱いを追加した work unit。

SKILL.md には必須構造と境界を残し、各セクションに何を書くかは `references/section-guide.md` に分けた。`deferred item` は後続 work unit の source になり得るため、work unit record の `先送り事項` として独立させた。

## 2. 起点 / ユースケース

source:

- ユーザ指摘。`work-unit-record` には deferred item、つまり先送り事項の欄が必要ではないかという判断。
- ユーザ指摘。必須セクションの説明を拡充したいが、SKILL.md に書きすぎると記載粒度が定まらないという懸念。
- 既存状態。`work-unit-record` skill は必須セクション名を列挙していたが、各欄の粒度を十分に説明していなかった。
- 既存状態。TDD Test List には `deferred` status があるが、それに対応する先送り事項の記録先が独立していなかった。

use case:

- agent が work unit record を作るとき、各セクションに何を書くかを `references/section-guide.md` で確認できる。
- agent が作業中に follow-up を見つけたとき、`対象外` と混ぜず `先送り事項` に観測、先送り理由、次の置き場を残せる。
- agent が TDD Test List の item を `deferred` にするとき、対応する先送り事項または不要とした理由へ辿れる。

## 3. 対象範囲

- `.agents/skills/work-unit-record/SKILL.md` に `先送り事項` を必須セクションとして追加する。
- `.agents/skills/work-unit-record/references/section-guide.md` を追加する。
- `.agents/skills/work-unit-record/agents/openai.yaml` を更新する。
- `spec/operations/work-unit-spec-tdd-flow.md` に先送り事項と後続 source の関係を追記する。

## 4. 対象外

- 既存 work unit record の一括移行。
- `tdd-workflow` skill の分割。
- protocol、BTstack、WinUSB、実機検証に関する値の変更。

## 5. 関連 spec / docs

- `spec/operations/work-unit-spec-tdd-flow.md`
- `.agents/skills/work-unit-record/SKILL.md`
- `.agents/skills/work-unit-record/references/section-guide.md`

## 6. 根拠監査

not applicable。

この work unit は operations policy と project skill guidance だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は追加しない。

## 7. 設計メモ

`対象外` と `先送り事項` は分ける。`対象外` は開始時点の境界であり、失敗や未完了ではない。`先送り事項` は作業中に見つかった follow-up であり、後続 work unit の source にできる粒度で残す。

SKILL.md は常に読むため、必須セクションと完了境界だけを置く。各セクションの記載粒度は reference に分け、work unit record の新規作成や大きな更新時に読む形にする。

## 8. 対象ファイル

- `.agents/skills/work-unit-record/SKILL.md`
- `.agents/skills/work-unit-record/agents/openai.yaml`
- `.agents/skills/work-unit-record/references/section-guide.md`
- `spec/operations/work-unit-spec-tdd-flow.md`
- `work-units/complete/local_034/WORK_UNIT_RECORD_SECTION_GUIDE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | work-unit-record skill includes `先送り事項` as a required section | regression | docs | no |
| refactor-done | section guide defines recording granularity for every required section | new | docs | no |
| refactor-done | operations spec states that deferred items can become later sources | regression | docs | no |
| refactor-done | skill validation passes for updated work-unit-record skill | verification | workflow | no |

TDD status:

- source: user指摘と既存 work-unit-record skill の粒度不足。
- use case: agent が work unit record の各欄を一定の粒度で記録し、先送り事項を後続 source として残す。
- item: work-unit-record skill includes `先送り事項` as a required section。
- state: refactor-done。
- commands:
  - `python3 /home/train/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/work-unit-record`
  - `git diff --check`
- notes: 既存 work unit record の一括移行は対象外にした。

## 10. 検証

- `python3 /home/train/.codex/skills/.system/skill-creator/scripts/quick_validate.py .agents/skills/work-unit-record`: pass。
- `git diff --check`: pass。

## 11. 実機実行条件

実機検証は不要。

この work unit は operations spec と project skill guidance だけを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 12. 先送り事項

none。

## 13. チェックリスト

- [x] `先送り事項` を必須セクションに追加した。
- [x] 各セクションの記載粒度を reference に分けた。
- [x] 先送り事項と後続 source の関係を operations spec に追記した。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] skill validation を実行した。
- [x] whitespace check を実行した。
