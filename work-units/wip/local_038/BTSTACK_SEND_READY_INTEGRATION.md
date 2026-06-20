# BTstack Send-Ready Integration

## 1. 概要

BTstack can-send event、subcommand reply queue、periodic input report scheduler を production adapter 境界で統合する work unit。

この work unit は software integration を主対象にする。Switch 実機が `0x21` reply priority と periodic `0x30` のずれを受け入れるかは、hardware bring-up で記録する。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_022/SUBCOMMAND_REPLY_SEND_QUEUE.md` の先送り事項。BTstack send-ready callback と periodic scheduler の exact integration は未実装である。
- `spec/references/btstack-subcommand-reply-send-queue.md` の未解決事項。
- `work-units/complete/local_019/BTSTACK_OUTPUT_REPORT_CALLBACKS.md` の先送り事項。subcommand reply send queue への接続、DATA / SET_REPORT の実機確認、callback timing が未完了である。
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md`。runtime は fake backend で output handler と report timer を接続しているが、実 BTstack send-ready path は未検証である。
- `spec/protocols/switch-hid-core.md`。Subcommand reply `0x21` は periodic `0x30` より優先する方針であり、BTstack can-send event 上の production integration はこの work unit で扱う。

use case:

- actor: BTstack bridge adapter。
- 入力または状態: Switch output report callback、dispatcher response、subcommand reply queue、periodic scheduler、BTstack can-send event。
- 期待する観測結果: output report から作られた `0x21` reply は queue に入り、can-send event で periodic `0x30` より優先して送られる。send failure では retry でき、periodic scheduler は reply を落とさないために必要な tick だけずらす。
- 制約: queue core は BTstack header に依存しない。production adapter は BTstack API 境界に閉じる。実機 acceptability はこの software work unit の完了条件にしない。
- 対象外: Switch pairing、HID advertising、実機 output report observation、report period 採用判断。
- source から use case へ変換した判断: 完了済み core は queue と scheduler を個別に検証している。後続作業は、BTstack can-send readiness を中心にした integration contract を固定すること。

## 3. 対象範囲

- output report handler から dispatcher response を受け、subcommand reply queue へ enqueue する adapter path を追加または整理する。
- can-send event で queued `0x21` reply を優先送信する。
- queued reply がない場合に periodic `0x30` scheduler send を進める。
- send failure で reply queue の head item を保持する。
- fake BTstack backend で can-send sequence、reply priority、periodic tick slip を検証する。
- production BTstack backend の compile boundary を確認する。

## 4. 対象外

- Switch 実機 acceptability の確認。
- DATA callback と SET_REPORT callback の実機上の選択結果の断定。
- report period の実機 tuning。
- new Switch protocol bytes の追加。
- rumble semantic decode。
- Windows native hardware run。

## 5. 関連 spec / docs

- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/protocols/switch-hid-core.md`
- `spec/references/btstack-subcommand-reply-send-queue.md`
- `spec/references/btstack-periodic-input-report-core.md`
- `spec/references/btstack-output-report-callbacks.md`
- `work-units/complete/local_019/BTSTACK_OUTPUT_REPORT_CALLBACKS.md`
- `work-units/complete/local_022/SUBCOMMAND_REPLY_SEND_QUEUE.md`
- `work-units/complete/local_023/BTSTACK_INPUT_REPORT_TIMER_ADAPTER.md`
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md`
- `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md`

## 6. 根拠監査

`source-audit` が必要である。

この work unit は BTstack can-send API と report priority policy に触れる。既存の root facts は `spec/references/btstack-subcommand-reply-send-queue.md` と `spec/references/btstack-periodic-input-report-core.md` にある。新しい BTstack API assumption、send ordering、report timing value を追加する場合は、関連 reference を更新する。

実機 acceptability は hardware observation として扱い、この work unit の software verification だけでは pass としない。

## 7. 設計メモ

- `0x21` reply は already-built report bytes として queue に入れる。
- queue full は explicit error とし、古い reply を黙って捨てない。
- can-send event が来たとき、queue に reply があれば先に送る。
- periodic `0x30` は reply 送信のために 1 tick ずれることを許容する。drop / delay policy は test で観測できる形にする。
- fake backend test は ordering と retry を証明する。Switch がそれを受け入れるかは証明しない。

## 8. 対象ファイル

- `swbt/btstack_bridge/output_report_handler.*`
- `swbt/btstack_bridge/output_report_callbacks.*`
- `swbt/btstack_bridge/subcommand_reply_queue.*`
- `swbt/btstack_bridge/input_report_timer_adapter.*`
- `swbt/daemon/runtime.*`
- `tests/btstack_output_report_callbacks_test.c`
- `tests/btstack_subcommand_reply_queue_test.c`
- `tests/btstack_input_report_timer_adapter_test.c`
- `tests/daemon_runtime_test.c`
- `spec/references/btstack-subcommand-reply-send-queue.md`
- `work-units/wip/local_038/BTSTACK_SEND_READY_INTEGRATION.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | output report dispatcher response enqueues an already-built `0x21` reply | new | unit | no |
| todo | can-send event sends queued `0x21` before periodic `0x30` | new | integration | no |
| todo | send failure keeps queued reply for retry | edge | unit | no |
| todo | queued reply can delay periodic report without losing scheduler state | edge | integration | no |
| todo | production BTstack backend compiles without moving BTstack API calls outside bridge | regression | build | no |
| deferred | Switch accepts prioritized `0x21` replies during a real HID session | characterization | hardware | yes |

## 10. 検証

未実行。

この record は follow-up を作成しただけであり、code、CTest、実機コマンドは実行していない。

## 11. 実機実行条件

この work unit の software integration では実機検証は不要である。

Switch pairing、HID advertising、DATA / SET_REPORT observation、reply acceptability を確認する場合は実機作業である。実行する場合は `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` の承認条件に従い、`docs/hardware-test-log.md` に記録する。

## 12. 先送り事項

- 観測: Switch が prioritized `0x21` reply を実機 HID session で受け入れるかは software integration では証明できない。
  先送り理由: Switch pairing、output report observation、periodic report loop が必要である。
  次の置き場: `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `docs/hardware-test-log.md`。

## 13. チェックリスト

- [ ] source audit を更新する必要があるか判断した。
- [ ] red test を追加した。
- [ ] send-ready integration を実装した。
- [ ] targeted CTest を実行した。
- [ ] `just debug` を実行した。
- [ ] sanitizer または Windows cross build の必要性を判断した。
- [ ] 実機状態を記録した。
