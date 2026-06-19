# Subcommand Reply Send Queue

## 1. 概要

Subcommand reply report を periodic input report より優先して送るための bounded send queue core を追加する計画 record。

local_015 で対象外にした Subcommand reply priority queue を扱う。

## 2. 対象範囲

- already-built subcommand reply report を queue に copy する。
- HID CID、report bytes、report size を queue item として保持する。
- queue は FIFO とし、subcommand reply を periodic report より優先する。
- send callback failure、queue full、invalid report size を explicit result で返す。
- BTstack send-ready adapter へ接続しやすい core API を追加する。

## 3. 対象外

- Subcommand reply bytes の構築。
- Subcommand dispatcher。
- BTstack の production callback 登録。
- periodic input report scheduler の全面改修。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-subcommand-reply-core.md`
- `work-units/complete/local_014/SWITCH_SUBCOMMAND_REPLY_CORE.md`
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md`
- `work-units/complete/local_020/SUBCOMMAND_DISPATCHER_CORE.md`

## 5. 根拠監査

この queue 自体は reply bytes を解釈しない設計にする。

reply bytes を構築または変更する場合は、実装前に根拠監査が必要である。

Subcommand reply を periodic report より優先する方針は initial design の設計方針であり、実機 acceptability は未検証である。

BTstack send-ready の exact integration は pinned BTstack source に対する追加確認が必要である。

## 6. 設計メモ

- queue は fixed capacity とし、動的 allocation を使わない。
- enqueue 時に report bytes を copy し、caller buffer の lifetime に依存しない。
- send failure では head item を残し、caller が retry できるようにする。
- queue core は BTstack header に依存しない。
- periodic scheduler との優先制御は adapter 層で行う。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/btstack_bridge/subcommand_reply_queue.h`
- `swbt/btstack_bridge/subcommand_reply_queue.c`
- `swbt/btstack_bridge/input_report_scheduler.h`
- `swbt/btstack_bridge/input_report_scheduler.c`
- `tests/btstack_subcommand_reply_queue_test.c`
- `spec/references/btstack-subcommand-reply-send-queue.md`
- `work-units/wip/local_022/SUBCOMMAND_REPLY_SEND_QUEUE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | enqueue then send preserves HID CID, report size, and report bytes | new | unit | no |
| todo | multiple queued replies are sent in FIFO order | new | unit | no |
| todo | full queue returns explicit error without overwriting existing items | edge | unit | no |
| todo | send callback failure leaves the head item queued for retry | edge | unit | no |
| todo | invalid arguments and invalid report size are rejected | edge | unit | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、red、green、refactor、実機検証は実行していない。

実装後は `make debug CTEST_ARGS="-R btstack_subcommand_reply_queue_test"` を実行する。

変更範囲に応じて `make asan` または `make windows-cross` を実行する。

## 10. 実機実行条件

この work unit の queue core unit test では実機検証は不要である。

Switch が reply timing を受け入れるかを確認する段階では実機検証が必要である。

実機検証を行う場合は、人間の明示承認を得る。

実機検証を行う場合は、専用 USB Bluetooth ドングルだけを使う。

実機検証を行う場合は、`SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。

実機検証を行う場合は、OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を `docs/hardware-test-log.md` に記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] 根拠監査を完了した。
- [ ] Subcommand reply send queue core を実装した。
- [ ] テストを追加した。
- [ ] 検証を実行した。
- [ ] 実機状態を記録した。
