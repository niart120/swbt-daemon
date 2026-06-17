---
name: swbt-dev-journal
description: "swbt-daemon の設計観測、未解決事項、先送りタスク、source audit note、report-rate finding、BTstack integration observation、実機以外の bring-up 判断を spec/dev-journal.md に記録する。Codex が memo、journal、follow-up の記録、先送り判断の記録、work-unit spec にするほどではない小さな観測の保存を求められたときに使う。"
---

# swbt dev journal（開発ジャーナル）

`spec/dev-journal.md` に簡潔な設計観測を追記するときに、この skill を使う。

## ファイル

`spec/dev-journal.md` に書く。
file が存在しない場合は次を作る。

```markdown
# 開発ジャーナル

swbt-daemon の設計観測、未解決事項、先送り判断の記録。
```

## 記録形式

末尾に entry を追記する。

```markdown
## YYYY-MM-DD: <題名>

### 現状

<現時点で成り立っていること>

### 観測

<わかったこと、または未解決のこと>

### 判断

<判断、workaround、再検討条件>
```

entry は事実ベースにする。
根拠の説明に必要な場合を除き、12行以内を目安にする。

## 記録する内容

- report period の疑問。
- BTstack source selection の観測。
- WinUSB/libusb backend の違い。
- daemon IPC 境界の判断。
- work unit にするほどではない source audit の疑問。
- 後で spec 化すべき deferred cleanup。

## 記録しない内容

- hardware run result。`docs/hardware-test-log.md` を使う。
- implementation task checklist。work-unit spec を使う。
- PR status。PR body または self-review report を使う。
- 不確実性の表示がない推測。

## 昇格

entry が implementation work になった場合は、`swbt-spec-format` を使って `spec/wip/local_{nnn}/FEATURE_NAME.md` を作り、journal entry を参照する。
