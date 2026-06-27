# Device Lifecycle API

## 1. 概要

daemon production 経路から、Bluetooth HID device の lifecycle 操作を socket 風の小さい API へ切り出す。

この work unit では、daemon の application logic や IPC wire format を変更しない。対象は BTstack bridge 側の device handle と `open` / `connect` / `send` / `recv` / `close` の関数群を追加し、production backend が platform start、HID register、active reconnect、HID event decode、platform stop を直接組み立てないようにすることである。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-27: daemon とビジネスロジックが密接にくっついているように見える。socket と同じように device の `open` / `connect` / `send` / `recv` / `close` を独立して呼べる API を生やしたい。
- `spec/architecture/daemon-architecture-cutover.md`: host は lifecycle / composition を所有し、BTstack adapter は daemon / IPC internal type を参照しない。application は BTstack vendor header、socket、WinUSB / libusb、daemon host internal type へ依存しない。
- `work-units/complete/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md`: production adapter は能力別 port group へ分割済みだが、production backend には platform / HID / reconnect / event decode の手続きが残っている。

use case:

- actor: daemon production backend。
- 入力または状態: production adapter ports、HID registration config、HID packet callback、optional reconnect address、raw HID input message、BTstack packet bytes。
- 期待する観測結果: device handle に対して `open`、`connect`、`send`、`recv`、`close` を個別に呼べる。`open` は platform start と HID register を束ね、失敗時は開始済み platform を閉じる。`close` は HID stop と platform stop を一度だけ実行する。`recv` は BTstack packet を typed HID event に変換する。production backend は直接 `hid_event_decode` を呼ばず、device API 経由で受け取る。
- 制約: Switch-facing report bytes、report period、BTstack source selection、HID registration config の値、shutdown neutral ordering は変更しない。
- 対象外: public C ABI への公開、複数 controller、Joy-Con、NFC / IR、reconnect persistence、adapter removal / reinsertion recovery。

source から use case への変換:

ユーザの「ステートレス」は、内部状態を持たないことではなく、daemon startup の大きな手続きに埋もれた device 操作を関数単位で独立して呼べることと解釈する。実装では caller-owned `swbt_btstack_device_t` を handle として使い、関数ごとの事前条件と失敗結果を明確にする。

## 3. 対象範囲

- `swbt/btstack_bridge/` に device lifecycle API を追加する。
- `open` は platform start と HID device register を順序付きで実行し、register failure では platform stop まで戻す。
- `connect` は active reconnect request を transport port へ委譲する。
- `send` は raw HID interrupt message を transport port へ委譲する。
- `recv` は packet bytes を typed HID event へ変換する。
- `close` は HID stop と platform stop を idempotent に実行する。
- production backend の HID packet handler と active reconnect request を device API 経由に差し替える。
- 対象 API の unit test と production backend regression test を更新する。
- work unit record に TDD status、検証、実機未実行理由を記録する。

## 4. 対象外

- `api/swbt.h` の public C ABI 追加。
- IPC protocol の変更。
- daemon protocol としての `tap`、`duration_ms`、`sequence`、`at_ms`。
- Switch-facing report bytes、subcommand bytes、SPI、rumble packet の変更。
- BTstack source selection の変更。
- HID registration config 値の変更。
- HCI power-on / power-off ordering の変更。
- 実機検証。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/complete/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md`
- `work-units/complete/local_072/ACTIVE_SWITCH_RECONNECT.md`
- `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md`
- `docs/status.md`

## 6. 根拠監査

not applicable。

この work unit は Switch HID report bytes、subcommand bytes、SPI address、rumble packet、BTstack source selection、report period、WinUSB/libusb facts を追加または変更しない。BTstack の既存 `hid_device_connect`、HID register、interrupt send、packet decode への呼び出し境界を整理するだけである。

HID registration config の値、active reconnect PSM、report timer behavior、shutdown power-off order を変更する場合は、この work unit から切り出して根拠監査と実機 gate を再判断する。

## 7. 設計メモ

- `swbt_btstack_device_t` は caller-owned handle とする。heap allocation は使わない。
- API は internal BTstack bridge API であり、release 互換を約束する public C ABI ではない。
- `open` は platform と HID registration の lifecycle を束ねる。platform だけを開く API は今回追加しない。
- `send` は raw HID interrupt message をそのまま渡す。input report timer が作る `0xA1` 付き message の意味は変えない。
- `recv` は packet polling を行わず、callback で受け取った packet bytes を typed event へ変換する。BTstack run loop の駆動は従来どおり host / production backend 側に残す。
- production adapter の `platform` / `hid` / `active_reconnect` group は `device` group に統合する。これは旧 runtime / ops table の復活ではない。

Tidy status:

- classification: behavior change
- decision: tidy first
- reason: internal API と production composition の観測可能な call surface が変わる。先に caller-owned device API をテストで固定すると、production backend の差し替えを小さくできる。
- verification: red / green の targeted build と CTest で確認する。

## 8. 対象ファイル

- `swbt/btstack_bridge/device.*`
- `swbt/btstack_bridge/production_adapter.h`
- `swbt/btstack_bridge/production_btstack.c`
- `swbt/daemon/production_backend.*`
- `tests/btstack_device_test.c`
- `tests/daemon_production_backend_test.c`
- `CMakeLists.txt`
- `work-units/complete/local_079/DEVICE_LIFECYCLE_API.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | device open starts platform then registers HID and closes platform on register failure | new | unit | no |
| green | device connect forwards address and HID PSM request without daemon backend state | new | unit | no |
| green | device send forwards raw HID interrupt bytes without report mutation | new | unit | no |
| green | device recv converts BTstack packet bytes to typed HID event without production backend decoding directly | new | unit/integration | no |
| green | production backend uses device API for HID open / close and active reconnect while preserving startup and shutdown order | regression | integration | no |

TDD status:

- source: user request, 2026-06-27。
- use case: production backend から device lifecycle 操作を独立関数として呼べるようにする。
- red:
  - `just build-debug`: expected fail。`tests/btstack_device_test.c` が `btstack_bridge/device.h` を要求し、header が未実装のため compile error。
- green:
  - `just build-debug`: pass。`swbt_btstack_device_t` と `open` / `connect` / `send` / `recv` / `close` の実装後に build が通る。
  - `$env:CTEST_ARGS='-R "(btstack_device_test|daemon_production_backend_test|btstack_production_hci_dump_test)" --output-on-failure'; just test-debug`: pass、3/3。
  - `just format`: pass。
  - `just test-debug`: pass、47/47。
  - `just verify`: pass。format-check、clang-tidy、fresh debug CTest、ASan CTest、Windows MinGW cross build を含む。
- refactor: refactor-skipped。device API 境界自体が今回の構造変更であり、green 後に追加の抽象化を入れると work unit の範囲を広げるため。

## 10. 検証

- `just build-debug`: red では `btstack_bridge/device.h` 未実装で expected fail。実装後は pass。
- `$env:CTEST_ARGS='-R "(btstack_device_test|daemon_production_backend_test|btstack_production_hci_dump_test)" --output-on-failure'; just test-debug`: pass、3/3。
- `just format`: pass。
- `just test-debug`: pass、47/47。
- `just verify`: pass。初回は `tests/btstack_device_test.c` の BTstack packet handler fake に対する `bugprone-easily-swappable-parameters` で clang-tidy failure。BTstack packet handler ABI に限定した `NOLINTNEXTLINE` を追加し、再実行で pass。

## 11. 実機実行条件

この work unit の通常範囲では実機検証を実行しない。理由は、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop を実行せず、unit / integration test の fake port で device API と production ordering を確認するためである。

次のいずれかを変更する場合は、`hardware-harness` の承認境界に従い、専用 USB Bluetooth dongle、WinUSB、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、`docs/hardware-test-log.md` への記録を条件にする。

- HCI power-on / power-off ordering。
- HID registration config の値。
- report period または timer scheduling。
- Switch-facing input report bytes。
- pairing、active reconnect、advertising の実機 sequence。

## 12. 先送り事項

- 観測: device API を `api/swbt.h` の public C ABI として公開する可能性がある。
  先送り理由: 現時点では release artifact や外部 client contract が未定であり、先に daemon 内部境界として安定させる必要がある。
  次の置き場: public C ABI 化が必要になった時点で別 work unit record を作る。

- 観測: `recv` は polling API ではなく callback packet decode API である。
  先送り理由: BTstack run loop の所有権を変えると実機 sequence と shutdown ordering の範囲が広がる。
  次の置き場: run loop / device polling 境界を再設計する場合は architecture spec と別 work unit で扱う。

- 観測: `swbt_btstack_device_send` は追加済みだが、periodic report timer の production backend は既存の `swbt_btstack_input_report_timer_backend_btstack()` から直接 HID port へ送信している。
  先送り理由: `send_interrupt_message` callback は context を持たず、device handle 経由へ差し替えるには input report timer backend contract の変更が必要になる。
  次の置き場: report timer send path を device API に統合する場合は別 work unit record を作る。

## 13. チェックリスト

- [x] source と use case を記録した。
- [x] TDD Test List を作成した。
- [x] red test を追加した。
- [x] device lifecycle API を追加した。
- [x] production backend を device API 経由に差し替えた。
- [x] targeted CTest を実行した。
- [x] 実機未実行理由を記録した。
- [x] work unit record を更新した。
