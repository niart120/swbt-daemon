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
- IPC endpoint:
- report period:
- command / procedure:
- result:
- daemon log:
- artifact root:
- cleanup:
- notes:
```

Windows native 実機検証は `spec/operations/windows-native-preflight.md` の gate を満たしてから記録する。report period comparison を行う場合は、各 period を別の記録に分ける。

NyX `swbt_hardware_bringup` macro を使う場合は、`artifact root` に `run_context.json`、`ipc_session.json`、`hardware_log_draft.md`、capture path を辿れる場所を記録する。daemon log は swbt-daemon 側で保存し、NyX の request id と時刻で対応付ける。
