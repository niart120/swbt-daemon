---
name: swbt-source-audit
description: "swbt-daemon の BTstack、Nintendo Switch HID、Linux hid-nintendo、joycontrol、実機ログを監査する source audit ワークフロー。Switch protocol 定数、HID report、subcommand、SPI address、rumble packet、report timing、BTstack source selection、WinUSB/libusb backend の挙動など、upstream code、文書、実測値で裏付けるべき値を追加または変更するときに使う。"
---

# swbt source audit（根拠監査）

Switch HID、BTstack、Bluetooth adapter の挙動を `swbt-daemon` の事実として扱う前に、この skill を使う。

## 根拠の分類

根拠は次のいずれかとして記録する。

| 分類 | 意味 |
|---|---|
| `source fact` | upstream source、文書、pinned commit で直接確認した事実。 |
| `implementation fact` | 既存の swbt code または test で確認した事実。 |
| `hardware observation` | Nintendo Switch 実機または Bluetooth dongle で測定した値。 |
| `inference` | 事実から推論したが、直接検証していない内容。 |
| `unverified hypothesis` | もっともらしいが、安定した契約として実装してはいけない未検証仮説。 |

これらの分類を混同しない。
reverse engineering note の値とローカル実機で測った値は、別の根拠として扱う。

## 優先する参照元

できるだけ次の参照元を使い、可能なら path、URL、commit、line を記録する。

- 親 repository が pin している `vendor/btstack` の source と documentation。
- dekuNukem の Nintendo Switch reverse engineering notes。
- Linux `hid-nintendo.c`。
- `joycontrol` の実装メモと挙動。
- swbt の実機検証で作成した `docs/hardware-test-log.md` の記録。
- packet layout や daemon behavior を characterise するローカル swbt test。

## 監査が必要な変更

次を追加または変更する前に監査する。

- HID descriptor bytes。
- input report ID、output report ID、report packing。
- subcommand ID と response payload。
- SPI flash address と返却 data。
- rumble packet layout。
- report period の default と fallback value。
- BTstack source file list と port selection。
- WinUSB/libusb backend の仮定。
- daemon、BTstack、Switch protocol の境界をまたぐ magic number。

## 記録ルール

監査した値ごとに次を記録する。

- 値と意味。
- 根拠の分類。
- source path または URL。
- source commit、version、tag。
- line number。取得できる場合だけでよい。
- その値が stable、configurable、hardware-observed only のどれか。
- 根拠が不足している場合の follow-up。

大きな判断は `spec/wip/local_{nnn}/FEATURE_NAME.md` の work-unit spec に記録する。
小さな観測や先送り事項は `spec/dev-journal.md` に記録する。
実機測定値は `docs/hardware-test-log.md` に記録する。

## 安全ルール

- work unit が submodule fork または upstream patch の必要性を明示的に判断していない限り、`vendor/btstack` を直接変更しない。
- source または characterization test がない Switch protocol constant を実装に埋め込まない。
- 測定根拠と fallback behavior の記録がない限り、report rate を固定の実機事実として扱わない。
- OS、driver、dongle、Switch firmware、BTstack commit が異なる実機観測を、差分の記録なしに混ぜない。

## 出力

監査の最後には次の形式を置く。

```markdown
### Source Audit（根拠監査）

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|

### 未解決事項

- ...
```
