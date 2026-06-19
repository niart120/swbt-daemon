# Player Lights Policy

## 1. 概要

Switch の player lights request を受けたときに、virtual Pro Controller として保持する player lights state と reply policy を定義するための計画 record。

local_020 の dispatcher から呼べる policy core として分離する。

## 2. 対象範囲

- player lights の session state を追加する。
- set player lights request を検証し、監査済み policy に従って state を更新する。
- player lights に reply payload が必要な場合は、根拠監査後に確定した policy に従って生成する。
- daemon status に将来載せる player index または lights state の内部表現を整理する。
- invalid payload と unsupported pattern を explicit result で返す。

## 3. 対象外

- 物理 LED の点灯制御。
- 複数 controller の slot assignment。
- GUI 表示。
- game-specific player indicator policy。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-subcommand-core.md`
- `spec/references/switch-subcommand-reply-core.md`
- `work-units/complete/local_014/SWITCH_SUBCOMMAND_REPLY_CORE.md`
- `work-units/complete/local_020/SUBCOMMAND_DISPATCHER_CORE.md`

## 5. 根拠監査

player lights の bit 意味、request payload、reply bytes は実装前に根拠監査が必要である。

Switch subcommand ID と ACK policy は実装前に根拠監査が必要である。

既存 classifier の定数は参照できるが、player lights の semantics としては断定しない。

実機 Switch での表示と acceptability は未検証である。

## 6. 設計メモ

- player lights state は `swbt_state_t` の button state と分ける。
- policy core は BTstack header に依存しない。
- set request は last-request-wins とし、複数 controller slot の自動割り当てはしない。
- reply が必要な request は現在の policy state から payload を組み立てる。
- default state は根拠監査後に決める。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/switch/switch_player_lights.h`
- `swbt/switch/switch_player_lights.c`
- `swbt/switch/switch_subcommand_dispatcher.h`
- `swbt/switch/switch_subcommand_dispatcher.c`
- `tests/switch_player_lights_test.c`
- `spec/references/switch-player-lights-policy.md`
- `work-units/wip/local_030/PLAYER_LIGHTS_POLICY.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | default player lights state is explicit and does not imply an unaudited slot | new | unit | no |
| todo | source-audited set player lights payload updates policy state | new | unit | no |
| todo | request requiring a reply returns payload from current policy state | new | unit | no |
| todo | invalid payload leaves state unchanged and returns explicit error | edge | unit | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、red、green、refactor、実機検証は実行していない。

実装後は `make debug CTEST_ARGS="-R switch_player_lights_test"` を実行する。

変更範囲に応じて `make asan` または `make windows-cross` を実行する。

## 10. 実機実行条件

この work unit の policy core unit test では実機検証は不要である。

Switch 上の player lights 表示と subcommand acceptability を確認する段階では実機検証が必要である。

実機検証を行う場合は、人間の明示承認を得る。

実機検証を行う場合は、専用 USB Bluetooth ドングルだけを使う。

実機検証を行う場合は、`SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。

実機検証を行う場合は、OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を `docs/hardware-test-log.md` に記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] 根拠監査を完了した。
- [ ] player lights policy core を実装した。
- [ ] テストを追加した。
- [ ] 検証を実行した。
- [ ] 実機状態を記録した。
