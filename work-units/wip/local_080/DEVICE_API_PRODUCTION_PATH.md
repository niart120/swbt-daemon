# Device API Production Send Path

## 1. 概要

`local_079` で追加した internal device API のうち、production の HID interrupt send 経路を `swbt_btstack_device_send` へ通す。

`local_080` は、device API 全体の再設計ではなく、未完了の send path を閉じる work unit とする。`open` / `listen` 分割、`recv` の削除または rename、event listener registration は、この work unit の完了条件に含めない。

この変更は `api/swbt.h` の public C ABI ではなく、BTstack bridge と production backend の内部境界整理である。Switch-facing report bytes、report period、HID registration config 値、BTstack source selection は変更しない。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-27: device API をきちんと生やして production の経路を通せるようにしたい。`recv` は socket API からの連想で付けたが実際は不要に見える。`open` / `listen` の分割や、抽象度の高い API が必要かを整理したい。
- user request, 2026-06-27 follow-up: event listener は実装が重くなり、当初の目的を外れそうなので `spec/dev-journal.md` に送る。
- `work-units/complete/local_079/DEVICE_LIFECYCLE_API.md`: `swbt_btstack_device_send` は追加済みだが、periodic report timer の production backend は既存の `swbt_btstack_input_report_timer_backend_btstack()` から直接 HID port へ送信している。
- `work-units/complete/local_082/CONTROL_RUNTIME_BOUNDARY_IMPLEMENTATION.md`: `swbt/runtime` は HID registration、output handler、report timer、neutral shutdown の resource lifecycle を持つ。daemon host は runtime host と IPC runner の利用者に下がった。
- `spec/architecture/daemon-architecture-cutover.md`: BTstack adapter は daemon / IPC internal type を参照しない。host は lifecycle / composition を所有する。
- `spec/dev-journal.md`: device event listener 方針の先送り判断。

use case:

- actor: daemon production backend。
- 入力または状態: caller-owned `swbt_btstack_device_t`、production adapter device port、HID cid、input report timer adapter、periodic report、subcommand reply、shutdown neutral send。
- 期待する観測結果:
  - production の periodic report、subcommand reply、shutdown neutral の HID interrupt message が `swbt_btstack_device_send` 経由で送信される。
  - input report timer は report message を組み立てるが、BTstack HID port を直接選ばない。
  - `swbt_btstack_hid_port_send_report` を直接呼ぶ production 経路は、production device port 実装だけに閉じる。
  - existing report bytes、timer advance、reply holdoff、shutdown neutral retry behavior は変えない。
- 制約: Switch-facing report bytes、report period、BTstack source selection、HID registration config 値、shutdown neutral ordering は変更しない。
- 対象外: public C ABI 化、`open` / `listen` 分割、`recv` rename、event listener registration、packet event queue、複数 controller、Joy-Con、NFC / IR、adapter removal / reinsertion recovery、run loop ownership の変更。

source から use case への変換:

ユーザの目的は、socket API の名前をそのまま模倣することではなく、daemon production backend が BTstack HID device の操作を明確な API 経由で呼べるようにすることである。現時点で目的に直結する未完了点は、report timer の実送信が device API を通っていないことである。

`open` / `listen` 分割は一見同じ device API 問題に見えるが、`local_082` 後は runtime host が `hid_register` / `hid_stop` を resource lifecycle として扱っている。この分割を local_080 に混ぜると、runtime lifecycle、BTstack platform start、HID registration、incoming connection readiness の命名と順序を同時に再設計することになる。local_080 では扱わず、link lifecycle API の別 work unit に送る。

## 3. 対象範囲

- input report timer adapter の送信依存を、BTstack HID port 固定ではなく caller-provided HID interrupt sender へ分離する。
- periodic report send が caller-provided sender を通ることを unit test で固定する。
- subcommand reply send と shutdown neutral send が同じ sender を通ることを unit test で固定する。
- production backend が report timer sender を `swbt_btstack_device_send` へ接続する。
- production device port 実装以外の production path から `swbt_btstack_hid_port_send_report` の直接呼び出しをなくす。
- `local_080` の work unit record に、send path へ絞った scope、TDD Test List、検証結果を記録する。

## 4. 対象外

- `api/swbt.h` の public C ABI 追加。
- IPC protocol の変更。
- `open` / `listen` 分割。
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
- `work-units/complete/local_082/CONTROL_RUNTIME_BOUNDARY_IMPLEMENTATION.md`
- `work-units/complete/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md`
- `work-units/complete/local_072/ACTIVE_SWITCH_RECONNECT.md`

## 6. 根拠監査

not applicable。

この work unit は Switch HID report bytes、subcommand bytes、SPI address、rumble packet、BTstack source selection、report period、WinUSB/libusb facts を追加または変更しない。送信先は同じ BTstack HID interrupt send であり、変更するのは production code からそこへ至る internal API 経路である。

HID registration config 値、PSM 値、report period、Switch-facing bytes、BTstack selected source list を変更する場合は、この work unit から切り出し、`source-audit` を使う。

## 7. 設計メモ

### device API の抽象度

device API は Bluetooth HID device lifecycle と HID interrupt send の境界に留める。この work unit で扱う責務は HID interrupt send の production 経路を `swbt_btstack_device_send` に集約することである。

device API に入れない責務:

- controller state の読み取り。
- report scheduler の周期、holdoff、timer 値更新。
- shutdown neutral policy。
- owner、heartbeat、IPC command handling。
- reconnect address の選択、保存、設定 file への永続化。
- packet event listener、queue、backpressure。

### timer adapter と sender

input report timer adapter は report bytes を組み立て、periodic / reply / neutral の送信タイミングを扱う。どの transport で送るかは adapter backend に固定しない。

production では、timer adapter に渡す HID interrupt sender が `swbt_btstack_device_send(&backend->device, hid_cid, message, message_size)` を呼ぶ。unit test では fake sender を渡し、message bytes と呼び出し回数を観測する。

この分離により、message builder / scheduler の責務を timer adapter に残しつつ、BTstack HID port への直接依存は production device port 実装へ閉じる。

### `open` / `listen`

`open` / `listen` 分割は local_080 では実装しない。

理由:

- current `swbt_btstack_device_open` は platform start と HID registration を束ねている。
- `local_082` 後は `swbt/runtime` が `hid_register` / `hid_stop` を runtime resource lifecycle として所有している。
- `listen` を追加すると、device API、runtime host、daemon production backend のどの層が incoming connection readiness を表すかを同時に決める必要がある。
- send path を device API に通す目的とは独立に検証できる。

link lifecycle の公開 API または runtime status schema を見直す work unit で、`open` / `listen` / `advertise` / `accept` の語彙を再検討する。

### packet / event 抽象

`recv`、`handle_packet`、event listener registration はこの work unit では実装しない。理由は、report send path を device API に通す当初目的に対して実装範囲が広がりやすいためである。

event listener が必要になった場合の再検討条件は `spec/dev-journal.md` に残す。現時点では production backend の既存 packet handler policy を維持する。

## 8. 対象ファイル

- `swbt/btstack_bridge/input_report_timer_adapter.*`
- `swbt/btstack_bridge/device.*`
- `swbt/btstack_bridge/production_adapter.h`
- `swbt/btstack_bridge/production_btstack.c`
- `swbt/daemon/production_backend.*`
- `tests/btstack_input_report_timer_adapter_test.c`
- `tests/daemon_production_backend_test.c`
- `tests/btstack_device_test.c`
- `tests/cmake/include_boundaries_test.cmake`
- `CMakeLists.txt`
- `work-units/wip/local_080/DEVICE_API_PRODUCTION_PATH.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | timer adapter periodic report sends HIDP input message through caller-provided sender | new | unit | no |
| todo | timer adapter subcommand reply sends HIDP input message through the same sender without changing reply holdoff behavior | regression | unit | no |
| todo | timer adapter shutdown neutral sends HIDP input message through the same sender and preserves pending retry behavior | regression | unit | no |
| todo | production backend wires report timer sender to `swbt_btstack_device_send` | new | integration | no |
| todo | no production path calls `swbt_btstack_hid_port_send_report` directly except the production device port implementation | regression | build/review | no |
| deferred | `open` / `listen` split for platform open vs incoming HID readiness | deferred | unit/integration | no |
| deferred | device event listener registration for typed BTstack HID events | deferred | unit/integration | no |

TDD status:

- source: user request, 2026-06-27。
- use case: production の report send path まで device API 経由にする。packet / event 抽象と `open` / `listen` 分割はこの work unit では扱わない。
- item: timer adapter periodic report sends HIDP input message through caller-provided sender。
- state: refactor-skipped。
- commands:
  - red: `just build-tests-debug`
    - result: expected fail。`swbt_btstack_input_report_timer_adapter_config_t` に `hid_sender` / `hid_sender_context` がなく、追加 test が compile error になった。
  - green: `just build-tests-debug`
    - result: pass。
  - green: `CTEST_ARGS="-R \"^btstack_input_report_timer_adapter_test$\" --output-on-failure" just test-debug`
    - result: pass。1/1 tests passed。
  - format: `just format`
    - result: pass。
  - verification after format: `CTEST_ARGS="-R \"^btstack_input_report_timer_adapter_test$\" --output-on-failure" just test-debug`
    - result: pass。1/1 tests passed。
  - refactor: skipped。
    - reason: 今の item は sender injection の最小 API と periodic path の観測だけを固定した。production wiring、reply、neutral は後続 item に残すため、この cycle で追加の構造変更を入れない。
- next red candidate: timer adapter subcommand reply sends HIDP input message through the same sender without changing reply holdoff behavior。

## 10. 検証

実行済み:

- `just build-tests-debug`
  - result: pass。sender injection 実装後に debug unit test target build が成功した。
- `CTEST_ARGS="-R \"^btstack_input_report_timer_adapter_test$\" --output-on-failure" just test-debug`
  - result: pass。1/1 tests passed。
- `just format`
  - result: pass。
- `CTEST_ARGS="-R \"^btstack_input_report_timer_adapter_test$\" --output-on-failure" just test-debug`
  - result: pass。formatter 後も 1/1 tests passed。

今後予定:

- targeted `just test-debug` with `btstack_input_report_timer_adapter_test|daemon_production_backend_test|btstack_device_test`
- `just format`
- `just debug`
- 変更範囲に応じて `just verify`

## 11. 実機実行条件

この work unit の通常範囲では実機検証を実行しない。理由は、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop を実行せず、unit / integration test の fake sender / fake device port で API 経路と送信委譲を確認するためである。

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

- 観測: `open` / `listen` 分割は、platform open と incoming HID readiness の語彙として有力だが、この work unit では扱わない。
  先送り理由: `swbt/runtime` の `hid_register` lifecycle と重なり、send path 整理より広い link lifecycle 設計になるため。
  次の置き場: link lifecycle API work unit または architecture spec。

- 観測: device event listener registration は、BTstack callback と device API の境界として有力だが、この work unit では扱わない。
  先送り理由: 実装範囲が packet dispatch、listener lifetime、同期処理時間、将来の queueing 判断へ広がり、production send path を device API に通す目的から外れやすい。
  次の置き場: `spec/dev-journal.md` の `2026-06-27: device event listener 方針の先送り` を source にして、必要になった時点で別 work unit record を作る。

## 13. チェックリスト

- [x] source と use case を記録した。
- [x] event listener 方針を dev-journal へ送る判断を記録した。
- [x] local_079 / local_082 後の前提で scope を見直した。
- [x] `open` / `listen` 分割を local_080 の対象外へ移した。
- [x] TDD Test List を send path に絞り直した。
- [ ] red test を追加した。
- [ ] timer adapter の sender 注入を実装した。
- [ ] production report send path を `swbt_btstack_device_send` 経由にした。
- [ ] targeted CTest を実行した。
- [ ] 実機未実行理由を維持または更新した。
- [ ] work unit record を更新した。
