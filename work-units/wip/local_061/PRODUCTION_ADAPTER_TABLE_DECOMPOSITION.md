# Production Adapter Table Decomposition

## 1. 概要

`swbt_btstack_production_adapter_t` の広い function table を、責務ごとの小さい port に分ける work unit。

`local_056` では旧 production backend ops table を削除し、production-facing 能力を `swbt_btstack_production_adapter_t` へ移した。`local_058` では、この adapter table が広くなっている点を先送り事項として残した。

この work unit では、旧 table を復活させず、supervisor、BTstack adapter、IPC reactor integration、clock / power / run loop の境界を見直す。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md` の先送り事項: production adapter の広い function table を分割する。
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md` の production backend ops table 削除。
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md` の kept inventory。履歴として参照し、現行判断は `local_056` を優先する。
- `spec/architecture/daemon-architecture-cutover.md` の production composition 方針。
- user follow-up, 2026-06-23: adapter table 分割でコードベースが増えるだけの再抽象化にならないよう、削除条件と差分の上限感を先に持つ。

use case:

- actor: maintainer、reviewer、future platform adapter。
- 入力または状態: production daemon startup、IPC pump、BTstack run loop、HID send、shutdown neutral、power-off。
- 期待する観測結果: production composition が能力ごとの port を受け取り、test double が必要な能力だけを実装できる。広い table を別名の広い table へ置き換えない。shutdown neutral failure cleanup は維持される。
- 制約: 旧 runtime / session / ops table を復活させない。実機-facing scheduling を変える場合は hardware gate を判定する。
- 対象外: bonded reconnect、release packaging、Windows native CI。

source から use case への変換:

`swbt_btstack_production_adapter_t` は cutover を成立させるための現行境界である。後続では、全能力を一つの struct に集める必要があるかを検証し、小さい port へ分ける。

## 3. 対象範囲

- `swbt_btstack_production_adapter_t` の field と呼び出し元を棚卸しする。
- shutdown neutral、IPC pump、power-off、timer / can-send、clock の責務を分類する。
- fake production test が必要な能力だけを差し替えられる設計にする。
- 旧 production backend ops table と同じ広い rollback point を作らない。
- composition 変更が H1 再実行条件に該当するか判断する。
- 分割前後で、adapter struct の field 数、fake が実装する callback 数、test helper の行数を比較する。
- 新しい port を追加する場合は、削除する field、削除する fake callback、または縮小する call surface と対応付ける。
- 既存 table を薄く包むだけの wrapper は完了条件に含めない。残す場合は current responsibility と削除条件を record に書く。

## 4. 対象外

- Switch-facing report bytes の変更。
- BTstack source selection の変更。
- bonded reconnect と bond store。
- parser fuzz、slow client hardening。
- release artifact 作成。
- 1 callback 1 file のような、責務分離より file 数増加が主目的になる分割。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`
- `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md`
- `docs/status.md`

## 6. 根拠監査

条件付き。

adapter table の構造だけを変える場合は not applicable。BTstack callback registration、HID send scheduling、timer behavior、shutdown power-off order を変える場合は、source fact または実機 H1 再実行条件を記録する。

## 7. 設計メモ

- `swbt_btstack_production_adapter_t` を分ける目的は test double と責務境界の明確化であり、旧 runtime abstraction の復活ではない。
- production host は lifecycle ordering を所有し続ける。
- BTstack run loop を system reactor として使う判断を変える場合は、architecture spec の更新対象にする。
- 分割後も shutdown neutral pending failure は power-off / run-loop exit へ進む必要がある。
- 「広い struct を小さい struct の束へ分けたが、全 call site が常に全 struct を受け取る」状態は改善と扱わない。
- 完了時は、追加した type / file / test helper と、削除または縮小した field / helper / dependency を並べて記録する。

## 8. 対象ファイル

- `swbt/btstack_bridge/production_adapter.h`
- `swbt/btstack_bridge/production_btstack.c`
- `swbt/daemon/production_backend.*`
- `swbt/daemon/host.*`
- `tests/daemon_production_backend_test.c`
- `tests/architecture_journey_test.c`
- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/wip/local_061/PRODUCTION_ADAPTER_TABLE_DECOMPOSITION.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | fake production test can provide IPC pump ability without implementing unrelated BTstack abilities | new | unit | no |
| green | shutdown neutral failure cleanup still reaches power-off after adapter decomposition | regression | integration | no |
| refactor-skipped | production host lifecycle uses explicit port groups instead of one wide table | new | integration | no |
| green | old production backend ops table symbols remain absent | regression | build | no |
| green | hardware gate need is recorded if composition or scheduling changes | characterization | docs | yes |
| green | decomposition reduces or justifies the production adapter call surface instead of wrapping it with another broad layer | verification | docs/integration | no |

## 10. 検証

TDD status:

- source: `local_058` の先送り事項。
- use case: fake production test が IPC pump start / stop だけを実装し、platform、HID、timer、power、run loop の callback を持たなくても IPC pump port を直接起動できる。
- item: fake production test can provide IPC pump ability without implementing unrelated BTstack abilities。
- state: refactor-skipped。
- commands:
  - red: `just build-debug` は `unknown type name 'swbt_btstack_production_ipc_pump_port_t'` と `swbt_btstack_production_adapter_t has no member named 'ipc_pump'` で fail。
  - green: `just build-debug` pass。
  - affected checks: `just test-debug` pass。`daemon_production_backend_test` はこの実行に含まれる。
  - format: `scripts/format.sh` pass。
- notes: `swbt_btstack_production_ipc_pump_port_t` を追加し、adapter の IPC pump callback 2 個を `.ipc_pump.start` / `.ipc_pump.stop` へ移した。`swbt_daemon_production_backend_init` は IPC pump port だけを要求し、実機承認後の production main は従来どおり full adapter callback を要求する。Switch-facing bytes、BTstack source selection、timer scheduling、shutdown power-off order は変更していない。
- refactor: green 後の追加構造変更は行わなかった。この item の構造変更は IPC pump port の導入に閉じる。

TDD status:

- source: `local_058` の shutdown neutral failure cleanup と、この work unit の adapter 分割後維持条件。
- use case: shutdown neutral send が pending になり、次の can-send 処理で失敗しても、daemon は power-off と run-loop exit へ進む。
- item: shutdown neutral failure cleanup still reaches power-off after adapter decomposition。
- state: green。
- commands:
  - green: `just test-debug` pass。`daemon_production_backend_test` の `pending_stop_request_finishes_after_failed_can_send_event` はこの実行に含まれる。
  - targeted attempt: `$env:CTEST_ARGS='-R daemon_production_backend_test --output-on-failure'; just test-debug` は Dev Container setup で fail したため、この item の根拠にしない。
- notes: この item では production adapter の追加分割は行わない。item 1 の IPC pump port 導入後も、shutdown neutral failure cleanup は既存 regression test で power-off 1 回、run-loop exit 1 回として観測できる。
- refactor: 変更なし。既存 test の green 確認のみ。

TDD status:

- source: `local_058` の production adapter table 分割先送り事項。
- use case: production host lifecycle が platform、HID、output handler、report timer、controller、clock、power、run loop を明示 port group として呼び出し、1 つの広い top-level callback table に依存しない。
- item: production host lifecycle uses explicit port groups instead of one wide table。
- state: refactor-skipped。
- commands:
  - red: `just build-debug` は `unknown type name 'swbt_btstack_production_platform_port_t'` と、`swbt_btstack_production_adapter_t` に `platform` / `hid` / `output_handler` / `report_timer` / `controller` / `clock` / `power` / `run_loop` がないことで fail。
  - green: `just build-debug` pass。
  - affected checks: `just test-debug` pass。`daemon_production_backend_test` と `btstack_production_hci_dump_test` はこの実行に含まれる。
  - format: `scripts/format.sh` pass。`scripts/check-format.sh` pass。
- notes: `swbt_btstack_production_adapter_t` の top-level は `ipc_pump`、`platform`、`hid`、`output_handler`、`report_timer`、`controller`、`clock`、`power`、`run_loop` の 9 group になった。production host backend の処理順序は変えず、呼び出し先だけを group field へ移した。HCI dump test は platform port 経由の起動失敗確認に更新した。
- refactor: green 後の追加構造変更は行わなかった。今回の構造変更は adapter field の group 化に閉じる。

TDD status:

- source: `local_056` の production backend ops table 削除条件。
- use case: adapter 分割作業中に、旧 `production_backend_ops` header や `swbt_daemon_production_backend_ops_t` を rollback point として戻していない。
- item: old production backend ops table symbols remain absent。
- state: green。
- commands:
  - green: `rg -n "production_backend_ops|swbt_daemon_production_backend_ops_t" swbt apps api tests CMakeLists.txt` は no matches。
  - green: `Test-Path swbt/daemon/production_backend_ops.h` は `False`。
  - green: `just test-debug` pass。`architecture_absence_cmake_test` はこの実行に含まれる。
- notes: `tests/cmake/architecture_absence_test.cmake` は `production_backend_` と `ops` を split token で組み立て、source / tests / build graph から旧 token を検出する。今回の adapter group 化ではこの absence test を変更していない。
- refactor: 変更なし。既存 absence gate の確認のみ。

TDD status:

- source: この record の実機実行条件と `local_057` H1 条件。
- use case: adapter table の分割が Switch-facing bytes、BTstack callback registration、timer scheduling、shutdown power-off order を変えたかを判定し、必要なら H1 再実行条件を記録する。
- item: hardware gate need is recorded if composition or scheduling changes。
- state: green。
- commands:
  - green: `git diff --name-only main..HEAD` で変更対象を確認。
  - green: `git diff main..HEAD -- swbt/daemon/production_backend.c swbt/btstack_bridge/production_btstack.c` で production-facing 差分を確認。
  - green: `just test-debug` pass。
- notes: `production_btstack.c` の差分は adapter 初期化子の group 化に閉じる。`production_backend.c` は port validation と callback field 経由の呼び出し先を変更したが、host start、HID register、timer init、power-on、run loop、shutdown neutral、power-off、host stop の順序は変えていない。Switch-facing bytes、BTstack source selection、timer period、BTstack timer scheduling、HID packet handler registration の意味は変えていないため、H1 実機再実行は不要。
- refactor: 変更なし。hardware gate 判定の記録のみ。

TDD status:

- source: user follow-up, 2026-06-23: adapter table 分割でコードベースが増えるだけの再抽象化にならないよう、削除条件と差分の上限感を先に持つ。
- use case: production adapter 分割が広い table を別名の広い wrapper へ置き換えるだけではなく、top-level call surface を縮小し、残す leaf callback を hardware-facing 能力として説明できる。
- item: decomposition reduces or justifies the production adapter call surface instead of wrapping it with another broad layer。
- state: green。
- commands:
  - green: `git show main:swbt/btstack_bridge/production_adapter.h` で分割前の adapter field を確認。
  - green: `Get-Content swbt/btstack_bridge/production_adapter.h` と `rg -n "typedef struct \\{|swbt_btstack_production_.*port_t|swbt_btstack_production_adapter_t|\\(\\*" swbt/btstack_bridge/production_adapter.h` で分割後の type と field を確認。
  - green: `rg -n "fake_.*_port|fake_backend_adapter|fake_ipc_pump_only_adapter" tests/daemon_production_backend_test.c` で fake helper の構成を確認。
  - green: `git diff --numstat main..HEAD` と `git diff --stat main..HEAD` で差分規模を確認。
- notes: 分割前の `swbt_btstack_production_adapter_t` は top-level に 22 callback field を持っていた。分割後の top-level は `ipc_pump`、`platform`、`hid`、`output_handler`、`report_timer`、`controller`、`clock`、`power`、`run_loop` の 9 group field である。leaf callback 数は 22 のまま残した。これは hardware-facing 能力自体を削除できないためであり、広い rollback table を復活させたわけではない。`backend_init` は IPC pump port だけで初期化できるため、test double は必要な能力だけを実装できる。
- diff: code files は `production_adapter.h` `+62/-29`、`production_btstack.c` `+50/-22`、`production_backend.c` `+97/-43`、`btstack_production_hci_dump_test.c` `+2/-2`、`daemon_production_backend_test.c` `+108/-22`。追加 file はない。
- refactor: 変更なし。完了判定の記録のみ。

## 11. 実機実行条件

software-only 分割で Switch-facing bytes、BTstack initialization、callback registration、timer / send scheduling、shutdown 実機順序を変えない場合は実機不要。

これらを変更する場合は `local_057` と同等の H1 条件を再評価する。実行する場合は専用 USB Bluetooth dongle、WinUSB、`SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、`docs/hardware-test-log.md` への記録を条件にする。

## 12. 先送り事項

none。起票時点の先送り事項は、この record の source として取り込んだ。

## 13. チェックリスト

- [x] source を `local_056` と `local_058` から特定した。
- [x] use case を production adapter table decomposition として定義した。
- [x] adapter table field を棚卸しした。
- [x] red test を追加した。
- [x] green 実装を行った。
- [x] hardware gate の要否を判定した。
- [x] `just debug` または targeted CTest を実行した。
- [x] 分割前後の field 数、fake callback 数、test helper 量を比較した。
- [x] 追加した port と削除または縮小した call surface を対応付けた。
