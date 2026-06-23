# Shutdown Neutral Retry Failure

## 1. 概要

architecture cutover 後の production shutdown で、shutdown neutral の即時送信が pending になり、その後の CAN_SEND_NOW 再送も失敗する場合に、daemon が power-off と run-loop exit へ進めるようにした work unit。

この work unit は PR #59 の destructive cutover 本体を戻さない。旧 runtime、旧 session、旧 mailbox、旧 production ops table は復活させず、新経路の失敗 cleanup だけを閉じた。

## 2. 起点 / ユースケース

source:

- user-provided review, 2026-06-23: shutdown neutral の即時送信失敗後、timer adapter と production 側が pending retry を続け、接続が閉じない条件では power-off と run-loop exit に進めない可能性がある。
- `spec/architecture/daemon-architecture-cutover.md`: neutral send failure 時は failure counter / log を残して cleanup と power-off を継続する。
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`: H1 pass は成功経路を確認したが、恒久的な send failure 経路は実機対象外である。

use case:

- actor: maintainer, production daemon shutdown path.
- 入力または状態: owner が non-neutral state を保持し、shutdown neutral の即時送信が pending になった後、CAN_SEND_NOW 再送も失敗する。
- 期待する観測結果: daemon は retry を無限継続せず、pending を解除し、failure を trace に残して power-off と run-loop exit へ進む。
- 制約: Switch-facing report byte、report period、BTstack source selection は変更しない。実機 H1 は再実行しない。
- 対象外: architecture journey の全面強化、target include 境界の compile-time 強制、metrics schema の再設計、広い production adapter table の分割。

source から use case への変換:

review の blocking item は実機成功経路ではなく failure cleanup 経路である。software fake で恒久 send failure を作り、production shutdown が必ず終了することを固定した。

## 3. 対象範囲

- shutdown neutral pending 中の再送失敗を production backend が終了条件として扱う。
- pending を解除し、power-off と run-loop exit に進める。
- 恒久的に neutral send が失敗する fake を追加し、daemon が shutdown 完了へ進むことを C test で確認する。
- `spec/architecture/daemon-architecture-cutover.md` と `docs/status.md` に failure cleanup 経路の状態を記録する。

## 4. 対象外

- Switch HID report byte、subcommand、SPI、rumble packet、report period の変更。
- 実機 pairing、HID advertising、report loop の再実行。
- `architecture_journey_test` の全面的な縦断化。
- CMake target include path の public/private 境界再設計。
- `swbt_btstack_production_adapter_t` の分割。
- status metrics の全接続。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `docs/status.md`
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`
- `tests/daemon_production_backend_test.c`
- `swbt/daemon/production_backend.c`
- `swbt/btstack_bridge/input_report_timer_adapter.c`

## 6. 根拠監査

not applicable。

この work unit は shutdown failure cleanup の制御フローを扱う。Switch protocol byte、BTstack source selection、report period、WinUSB/libusb fact は追加または変更していない。

## 7. 設計メモ

- 初回 neutral send が pending を返した場合、次の CAN_SEND_NOW で一度だけ completion を試す。
- pending 中の CAN_SEND_NOW が失敗した場合は、pending を解除して shutdown finish へ進む。
- 接続が閉じた場合も pending を解除し、shutdown finish へ進む。
- retry 回数を増やす判断はこの work unit ではしない。永久 retry を避けることを優先した。

## 8. 変更ファイル

- `swbt/daemon/production_backend.c`
- `tests/daemon_production_backend_test.c`
- `spec/architecture/daemon-architecture-cutover.md`
- `docs/status.md`
- `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | shutdown neutral pending 中の CAN_SEND_NOW 再送が失敗しても power-off と run-loop exit へ進む | regression | integration | no |
| deferred | architecture journey が JSON から fake HID trailing neutral と power-off まで一経路で通る | regression | integration | no |
| deferred | target include boundary を CMake script ではなく compile error で強制する | structure | build | no |
| deferred | production adapter の広い function table を supervisor / BTstack adapter / IPC reactor integration に分割する | structure | integration | no |
| deferred | report tick と IPC rejected/coalesced metrics を production path へ接続するか、未観測として表現する | regression | integration | no |

TDD status:

- source: user-provided review and architecture cutover spec failure cleanup rule.
- use case: shutdown neutral retry failure must not keep production run loop alive forever.
- item: shutdown neutral pending 中の CAN_SEND_NOW 再送が失敗しても power-off と run-loop exit へ進む。
- red: `just debug` failed in `daemon_production_backend_test`; added fake saw `trigger exit calls: expected 1, got 0`.
- green: production backend now clears `shutdown_neutral_pending` and calls shutdown finish when pending `CAN_SEND_NOW` returns non-zero.
- state: done.
- notes: failure path is automatic and does not require hardware.

## 10. 検証

- red: `just debug`
  - result: failed as expected.
  - failure: `daemon_production_backend_test`, `trigger exit calls: expected 1, got 0`.
- green: `just debug`
  - result: pass.
  - CTest: 38 tests passed.
- full verification: `just verify`
  - result: pass.
  - covered: format check, clang-tidy preset build, linux-debug build/test, linux-asan build/test, windows-mingw-debug cross build.

## 11. 実機実行条件

実機不要。

この work unit は fake production adapter による shutdown failure cleanup 経路を扱う。Bluetooth adapter open、Switch pairing、HID advertising、report loop は実行していない。Switch-facing bytes、report period、BTstack source selection は変更していないため、H1 の再実行条件に該当しない。

## 12. 先送り事項

- architecture journey 強化。
  観測: 現行 journey は production power-off までの縦断ではない。
  先送り理由: 今回の blocking item は shutdown failure cleanup であり、journey 強化は別の integration test 整理として扱う。
  次の置き場: 後続 work unit。
- include boundary の compile-time 強制。
  観測: 現行 target は `swbt/` 全体を include path として公開し、禁止依存は CMake script の文字列検査に寄っている。
  先送り理由: build graph の構造変更を伴うため、この failure cleanup には混ぜない。
  次の置き場: 後続 work unit。
- production adapter table 分割。
  観測: `swbt_btstack_production_adapter_t` は広い function table である。
  先送り理由: 今回は旧 table 復活を避けながら failure cleanup を閉じる。
  次の置き場: 後続 work unit。
- status metrics 接続。
  観測: report tick と IPC rejected/coalesced の production 計測点は不完全である。
  先送り理由: metrics schema と観測範囲の判断が必要であり、shutdown termination fix から分ける。
  次の置き場: 後続 work unit。

## 13. チェックリスト

- [x] work unit record の source / use case / TDD Test List を更新した。
- [x] 恒久 send failure の red test を確認した。
- [x] pending retry failure で power-off と run-loop exit へ進む実装にした。
- [x] targeted CTest 相当の `daemon_production_backend_test` を `just debug` 内で確認した。
- [x] full verification を実行して結果を記録した。
- [x] 実機未実行理由を記録した。
- [x] 先送り事項を後続 source として使える粒度で記録した。
