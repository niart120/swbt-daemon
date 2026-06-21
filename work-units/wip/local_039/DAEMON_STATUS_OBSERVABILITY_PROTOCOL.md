# Daemon Status Observability Protocol

## 1. 概要

daemon status と observability の公開範囲を定義する work unit。

現在の metrics / logging は in-process API と log sink であり、stable IPC metrics protocol ではない。この work unit では `get_status` の既存 `owner` / `state` / `rumble` schema を前提に、metrics や Switch connection state を追加するか、別 command に分けるかを決める。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_026/REPORT_METRICS_AND_LOGGING.md` の先送り事項。stable IPC metrics protocol はまだ定義しない。
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` の `get_status` 例と metrics 候補。
- `spec/protocols/daemon-ipc-v1.md` の未解決事項。metrics と Switch connection state を IPC status にどう載せるかは未定義である。
- `spec/architecture/daemon-runtime-boundaries.md`。metrics と logging は現時点では in-process API と log sink である。

use case:

- actor: debug client、maintainer、future observer client。
- 入力または状態: daemon runtime state、既存 `get_status` の owner / state / rumble schema、report metrics snapshot、hardware observation availability。
- 期待する観測結果: client が `get_status` または別 command で daemon diagnostics を確認できる。実機由来ではない fake metrics と、実機で測った adapter / driver / jitter を混同しない。
- 制約: state snapshot protocol を主経路に保つ。metrics export を入力 report ごとの event stream にしない。hardware field は未観測なら unavailable と表現する。
- 対象外: external telemetry backend、dashboard、GUI、hardware measurement itself。
- source から use case へ変換した判断: initial design の status 例をそのまま contract にせず、実装値、fake test 値、hardware observation の分類を先に固定する。

## 3. 対象範囲

- `get_status` の既存 stable fields との互換性を確認する。
- daemon runtime state、metrics、Switch connection state の公開可否を決める。
- metrics fields の名前、単位、unavailable state を定義する。
- fake timestamp 由来の metrics と hardware observation 由来の metrics を分ける。
- IPC JSON tests で status schema を固定する。
- debug client が status を表示するための最低限の contract を用意する。

## 4. 対象外

- external telemetry backend。
- dashboard / GUI。
- input report ごとの high-frequency event stream。
- Switch 実機での report rate measurement。
- adapter driver state の実測。
- authentication token と subscribe / event delivery。

## 5. 関連 spec / docs

- `spec/protocols/daemon-ipc-v1.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_026/REPORT_METRICS_AND_LOGGING.md`
- `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`

## 6. 根拠監査

not applicable。

この work unit は daemon IPC status schema と observability boundary を扱う。Switch HID report bytes、BTstack source selection、report period、WinUSB/libusb facts を追加しない。

hardware-derived values を公開する場合は、値そのものは `docs/hardware-test-log.md` の hardware observation に基づく。未観測値を measured value として返さない。

## 7. 設計メモ

- `get_status` は低頻度診断用の surface とする。
- metrics fields は単位を名前または schema で明示する。
- unavailable hardware fields は missing value ではなく explicit unavailable state とする。
- 既存 `rumble` schema は input state と分かれているため、metrics schema 設計で混ぜない。
- stable schema に入れる前に、debug client と IPC JSON tests の期待値を揃える。

## 8. 対象ファイル

- `swbt/ipc/ipc_json.*`
- `swbt/ipc/ipc_session.*`
- `swbt/core/metrics.*`
- `swbt/core/logging.*`
- `swbt/switch/switch_rumble.*`
- `tests/ipc_json_test.c`
- `tests/report_metrics_test.c`
- `spec/protocols/daemon-ipc-v1.md`
- `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | status schema returns daemon and owner state with stable field names | new | unit | no |
| todo | metrics fields include explicit units and unavailable hardware state | new | unit | no |
| todo | fake timestamp metrics are not labeled as hardware observations | regression | unit | no |
| todo | existing rumble status remains separate from controller input state when metrics fields are added | regression | unit | no |
| todo | debug client can display status without depending on unstable fields | integration | integration | no |

## 10. 検証

未実行。

この record は follow-up を作成しただけであり、code、CTest、実機コマンドは実行していない。

## 11. 実機実行条件

通常の status schema と IPC JSON test では実機検証は不要である。

adapter identity、driver state、actual report rate、jitter、disconnect / reconnect count を measured value として扱う場合は実機作業である。実行する場合は `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` の承認条件に従い、`docs/hardware-test-log.md` に記録する。

## 12. 先送り事項

- 観測: 実機由来 metrics の値はこの protocol-design work unit だけでは得られない。
  先送り理由: hardware observation が必要である。
  次の置き場: `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `docs/hardware-test-log.md`。

## 13. チェックリスト

- [ ] status schema の source / use case を確認した。
- [ ] red test を追加した。
- [ ] IPC status schema を実装した。
- [ ] metrics unavailable state を schema に反映した。
- [ ] debug client 表示との整合性を確認した。
- [ ] targeted CTest を実行した。
- [ ] 実機状態を記録した。
