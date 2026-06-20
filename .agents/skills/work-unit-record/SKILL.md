---
name: work-unit-record
description: "swbt-daemon の work unit record を作成、更新、完了する。Codex が source、use case、work unit の範囲定義、関連 spec/docs の参照、TDD list、根拠監査状態、検証結果、実機実行条件、先送り事項、チェックリストを work-units/wip または work-units/complete に記録するときに使う。source には user request、roadmap TODO、journal entry、deferred item、bug、実機観測、根拠監査 finding、既存 spec の未解決事項を含む。"
---

# Work Unit Record

work unit record を作成または更新するときに、この skill を使う。

work unit record は、1 つの work unit の管理記録である。spec そのものではない。安定した設計、protocol、挙動、方針は関連 spec または docs に分け、work unit record から参照する。

work unit は無から作らない。ユーザ要求、roadmap TODO、journal entry、deferred item、bug、実機観測、根拠監査 finding、既存 spec の未解決事項などの source を確認し、use case または観測したい振る舞いへ変換してから work unit record を作る。work unit、spec、TDD Test List、先送り事項の関係は `spec/operations/work-unit-spec-tdd-flow.md` に従う。

新しい work unit record を作る場合、または既存 record の構成や粒度を直す場合は、`references/section-guide.md` も読む。SKILL.md は必須構造と境界だけを定め、各セクションの記載粒度は reference に置く。

## 配置先

| 目的 | path |
|---|---|
| 作業中の work unit record | `work-units/wip/local_{nnn}/FEATURE_NAME.md` |
| 完了した work unit record | `work-units/complete/local_{nnn}/FEATURE_NAME.md` |
| architecture spec | `spec/architecture/` |
| protocol spec | `spec/protocols/` |
| operations spec | `spec/operations/` |
| references | `spec/references/` |
| 小さな観測と先送り判断 | `spec/dev-journal.md` |
| 実機観測 | `docs/hardware-test-log.md` |

`FEATURE_NAME.md` は UPPER_SNAKE_CASE にする。

新しい work unit record を作るときは、既存の `work-units/wip/local_*` と `work-units/complete/local_*` ディレクトリを確認し、次の番号を選ぶ。関連 spec を作成または更新する必要がある場合は `spec-page` を使う。

## 必須セクション

work unit record には次を含める。

```markdown
# <機能名>

## 1. 概要
## 2. 起点 / ユースケース
## 3. 対象範囲
## 4. 対象外
## 5. 関連 spec / docs
## 6. 根拠監査
## 7. 設計メモ
## 8. 対象ファイル
## 9. TDD Test List（TDD テスト一覧）
## 10. 検証
## 11. 実機実行条件
## 12. 先送り事項
## 13. チェックリスト
```

各セクションは、完了判定、handoff、後続 work unit の source 化に必要な粒度で書く。大きな参考資料は別の spec または docs file に分け、根拠の要約だけを残す場合は `spec/references/` を使う。該当しないセクションも削除せず、`not applicable` または `none` と理由を書く。

## 起点 / ユースケース

このセクションには次を含める。

- source。ユーザ要求、roadmap TODO、journal entry、deferred item、bug、実機観測、根拠監査 finding、既存 spec の未解決事項など。
- use case。actor または境界、入力または状態、期待する観測結果、制約、対象外。
- source から use case へ変換した判断。

roadmap TODO、journal entry、deferred item を、そのまま TDD Test List にしない。source から use case を取り出せない場合は、実装に入らず journal entry または draft spec で整理する。

## 対象範囲、対象外、先送り事項

`対象範囲` は、この work unit の完了条件に含める作業を書く。`対象外` は、最初からこの work unit で扱わない境界を書く。`先送り事項` は、作業中に見つかったがこの work unit では完了しない follow-up を書く。

対象外にした項目も、後続 work unit の source になり得る。`対象外` は現在の work unit の完了条件を閉じるために使い、`先送り事項` は後続で追跡する follow-up を残すために使う。

同じ内容を `対象外` と `先送り事項` に重複して書かない。対象外にした領域から後続候補を残す場合、先送り事項には対象外の再掲ではなく、なぜ今扱わないか、後続の source として使える観測、次の置き場を書く。候補は `work-units/wip/local_{nnn}/...`、`spec/dev-journal.md`、`spec/operations/`、`spec/architecture/`、`spec/protocols/`、`docs/` のいずれかにする。TDD Test List の `deferred` item は、対応する先送り事項か、先送り不要と判断した理由へ辿れるようにする。

## 根拠監査

work unit が次を含む場合は `source-audit` を使う。

- Switch HID report bytes。
- BTstack source selection。
- report period。
- subcommand、SPI、rumble、descriptor data。
- WinUSB/libusb の挙動。

protocol、実機、BTstack、backend facts に触れない作業だけ、根拠監査を `not applicable` としてよい。

## TDD Test List（TDD テスト一覧）

テスト項目は挙動に着目して書く。TDD Test List は、`起点 / ユースケース` の use case から生成する。
次を含める。

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | observable behavior | new/regression/edge/characterization | unit/integration/hardware | no/yes |

状態値は `todo`、`red`、`green`、`refactor-done`、`refactor-skipped`、`deferred` とする。実装都合、file list、roadmap TODO だけを test item にしない。

## 実機実行条件

実機承認を必要とする作業では次を明記する。

- 必要な承認。
- adapter assumptions。
- environment variables。
- expected log target。
- cleanup requirements。

実機が不要な作業では、その理由を書く。

## work unit の完了

次が終わってからだけ、`work-units/wip/local_{nnn}` を `work-units/complete/local_{nnn}` へ移す。

- チェックリストが更新されている。
- 検証コマンドと結果が記録されている。
- 根拠監査の状態が明確である。
- 実機状態が明確である。
- 実装、non-goals、関連 spec の関係が明確である。
- source、use case、TDD Test List の関係が明確である。
- 先送り事項が `none` か、後続 source として使える粒度で記録されている。

不確かな根拠を pass として扱わない。
