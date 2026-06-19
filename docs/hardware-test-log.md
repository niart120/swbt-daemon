# 実機テストログ

手動の実機検証結果をここに記録する。

## テンプレート

```markdown
## YYYY-MM-DD: <短い題名>

- OS:
- environment:
- dongle:
- USB VID/PID:
- driver:
- backend:
- BTstack:
- swbt:
- Switch firmware:
- approval scope:
- environment variables:
- report period:
- command / procedure:
- result:
- cleanup:
- notes:
```

Windows native 実機検証は `spec/operations/windows-native-preflight.md` の gate を満たしてから記録する。report period comparison を行う場合は、各 period を別の記録に分ける。
