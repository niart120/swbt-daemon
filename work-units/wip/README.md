# Work Unit Records

作業中の work unit record は次の形式で作る。

```text
work-units/wip/local_{nnn}/FEATURE_NAME.md
```

`FEATURE_NAME.md` は UPPER_SNAKE_CASE にする。
work unit record は spec ではない。
関連する spec や docs を参照し、範囲、検証、実機状態、根拠監査状態、チェックリストを記録する。

完了条件の根拠を更新してからだけ、work unit record を `work-units/complete/local_{nnn}/` に移す。
