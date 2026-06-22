# Daemon Status Observability Protocol

## 1. 概要

daemon status と observability の公開範囲を定義する work unit。

現在の metrics / logging は in-process API と log sink であり、stable IPC metrics protocol ではない。この work unit では `get_status` の既存 `owner` / `state` / `rumble` schema を前提に、metrics や Switch connection state を追加するか、別 command に分けるかを決める。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_026/REPORT_METRICS_AND_LOGGING.md` の先送り事項。stable IPC metrics protocol はまだ定義しない。
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` の `get_status` 例と metrics 候補。
- `spec/protocols/daemon-ipc-v1.md` の当時の未解決事項。metrics と Switch connection state を IPC status にどう載せるかは未定義だった。
- `spec/architecture/daemon-runtime-boundaries.md`。metrics と logging は現時点では in-process API と log sink である。
- 2026-06-22 の一時 roadmap note review。実機 bring-up 後の status には production backend、hardware approval、adapter / Switch / HID channel state、report counters、last error を含める候補がある。ただし一時 note の work unit 番号は現行 repository と衝突するため採用しない。

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
- production backend、hardware approval、adapter state、Switch connection state、HID channel state を stable status に含めるか決める。
- report period、heartbeat timeout、reports sent、send failures、disconnects、last error の field 名と単位を決める。
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
- field 候補は `protocol_version`、`daemon_version`、`backend`、`lifecycle_state`、`hardware_approval`、`adapter_state`、`switch_connection_state`、`hid_channel_state`、`owner`、`report_period_us`、`heartbeat_timeout_ms`、`reports_sent_total`、`send_failures_total`、`disconnects_total`、`last_error` とする。
- `adapter_state`、`switch_connection_state`、`hid_channel_state` の値は、実装で観測できる state に合わせて絞る。単に見栄えのための state は追加しない。

## 8. 対象ファイル

- `swbt/ipc/ipc_json.*`
- `swbt/ipc/ipc_session.*`
- `swbt/core/metrics.*`
- `swbt/core/logging.*`
- `swbt/switch/switch_rumble.*`
- `tests/ipc_json_test.c`
- `tests/report_metrics_test.c`
- `spec/protocols/daemon-ipc-v1.md`
- `work-units/complete/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | status schema returns daemon and owner state with stable field names | new | unit | no |
| refactor-skipped | metrics fields include explicit units and unavailable hardware state | new | unit | no |
| refactor-skipped | fake timestamp metrics are not labeled as hardware observations | regression | unit | no |
| refactor-skipped | existing rumble status remains separate from controller input state when metrics fields are added | regression | unit | no |
| refactor-skipped | production status exposes backend and hardware approval state without implying measured hardware values | new | unit | no |
| refactor-skipped | adapter, Switch connection, and HID channel fields represent unavailable state explicitly on noop backend | new | unit | no |
| refactor-skipped | debug client can display status without depending on unstable fields | integration | integration | no |

## 10. 検証

TDD status:

- item: status schema returns daemon and owner state with stable field names
- state: refactor-skipped
- commands:
  - `$env:CTEST_ARGS='-R ipc_json_test'; just test-debug`
    - 既存 build を実行したため pass。red 判定には使わない。
  - `just build-debug`
    - pass。
  - `$env:CTEST_ARGS='-R ipc_json_test'; just test-debug`
    - expected failure。`get_status` response に `daemon.protocol_version` と `daemon.daemon_version` がまだないため `ipc_json_test` が失敗した。
  - `just build-debug`
    - pass。
  - `$env:CTEST_ARGS='-R ipc_json_test'; just test-debug`
    - pass。
- notes: `get_status` の既存 `owner` / `state` / `rumble` 互換は維持し、最初の cycle では metrics、backend、hardware state は追加しない。green 後の構造変更は、status response の request_id 有無で既存重複があるため大きくなりやすく、今回の item では refactor-skipped とする。

TDD status:

- item: metrics fields include explicit units and unavailable hardware state
- state: refactor-skipped
- commands:
  - `just build-debug`
    - pass。
  - `$env:CTEST_ARGS='-R ipc_json_test'; just test-debug`
    - expected failure。`get_status` response に `metrics.hardware_status` と unit suffix 付き counter fields がまだないため `ipc_json_test` が失敗した。
  - `just build-debug`
    - pass。
  - `$env:CTEST_ARGS='-R ipc_json_test'; just test-debug`
    - pass。
- notes: この cycle では metrics counter の runtime 接続は行わない。未観測 hardware-derived values は `hardware_status:"unavailable"` と zero value で公開し、実機測定値とは表現しない。green 後の構造変更は、次の cycle で metrics wiring の要否を見てから判断するため refactor-skipped とする。

TDD status:

- item: fake timestamp metrics are not labeled as hardware observations
- state: refactor-skipped
- commands:
  - `just build-debug`
    - expected failure。`ipc_session_test` が `swbt_ipc_record_report_tick` を呼ぶが、session API がまだないため implicit declaration で build が失敗した。
  - `just build-debug`
    - pass。
  - `$env:CTEST_ARGS='-R ipc_session_test'; just test-debug`
    - environment failure。Dev Container CLI が起動段階の `docker ps` で失敗し、CTest には到達しなかったため green 判定には使わない。
  - `just test-debug`
    - pass。40/40 tests passed。
- notes: fake timestamp で生成した report tick counters は status metrics に出すが、`hardware_status` は `unavailable` のままにし、`actual_report_rate_hz` と `jitter_max_us` を実機観測値として扱わない。green 後の構造変更は、session から metrics API へ委譲する薄い追加に留まっており、今回の item では refactor-skipped とする。

TDD status:

- item: production status exposes backend and hardware approval state without implying measured hardware values
- state: refactor-skipped
- commands:
  - `just build-debug`
    - expected failure。`daemon_production_backend_test` が production run loop 中の `swbt_ipc_status_t.daemon` を確認するが、daemon metadata field と enum がまだないため build が失敗した。
  - `just build-debug`
    - pass。
  - `just test-debug`
    - pass。40/40 tests passed。
- notes: production backend と hardware approval は status に出すが、`metrics.hardware_status` は実機測定を追加するまで `unavailable` のままにする。green 後の構造変更は、runtime から session metadata へ渡す薄い経路と JSON 文字列化で完結しており、既存 response duplication の解消は別 item の範囲に膨らむため refactor-skipped とする。

TDD status:

- item: adapter, Switch connection, and HID channel fields represent unavailable state explicitly on noop backend
- state: refactor-skipped
- commands:
  - `just build-debug`
    - expected failure。`daemon_runtime_test` が noop runtime status の `status.hardware.adapter_state`、`switch_connection_state`、`hid_channel_state` を確認するが、hardware channel metadata field と enum がまだないため build が失敗した。
  - `just build-debug`
    - pass。
  - `just test-debug`
    - pass。40/40 tests passed。
- notes: noop backend は adapter、Switch connection、HID channel を実機観測できないため、missing field ではなく explicit `unavailable` として公開する。green 後の構造変更は、現時点の state が `unavailable` のみで抽象化余地が小さいため refactor-skipped とする。

TDD status:

- item: existing rumble status remains separate from controller input state when metrics fields are added
- state: refactor-skipped
- commands:
  - `just test-debug`
    - pass。40/40 tests passed。
- notes: `ipc_json_test` は metrics と hardware object を追加した後の同じ status response で、`state` と `rumble` が別 object として残ることを確認している。この item は前段の schema 拡張で既に守られており、追加の実装や構造変更は不要なため refactor-skipped とする。

TDD status:

- item: debug client can display status without depending on unstable fields
- state: refactor-skipped
- commands:
  - `just build-debug`
    - pass。
  - `just test-debug`
    - pass。40/40 tests passed。
- notes: `debug_ipc_client_test` の fake status response を daemon / metrics / hardware object を含む新 schema に拡張した。debug client は status body の unstable fields を parse せず、error 判定後に response をそのまま表示するため、client code の変更は不要である。

Final verification:

- self-review finding:
  - `get_status` schema 拡張により、最大桁数に近い status response が旧 `SWBT_IPC_JSON_RESPONSE_MAX=1024` を超える可能性があった。`SWBT_IPC_JSON_RESPONSE_MAX` を 2048 に拡張し、`ipc_json_test` に large status response の buffer regression test を追加した。
- `just format`
  - pass。
- `just build-debug`
  - pass。
- `just test-debug`
  - pass。40/40 tests passed。
- `just verify`
  - first rerun: fail。large status response test で使った `memset` が clang-tidy analyzer の insecure API 指摘を受けたため、固定長 loop に置き換えた。
  - final rerun: pass。format-check、clang-tidy、linux-debug、linux-asan、windows-mingw-debug を確認した。

## 11. 実機実行条件

通常の status schema と IPC JSON test では実機検証は不要である。

adapter identity、driver state、actual report rate、jitter、disconnect / reconnect count を measured value として扱う場合は実機作業である。実行する場合は `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` の承認条件に従い、`docs/hardware-test-log.md` に記録する。

## 12. 先送り事項

- 観測: 実機由来 metrics の値はこの protocol-design work unit だけでは得られない。
  先送り理由: hardware observation が必要である。
  次の置き場: `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `docs/hardware-test-log.md`。

## 13. チェックリスト

- [x] status schema の source / use case を確認した。
- [x] red test を追加した。
- [x] IPC status schema を実装した。
- [x] metrics unavailable state を schema に反映した。
- [x] debug client 表示との整合性を確認した。
- [x] targeted CTest を実行した。
- [x] 実機状態を記録した。
