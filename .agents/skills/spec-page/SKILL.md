---
name: spec-page
description: "swbt-daemon の安定した spec page を作成、更新、整理する。Codex が複数の work unit record から参照され得る architecture、protocol、operations の spec を書く、source / use case / work unit との関係を整理する、references を根拠として整理する、置き換え済み spec を archive へ移す、または spec と work unit record のリンクを整えるときに使う。"
---

# Spec Page

安定した spec page を作成または更新するときに、この skill を使う。

通常は work unit を先に立てる。複数の work unit から参照される設計、protocol、運用判断が必要になった場合だけ、spec page を作成または更新する。

spec page は work queue ではない。source、use case、TDD Test List、検証結果、完了チェックリストは work unit record に置く。work unit、spec、TDD Test List の作成順序は `spec/operations/work-unit-spec-tdd-flow.md` に従う。

## 配置先

| 目的 | path |
|---|---|
| architecture spec | `spec/architecture/` |
| protocol spec | `spec/protocols/` |
| operations spec | `spec/operations/` |
| 根拠と外部資料 | `spec/references/` |
| 置き換え済み spec | `spec/archive/` |
| 初期方針と設計背景 | `spec/initial/` |
| 未確定の観測 | `spec/dev-journal.md` |

`spec/references/` は規範ではない。実装が従う判断は `architecture`、`protocols`、`operations` のいずれかに置く。

## 作成判断

次の場合は spec page を作る。

- 複数の work unit record から参照される設計判断である。
- protocol、packet layout、IPC contract、daemon boundary、BTstack boundary を安定した判断として残す。
- 実機検証や WinUSB 運用の手順を繰り返し使う。
- source、use case、work unit、TDD Test List の関係のような、複数 work unit に効く運用方針である。
- journal entry や tmp note が、今後の work unit の前提になった。

次の場合は spec page にしない。

- 単発作業の範囲、検証結果、チェックリスト。work unit record に書く。
- source からまだ use case を取り出していない作業候補。work unit record または journal entry で先に整理する。
- TDD Test List の item 群。work unit record に書く。
- 未確定の観測や先送り判断。`spec/dev-journal.md` に書く。
- 外部資料の要約だけ。`spec/references/` に書く。

## 必須セクション

spec page には次を含める。

```markdown
# <spec 名>

## 1. 状態
## 2. 目的
## 3. 適用範囲
## 4. 決定事項
## 5. 根拠
## 6. 関連 work units
## 7. 未解決事項
```

`状態` には `current`、`draft`、`superseded` のいずれかを書く。
`superseded` の spec は `spec/archive/` に移す。

## 根拠監査

spec page が次を含む場合は `source-audit` を使う。

- Switch HID report bytes。
- BTstack source selection。
- report period。
- subcommand、SPI、rumble、descriptor data。
- WinUSB/libusb の挙動。

根拠は `spec/references/`、source path、URL、commit、実機ログのいずれかへ辿れるようにする。

## リンク

spec page と work unit record は相互に辿れるようにする。spec が journal entry から昇格した場合は journal entry を根拠として残す。spec が work unit を生む場合でも、work unit record 側には source と use case を明記する。

work unit record には次を置く。

```markdown
## 関連 spec / docs
- spec/...
```

spec page には次を置く。

```markdown
## 関連 work units
- work-units/...
```
