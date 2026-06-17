# 作業中の仕様

作業中の implementation work unit は次の形式で作る。

```text
spec/wip/local_{nnn}/FEATURE_NAME.md
```

`FEATURE_NAME.md` は uppercase snake case にする。
requirements、source audit、tests、hardware status、checklist evidence を更新してからだけ、完了した work unit を `spec/complete/local_{nnn}/` に移す。
