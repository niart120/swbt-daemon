# Subcommand Reply Send Queue

## 1. 概要

Subcommand reply report を periodic input report より優先して送るための bounded send queue core を追加した。

この work unit は local_015 で対象外にした Subcommand `0x21` priority queue の core 部分だけを扱う。
queue は already-built report bytes を copy し、BTstack header や production callback 登録には依存しない。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md` の対象外項目。
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` の `0x21` reply priority policy。
- `work-units/complete/local_020/SUBCOMMAND_DISPATCHER_CORE.md` の先送り事項。

use case:

- 境界: BTstack bridge の send-ready adapter 前段。
- 入力: already-built subcommand reply report、HID CID、report size。
- 期待する観測結果: report bytes が enqueue 時に copy され、send 時に FIFO で send callback へ渡る。
- 制約: send callback failure では head item を残し、retry できる。
- 対象外: reply bytes の構築、BTstack production callback 登録、実機 report loop。

## 3. 対象範囲

- already-built subcommand reply report を queue に copy する。
- HID CID、report bytes、report size を queue item として保持する。
- queue は fixed capacity FIFO とする。
- send callback failure、queue full、invalid report size を explicit result で返す。
- BTstack send-ready adapter へ接続しやすい callback shape を持つ core API を追加する。

## 4. 対象外

- Subcommand reply bytes の構築。
- Subcommand dispatcher。
- BTstack の production callback 登録。
- periodic input report scheduler の全面改修。
- 実機 pairing、HID advertising、report loop。

## 5. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-subcommand-reply-core.md`
- `spec/references/btstack-subcommand-reply-send-queue.md`
- `work-units/complete/local_014/SWITCH_SUBCOMMAND_REPLY_CORE.md`
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md`
- `work-units/complete/local_020/SUBCOMMAND_DISPATCHER_CORE.md`

## 6. 根拠監査

`source-audit` を使い、`spec/references/btstack-subcommand-reply-send-queue.md` を追加した。

この queue 自体は reply bytes を解釈しない。
report size は `spec/references/switch-subcommand-reply-core.md` の 50-byte `0x21` reply contract を使う。

Subcommand reply を periodic report より優先する方針は initial design の設計方針であり、実機 acceptability は未検証である。
BTstack send-ready の exact integration は後続 work unit に残す。

## 7. 設計メモ

- queue は fixed capacity とし、動的 allocation を使わない。
- enqueue 時に report bytes を copy し、caller buffer の lifetime に依存しない。
- send failure では head item を残し、caller が retry できるようにする。
- queue core は BTstack header に依存しない。
- periodic scheduler との優先制御は adapter 層で行う。

## 8. 対象ファイル

- `CMakeLists.txt`
- `swbt/btstack_bridge/subcommand_reply_queue.h`
- `swbt/btstack_bridge/subcommand_reply_queue.c`
- `tests/btstack_subcommand_reply_queue_test.c`
- `spec/references/btstack-subcommand-reply-send-queue.md`
- `spec/references/README.md`
- `work-units/complete/local_022/SUBCOMMAND_REPLY_SEND_QUEUE.md`

## 9. TDD Test List

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | enqueue then send preserves HID CID, report size, and report bytes | new | unit | no |
| done | multiple queued replies are sent in FIFO order | new | unit | no |
| done | full queue returns explicit error without overwriting existing items | edge | unit | no |
| done | send callback failure leaves the head item queued for retry | edge | unit | no |
| done | invalid arguments and invalid report size are rejected | edge | unit | no |

## 10. 検証

- red: `just debug` failed as expected because `tests/btstack_subcommand_reply_queue_test.c` included missing `btstack_bridge/subcommand_reply_queue.h`.
- green: `just debug` passed. 20 tests passed, including `btstack_subcommand_reply_queue_test`.
- first verify: `just verify` failed in clang-tidy because the test helper `fill_report` had easily swappable adjacent parameters.
- final verify: `just verify` passed. Format check, clang-tidy, linux debug tests, ASan/UBSan tests, and Windows MinGW cross build passed.

## 11. 実機実行条件

この work unit の queue core unit test では実機検証は不要である。
Bluetooth adapter、Switch pairing、HID advertising、report loop に触れていない。

Switch が reply timing を受け入れるかを確認する段階では実機検証が必要である。
実機検証を行う場合は、人間の明示承認を得る。
実機検証を行う場合は、専用 USB Bluetooth ドングルだけを使う。
実機検証を行う場合は、`SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。
実機検証を行う場合は、OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を `docs/hardware-test-log.md` に記録する。

## 12. 先送り事項

- BTstack send-ready callback と periodic scheduler の exact integration は未実装である。queue core が BTstack header に依存しない境界を保つため、production adapter work unit の source として `spec/references/btstack-subcommand-reply-send-queue.md` の未解決事項に残す。
- Switch 実機で `0x21` reply を priority queue 経由で送ったときの acceptability は未検証である。実機承認を得た work unit で `docs/hardware-test-log.md` に記録する。

## 13. チェックリスト

- [x] work unit record を新形式へ更新した。
- [x] 根拠監査を記録した。
- [x] Subcommand reply send queue core を実装した。
- [x] テストを追加した。
- [x] red を確認した。
- [x] green を確認した。
- [x] `just verify` を実行した。
- [x] 実機未実行理由を記録した。
