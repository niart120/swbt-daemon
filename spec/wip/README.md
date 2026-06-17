# 作業中の仕様

作業中の実装作業単位は次の形式で作る。

```text
spec/wip/local_{nnn}/FEATURE_NAME.md
```

`FEATURE_NAME.md` は UPPER_SNAKE_CASE にする。
要件、根拠監査、テスト、実機状態、チェックリストの根拠を更新してからだけ、完了した作業単位を `spec/complete/local_{nnn}/` に移す。
