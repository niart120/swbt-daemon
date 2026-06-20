# Windows Hardware Bring-Up

## 1. 概要

Windows native + 専用 USB Bluetooth dongle + WinUSB で、production daemon が Switch と実機接続できるか確認する work unit。

この work unit は `spec/operations/windows-native-preflight.md` の gate を満たした後に実行する。Switch pairing、HID advertising、periodic report loop、debug IPC client からの入力反映、report period comparison、neutral fail-safe を `docs/hardware-test-log.md` に記録する。

NyX handoff は、Project NyX の `swbt_hardware_bringup` マクロを一時的な debug IPC client として使う実行手順である。swbt-daemon 側は daemon 起動、adapter / driver / pairing / log 記録を担当し、NyX の導入、設定更新、macro 実行は Project NyX 側で行う。

## 2. 起点 / ユースケース

source:

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md` の Phase 5: Windows 実機確認。
- `spec/operations/windows-native-preflight.md` の未解決事項。Windows native execution、WinUSB driver assignment、Bluetooth dongle recognition、Switch pairing、report period comparison、neutral fail-safe は未検証である。
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md` の先送り事項。実 Bluetooth adapter を開く順序と失敗時 cleanup は fake backend test だけでは証明できない。
- `work-units/complete/local_026/REPORT_METRICS_AND_LOGGING.md` の先送り事項。actual report rate、jitter、adapter identity、driver state は fake timestamp からは証明できない。
- `spec/protocols/switch-hid-core.md` の未解決事項。current HID descriptor、subcommand replies、player lights replies、virtual SPI seed、rumble output handling の実機 acceptability は未検証である。
- NyX handoff。NyX macro を起動済み daemon の local TCP JSON Lines IPC client として使い、Switch capture と daemon log を突き合わせる一時手順。

use case:

- actor: hardware bring-up operator。
- 入力または状態: 専用 USB Bluetooth dongle、WinUSB driver assignment、Windows native daemon build、Switch pairing 画面、debug IPC client または NyX `swbt_hardware_bringup` macro。
- 期待する観測結果: daemon が対象 dongle を開き、Switch と pairing し、periodic input report を送り、IPC client からの state snapshot が button / stick 入力として観測できる。NyX macro を使う場合は、IPC request、Switch capture、daemon log が同じ実行条件へ対応付けられる。disconnect、owner release、timeout、process exit では neutral state へ戻る。
- 制約: 対象 adapter を曖昧にしない。内蔵または普段使いの Bluetooth adapter を使わない。実機コマンドは人間の明示承認なしに実行しない。
- 対象外: 自動 hardware test framework、複数 controller、binary release、NFC/IR、rumble semantic decode。
- source から use case へ変換した判断: 複数の record に散らばる「実機未検証」は同じ安全境界に属するため、個別に完了扱いせず、Windows hardware bring-up の record に集約する。

## 3. 対象範囲

- `spec/operations/windows-native-preflight.md` に従って、実行前確認項目を記録する。
- 専用 USB Bluetooth dongle の VID/PID と driver state を確認する。
- Windows native daemon build を起動する。
- Switch pairing を実行する。
- HID advertising と connection state を daemon log で確認する。
- report period `8000 / 8333 / 15000 / 16667 us` を比較する。
- debug IPC client から neutral、button、stick state を送る。
- NyX handoff を使う場合は、swbt commit、BTstack ref、dongle VID/PID、driver state、Switch firmware、IPC endpoint、daemon log path を Project NyX 側へ渡す。
- NyX artifact root、`run_context.json`、`ipc_session.json`、`hardware_log_draft.md`、daemon log を突き合わせる。
- owner disconnect、heartbeat timeout、daemon shutdown の neutral fail-safe を確認する。
- `docs/hardware-test-log.md` に OS、dongle、driver、backend、BTstack commit、swbt commit、Switch firmware、report period、result、cleanup を記録する。

## 4. 対象外

- 普段使いの Bluetooth adapter を使う検証。
- 内蔵 Bluetooth を使う検証。
- Linux/libusb 実機 bring-up。
- 自動 hardware test label の追加。
- swbt-daemon repo への NyX 用仮想環境作成。
- マシン全体への `nyxpy` 導入。
- Project NyX repo の `resources/swbt_hardware_bringup/settings.toml` 更新と NyX macro 実行。
- firmware / driver の一般互換性保証。
- NFC/IR、amiibo、複数 controller、rumble semantic decode。
- binary release。

## 5. 関連 spec / docs

- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/protocols/daemon-ipc-v1.md`
- `spec/protocols/switch-hid-core.md`
- `spec/operations/windows-native-preflight.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`
- `work-units/wip/local_038/BTSTACK_SEND_READY_INTEGRATION.md`
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md`
- `work-units/complete/local_026/REPORT_METRICS_AND_LOGGING.md`
- `work-units/complete/local_027/WINDOWS_NATIVE_PREFLIGHT.md`

## 6. 根拠監査

`source-audit` と `hardware-harness` を使う。

この work unit では hardware observation を追加する。OS、driver、dongle、Switch firmware、BTstack commit、swbt commit、backend、report period を分けて記録し、別環境の一般的事実として扱わない。

report period の採用判断は実測後に行う。`8000us` は current configurable default であり、実機で最適値として確認済みの値ではない。

## 7. 設計メモ

- 実機実行前に adapter identity を固定する。
- daemon cleanup behavior が不明な code path は実行しない。
- repo-local debug IPC client は software integration 済みである。NyX handoff の macro は、Project NyX 側の capture と request artifact を同じ実機 run に対応付けたい場合の外部 client 経路として扱う。
- swbt-daemon 側 agent は NyX を導入しない。swbt 側は daemon 起動、IPC endpoint、daemon log、adapter / driver / firmware 情報を渡し、NyX 側 agent が Project NyX repo で `uv run nyxpy run swbt_hardware_bringup` を実行する。
- NyX artifact と daemon log の対応は、NyX `request.id`、時刻、report period、daemon log excerpt で確認する。
- report period comparison は period ごとに別記録にする。
- 実機結果は `docs/hardware-test-log.md` を正本にし、work unit record には要約と実行条件を残す。

## 8. 対象ファイル

- `docs/hardware-test-log.md`
- `spec/operations/windows-native-preflight.md`
- `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- 実行時に必要な daemon / debug client files。

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | preflight records dedicated dongle identity and WinUSB driver state before daemon run | characterization | docs | yes |
| todo | Windows native daemon opens the selected dongle without touching an ambiguous adapter | new | hardware | yes |
| todo | Switch pairing reaches HID connection state and is recorded in hardware log | new | hardware | yes |
| todo | periodic input report loop runs at each selected report period and records result | characterization | hardware | yes |
| todo | IPC client or NyX macro state updates are observed as button and stick changes | new | hardware | yes |
| todo | owner disconnect, heartbeat timeout, and daemon shutdown leave neutral state | edge | hardware | yes |

## 10. 検証

未実行。

この record は実機 bring-up の作業単位を作成しただけであり、daemon run、Switch pairing、HID advertising、report loop は実行していない。

## 11. 実機実行条件

実機承認が必要である。

実行前にユーザから、対象 dongle、pairing、HID advertising、report loop、IPC input、cleanup 確認の範囲について明示承認を得る。

NyX handoff を使う場合も承認範囲は swbt-daemon 側で記録する。NyX macro の `hardware_approved = true` は Project NyX 側の実行条件であり、swbt 側の daemon 起動、pairing、advertising、report loop、cleanup 承認を代替しない。

必要条件:

- 専用 USB Bluetooth dongle を使う。
- 内蔵または普段使いの Bluetooth adapter を使わない。
- Windows native host で WinUSB driver assignment を確認する。
- `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。
- `docs/hardware-test-log.md` に実行記録を残す。
- cleanup と neutral fail-safe の確認手順を実行前に決める。

## 12. 先送り事項

none。

この record 自体が、既存の実機未検証事項を集約した後続 work unit である。

## 13. チェックリスト

- [ ] 実行前承認範囲を記録した。
- [ ] 専用 dongle identity と driver state を記録した。
- [ ] Windows native daemon run を実行した。
- [ ] Switch pairing 結果を記録した。
- [ ] report period comparison を記録した。
- [ ] IPC input 反映を記録した。
- [ ] NyX handoff を使った場合は artifact root と daemon log path を記録した。
- [ ] neutral fail-safe を記録した。
- [ ] `docs/hardware-test-log.md` を更新した。
