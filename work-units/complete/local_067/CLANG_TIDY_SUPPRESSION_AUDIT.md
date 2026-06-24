# Clang-Tidy Suppression Audit

## 1. 概要

この work unit は、clang-tidy の `NOLINT` 抑制を全量確認し、妥当な抑制と設計で消すべき抑制を分ける。

完了後は、clang-tidy が実行できない環境で tidy gate が成功扱いにならず、自前 API の隣接引数警告を `NOLINT` で固定しない状態にする。外部 ABI や C 標準 library fallback のために残す抑制には、理由をコード上に残す。

結果:

- 開始時の `NOLINT`: 68 行。
- 完了時の `NOLINT`: 30 行。
- 自前 API / helper の `bugprone-easily-swappable-parameters` 抑制は options struct 化で削除した。
- 残存抑制は BTstack callback ABI、POSIX feature test macro、境界付き整形、診断出力、テスト補助に限定した。

## 2. 起点 / ユースケース

source:

- User request: clang-tidy の警告を握りつぶしていた場面が複数あったため、work unit を立ち上げて全量監査し、妥当でない対処方針を修正する。
- Audit finding: `NOLINT` は `api/`, `apps/`, `swbt/`, `tests/` の C/C header に 68 行あり、`swbt/` 本番コード側に 37 行ある。

use case:

- actor: reviewer / maintainer。
- input/state: 現在の source tree にある全 `NOLINT`。
- expected observation: 抑制の理由が分類され、自前 API の設計負債は options struct などで警告自体を消し、外部 ABI など避けられない抑制は局所化と理由コメントで残る。
- constraints: BTstack callback ABI、POSIX feature test macro、C11 Annex K 非対応 toolchain への配慮は破らない。

source から use case への判断:

- `bugprone-easily-swappable-parameters` は、BTstack callback ABI や POSIX macro ではなく自前 API に出ている場合、抑制ではなく call shape を変える対象にする。
- `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling` は、`snprintf` / `fprintf` / `memset` の使用を全て禁止するのではなく、対象用途と境界確認を見て判断する。

## 3. 対象範囲

- `api/`, `apps/`, `swbt/`, `tests/` 配下の C/C header にある `NOLINT` を全量分類する。
- `SWBT_ENABLE_CLANG_TIDY=ON` で `clang-tidy` が見つからない場合を configure failure にする。
- 自前 API / helper の `bugprone-easily-swappable-parameters` 抑制を options struct などで削除する。
- 外部 ABI、POSIX feature test macro、C11 Annex K 非対応回避で残す抑制には理由を明記する。
- `vsnprintf` 抑制の周辺で見つかった JSON response string の未エスケープを、IPC codec の観測可能な挙動として修正する。

## 4. 対象外

- BTstack submodule の source 修正。
- `.clang-tidy` の check set 拡張。
- Switch protocol byte、report period、HID descriptor、subcommand、SPI、rumble の値変更。
- 実機 pairing、HID advertising、report loop の実行。

## 5. 関連 spec / docs

- `AGENTS.md`
- `spec/operations/development-tooling.md`
- `.clang-tidy`
- `cmake/compiler_warnings.cmake`

## 6. 根拠監査

not applicable。

この work unit は static analysis gate と C API shape を扱う。Switch HID report bytes、BTstack source selection、report period、subcommand、SPI、rumble、descriptor data、WinUSB/libusb の挙動は変更しない。

## 7. 設計メモ

### 抑制分類

| group | locations | current judgment | action |
|---|---:|---|---|
| POSIX feature test macro | 3 | reserved identifier だが POSIX API 有効化に必要 | keep, reason comment is present |
| BTstack callback ABI | 4 groups | 外部 ABI に合わせる必要がある | keep, narrow and add ABI reason |
| C formatting / diagnostics | 20 lines | Annex K 非対応 toolchain で代替がない箇所が多い | keep only where bounded or diagnostic, add reason where missing |
| test helper readability | 2 lines | test helper name で十分に区別されている | keep with existing reason |
| self-owned adjacent-parameter API/helper | 12 groups | `NOLINT` 固定は妥当でない | replace with options struct or equivalent |

修正対象の self-owned group:

- `apps/swbt-debug-client/debug_client.h` / `.c`: `send_set_state` と static parse / send helpers。
- `swbt/application/app.h` / `.c`: `swbt_app_set_state`, `swbt_app_revoke`。
- `swbt/application/control_lease.h` / `.c`: `swbt_control_lease_accept_sequence`。
- `swbt/core/metrics.h` / `.c`: `swbt_metrics_record_report_tick`。
- `swbt/ipc/ipc_server.h` / `.c`: `swbt_ipc_server_listen`。
- `swbt/ipc/ipc_json.c`: `swbt_json_find_key_value`。
- `swbt/btstack_bridge/input_report_scheduler.h` / `.c`: `swbt_btstack_input_report_scheduler_start`。
- `swbt/btstack_bridge/input_report_timer_adapter.c`: `notify_report_tick`。
- `swbt/btstack_bridge/output_report_handler.h` / `.c`: `swbt_btstack_output_report_handler_handle`。

### 実装結果

- `cmake/compiler_warnings.cmake` は `SWBT_ENABLE_CLANG_TIDY=ON` かつ `clang-tidy` 未検出時に `FATAL_ERROR` とする。
- self-owned group は options struct へ変え、呼び出し側の引数名を call site に残した。
- `swbt_ipc_json_encode_response` は raw output できない `request_id` / error message を拒否し、失敗時は response buffer を空にする。
- `NOLINTNEXTLINE` は clang-format で折り返されると無効になるため、長い理由は直前の通常コメントへ分離した。

## 8. 対象ファイル

- `cmake/compiler_warnings.cmake`
- `apps/swbt-debug-client/debug_client.h`
- `apps/swbt-debug-client/debug_client.c`
- `swbt/application/app.h`
- `swbt/application/app.c`
- `swbt/application/control_lease.h`
- `swbt/application/control_lease.c`
- `swbt/core/metrics.h`
- `swbt/core/metrics.c`
- `swbt/ipc/ipc_server.h`
- `swbt/ipc/ipc_server.c`
- `swbt/ipc/ipc_json.c`
- `swbt/btstack_bridge/input_report_scheduler.h`
- `swbt/btstack_bridge/input_report_scheduler.c`
- `swbt/btstack_bridge/input_report_timer_adapter.c`
- `swbt/btstack_bridge/output_report_handler.h`
- `swbt/btstack_bridge/output_report_handler.c`
- tests that call changed APIs

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | `SWBT_ENABLE_CLANG_TIDY=ON` で `clang-tidy` が見つからない場合、configure が warning ではなく失敗する | regression | build | no |
| done | request id に quote を含む IPC command は decode で拒否され、壊れた response JSON を生成しない | regression | unit | no |
| done | 自前 API の隣接引数抑制が options struct 化され、`just tidy` が `NOLINT` 追加なしで通る | regression | static | no |
| done | 外部 ABI と診断用途の残存抑制には理由コメントがあり、`just tidy` が通る | characterization | static | no |

## 10. 検証

- `just tidy`: pass。
- `just debug`: pass。CTest 39/39 passed。
- `just verify`: pass。format-check、clang-tidy、Debug/CTest、ASan、Windows MinGW cross build が通過。
- `rg -n "NOLINT" api apps swbt tests --glob "*.c" --glob "*.h"`: 30 行。残存箇所は分類済み。

## 11. 実機実行条件

実機不要。

Bluetooth adapter、Switch pairing、HID advertising、report loop を実行しない静的解析と単体テストの work unit である。

## 12. 先送り事項

なし。

## 13. チェックリスト

- [x] 全 `NOLINT` を再検索し、残存箇所の分類が record と一致する。
- [x] 自前 API / helper の妥当でない `bugprone-easily-swappable-parameters` 抑制を削除する。
- [x] 残す抑制に理由コメントを付ける。
- [x] `clang-tidy` 未検出時の tidy gate を fail-fast にする。
- [x] IPC request id の quote 混入に対する regression を追加する。
- [x] 検証結果と実機未実行理由を記録する。
