# App Snapshot View Decomposition

## 1. 概要

`swbt_app_snapshot_t` が report scheduler、IPC status、rumble/status 検証の複数用途を 1 つの構造体で支えている状態を見直し、用途別 read API へ分割する。

この work unit では、`swbt_state_t` を分割しない。`swbt_app_snapshot_t` の利用者を分類し、report scheduler が必要とする controller state、IPC status が必要とする owner / state / rumble / metrics / daemon / hardware、rumble 検証が必要とする state を分けて読める API に移す。

共有設計メモを取り込んだ後の結論は、`swbt_app_snapshot_t` を単に太い `app status view` へ改名することではない。`swbt_app_t` は app-owned state の読み取り API を持ち、IPC status や public C ABI の `get_status` は将来の `swbt/control` 層で app status と runtime / link status を合成する、という分割を優先する。

この実装では、generic な `swbt_app_snapshot_t` / `swbt_app_snapshot` を削除し、`swbt_app_read_controller_state` と `swbt_app_read_status` を追加した。production の report scheduler state provider は controller-only read API を使い、IPC status adapter は transitional status read API を使う。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-27: `state snapshot` 自体が太りすぎており、`local_079` / `local_080` の分割の正当性も分からなくなってきた。先に `swbt_app_snapshot_t` の分割検討を済ませたい。
- `swbt/application/app.h`: `swbt_app_snapshot_t` は owner、last sequence、controller state、rumble、metrics、daemon status、hardware status をまとめて返す。
- `swbt/daemon/host.c`: report scheduler の state provider は `swbt_app_snapshot` を呼び、`snapshot.state` だけを返す。
- `swbt/ipc/ipc_adapter.c`: IPC status は `swbt_app_snapshot` から owner、sequence、state、rumble、metrics、daemon、hardware をすべて copy する。
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`: controller state と command metadata は分離済みで、report generation は controller state のみを見る方針を持つ。
- `spec/architecture/daemon-architecture-cutover.md`: controller state、control lease、rumble、SPI、player lights、logical link state、report scheduling policy は application だけが更新する。
- shared design note, 2026-06-27: https://chatgpt.com/share/6a3fc217-d5ec-83ee-b68c-1b730f0721c5
  - 要点: public C ABI と JSON Lines IPC は同じ `swbt/control` の意味論を使い、`swbt/runtime` は IPC を含まない実機 runtime として切り出す。`daemon_host` は最終的に runtime host と IPC runner の利用者へ下げる。IPC status は app status と runtime / link status の合成として扱う。

use case:

- actor: architecture reviewer / implementation planner。
- 入力または状態: `swbt_app_snapshot_t` の定義、`swbt_app_snapshot` call site、IPC status contract、report scheduler state provider、rumble/status test。
- 期待する観測結果:
  - `swbt_app_snapshot_t` の利用者を用途別に分類できる。
  - `swbt_state_t` 自体を分割すべきか、`swbt_app_snapshot_t` を view 別 API に分ければ足りるかを判断できる。
  - report scheduler が full app snapshot に依存しない API 形を提案できる。
  - IPC status の wire format を変えずに、app-owned status と runtime / link status の責務境界を提案し、現実装で必要な transitional status read API を用意できる。
  - public C ABI と IPC が別々に `swbt_app_*` を直接呼ぶ二重化を避ける方向性を記録し、後続 `swbt/control` の source を残せる。
  - `local_080` の device send path work unit を続行してよいか、app snapshot 分割を先に実装すべきかを判断できる。
- 制約: IPC wire format、Switch-facing report bytes、report period、BTstack device API は変更しない。
- 対象外: IPC JSON contract の変更、`swbt_state_t` field の削除、device API の追加実装。

source から use case への変換:

疑問点は、device API の妥当性だけでは解けない。現行 code では BTstack device boundary と application snapshot boundary が別々の結合点になっている。`local_079` / `local_080` の継続可否を判断する前に、application snapshot の読み手ごとの責務を分ける必要がある。

## 3. 対象範囲

- `swbt_app_snapshot_t` の field と call site を分類する。
- report scheduler が必要な controller state view を定義する。
- IPC status が必要な status view を application 層に置くべきか、control 層で合成すべきかを判断する。
- rumble / metrics / daemon / hardware status の取得責務を status view に含めるか、個別 view に分けるかを判断する。
- public C ABI / JSON Lines IPC / control / runtime の境界案が `swbt_app_snapshot_t` 分割判断へ与える影響を記録する。
- `swbt_state_t` を分割する必要があるか、現時点では不要かを判断する。
- `swbt_app_read_controller_state` を実装し、report scheduler state provider を full snapshot 依存から外す。
- transitional な `swbt_app_read_status` を実装し、IPC status adapter と status/rumble tests を移す。
- `swbt_app_snapshot_t` / `swbt_app_snapshot` の C code references を削除する。
- 関連 tests と architecture spec を新 API に合わせる。
- `local_080` の production send path work unit を続行してよい条件を記録する。

## 4. 対象外

- `swbt_state_t` の field 追加、削除、構造変更。
- IPC JSON wire format の変更。
- daemon protocol としての partial update、button tap、hold duration、future scheduling。
- Switch-facing report bytes、report period、BTstack source selection の変更。
- `local_080` の device API / report send path 実装。
- `swbt/control` / `swbt/runtime` の実装。
- daemon / hardware / link status を app から runtime へ移す構造変更。
- 実機検証。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `spec/protocols/daemon-ipc-v1.md`
- `spec/operations/work-unit-spec-tdd-flow.md`
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`
- `work-units/complete/local_079/DEVICE_LIFECYCLE_API.md`
- `work-units/wip/local_080/DEVICE_API_PRODUCTION_PATH.md`
- external input: https://chatgpt.com/share/6a3fc217-d5ec-83ee-b68c-1b730f0721c5

## 6. 根拠監査

not applicable。

この work unit は Switch HID report bytes、subcommand bytes、SPI address、rumble packet、BTstack source selection、report period、WinUSB/libusb facts を追加または変更しない。対象は application read API の責務分類である。

`swbt_state_t` の field semantics、Switch report packing、IPC wire field semantics を変更する場合は、この work unit から切り出し、必要に応じて `source-audit` と protocol spec 更新を判断する。

## 7. 設計メモ

### 7.1 現時点の観測

- `swbt_state_t` は buttons、sticks、accelerometer、gyro だけを持つ。command sequence metadata は `local_050` で外れている。
- 変更前の `swbt_app_snapshot_t` は controller state だけでなく owner、sequence、rumble、metrics、daemon status、hardware status を同時に持っていた。
- 変更前の report scheduler は `snapshot.state` だけを使っていた。
- 変更前の IPC status は `swbt_app_snapshot_t` のほぼ全体を status response へ写していた。
- 変更前の rumble / output report 関連 test は `snapshot.rumble` を読んでいた。

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
- report scheduler 向けには `swbt_app_read_controller_state(const swbt_app_t *, swbt_state_t *)` を追加し、status surface への依存を切る。
- IPC status 向けには wire format を維持するための view が必要だが、その view をそのまま application 層に置くと、public C ABI と IPC の共通意味論を置く場所が曖昧になる。
- app 層に置くべきものは owner、last sequence、controller state、rumble、app metrics のような app-owned status までである。pairing、connect、HID open、link state、runtime error などは `swbt_app_t` ではなく runtime / link 側の責務にする。
- IPC `get_status` と public C ABI `get_status` は、将来の `swbt/control` 層で app-owned status と runtime / link status を合成する方がよい。IPC adapter は transport adapter として control に委譲し、direct API と IPC で app 操作の意味論を二重化しない。
- rumble は app-owned output report status として app status に含めてよい。ただし、diagnostics 全体の公開単位は control status が決める。
- metrics、daemon、hardware は分けて扱う。app metrics は app-owned status に残せるが、daemon lifecycle や hardware / link status は runtime / daemon 側へ寄せる候補である。
- generic な `swbt_app_snapshot_t` / `swbt_app_snapshot` は用途が広すぎるため削除する。status 合成は `swbt_app_status_snapshot_t` への単純改名ではなく、`swbt/control` の導入と合わせて再設計する。

### 7.4 共有設計メモを取り込んだ補正

共有設計メモは、`daemon_host` を runtime の中心にし続けるのではなく、IPC を含まない `swbt_runtime_host` と、public C ABI / JSON Lines IPC が共通に使う `swbt/control` を置く方向を示している。この観点を入れると、`swbt_app_snapshot_t` 分割の主眼は「IPC status 用の太い app snapshot を作る」ことではなくなる。

補正後の境界は次の通りである。

```text
swbt_app
  owner / sequence / controller state / rumble / app metrics

swbt_runtime
  HID / report timer / link / pairing / connection state

swbt_control
  submit_state / submit_neutral / get_status の意味論
  app status + runtime / link status を合成

swbt_daemon
  IPC server と process lifecycle
```

この境界を採る場合、`swbt_app_read_controller_state` は単独でも価値がある。report scheduler は status 合成を必要とせず、latest controller state だけを読むためである。

一方、`swbt_app_read_status` を先に大きく設計するのは避ける。IPC status の shape を満たすために application 層へ daemon / hardware / link の概念を残すと、後で `swbt/control` と `swbt/runtime` へ分ける時に再移動が必要になる。

### 7.5 実装結果

- `swbt/application/app.h`
  - `swbt_app_snapshot_t` を削除した。
  - `swbt_app_status_snapshot_t` を追加した。
  - `swbt_app_read_controller_state(const swbt_app_t *, swbt_state_t *)` を追加した。
  - `swbt_app_read_status(const swbt_app_t *, swbt_app_status_snapshot_t *)` を追加した。
- `swbt/application/app.c`
  - controller-only read は lock 下で `app->state` だけを copy する。
  - status read は既存 IPC status 互換のため owner、last sequence、state、rumble、metrics、daemon、hardware を copy する。これは transitional API であり、daemon / hardware / link status の最終的な置き場は control / runtime 分離時に再評価する。
- `swbt/daemon/host.c`
  - report scheduler state provider は `swbt_app_read_controller_state` を使う。
- `swbt/ipc/ipc_adapter.c`
  - `get_status` は `swbt_app_read_status` を使う。
- tests
  - app contract / rumble / status の確認は `swbt_app_read_status` を使う。
  - state-only の確認は `swbt_app_read_controller_state` を使う。
- `spec/architecture/daemon-architecture-cutover.md`
  - application read API の例を `swbt_app_read_controller_state` / `swbt_app_read_status` に更新した。

`rg -n "swbt_app_snapshot_t|swbt_app_snapshot\\(" swbt tests spec work-units` の結果、旧 API 名はこの work unit record の履歴説明だけに残っている。

### 7.6 `local_080` への影響

`local_080` の device send path は BTstack HID device boundary の問題であり、`swbt_app_snapshot_t` の分割実装そのものを完了条件にしない。ただし、`local_080` で report scheduler / host state provider に触る場合、先に controller-only read API を入れる方が責務境界は明確になる。

このため、推奨順序は次の通りである。

1. `local_081` で `swbt_app_read_controller_state` / `swbt_app_read_status` への分割実装を完了する。
2. `swbt/control` / `swbt/runtime` の境界を別 work unit または spec で立てる。
3. `local_080` の production send path を再開する。

`local_080` をすぐ再開してよい条件は、変更範囲を BTstack device send path に閉じ、`swbt_app_snapshot_t` の利用拡大、IPC status contract 変更、control / runtime 分離を混ぜないことである。

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
| green | shared control/runtime boundary input is reflected in app snapshot split decision | decision | review | no |
| green | production report scheduler reads controller state through controller-only app API | regression | unit | no |
| green | IPC status reads through transitional app status API without changing wire format | regression | integration | no |
| green | generic app snapshot API no longer exists in C code and tests | characterization | review | no |

TDD status:

- source: user request, 2026-06-27。
- use case: `swbt_app_snapshot_t` の分割検討を先に済ませ、device API work の前提を整理する。
- state: green。
- refactor: done。generic snapshot read を用途別 read API へ分け、production caller と tests を移行した。

## 10. 検証

実行:

- `rg -n "swbt_app_snapshot_t|swbt_app_snapshot\\(" swbt tests spec work-units`
  - result: `swbt/daemon/host.c`、`swbt/ipc/ipc_adapter.c`、関連 tests、architecture spec、`local_081` record の利用箇所を確認した。
- `rg -n -C 4 "swbt_app_snapshot|snapshot\\." swbt/daemon/host.c swbt/ipc/ipc_adapter.c`
  - result: production caller は report scheduler state provider と IPC status adapter の 2 系統であることを確認した。
- `rg -n -C 3 "swbt_app_snapshot|snapshot\\.|status\\." tests/application_command_test.c tests/daemon_host_test.c tests/daemon_ipc_runner_test.c tests/btstack_output_report_handler_test.c`
  - result: tests の direct snapshot 利用は owner/state/metrics、state provider、rumble/status、IPC runner state confirmation に分類できることを確認した。
- `Invoke-WebRequest -Uri 'https://chatgpt.com/share/6a3fc217-d5ec-83ee-b68c-1b730f0721c5' -UseBasicParsing -TimeoutSec 20`
  - result: shared design note `ChatGPT - API設計と構成案` を確認し、public C ABI / JSON Lines IPC / `swbt/control` / `swbt/runtime` の境界案を local_081 の判断に反映した。
- `scripts/format.sh`
  - result: pass。
- `just debug`
  - result: pass。47 tests passed, 0 failed。
- `rg -n "swbt_app_snapshot_t|swbt_app_snapshot\\(" swbt tests spec work-units`
  - result: old API references はこの work unit record の履歴説明だけに残る。C code、tests、architecture spec には残っていない。

## 11. 実機実行条件

この work unit の通常範囲では実機検証を実行しない。理由は、application snapshot の利用者分類と API 分割判断だけを行い、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop を実行しないためである。

実装 work unit へ進み、Switch-facing report bytes、report period、HID registration、BTstack source selection、pairing sequence を変更する場合は、`hardware-harness` と根拠監査の要否を再判断する。

## 12. 先送り事項

- 観測: `swbt_app_snapshot_t` は report scheduler と IPC status を同じ構造体に結び付けている。
  処理結果: generic な `swbt_app_snapshot_t` / `swbt_app_snapshot` は削除し、controller state read と transitional status read に分けた。
  残る先送り: status 合成は `swbt_app_status_snapshot_t` の単純追加ではなく、`swbt/control` / `swbt/runtime` の境界整理と合わせて扱う。
  次の置き場: `swbt/control` / `swbt/runtime` の work unit record または architecture spec。

- 観測: `swbt_state_t` 自体を buttons、sticks、IMU などへ分ける可能性がある。
  先送り理由: 現時点では分割不要と判断した。`swbt_state_t` は daemon IPC の full controller state snapshot と Switch report generation の入力単位に合っている。
  次の置き場: protocol / switch layer で buttons、sticks、IMU の個別更新や別寿命が必要になった時点で、別 work unit record を作る。

- 観測: IPC adapter と public C ABI がそれぞれ `swbt_app_acquire` / `swbt_app_set_state` / `swbt_app_snapshot` を直接呼ぶ形にすると、owner、sequence、neutral、status 合成の意味論が二重化する。
  先送り理由: この work unit は snapshot 分割判断であり、control layer 実装は含めない。
  次の置き場: `swbt/control` の work unit record または architecture spec を作り、IPC adapter と public C ABI が共有する operation API を定義する。

## 13. チェックリスト

- [x] source と use case を記録した。
- [x] review 対象の call site を特定した。
- [x] TDD Test List を作成した。
- [x] app snapshot call site 分類を記録した。
- [x] controller-only read API の要否を判断した。
- [x] status view API の要否を判断した。
- [x] `swbt_state_t` 分割要否を判断した。
- [x] `local_080` 継続可否を判断した。
- [x] 共有設計メモの control / runtime 境界案を判断に反映した。
- [x] `swbt_app_read_controller_state` を実装した。
- [x] `swbt_app_read_status` を実装した。
- [x] production report scheduler state provider を controller-only read API に移した。
- [x] IPC status adapter を status read API に移した。
- [x] old `swbt_app_snapshot_t` / `swbt_app_snapshot` references を C code と tests から削除した。
- [x] architecture spec を新 API に合わせた。
- [x] 検証または未実行理由を記録した。
- [x] work unit record を更新した。
