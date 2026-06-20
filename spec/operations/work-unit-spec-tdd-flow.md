# Work Unit, Spec, And TDD Flow

## 1. 状態

current。

この spec は、`work unit`、`work unit record`、`spec`、TDD Test List、先送り事項の使い分けと作成順序を定める。

## 2. 目的

`work unit` は無から作らない。ユーザ要求、roadmap TODO、journal entry、deferred item、bug、実機観測、根拠監査 finding、既存 spec の未解決事項などの source から起こす。

ただし、source をそのまま TDD Test List にしない。source はまず use case または観測したい振る舞いへ変換し、TDD Test List はその use case から生成する。

work unit 内で完了しない follow-up は、先送り事項として記録する。先送り事項は、後続 work unit を起こすときの source になり得る。

## 3. 適用範囲

この spec は、新しい work unit record の作成、既存 wip work unit record の再開、operations / architecture / protocols の spec 作成判断、TDD Test List の作成、TDD cycle の進行、先送り事項の記録、project-local skill の guidance に適用する。

次は対象外である。

- Switch HID protocol の個別 byte 値。
- BTstack source selection の個別判断。
- 実機検証手順の詳細。
- CI、formatter、task runner の個別実装。

## 4. 決定事項

**source** は、作業を起こす根拠である。ユーザ要求、`spec/initial/` の roadmap TODO、`spec/dev-journal.md` の journal entry、既存 work unit の deferred item、bug、実機観測、根拠監査 finding、既存 spec の未解決事項を source として扱う。

**use case** は、source から取り出した観測可能な振る舞いである。use case には、actor または境界、入力または状態、期待する観測結果、制約、対象外、根拠状態を含める。

**work unit** は、1 つ以上の use case を実装、検証、完了判定する作業単位である。work unit は文書そのものではない。

**work unit record** は、work unit の source、use case、範囲、関連 spec、TDD Test List、検証、実機状態、根拠監査状態、先送り事項を束ねる記録である。

**spec** は、複数の work unit から参照される安定した設計、protocol、挙動、運用方針を書く文書である。spec は work queue ではない。

**TDD Test List** は、use case から生成する観測可能な test item の一覧である。file list、実装都合、roadmap TODO だけから TDD Test List を作らない。

**先送り事項** は、作業中に見つかったがこの work unit では完了しない follow-up である。先送り事項には、観測、先送り理由、後続 source として使うための次の置き場を残す。

**対象外** は、この work unit の完了条件に含めない境界である。対象外は未完了ではない。対象外にした項目も、後続 work unit の source になり得る。

標準フローは次の順序にする。

```text
source
  -> use case / behavior brief
  -> work unit
  -> TDD Test List
  -> TDD cycle
  -> spec update when stable

work unit
  -> deferred item
  -> later source
```

roadmap TODO と journal entry は source であり、use case ではない。roadmap TODO や journal entry から work unit を作る場合も、source から use case を取り出してから work unit record と TDD Test List を作る。

既存 work unit の `対象外`、`deferred`、`未解決事項` から後続 work unit を作る場合も、その項目を use case へ変換する。spec の未解決事項から work unit を作る場合も同じ扱いにする。

spec は、安定した判断が必要になったときに作る。単発の作業範囲、検証結果、TDD status、完了チェックリストは work unit record に置き、複数 work unit が従う方針、protocol contract、daemon boundary、BTstack boundary、運用判断は spec に置く。

対象外と先送り事項を迷った場合は、record 内の役割で判断する。現在の work unit の完了条件を閉じる境界は対象外に置く。後で追跡する follow-up は先送り事項に置く。同じ内容を両方に再掲せず、対象外にした領域から後続候補を残す場合は、先送り事項側に観測、先送り理由、次の置き場を持つ別 item として書く。

TDD cycle では、TDD Test List から item を 1 つ選ぶ。Red では、選んだ item の観測結果がまだ満たされていないことを示す。Green では、選んだ item と関連する既存契約を満たす実装にする。必要なら構造を大きく変えてよいが、選んだ item 以外の新しい振る舞いを同じ cycle に混ぜない。

TDD Test List の item を `deferred` にする場合は、対応する先送り事項を work unit record に残す。対応する後続作業が不要だと判断した場合は、その理由を work unit record に残す。

Refactor では、観測可能な振る舞いを変えずに構造を変更する。behavior change と structure change は、差分、説明、検証を分ける。構造変更が不要、または行うべきでない場合は `refactor-skipped` として判断を残す。

`tdd-workflow` skill は、TDD 全体の入口とし、実作業は `tdd-test-list`、`tdd-one-cycle`、`refactor-after-green`、`tidy-first`、`test-desiderata-review` に分ける。入口 skill は source -> use case -> TDD Test List -> one cycle -> refactor-after-green / tidy / desiderata review -> work unit record update の順序を案内する。分割 skill は、それぞれの入力、出力、検証、record 更新境界を明確にする。

既存 completed work unit record は historical evidence として残す。既存 wip work unit record は、再開時に source と use case を追記する。

## 5. 根拠

`AGENTS.md` は、`work unit`、`work unit record`、`spec`、`journal entry` を別概念として定義している。`work-unit-record` skill は、work unit record を spec そのものではないと定義している。`spec-page` skill は、spec page を複数の work unit record から参照され得る安定判断として扱っている。

この spec は、その定義に TDD の入力順序を追加する operations policy である。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値を追加しないため、根拠監査は not applicable とする。

## 6. 関連 work units

- `work-units/complete/local_033/WORK_UNIT_SPEC_TDD_FLOW.md`
- `work-units/complete/local_034/WORK_UNIT_RECORD_SECTION_GUIDE.md`
- `work-units/complete/local_035/TDD_WORKFLOW_SKILL_GROUP_REIMPLEMENTATION.md`
- `work-units/complete/local_040/TDD_REFACTOR_GUIDANCE_SKILL.md`

## 7. 未解決事項

- 既存 wip work unit record へ source と use case を一括追記するか、再開時に個別更新するかは未決定である。
