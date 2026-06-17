---
name: spec-format
description: "spec/initial、spec/wip、spec/complete、spec/dev-journal に置く swbt-daemon spec を作成、更新、レビュー、完了する。Codex が spec 作成、feature 設計、local 作業単位への分割、complete への移動、TDD list 記録、実機実行条件の定義、安定した tmp note の恒久文書化を求められたときに使う。"
---

# 仕様書式

spec を作成または更新するときに、この skill を使う。

## 配置先

| 目的 | path |
|---|---|
| 初期方針と長期設計文脈 | `spec/initial/` |
| 作業中の作業単位 | `spec/wip/local_{nnn}/FEATURE_NAME.md` |
| 完了した作業単位 | `spec/complete/local_{nnn}/FEATURE_NAME.md` |
| 小さな観測と先送り判断 | `spec/dev-journal.md` |
| 実機観測 | `docs/hardware-test-log.md` |

`FEATURE_NAME.md` は UPPER_SNAKE_CASE にする。

新しい作業単位を作るときは、既存の `spec/wip/local_*` と `spec/complete/local_*` ディレクトリを確認し、次の番号を選ぶ。

## 必須セクション

spec には次を含める。

```markdown
# <機能名>

## 1. 概要
## 2. 対象範囲
## 3. 対象外
## 4. 根拠監査
## 5. 設計
## 6. 対象ファイル
## 7. TDD Test List（TDD テスト一覧）
## 8. 検証
## 9. 実機実行条件
## 10. チェックリスト
```

各セクションは簡潔に保つ。
大きな参考資料は別の仕様または docs file に分ける。

## 根拠監査

仕様が次を含む場合は `source-audit` を使う。

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

## 作業単位の完了

次が終わってからだけ、`spec/wip/local_{nnn}` を `spec/complete/local_{nnn}` へ移す。

- チェックリストが更新されている。
- 検証コマンドと結果が記録されている。
- 根拠監査の状態が明確である。
- 実機状態が明確である。
- 実装と non-goals が spec と一致している。

不確かな根拠を pass として扱わない。
