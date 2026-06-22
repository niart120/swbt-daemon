# Daemon Application Boundary Rearchitecture

## 1. 概要

daemon の状態所有権を IPC session から application boundary へ寄せるための最初の work unit。

この work unit では、リアーキテクチャ全体を一度に実装しない。対象は、controller state と command metadata の分離、IPC 非依存の control lease 抽出、release / disconnect / heartbeat timeout の neutral 化規則を一つの session-side helper へ寄せる準備である。

## 2. 起点 / ユースケース

source:

- 2026-06-22 のユーザ要求。一時的な rearchitecture proposal と roadmap note を読み、妥当な内容を spec と work unit record へ取り込む。
- `spec/architecture/daemon-application-boundary-rearchitecture.md`。
- 作業開始時の code fact。`swbt_state_t` は `client_seq` を持ち、`ipc_session` は owner、state、rumble、mailbox、neutral 化を持っていた。
- 完了後の code fact。`swbt_state_t` は command sequence metadata を持たず、`swbt_ipc_status_t.last_seq` と `swbt_control_lease_t` が latest accepted sequence を持つ。
- `spec/architecture/daemon-runtime-boundaries.md`。現行 runtime boundary は current spec であり、実機到達済み経路を壊さない。

use case:

- actor: maintainer。
- 入力または状態: `set_state` command、owner lease、release、owner disconnect、heartbeat timeout、existing IPC status、Switch input report builder。
- 期待する観測結果: Switch report bytes は変えずに、controller state から配送メタデータを分離する。owner policy は IPC transport に依存しない小さな型で試験できる。既存 IPC JSON response と debug client の挙動は互換である。
- 制約: BTstack callback registration、HID send、report timer、production composition、実機 gate はこの work unit では変更しない。
- 対象外: bonded reconnect、status protocol v1、CMake target 分割、BTstack port interface 導入。

source から use case への変換:

一時提案文書の大きな phase plan は、そのまま roadmap として採用しない。まず現行 code で裏が取れた境界だけを、Switch-facing behavior を変えない小さな work unit にする。

## 3. 対象範囲

- 最新 `main` の include graph と、関係する call path を確認する。
- report builder が command sequence metadata に依存しないことを test で固定する。
- controller state と set-state command metadata の分離方針を実装する。
- owner ID、active/inactive、last accepted sequence、authorization を IPC 非依存の `control_lease` 相当へ抽出する。
- `ipc_session` は互換 wrapper として残し、existing IPC JSON tests を維持する。
- release、owner disconnect、heartbeat timeout が同じ neutral 化規則を通るよう整理する。
- 関連 spec と work unit record を更新する。

## 4. 対象外

- BTstack adapter の port interface 化。
- production backend ops table の分割。
- `swbt_core` の target 分割。
- daemon IPC wire protocol から `seq` を削除すること。
- status protocol v1 の field 追加。
- bonded reconnect、bond store、adapter recovery。
- 実機検証。

## 5. 関連 spec / docs

- `spec/architecture/daemon-application-boundary-rearchitecture.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/protocols/daemon-ipc-v1.md`
- `spec/protocols/switch-hid-core.md`
- `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`
- `work-units/complete/local_008/IPC_SESSION_CORE.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`
- `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md`
- `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md`
- `work-units/wip/local_051/DAEMON_APPLICATION_COMMAND_API.md`
- `work-units/wip/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`

## 6. 根拠監査

not applicable。

この work unit は Switch HID report bytes、subcommand bytes、SPI address、rumble packet、BTstack source selection、report period、WinUSB/libusb facts を追加しない。実装中に Switch-facing bytes、BTstack callback registration、timer behavior、HID send order を変更する必要が出た場合は、この work unit の範囲を閉じて別 work unit と根拠監査に分ける。

## 7. 設計メモ

- `seq` は IPC command metadata として残す。daemon protocol の `seq` 互換性は維持する。
- report generation は controller state のみを見る。metadata が違うだけの入力で report bytes が変わらないことを regression test にする。
- `control_lease` は socket、JSON、mailbox、rumble を知らない型にする。
- `ipc_session` は最初から削除しない。既存 tests と debug client を通しながら、内側の owner policy だけを移す。
- release、disconnect、heartbeat timeout は、最終的に reason 付き revoke 処理へ寄せる。ただし shutdown path と BTstack timer の呼び出し順はこの work unit で変えない。
- この work unit で残す compatibility wrapper は一時扱いである。削除または責務確定は `local_051` と `local_055` の完了条件に含める。

## 8. 対象ファイル

- `swbt/switch/switch_controller_state.*`
- `swbt/switch/switch_report.*`
- `swbt/application/control_lease.*`
- `swbt/ipc/ipc_session.*`
- `swbt/ipc/ipc_json.*`
- `swbt/daemon/runtime.*`
- `tests/switch_report_test.c`
- `tests/application_control_lease_test.c`
- `tests/ipc_session_test.c`
- `tests/ipc_json_test.c`
- `tests/daemon_runtime_test.c`
- `spec/architecture/daemon-application-boundary-rearchitecture.md`
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | report builder produces identical input report bytes when only command sequence metadata changes | regression | unit | no |
| green | set-state command keeps last sequence for status without making sequence part of controller state | new | unit | no |
| green | control lease acquire / release / not-owner checks do not include IPC session or JSON headers | new | unit | no |
| refactor-done | release, owner disconnect, and heartbeat timeout clear owner and publish neutral state through the same policy path | regression | unit | no |
| green | existing IPC JSON acquire / set_state / get_status / release responses remain wire-compatible | regression | unit | no |
| green | daemon runtime shutdown neutral behavior remains covered without changing production BTstack composition | regression | unit | no |

## 10. 検証

TDD status:

- source: 2026-06-22 のユーザ要求と `spec/architecture/daemon-application-boundary-rearchitecture.md`。
- use case: IPC `seq` を command metadata として扱い、controller state と Switch report bytes から切り離す。
- item: set-state command keeps last sequence for status without making sequence part of controller state。
- state: refactor-done。
- commands:
  - `just build-debug`: red。`swbt_ipc_status_t.last_seq` と `swbt_ipc_set_state(..., sequence)` が未実装で、`ipc_session_test.c` の build が期待どおり失敗した。
  - `just build-debug`: green。`swbt_state_t` から `client_seq` を削除し、`swbt_ipc_status_t.last_seq` と `swbt_control_lease_t` へ移した後に通過した。
  - `just test-debug`: green。33/33 tests passed。
  - `just test-debug`: refactor-done。neutral publish helper へ寄せた後も 33/33 tests passed。
  - `just format`: pass。
  - `just verify`: pass。format-check、clang-tidy、debug CTest、asan CTest、Windows MinGW cross build が通過した。
- notes: `application_control_lease_test` は `application/control_lease.h` だけを include し、IPC session、JSON、mailbox、rumble を要求しない。`ipc_session_test` は sequence だけを変えた同一 controller state から同一 report bytes が出ることを確認する。

検索確認:

- `rg -n "client_seq" swbt apps tests api`: no matches。
- `rg -n "#include .*\\b(ipc|json|mailbox|rumble|socket)\\b|ipc_|json_|mailbox|rumble|socket" swbt/application/control_lease.h swbt/application/control_lease.c`: no matches。

## 11. 実機実行条件

この work unit の通常範囲では実機検証は不要である。理由は、controller state の内部境界、owner policy、IPC JSON 互換性を unit / integration test で確認し、Bluetooth adapter open、Switch pairing、HID advertising、report loop を開始しないためである。

次のいずれかを変更する場合は、この work unit から切り出して実機 gate を再判断する。

- BTstack callback registration。
- HID send または can-send の順序。
- report timer / scheduling。
- production executable の composition。
- shutdown 時の HCI power-off と neutral send の呼び出し順。
- Switch-facing report bytes。

## 12. 先送り事項

- 観測: 一時 roadmap note の bonded reconnect、acceptance harness、lifecycle hardening、IPC / platform hardening、release / license boundary は有用な候補である。
  先送り理由: この work unit は application boundary の最初の構造変更に閉じるため、接続永続化、受入試験、release 条件を混ぜない。
  次の置き場: acceptance と compatibility cleanup は `work-units/wip/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`。bonded reconnect は `docs/status.md` の未確認項目から別 work unit を起こす。

- 観測: `swbt_core` target 分割は依存方向を強制するには有効である。
  先送り理由: source の所有者を狭める前に target を分けると、compatibility wrapper と既存 tests の移動が大きくなる。
  次の置き場: application boundary の first slice が通った後、CMake boundary work unit として起こす。

## 13. チェックリスト

- [x] include graph と call path を確認した。
- [x] red test を追加した。
- [x] controller state と command metadata を分離した。
- [x] control lease を IPC 非依存に抽出した。
- [x] IPC JSON 互換性を維持した。
- [x] release / disconnect / heartbeat timeout の neutral policy を整理した。
- [x] targeted CTest を実行した。
- [x] 実機未実行理由を確認した。
- [x] 残した compatibility wrapper の削除先を `local_051` または `local_055` に紐付けた。
