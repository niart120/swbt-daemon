---
name: work-unit-record
description: "swbt-daemon の work unit record を作成、更新、完了する。Codex が work unit の範囲定義、関連 spec/docs の参照、TDD list、根拠監査状態、検証結果、実機実行条件、チェックリストを work-units/wip または work-units/complete に記録するときに使う。"
---

# Work Unit Record

work unit record を作成または更新するときに、この skill を使う。

work unit record は、1 つの work unit の管理記録である。
spec そのものではない。
安定した設計、protocol、挙動、方針は関連 spec または docs に分け、work unit record から参照する。

## 配置先

| 目的 | path |
|---|---|
| 作業中の work unit record | `work-units/wip/local_{nnn}/FEATURE_NAME.md` |
| 完了した work unit record | `work-units/complete/local_{nnn}/FEATURE_NAME.md` |
| 安定した spec | `spec/` |
| 小さな観測と先送り判断 | `spec/dev-journal.md` |
| 実機観測 | `docs/hardware-test-log.md` |

`FEATURE_NAME.md` は UPPER_SNAKE_CASE にする。

新しい work unit record を作るときは、既存の `work-units/wip/local_*` と `work-units/complete/local_*` ディレクトリを確認し、次の番号を選ぶ。

## 必須セクション

work unit record には次を含める。

```markdown
# <機能名>

## 1. 概要
## 2. 対象範囲
## 3. 対象外
## 4. 関連 spec / docs
## 5. 根拠監査
## 6. 設計メモ
## 7. 対象ファイル
## 8. TDD Test List（TDD テスト一覧）
## 9. 検証
## 10. 実機実行条件
## 11. チェックリスト
```

各セクションは簡潔に保つ。
大きな参考資料は別の spec または docs file に分ける。

## 根拠監査

work unit が次を含む場合は `source-audit` を使う。

- Switch HID report bytes。
- BTstack source selection。
- report period。
- subcommand、SPI、rumble、descriptor data。
- WinUSB/libusb の挙動。

protocol、実機、BTstack、backend facts に触れない作業だけ、根拠監査を `not applicable` としてよい。

## TDD Test List（TDD テスト一覧）

テスト項目は挙動に着目して書く。
次を含める。

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | observable behavior | new/regression/edge/characterization | unit/integration/hardware | no/yes |

状態値は `todo`、`red`、`green`、`refactor-done`、`deferred` とする。

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

不確かな根拠を pass として扱わない。
