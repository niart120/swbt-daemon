# Report Metrics And Logging

## 1. 概要

report loop、IPC update、daemon lifecycle の metrics と logging を追加する work unit。

initial design の metrics 名は候補として扱い、実装で記録する値と実機で観測した値を分ける。

この work unit は in-process counters と callback-based log sink を固定する。stable IPC metrics protocol、外部 telemetry backend、実機 report rate 判定は扱わない。

## 2. 起点 / ユースケース

source:

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` は report timer callback の擬似コードで `metrics_record_report_tick(monotonic_now_us())` を候補としている。
- `work-units/complete/local_023/BTSTACK_INPUT_REPORT_TIMER_ADAPTER.md` は fake BTstack timer と can-send callback の接続を追加した。
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md` は mailbox snapshot に coalesced update 数を追加した。
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md` は runtime lifecycle と cleanup 境界を追加した。

use case:

- actor: daemon runtime、report scheduler adapter、IPC state path。
- 入力または状態: fake monotonic timestamp、send result、accepted / rejected state update、mailbox coalesced count、startup / shutdown。
- 期待する観測結果: in-process metrics snapshot は report tick interval、send result、IPC state update count、hardware unavailable state を返す。log sink は startup / shutdown event を受け取り、実機観測なしの hardware field を unavailable として表す。
- 制約: 実機値と実装値を混ぜない。metrics 名は stable IPC protocol ではない。
- 対象外: 外部 telemetry、dashboard、report period tuning、実 report rate 判定。

source から use case への変換:

initial design の候補をそのまま公開 protocol にせず、C unit test で観測できる in-process API と log sink に落とす。report rate と jitter の実機値は未検証として保持し、fake timestamp から計算した interval metrics だけを implementation fact として扱う。

## 3. 対象範囲

- in-process metrics struct と update API を追加する。
- report tick interval と send result を fake clock unit test で記録する。
- IPC state update の accepted、rejected、coalesced count を記録する。
- startup log と shutdown log の sink を追加する。
- hardware-derived metrics は未実行として区別できる出力にする。

## 4. 対象外

- HID report 内への latency 計測データ埋め込み。
- 外部 telemetry backend。
- stable IPC metrics protocol。
- report period の tuning。
- Switch 実機での report rate 判定。
- GUI や dashboard。

## 5. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`
- `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md`
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md`
- `work-units/complete/local_023/BTSTACK_INPUT_REPORT_TIMER_ADAPTER.md`
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`
- `work-units/complete/local_025/DAEMON_RUNTIME_INTEGRATION.md`
- `docs/hardware-test-log.md`

## 6. 根拠監査

この work unit は Switch protocol bytes、BTstack source selection、HID descriptor 値、report layout を追加または変更しない。

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| metrics schema | in-process API only | design policy | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`, `tests/report_metrics_test.c` | tested |
| report interval metrics | fake clock only | implementation fact after tests | `tests/report_metrics_test.c` | tested |
| real report rate and jitter | 未記録 | hardware observation required | `docs/hardware-test-log.md` | not run |
| selected adapter and driver log fields | 未記録 | backend fact and hardware observation required | runtime backend, `docs/hardware-test-log.md` | pending |

`actual_report_rate_hz` や jitter を実機事実として断定しない。

startup log に出す adapter identity と driver state は backend 実装または実機記録で裏付ける。

metrics 名は initial design 由来の候補であり、stable protocol surface ではない。

## 7. 設計メモ

- metrics update API は runtime や scheduler から明示的に呼べる形にし、global mutable state に依存しない。
- logger は sink callback を受け取り、unit test では memory sink を使う。
- report interval は fake monotonic timestamp から計算し、real clock 精度はこの work unit で保証しない。
- state update coalescing は mailbox metadata から記録する。
- unavailable hardware fields は空文字ではなく unavailable state として表現する。
- runtime や scheduler への実接続は、実 adapter 由来の値を混ぜないためこの work unit では行わない。API は global mutable state に依存せず、後続 runtime integration から明示的に呼べる形にする。

## 8. 対象ファイル

- `CMakeLists.txt`
- `swbt/core/metrics.h`
- `swbt/core/metrics.c`
- `swbt/core/logging.h`
- `swbt/core/logging.c`
- `tests/report_metrics_test.c`
- `work-units/complete/local_026/REPORT_METRICS_AND_LOGGING.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | metrics init returns zero counters and unavailable hardware fields | new | unit | no |
| done | report tick events update interval average, max, and send counters from fake timestamps | new | unit | no |
| done | state update accepted, rejected, and coalesced events update separate counters | new | unit | no |
| done | startup and shutdown log sink emits configured fields without claiming hardware observations | new | unit | no |
| done | metrics snapshot does not mutate live counters | edge | unit | no |

TDD status:

- source: initial design の metrics candidate と runtime lifecycle / mailbox metadata。
- use case: fake timestamp と callback sink で metrics と log event を観測する。
- item: metrics snapshot does not mutate live counters。
- state: done。
- commands:
  - red: `just build-debug` は missing `core/logging.h` のため compile 失敗。
  - green: `just debug` pass。CTest 23/23。
  - refactor: clang-tidy の指摘に合わせ、report tick API の NOLINT を局所化し、test の enum 比較 helper と `SWBT_LOG_EVENT_NONE` を追加した。
  - final: `just verify` pass。format check、clang-tidy、linux debug tests、ASan/UBSan tests、Windows MinGW cross build が通った。

Test desiderata:

- purpose: report tick interval、send result、IPC state update count、startup / shutdown log event を fake timestamp と memory sink で観測する。
- key trade-offs: deterministic / fast / specific を優先した。runtime や scheduler へ自動接続しないことで、実装値と実機値の混同を避けた。
- risks: actual report rate、jitter、adapter identity、driver state、disconnect / reconnect は未検証である。
- action: 実機由来 field は unavailable state として保持し、後続の実機 bring-up で `docs/hardware-test-log.md` に記録する。

## 10. 検証

実行済み:

- red: `just build-debug` は missing `core/logging.h` のため compile 失敗。
- green: `just debug` pass、CTest 23/23。
- first verify: `just verify` は `bugprone-easily-swappable-parameters` で失敗。`swbt_metrics_record_report_tick` の timestamp / send result API に局所 NOLINT を追加した。
- second verify: `just verify` は enum 比較と zero 初期化に対する clang-tidy 指摘で失敗。enum 専用 helper と `SWBT_LOG_EVENT_NONE` を追加した。
- final verify: `just verify` pass。format check、clang-tidy、linux debug tests、ASan/UBSan tests、Windows MinGW cross build が通った。

## 11. 実機実行条件

通常の metrics unit test では実機検証は不要である。

real report rate、jitter、adapter identity、driver state、disconnect、reconnect を測る作業は実機作業として扱う。

実機作業はユーザの明示承認を必要とする。

実機作業は専用 USB Bluetooth ドングルを使う。

実機作業は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定して実行する。

実機結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、driver、BTstack commit、swbt commit、Switch firmware、report period、metrics、結果、cleanup を記録する。

この work unit では実機コマンドを実行していない。理由は、fake timestamp と memory sink を使う unit test だけを実行し、Bluetooth adapter、Switch pairing、HID advertising、report loop を開始していないためである。

## 12. 先送り事項

- 観測: actual report rate、jitter、adapter identity、driver state は fake timestamp からは証明できない。
  先送り理由: 実機実行条件と backend 実装が必要である。
  次の置き場: `docs/hardware-test-log.md` または実機 bring-up work unit。
- 観測: stable IPC metrics protocol はまだ定義しない。
  先送り理由: daemon protocol は state snapshot を主経路にしており、metrics export 方式は要件が固まっていない。
  次の置き場: 後続 observability work unit。

## 13. チェックリスト

- [x] work unit record を作成した。
- [x] work unit record を新形式へ更新した。
- [x] red を確認した。
- [x] metrics core を追加した。
- [x] logging sink を追加した。
- [x] unit test を追加した。
- [x] `just` 経由の検証を実行した。
- [x] sanitizer または cross build の必要性を判断した。
- [x] 実機状態を記録した。
