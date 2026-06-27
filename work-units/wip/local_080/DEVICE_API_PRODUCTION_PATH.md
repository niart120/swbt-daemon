# Device API Production Path

## 1. 概要

`local_079` で追加した internal device API を見直し、production の実送信経路まで device API を通す。

この work unit では、`recv` という socket 風の名前をやめる。BTstack callback packet を caller が明示的に `handle_packet` へ渡す API も主案にしない。代わりに、`listen` 時に event listener を登録し、device API が BTstack packet handler と packet decode を所有して typed event を listener へ通知する形にする。

あわせて、`open` が platform start と HID service registration を同時に持っている状態を `open` / `listen` に分け、production backend が Bluetooth HID device lifecycle を device API 経由で組み立てられる状態にする。

この変更は `api/swbt.h` の public C ABI ではなく、BTstack bridge 内部 API の整理である。Switch-facing report bytes、report period、HID registration config 値、BTstack source selection は変更しない。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-27: device API をきちんと生やして production の経路を通せるようにしたい。`recv` は socket API からの連想で付けたが実際は不要に見える。`open` / `listen` の分割や、抽象度の高い API が必要かを整理したい。
- user request, 2026-06-27 follow-up: `handle_packet` も要らない可能性がある。応答頻度を考えると caller が都度扱うより、event listener の登録の方が扱いやすいのではないか。
- `work-units/complete/local_079/DEVICE_LIFECYCLE_API.md`: `swbt_btstack_device_send` は追加済みだが、periodic report timer の production backend は既存の `swbt_btstack_input_report_timer_backend_btstack()` から直接 HID port へ送信している。
- `spec/architecture/daemon-architecture-cutover.md`: BTstack adapter は daemon / IPC internal type を参照しない。host は lifecycle / composition を所有する。

use case:

- actor: daemon production backend。
- 入力または状態: caller-owned `swbt_btstack_device_t`、production adapter device port、HID registration config、device event listener、HID cid、raw HID interrupt message、report timer callbacks。
- 期待する観測結果:
  - platform resource を `open` で開き、HID service registration を `listen` で開始できる。
  - production backend の periodic report、subcommand reply、shutdown neutral の送信が `swbt_btstack_device_send` 経由になる。
  - `listen` は device event listener を受け取り、BTstack packet callback を device API 内部の handler へ向ける。
  - device API は packet decode 後に typed event を listener へ通知し、production backend は listener 内で event に応じた policy だけを扱う。
  - `recv` と public `handle_packet` のような caller-driven packet API は消えるか、production 経路で使われなくなる。
- 制約: Switch-facing report bytes、report period、BTstack source selection、HID registration config 値、shutdown neutral ordering は変更しない。
- 対象外: public C ABI 化、複数 controller、Joy-Con、NFC / IR、adapter removal / reinsertion recovery、run loop ownership の変更。

source から use case への変換:

ユーザの目的は、socket API の名前をそのまま模倣することではなく、daemon production backend が BTstack HID device の操作を明確な API 経由で呼べるようにすることである。BTstack の callback は高頻度の `CAN_SEND_NOW` を含み得るため、caller が `recv` / `handle_packet` を逐次呼ぶ形ではなく、BTstack callback と同じ push 型の listener API として表現する。抽象度は Bluetooth HID device lifecycle に留め、application state、report scheduling policy、shutdown neutral policy は device API に入れない。

## 3. 対象範囲

- `swbt_btstack_device_open` を platform open の責務に絞る。
- `swbt_btstack_device_listen` を追加し、HID service registration、BTstack packet handler registration、device event listener registration を開始する。
- `swbt_btstack_device_recv` を削除するか production 経路から外し、caller-driven packet API を残さない。
- device event listener の責務、入力、呼び出し条件、失敗時の振る舞いを unit test で固定する。
- `swbt_btstack_input_report_timer_backend_t` の send callback に context を持たせる、または同等の経路で device handle を参照できるようにする。
- production report timer の periodic report、subcommand reply、shutdown neutral が `swbt_btstack_device_send` 経由で送信されるようにする。
- production backend regression test を更新し、report timer send path が device API を通ることを観測する。
- `local_079` の record または architecture spec に、必要な追記があれば最小限で反映する。

## 4. 対象外

- `api/swbt.h` の public C ABI 追加。
- IPC protocol の変更。
- Switch-facing report bytes、subcommand bytes、SPI、rumble packet の変更。
- report period、timer scheduling policy、shutdown neutral ordering の変更。
- BTstack source selection の変更。
- HID registration config 値の変更。
- 実機検証。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `spec/operations/work-unit-spec-tdd-flow.md`
- `work-units/complete/local_079/DEVICE_LIFECYCLE_API.md`
- `work-units/complete/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md`
- `work-units/complete/local_072/ACTIVE_SWITCH_RECONNECT.md`

## 6. 根拠監査

not applicable。

この work unit は Switch HID report bytes、subcommand bytes、SPI address、rumble packet、BTstack source selection、report period、WinUSB/libusb facts を追加または変更しない。送信先は同じ BTstack HID interrupt send であり、変更するのは production code からそこへ至る internal API 経路である。

HID registration config 値、PSM 値、report period、Switch-facing bytes、BTstack selected source list を変更する場合は、この work unit から切り出し、`source-audit` を使う。

## 7. 設計メモ

### device API の抽象度

device API は Bluetooth HID device lifecycle の境界に留める。含める責務は platform open / close、HID listen / stop、active connect、HID interrupt send、BTstack packet event decode、device event listener notification である。

device API に入れない責務:

- controller state の読み取り。
- report scheduler の周期、holdoff、timer 値更新。
- shutdown neutral policy。
- owner、heartbeat、IPC command handling。
- reconnect address の選択、保存、設定 file への永続化。

### `open` / `listen`

`open` は platform resource を開く操作とする。現在の production 経路では HCI platform start に対応する。

`listen` は HID service registration を開始し、BTstack packet handler と device event listener を登録する操作とする。Switch からの incoming connection を受けられる状態は `listen` 後である。`connect` は active reconnect request であり、`listen` 済み device に対して呼ぶ。

`close` は idempotent とし、listening 中なら HID stop を先に行い、その後 platform stop を行う。

### device event listener

責務:

- device API 内部の BTstack packet handler が callback packet を受け取る。
- device API は `swbt_btstack_hid_event_decode` で typed `swbt_btstack_hid_event_t` に変換する。
- decode result が `OK` の場合だけ、登録済み listener に event を通知する。
- production backend は listener 内で `USER_CONFIRMATION_REQUEST`、`CONNECTION_OPENED`、`CAN_SEND_NOW`、`CONNECTION_CLOSED` に応じた policy だけを扱う。

入力:

- `swbt_btstack_device_t *device`
- `swbt_btstack_device_listen_options_t`
  - HID registration config
  - service buffer
  - event listener callback
  - event listener context

出力:

- `swbt_btstack_device_listen` は registration 成功時に `SWBT_BTSTACK_DEVICE_OK` を返す。
- BTstack callback から listener への event 通知は戻り値を持たない。
- listener は短時間で戻る。高頻度の `CAN_SEND_NOW` を処理するため、blocking operation や重い work は listener 内へ入れない。
- decode result が `IGNORED` または error の場合、listener は呼ばれない。

振る舞い:

- packet polling、blocking receive、queue consume は行わない。
- device API は SSP confirmation、report timer start / stop、address persistence、shutdown finish を行わない。
- BTstack の packet handler signature は context を持たないため、初期実装では listening device を 1 つだけ扱う。複数 controller は対象外であり、2 つ目の listen は error にするか、既存 listener を明示的に close してから登録する。
- listener は device API から同期的に呼ばれる。queueing や backpressure はこの work unit では導入しない。

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
- `work-units/wip/local_080/DEVICE_API_PRODUCTION_PATH.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | device open starts only platform and device listen registers HID service in a separate call | regression | unit | no |
| todo | device close stops HID service before platform and remains idempotent after open-only or listen state | regression | unit | no |
| todo | device listen registers an event listener and emits typed events for valid BTstack HID callback packets | new | unit | no |
| todo | device listener is not called for ignored or invalid BTstack packets | edge | unit | no |
| todo | production backend receives device events through listener and keeps SSP confirmation, timer start/stop, and shutdown policy outside device API | regression | integration | no |
| todo | report timer periodic send path forwards HID interrupt message through swbt_btstack_device_send | new | integration | no |
| todo | report timer subcommand reply and shutdown neutral send paths forward HID interrupt message through swbt_btstack_device_send | regression | integration | no |
| todo | no production path calls swbt_btstack_hid_port_send_report directly except the production device port implementation | regression | integration | no |

TDD status:

- source: user request, 2026-06-27。
- use case: production の report send path まで device API 経由にし、`recv` / `handle_packet` の曖昧さを event listener registration へ整理する。
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

## 13. チェックリスト

- [x] source と use case を記録した。
- [x] device event listener の責務、入力、出力、振る舞いを記録した。
- [x] TDD Test List を作成した。
- [ ] red test を追加した。
- [ ] `open` / `listen` 分割を実装した。
- [ ] `recv` / `handle_packet` を event listener registration へ置き換えた。
- [ ] production report send path を device API 経由にした。
- [ ] targeted CTest を実行した。
- [ ] 実機未実行理由を記録した。
- [ ] work unit record を更新した。
