---
name: swbt-spec-format
description: "spec/initial、spec/wip、spec/complete、spec/dev-journal に置く swbt-daemon work-unit 仕様を作成、更新、review、完了する。Codex が spec 作成、feature 設計、local work unit への分割、complete への移動、TDD list 記録、hardware gate 定義、安定した tmp note の恒久文書化を求められたときに使う。"
---

# swbt spec format（仕様書式）

swbt の work-unit 仕様を作成または更新するときに、この skill を使う。

## 配置先

| 目的 | path |
|---|---|
| 初期方針と長期設計文脈 | `spec/initial/` |
| 作業中の work unit | `spec/wip/local_{nnn}/FEATURE_NAME.md` |
| 完了した work unit | `spec/complete/local_{nnn}/FEATURE_NAME.md` |
| 小さな観測と先送り判断 | `spec/dev-journal.md` |
| 実機観測 | `docs/hardware-test-log.md` |

`FEATURE_NAME.md` は uppercase snake case にする。

新しい work unit を作るときは、既存の `spec/wip/local_*` と `spec/complete/local_*` directory を確認し、次の番号を選ぶ。

## 必須セクション

work-unit spec には次を含める。

```markdown
# <機能名>

## 1. 概要
## 2. 対象範囲
## 3. 対象外
## 4. Source Audit（根拠監査）
## 5. 設計
## 6. 対象ファイル
## 7. TDD Test List（TDD テスト一覧）
## 8. 検証
## 9. Hardware Gate（実機ゲート）
## 10. チェックリスト
```

各セクションは簡潔に保つ。
大きな参考資料は別の spec または docs file に分ける。

## Source Audit

spec が次を含む場合は `swbt-source-audit` を使う。

- Switch HID report bytes。
- BTstack source selection。
- report period。
- subcommand、SPI、rumble、descriptor data。
- WinUSB/libusb behavior。

protocol、hardware、BTstack、backend facts に触れない作業だけ、source audit を `not applicable` としてよい。

## TDD Test List（TDD テスト一覧）

test item は behavior-focused にする。
次を含める。

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | observable behavior | new/regression/edge/characterization | unit/integration/hardware | no/yes |

status value は `todo`、`red`、`green`、`refactor-done`、`deferred` とする。

## Hardware Gate（実機ゲート）

hardware-gated work では次を明記する。

- 必要な承認。
- adapter assumptions。
- environment variables。
- expected log target。
- cleanup requirements。

non-hardware work では、hardware が不要な理由を書く。

## Work unit の完了

次が終わってからだけ、`spec/wip/local_{nnn}` を `spec/complete/local_{nnn}` へ移す。

- checklist が更新されている。
- verification command と結果が記録されている。
- source audit status が明確である。
- hardware status が明確である。
- 実装と non-goals が spec と一致している。

不確かな根拠を pass として扱わない。
