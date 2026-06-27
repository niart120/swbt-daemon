# Device API Production Path

## 1. 概要

`local_079` で追加した internal device API を見直し、production の実送信経路まで `swbt_btstack_device_send` を通す。

この work unit では、platform open と HID service registration の責務を `open` / `listen` に分ける。`listen` は現時点では HID service registration と既存 BTstack packet handler registration の境界に留める。`recv`、`handle_packet`、event listener registration のような packet / event 抽象は、この work unit では扱わない。

この変更は `api/swbt.h` の public C ABI ではなく、BTstack bridge 内部 API の整理である。Switch-facing report bytes、report period、HID registration config 値、BTstack source selection は変更しない。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-27: device API をきちんと生やして production の経路を通せるようにしたい。`recv` は socket API からの連想で付けたが実際は不要に見える。`open` / `listen` の分割や、抽象度の高い API が必要かを整理したい。
- user request, 2026-06-27 follow-up: event listener は実装が重くなり、当初の目的を外れそうなので `spec/dev-journal.md` に送る。
- `work-units/complete/local_079/DEVICE_LIFECYCLE_API.md`: `swbt_btstack_device_send` は追加済みだが、periodic report timer の production backend は既存の `swbt_btstack_input_report_timer_backend_btstack()` から直接 HID port へ送信している。
- `spec/architecture/daemon-architecture-cutover.md`: BTstack adapter は daemon / IPC internal type を参照しない。host は lifecycle / composition を所有する。
- `spec/dev-journal.md`: device event listener 方針の先送り判断。

use case:

- actor: daemon production backend。
- 入力または状態: caller-owned `swbt_btstack_device_t`、production adapter device port、HID registration config、HID cid、raw HID interrupt message、report timer callbacks。
- 期待する観測結果:
  - platform resource を `open` で開き、HID service registration を `listen` で開始できる。
  - production backend の periodic report、subcommand reply、shutdown neutral の送信が `swbt_btstack_device_send` 経由になる。
  - production backend の BTstack packet handler policy は、この work unit では新しい listener 抽象へ移さない。
  - `swbt_btstack_hid_port_send_report` を直接呼ぶ production 経路は、production device port 実装だけに閉じる。
- 制約: Switch-facing report bytes、report period、BTstack source selection、HID registration config 値、shutdown neutral ordering は変更しない。
- 対象外: public C ABI 化、event listener registration、packet event queue、複数 controller、Joy-Con、NFC / IR、adapter removal / reinsertion recovery、run loop ownership の変更。

source から use case への変換:

ユーザの目的は、socket API の名前をそのまま模倣することではなく、daemon production backend が BTstack HID device の操作を明確な API 経由で呼べるようにすることである。現時点で最も目的に直結する未完了点は、report timer の実送信が device API を通っていないことである。event listener は packet handling の抽象として有力だが、実装が重くなり、send path 整理の目的から外れやすいため、この work unit では扱わない。

## 3. 対象範囲

- `swbt_btstack_device_open` を platform open の責務に絞る。
- `swbt_btstack_device_listen` を追加し、HID service registration と BTstack packet handler registration を開始する。
- `swbt_btstack_input_report_timer_backend_t` の send callback に context を持たせる、または同等の経路で device handle を参照できるようにする。
- production report timer の periodic report、subcommand reply、shutdown neutral が `swbt_btstack_device_send` 経由で送信されるようにする。
- production backend regression test を更新し、report timer send path が device API を通ることを観測する。
- `local_079` の record または architecture spec に、必要な追記があれば最小限で反映する。

## 4. 対象外

- `api/swbt.h` の public C ABI 追加。
- IPC protocol の変更。
- `recv`、`handle_packet`、event listener registration の packet / event 抽象再設計。
- packet event queue、backpressure、async dispatch。
- Switch-facing report bytes、subcommand bytes、SPI、rumble packet の変更。
- report period、timer scheduling policy、shutdown neutral ordering の変更。
- BTstack source selection の変更。
- HID registration config 値の変更。
- 実機検証。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `spec/operations/work-unit-spec-tdd-flow.md`
- `spec/dev-journal.md`
- `work-units/complete/local_079/DEVICE_LIFECYCLE_API.md`
- `work-units/complete/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md`
- `work-units/complete/local_072/ACTIVE_SWITCH_RECONNECT.md`

## 6. 根拠監査

not applicable。

この work unit は Switch HID report bytes、subcommand bytes、SPI address、rumble packet、BTstack source selection、report period、WinUSB/libusb facts を追加または変更しない。送信先は同じ BTstack HID interrupt send であり、変更するのは production code からそこへ至る internal API 経路である。

HID registration config 値、PSM 値、report period、Switch-facing bytes、BTstack selected source list を変更する場合は、この work unit から切り出し、`source-audit` を使う。

## 7. 設計メモ

### device API の抽象度

device API は Bluetooth HID device lifecycle の境界に留める。この work unit で扱う責務は platform open / close、HID listen / stop、active connect、HID interrupt send である。

device API に入れない責務:

- controller state の読み取り。
- report scheduler の周期、holdoff、timer 値更新。
- shutdown neutral policy。
- owner、heartbeat、IPC command handling。
- reconnect address の選択、保存、設定 file への永続化。
- packet event listener、queue、backpressure。

### `open` / `listen`

`open` は platform resource を開く操作とする。現在の production 経路では HCI platform start に対応する。

`listen` は HID service registration を開始し、既存の BTstack packet handler を登録する操作とする。Switch からの incoming connection を受けられる状態は `listen` 後である。`connect` は active reconnect request であり、`listen` 済み device に対して呼ぶ。

`close` は idempotent とし、listening 中なら HID stop を先に行い、その後 platform stop を行う。

### packet / event 抽象

`recv`、`handle_packet`、event listener registration はこの work unit では実装しない。理由は、report send path を device API に通す当初目的に対して実装範囲が広がりやすいためである。

event listener が必要になった場合の再検討条件は `spec/dev-journal.md` に残す。現時点では production backend の既存 packet handler policy を維持する。

## 8. 対象ファイル

- `swbt/btstack_bridge/device.*`
- `swbt/btstack_bridge/input_report_timer_adapter.*`
- `swbt/btstack_bridge/production_adapter.h`
- `swbt/btstack_bridge/production_btstack.c`
- `swbt/daemon/production_backend.*`
- `tests/btstack_device_test.c`
- `tests/btstack_input_report_timer_adapter_test.c`
- `tests/daemon_production_backend_test.c`
- `CMakeLists.txt`
- `spec/dev-journal.md`
- `work-units/wip/local_080/DEVICE_API_PRODUCTION_PATH.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | device open starts only platform and device listen registers HID service in a separate call | regression | unit | no |
| todo | device close stops HID service before platform and remains idempotent after open-only or listen state | regression | unit | no |
| todo | report timer periodic send path forwards HID interrupt message through swbt_btstack_device_send | new | integration | no |
| todo | report timer subcommand reply and shutdown neutral send paths forward HID interrupt message through swbt_btstack_device_send | regression | integration | no |
| todo | no production path calls swbt_btstack_hid_port_send_report directly except the production device port implementation | regression | integration | no |
| deferred | device event listener registration for typed BTstack HID events | deferred | unit/integration | no |

TDD status:

- source: user request, 2026-06-27。
- use case: production の report send path まで device API 経由にする。packet / event 抽象はこの work unit では扱わない。
- state: todo。
- next red candidate: report timer periodic send path forwards HID interrupt message through `swbt_btstack_device_send`。

## 10. 検証

未実行。

予定:

- `just build-debug`
- targeted `just test-debug` with `btstack_device_test|btstack_input_report_timer_adapter_test|daemon_production_backend_test`
- `just format`
- `just test-debug`
- 変更範囲に応じて `just verify`

## 11. 実機実行条件

この work unit の通常範囲では実機検証を実行しない。理由は、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop を実行せず、unit / integration test の fake port で API 経路と送信委譲を確認するためである。

次のいずれかを変更する場合は、`hardware-harness` の承認境界に従い、専用 USB Bluetooth dongle、WinUSB、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、`docs/hardware-test-log.md` への記録を条件にする。

- HCI power-on / power-off ordering。
- HID registration config の値。
- report period または timer scheduling。
- Switch-facing input report bytes。
- pairing、active reconnect、advertising の実機 sequence。

## 12. 先送り事項

- 観測: device API を `api/swbt.h` の public C ABI として公開する可能性がある。
  先送り理由: 現時点では internal BTstack bridge API の責務を固める段階であり、外部 contract にすると変更余地を狭める。
  次の置き場: public C ABI 化が必要になった時点で別 work unit record を作る。

- 観測: connection handle を device が所有するか、caller が `hid_cid` を持ち続けるかは未確定である。
  先送り理由: 複数 controller は対象外であり、現行 report timer は `hid_cid` を start options として受け取る構造で動いている。
  次の置き場: connection state ownership を見直す場合は別 work unit record を作る。

- 観測: device event listener registration は、BTstack callback と device API の境界として有力だが、この work unit では扱わない。
  先送り理由: 実装範囲が packet dispatch、listener lifetime、同期処理時間、将来の queueing 判断へ広がり、production send path を device API に通す目的から外れやすい。
  次の置き場: `spec/dev-journal.md` の `2026-06-27: device event listener 方針の先送り` を source にして、必要になった時点で別 work unit record を作る。

## 13. チェックリスト

- [x] source と use case を記録した。
- [x] event listener 方針を dev-journal へ送る判断を記録した。
- [x] TDD Test List を作成した。
- [ ] red test を追加した。
- [ ] `open` / `listen` 分割を実装した。
- [ ] production report send path を device API 経由にした。
- [ ] targeted CTest を実行した。
- [ ] 実機未実行理由を記録した。
- [ ] work unit record を更新した。
