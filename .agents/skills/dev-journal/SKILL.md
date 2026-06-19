---
name: dev-journal
description: "swbt-daemon の設計観測、未解決事項、先送りタスク、根拠監査メモ、report-rate finding、BTstack integration observation、実機以外の bring-up 判断を spec/dev-journal.md に記録し、後続 work unit / spec の source として扱う。Codex が memo、journal、follow-up の記録、先送り判断の記録、work unit record や spec にするほどではない小さな観測の保存、または journal entry から work unit / spec への昇格判断を求められたときに使う。"
---

# 開発ジャーナル

`spec/dev-journal.md` に簡潔な設計観測を追記するときに、この skill を使う。
journal entry は、後続 work unit または spec の source になり得る。work unit、spec、TDD Test List の作成順序は `spec/operations/work-unit-spec-tdd-flow.md` に従う。

## ファイル

`spec/dev-journal.md` に書く。
ファイルが存在しない場合は次を作る。

```markdown
# 開発ジャーナル

swbt-daemon の設計観測、未解決事項、先送り判断の記録。
```

## 記録形式

末尾に記録を追記する。

```markdown
## YYYY-MM-DD: <題名>

### 現状

<現時点で成り立っていること>

### 観測

<わかったこと、または未解決のこと>

### 判断

<判断、workaround、再検討条件>
```

記録は事実ベースにする。
根拠の説明に必要な場合を除き、12行以内を目安にする。

## 記録する内容

- report period の疑問。
- BTstack source selection の観測。
- WinUSB/libusb バックエンドの違い。
- daemon IPC 境界の判断。
- work unit record にするほどではない根拠監査の疑問。
- 後で spec page に昇格すべき deferred cleanup。

## 記録しない内容

- 実機実行結果。`docs/hardware-test-log.md` を使う。
- 実装タスクのチェックリスト。work unit record を使う。
- PR 状態。PR body またはセルフRvレポートを使う。
- 不確実性の表示がない推測。

## 昇格

記録が実装作業になった場合は、journal entry を source として扱い、use case または観測したい振る舞いへ変換してから `work-unit-record` を使う。作成する work unit record には、参照元の journal entry と、source から use case へ変換した判断を残す。

複数の work unit から参照される判断になった場合は、`spec-page` を使って spec page を作る。作成する spec page には、参照元の journal entry を根拠として残す。
