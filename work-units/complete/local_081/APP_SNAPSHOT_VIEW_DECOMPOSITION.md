# App Snapshot View Decomposition

## 1. 概要

`swbt_app_snapshot_t` が report scheduler、IPC status、rumble/status 検証の複数用途を 1 つの構造体で支えている状態を見直す。

この work unit では、すぐに `swbt_state_t` を分割しない。まず `swbt_app_snapshot_t` の利用者を分類し、report scheduler が必要とする controller state、IPC status が必要とする owner / state / rumble / metrics / daemon / hardware、rumble 検証が必要とする state を分けて読める API が必要かを判定する。

`local_079` / `local_080` の device API 分割は BTstack HID device lifecycle の境界であり、application snapshot の責務分割とは別問題として扱う。この work unit の完了後に、`local_080` の実装を続けるか、先に app snapshot 分割実装 work unit を起こすかを判断する。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-27: `state snapshot` 自体が太りすぎており、`local_079` / `local_080` の分割の正当性も分からなくなってきた。先に `swbt_app_snapshot_t` の分割検討を済ませたい。
- `swbt/application/app.h`: `swbt_app_snapshot_t` は owner、last sequence、controller state、rumble、metrics、daemon status、hardware status をまとめて返す。
- `swbt/daemon/host.c`: report scheduler の state provider は `swbt_app_snapshot` を呼び、`snapshot.state` だけを返す。
- `swbt/ipc/ipc_adapter.c`: IPC status は `swbt_app_snapshot` から owner、sequence、state、rumble、metrics、daemon、hardware をすべて copy する。
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`: controller state と command metadata は分離済みで、report generation は controller state のみを見る方針を持つ。
- `spec/architecture/daemon-architecture-cutover.md`: controller state、control lease、rumble、SPI、player lights、logical link state、report scheduling policy は application だけが更新する。

use case:

- actor: architecture reviewer / implementation planner。
- 入力または状態: `swbt_app_snapshot_t` の定義、`swbt_app_snapshot` call site、IPC status contract、report scheduler state provider、rumble/status test。
- 期待する観測結果:
  - `swbt_app_snapshot_t` の利用者を用途別に分類できる。
  - `swbt_state_t` 自体を分割すべきか、`swbt_app_snapshot_t` を view 別 API に分ければ足りるかを判断できる。
  - report scheduler が full app snapshot に依存しない API 形を提案できる。
  - IPC status の wire format を変えずに、status view を取得する API 形を提案できる。
  - `local_080` の device send path work unit を続行してよいか、app snapshot 分割を先に実装すべきかを判断できる。
- 制約: IPC wire format、Switch-facing report bytes、report period、BTstack device API は変更しない。
- 対象外: `swbt_app_snapshot_t` 分割の実装、IPC JSON contract の変更、`swbt_state_t` field の削除、device API の追加実装。

source から use case への変換:

疑問点は、device API の妥当性だけでは解けない。現行 code では BTstack device boundary と application snapshot boundary が別々の結合点になっている。`local_079` / `local_080` の継続可否を判断する前に、application snapshot の読み手ごとの責務を分ける必要がある。

## 3. 対象範囲

- `swbt_app_snapshot_t` の field と call site を分類する。
- report scheduler が必要な controller state view を定義する。
- IPC status が必要な status view を定義する。
- rumble / metrics / daemon / hardware status の取得責務を status view に含めるか、個別 view に分けるかを判断する。
- `swbt_state_t` を分割する必要があるか、現時点では不要かを判断する。
- `local_080` の production send path work unit を続行してよい条件を記録する。
- 実装 work unit が必要な場合、後続候補を先送り事項に記録する。

## 4. 対象外

- `swbt_app_snapshot_t` 分割の実装。
- `swbt_state_t` の field 追加、削除、構造変更。
- IPC JSON wire format の変更。
- daemon protocol としての partial update、button tap、hold duration、future scheduling。
- Switch-facing report bytes、report period、BTstack source selection の変更。
- `local_080` の device API / report send path 実装。
- 実機検証。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `spec/protocols/daemon-ipc-v1.md`
- `spec/operations/work-unit-spec-tdd-flow.md`
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`
- `work-units/complete/local_079/DEVICE_LIFECYCLE_API.md`
- `work-units/wip/local_080/DEVICE_API_PRODUCTION_PATH.md`

## 6. 根拠監査

not applicable。

この work unit は Switch HID report bytes、subcommand bytes、SPI address、rumble packet、BTstack source selection、report period、WinUSB/libusb facts を追加または変更しない。対象は application read API の責務分類である。

`swbt_state_t` の field semantics、Switch report packing、IPC wire field semantics を変更する場合は、この work unit から切り出し、必要に応じて `source-audit` と protocol spec 更新を判断する。

## 7. 設計メモ

### 7.1 現時点の観測

- `swbt_state_t` は buttons、sticks、accelerometer、gyro だけを持つ。command sequence metadata は `local_050` で外れている。
- `swbt_app_snapshot_t` は controller state だけでなく owner、sequence、rumble、metrics、daemon status、hardware status を同時に持つ。
- report scheduler は `snapshot.state` だけを使う。
- IPC status は `swbt_app_snapshot_t` のほぼ全体を status response へ写す。
- rumble / output report 関連 test は `snapshot.rumble` を読む。

### 7.2 Call site 分類

| caller | role | required fields | 判断 |
|---|---|---|---|
| `swbt/daemon/host.c` `swbt_daemon_host_read_state` | report scheduler state provider | `state` only | full app snapshot へ依存させる理由はない。controller state 専用 read API に寄せる。 |
| `swbt/ipc/ipc_adapter.c` `swbt_ipc_adapter_get_status` | IPC `get_status` response source | owner、last sequence、state、rumble、metrics、daemon、hardware | IPC status view として一括取得する。wire format は変えない。 |
| `tests/application_command_test.c` | app lease / state / metrics behavior | owner、last sequence、state、metrics | production caller ではなく app contract の検証。後続実装では status view か用途別 helper に置き換える。 |
| `tests/daemon_host_test.c` | host lifecycle / state provider / rumble / status behavior | state、rumble、daemon、hardware | 1 つの test file 内でも用途が分かれている。state provider は controller state view、rumble/status は status view を使う。 |
| `tests/daemon_ipc_runner_test.c` | IPC runner が app state を更新または neutralize することの検証 | mostly `state`; 一部 owner/response は IPC 経由 | app 内部確認は controller state view で足りる箇所が多い。wire response は既存 debug client assertion を残す。 |
| `tests/btstack_output_report_handler_test.c` | output report が rumble status を更新することの検証 | `rumble` only | production caller ではない。status view で読むか、後続で test helper / rumble view を検討する。 |

### 7.3 採用判断

- すぐに分割すべき候補は `swbt_state_t` ではなく `swbt_app_snapshot_t` の read API である。
- `swbt_state_t` は現時点で buttons、sticks、IMU を含む latest controller state の単位として成立している。daemon IPC の `set_state` も full controller state snapshot を受け取る contract なので、buttons / sticks / IMU をここで分割する根拠は弱い。
- report scheduler 向けには `swbt_app_read_controller_state(const swbt_app_t *, swbt_state_t *)` のような controller-only read API があると、status surface への依存を切れる。
- IPC status 向けには wire format を維持するための status view が必要であり、`swbt_app_status_snapshot_t` と `swbt_app_read_status(const swbt_app_t *, swbt_app_status_snapshot_t *)` のような型と関数が候補になる。
- rumble は IPC status で公開済みなので、最初は status view に含める。output report / diagnostics 専用の rumble read API は、production caller が出るまで増やさない。
- metrics、daemon、hardware は low-frequency diagnostics として status view に含める。report scheduler の state read path には入れない。
- `swbt_app_snapshot_t` は現在の名前だと用途が広すぎる。後続実装では status view へ改名するか、互換 wrapper として一時的に残して移行する。

### 7.4 `local_080` への影響

`local_080` の device send path は BTstack HID device boundary の問題であり、`swbt_app_snapshot_t` の分割実装そのものを完了条件にしない。ただし、`local_080` で report scheduler / host state provider に触る場合、先に controller-only read API を入れる方が責務境界は明確になる。

このため、推奨順序は次の通りである。

1. `local_081` で分割方針を閉じる。
2. 必要なら別 work unit で `swbt_app_read_controller_state` と status view を実装する。
3. `local_080` の production send path を再開する。

`local_080` をすぐ再開してよい条件は、変更範囲を BTstack device send path に閉じ、`swbt_app_snapshot_t` の利用拡大や IPC status contract 変更を混ぜないことである。

## 8. 対象ファイル

- `swbt/application/app.*`
- `swbt/daemon/host.c`
- `swbt/ipc/ipc_adapter.c`
- `tests/application_command_test.c`
- `tests/daemon_host_test.c`
- `tests/daemon_ipc_runner_test.c`
- `tests/btstack_output_report_handler_test.c`
- `spec/architecture/daemon-architecture-cutover.md`
- `spec/protocols/daemon-ipc-v1.md`
- `work-units/wip/local_081/APP_SNAPSHOT_VIEW_DECOMPOSITION.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | app snapshot call sites are classified by required fields and caller role | characterization | review | no |
| green | report scheduler state provider can be described as needing controller state only | characterization | unit/review | no |
| green | IPC status can preserve existing wire fields through a status view without depending on report scheduler view | characterization | integration/review | no |
| green | rumble and metrics consumers are classified as status view or separate view candidates | characterization | review | no |
| green | decision records whether swbt_state_t must change or app snapshot view APIs are sufficient | decision | review | no |
| green | decision records whether local_080 can proceed before app snapshot split implementation | decision | review | no |

TDD status:

- source: user request, 2026-06-27。
- use case: `swbt_app_snapshot_t` の分割検討を先に済ませ、device API work の前提を整理する。
- state: green。
- refactor: skipped。文書上の設計判断 work unit であり、実装差分は後続 work unit に分ける。

## 10. 検証

実行:

- `rg -n "swbt_app_snapshot_t|swbt_app_snapshot\\(" swbt tests spec work-units`
  - result: `swbt/daemon/host.c`、`swbt/ipc/ipc_adapter.c`、関連 tests、architecture spec、`local_081` record の利用箇所を確認した。
- `rg -n -C 4 "swbt_app_snapshot|snapshot\\." swbt/daemon/host.c swbt/ipc/ipc_adapter.c`
  - result: production caller は report scheduler state provider と IPC status adapter の 2 系統であることを確認した。
- `rg -n -C 3 "swbt_app_snapshot|snapshot\\.|status\\." tests/application_command_test.c tests/daemon_host_test.c tests/daemon_ipc_runner_test.c tests/btstack_output_report_handler_test.c`
  - result: tests の direct snapshot 利用は owner/state/metrics、state provider、rumble/status、IPC runner state confirmation に分類できることを確認した。

未実行:

- CTest。理由は、この work unit では C source / header を変更せず、`swbt_app_snapshot_t` の分割判断と follow-up 範囲だけを記録したためである。

## 11. 実機実行条件

この work unit の通常範囲では実機検証を実行しない。理由は、application snapshot の利用者分類と API 分割判断だけを行い、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop を実行しないためである。

実装 work unit へ進み、Switch-facing report bytes、report period、HID registration、BTstack source selection、pairing sequence を変更する場合は、`hardware-harness` と根拠監査の要否を再判断する。

## 12. 先送り事項

- 観測: `swbt_app_snapshot_t` は report scheduler と IPC status を同じ構造体に結び付けている。
  先送り理由: この work unit は分割方針を閉じる review work unit であり、実装差分を混ぜない。
  次の置き場: 別 work unit record を作り、`swbt_app_read_controller_state` と status view API を実装する。

- 観測: `swbt_state_t` 自体を buttons、sticks、IMU などへ分ける可能性がある。
  先送り理由: 現時点では分割不要と判断した。`swbt_state_t` は daemon IPC の full controller state snapshot と Switch report generation の入力単位に合っている。
  次の置き場: protocol / switch layer で buttons、sticks、IMU の個別更新や別寿命が必要になった時点で、別 work unit record を作る。

## 13. チェックリスト

- [x] source と use case を記録した。
- [x] review 対象の call site を特定した。
- [x] TDD Test List を作成した。
- [x] app snapshot call site 分類を記録した。
- [x] controller-only read API の要否を判断した。
- [x] status view API の要否を判断した。
- [x] `swbt_state_t` 分割要否を判断した。
- [x] `local_080` 継続可否を判断した。
- [x] 検証または未実行理由を記録した。
- [x] work unit record を更新した。
