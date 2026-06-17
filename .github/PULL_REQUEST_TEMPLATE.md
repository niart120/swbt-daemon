## 概要

<!-- この変更の目的を1から3行で書く。 -->

## 関連

- 仕様:
- 開発ジャーナル:
- Issue:
- 根拠監査:

## 変更内容

-

## 根拠監査

<!-- Switch HID protocol、BTstack source selection、report timing、バックエンドの挙動、実機向け定数を変更する場合は必須。理由がある場合だけ "not applicable" を使う。 -->

| 項目 | 状態 | 根拠 |
|---|---|---|
| Switch HID / protocol facts |  |  |
| BTstack source / bridge への影響 |  |  |
| WinUSB / libusb バックエンドの仮定 |  |  |

## テスト

```text
# コマンドと結果
```

## 実機

- [ ] not applicable
- [ ] not run
- [ ] Nintendo Switch hardware で検証済み

理由または根拠:

- 承認範囲:
- OS / environment:
- Bluetooth ドングル:
- ドライバー:
- バックエンド:
- Switch firmware:
- report period:
- ログパス:
- cleanup:

## BTstack / License 影響

- [ ] `vendor/btstack` は未変更
- [ ] BTstack source selection を変更
- [ ] `THIRD_PARTY_NOTICES.md` を確認済み
- [ ] license / notice 影響は not applicable

メモ:

## チェックリスト

- [ ] 変更範囲が spec または request と一致している
- [ ] Non-goals が scope 外に残っている
- [ ] CMake configure/build/test の実行結果または未実行理由を記録した
- [ ] sanitizer/cross-build の実行結果または未実行理由を記録した
- [ ] 根拠監査の状態を記録した
- [ ] 実機状態を記録した
- [ ] commit prefix が変更理由と一致している
