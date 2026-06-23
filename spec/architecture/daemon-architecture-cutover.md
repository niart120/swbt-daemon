# Daemon Architecture Cutover

## 1. 状態

current。

この spec は、2026-06-22 の外部レビュー結果を swbt-daemon の current architecture policy として採用する。`spec/archive/daemon-runtime-boundaries-before-architecture-cutover.md` と `spec/archive/daemon-application-boundary-rearchitecture-before-cutover.md` がこの spec と衝突する場合、この spec を優先する。

この spec の採用により、内部 API、内部型、内部 target、内部 call path の互換性は維持要件ではなくなる。旧構造への rollback は runtime flag や互換 layer ではなく Git で行う。

## 2. 目的

daemon の論理状態、IPC transport、BTstack adapter、host composition、platform shutdown の責務を一方向の依存関係へ切り替える。

この切り替えでは、旧 `swbt_ipc_session_t`、`state_mailbox`、`swbt_daemon_runtime_t`、`swbt_daemon_runtime_backend_t`、production backend ops table、`swbt_core` aggregate target を削除対象とする。新経路を追加して旧経路を残す状態は完了と扱わない。

## 3. 適用範囲

- `swbt/application/` の daemon logical state、owner、sequence、controller state、rumble、SPI、player lights、logical link state、report scheduling policy。
- `swbt/ipc/` の transport、framing、JSON codec、client identity、application command mapping。
- `swbt/btstack_bridge/` と後続 adapter target の BTstack callback、HID send、timer、IPC pump scheduling。
- daemon host / production composition / platform shutdown ordering。
- `CMakeLists.txt` の target 境界、include 境界、旧 aggregate target 削除。
- architecture journey、absence acceptance、software gate、Hardware Gate H1。

次は、この architecture cutover の対象外である。

- bonded reconnect persistence。
- bond store。
- adapter removal / reinsertion recovery。
- sleep / resume recovery。
- status / observability protocol の新規固定。
- parser fuzz。
- Windows native CI。
- release packaging。
- BTstack license を含む配布方針。
- 複数 controller、Joy-Con、NFC / IR semantic support。

## 4. 決定事項

この spec は、本章の決定と「8. 採用した外部レビュー本文」を合わせて規範とする。本文内の「判定」「禁止」「完了条件」「Hardware Gate H1」は、この repository の作業判断として採用する。

採用する主要判断は次である。

- 内部互換性は維持しない。公開 IPC wire format、Switch-facing bytes、ユーザが実行する CLI / environment variable、永続化済み bond data、release artifact だけを外部契約として扱う。
- compatibility layer は既定で禁止する。外部契約保護に必要で、削除期限と削除 work unit が同時にある場合だけ例外にする。
- controller state、control lease、rumble、SPI、player lights、logical link state、report scheduling policy は application だけが更新する。
- IPC は transport、framing、codec、client identity に閉じる。
- BTstack adapter は daemon / IPC internal type を参照しない。
- host が startup、shutdown、cleanup、power、run loop、platform listener の順序を所有する。
- `swbt_core` を互換 aggregate target として残さない。production executable と test は必要 target を直接 link する。
- 旧 symbol、旧 header、旧 target、旧 test の不存在を acceptance に含める。
- 実機 H1 は三つの cutover が終わった後に一回だけ実行する。実機承認がない場合は未実行理由と必要条件を記録し、pass と偽らない。

## 5. 根拠

- source: user request, 2026-06-23, `tmp/swbt-daemon-architecture-cutover-handoff.md` の正式採用指示。
- external review: 2026-06-22、基準 commit `74508a7`、公開 source / CMake / tests / spec / work unit / commit history に対する静的レビュー。
- repository state: 旧段階移行 spec と `local_055` は互換層を current responsibility 付きで残す方針だった。今回の user request は、その方針と衝突する部分で外部レビュー文書を優先すると明示した。

この spec 自体は新しい Switch protocol byte、BTstack source selection、report period、WinUSB/libusb 実測値を追加しない。実装時にそれらを変更する場合は `source-audit` を使う。

## 6. 関連 work units

- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`
- `work-units/complete/local_051/DAEMON_APPLICATION_COMMAND_API.md`
- `work-units/complete/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md`
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`
- `work-units/complete/local_054/DAEMON_HOST_AND_BUILD_BOUNDARIES.md`
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`

## 7. 実装状態

- 2026-06-23 の software cutover 実装では、旧 session、mailbox、runtime、backend ops、`swbt_core` を source / tests / build graph から削除した。`just verify` は pass した。
- 不要になった旧資材群の処理は `local_056` を正とする。旧 session / mailbox の source と tests は削除し、runtime は host に置換し、production backend ops は BTstack adapter 境界へ移し、旧 cutover journey / acceptance は新しい architecture journey / absence acceptance へ置換した。旧 architecture spec は history として `spec/archive/` に残した。
- H1 artifact は削除対象ではない。raw HCI dump と daemon trace は実機証跡として `tmp/hardware/local_057/20260623-105416-architecture-cutover-h1` に保持し、durable docs では要約と artifact path だけを参照する。
- Hardware Gate H1 は 2026-06-23 に `local_057` で pass。承認済みの CSR8510 A10、WinUSB、Switch2 firmware `22.1.0` baseline、`8000 us` report period、production backend で実行した。
- H1 artifact は `tmp/hardware/local_057/20260623-105416-architecture-cutover-h1`。HCI dump は line `953` Button A、line `954` trailing neutral、line `955` `hci_power_control: 0` の順を記録した。current connection の `invalid size` と `non-registered handle` は `0` 件である。
- 「8. 採用した外部レビュー本文」内の未完了表記は、採用時点の作業指示として残す。現在の実装状態はこの章、`local_056`、`local_057`、`docs/status.md` を正とする。
- 外部契約を破壊する必要が出た場合は、同じ PR に変更理由と migration note を含める。

## 8. 採用した外部レビュー本文

- 対象リポジトリ: <https://github.com/niart120/swbt-daemon>
- 対象ブランチ: `main`
- レビュー基準コミット: [`74508a7`](https://github.com/niart120/swbt-daemon/commit/74508a7)
- レビュー日: 2026-06-22
- 文書の位置づけ: 外部レビュー結果と、後続作業者への破壊的リアーキテクチャ指針
- レビュー方式: 公開されているソース、CMake、テスト、spec、work unit、コミット履歴を用いた静的レビュー

## 1. 結論

現在のリアーキテクチャは、設計方向としては正しい。

controller state から sequence metadata を外し、owner、sequence、controller state、release・disconnect・heartbeat timeout・shutdown の revoke policy を `swbt_app_t` へ移した点は、実質的な改善である。JSON codec と command execution の分離、BTstack HID event・HID port・timer port の導入も、外部依存を adapter 側へ押し出すための足場になっている。

ただし、今後は互換層を維持しながら徐々に寄せる方針を採らない。

このリポジトリは、内部 API の長期互換を守る必要がある公開ライブラリではない。現時点で優先すべきなのは、旧構造を壊さずに残すことではなく、新しい依存方向を短い期間で唯一の経路にすることである。

今後の基本方針を次のように変更する。

> **内部 API、内部型、内部 target、内部 call path の互換性は保証しない。新経路へ切り替える作業単位では、旧経路を同時に削除する。rollback は compatibility layer ではなく Git で行う。**

現在完了しているのは、制御ポリシー境界の移行である。

```text
IPC
  → swbt_ipc_session_t
      → swbt_app_t
          → owner
          → latest sequence
          → controller state
          → revoke policy
```

最終的に必要な構造は次である。

```text
IPC adapter ───────────────┐
BTstack adapter ───────────┼─> application ─> protocol / ports
platform shutdown ─────────┘

daemon host
  └─ application、IPC adapter、BTstack adapter、platform adapter を構成
```

この差分を埋めるために、以下を積極的に削除する。

- `swbt_ipc_session_t`
- `state_mailbox`
- 広い runtime backend table
- production backend ops table
- daemon 内部を参照する BTstack glue
- `swbt_core`
- 旧 compatibility journey
- 旧 cutover acceptance
- 二重化された state
- 旧 API への forwarding wrapper
- 移行専用 typedef、alias、aggregate target

削除できないものは「残す」のではなく、外部契約として本当に必要かを再判定する。

## 2. 強い方針

### 2.1 内部互換性を要件にしない

次は破壊してよい。

- C の内部 function signature
- struct layout
- internal header path
- CMake target name
- test fixture API
- fake backend API
- daemon runtime internal API
- IPC server と application の結合 API
- BTstack bridge の callback API

次だけは、明示的な判断なしに破壊しない。

- daemon が公開している IPC wire format
- Switch へ送る wire bytes
- ユーザーが実行する CLI / environment variable
- 永続化済み bond data
- release artifact の名前と配置

これらも絶対互換ではない。破壊する場合は、変更理由と migration note を一つの PR に含める。

### 2.2 compatibility layer を既定で禁止する

新しい wrapper、facade、adapter alias、bridge、dual-write を追加してはならない。

例外を認める条件はすべて満たす必要がある。

```text
1. 外部契約を守るために必要
2. 削除 commit または削除 work unit が同時に作られる
3. 削除期限が同じ文書に書かれる
4. authoritative state を持たない
5. fallback path を持たない
6. 新旧双方への書き込みをしない
```

「後で消す」「安全のため残す」「段階移行のため必要」は理由として認めない。

### 2.3 rollback は Git で行う

旧 runtime path を production binary に残して rollback 手段にしない。

rollback 方法:

```text
git revert
release tag へ戻す
integration branch を破棄する
```

禁止する rollback 方法:

```text
runtime flag で旧経路へ切り替える
environment variable で旧 backend を選ぶ
新旧 application を同時初期化する
旧 target を aggregate target に残す
```

### 2.4 一つの状態に一つの writer

controller state、control lease、rumble、SPI、player lights、logical link state、report scheduling policy は application だけが更新する。

禁止:

```text
application と mailbox の dual-write
application と runtime の dual-write
session と application の sequence 二重管理
BTstack callback から device state を直接更新
```

snapshot の複製は許可する。authoritative state の複製は許可しない。

### 2.5 旧経路の削除を完了条件に含める

「新経路が動く」は完了ではない。

各 work unit の完了条件に、旧 symbol、旧 include、旧 target、旧 test の不存在を含める。

例:

```sh
! grep -R "swbt_ipc_session_t" swbt tests
! grep -R "swbt_state_mailbox" swbt tests
! grep -R "swbt_daemon_runtime_backend_t" swbt tests
! grep -R "swbt_daemon_production_backend_ops_t" swbt tests
! grep -R "swbt_core" CMakeLists.txt swbt tests cmake
```

## 3. 現在地の判定

| 領域 | 現在の判定 | 次の処置 |
|---|---|---|
| controller state と sequence metadata の分離 | 完了 | 維持 |
| owner・sequence・neutral 化の application 集約 | 完了 | application へ残りの論理状態を移す |
| JSON codec と副作用の分離 | 完了 | session を介さず application handler へ接続 |
| BTstack typed event / port | 部分完了 | daemon 内部依存を切り、production 経路を一本化 |
| application による全論理状態の所有 | 未完了 | 破壊的に一本化 |
| IPC transport 専任化 | 未完了 | `swbt_ipc_session_t` を削除 |
| host / composition root | 未完了 | runtime / production backend を分解して削除 |
| CMake 依存方向 | 未完了 | `swbt_core` を削除 |
| architecture journey | 未完了 | 旧 journey を置換 |
| shutdown neutral ordering | 未完了 | host test と H1 で固定 |
| production hardware gate | 未実施 | 最終 cutover 後に一回だけ実施 |

## 4. 最終着地点

### 4.1 module 構造

```text
swbt/
  controller/
  switch_protocol/
  application/
  adapters/
    ipc/
    btstack/
  host/
  platform/
```

### 4.2 依存方向

```text
host
  ├─ application
  ├─ IPC adapter
  ├─ BTstack adapter
  └─ platform adapter

IPC adapter
  → application public command / event API

BTstack adapter
  → application public event API
  → application port interface の実装

application
  → controller model
  → Switch protocol
  → HID / timer / clock port

Switch protocol
  → controller model

controller model
  → C standard library
```

### 4.3 禁止依存

```text
application
  → BTstack vendor header
  → socket
  → WinUSB / libusb
  → daemon host internal type

IPC adapter
  → BTstack adapter internal type
  → runtime internal type

BTstack adapter
  → IPC session internal type
  → runtime internal type
  → production backend internal type

protocol
  → JSON
  → socket
  → BTstack run loop
  → client ID
  → command sequence
```

### 4.4 状態所有

`swbt_app_t` を opaque にする。

```c
typedef struct swbt_app swbt_app_t;
```

application が所有する。

```text
control lease
latest sequence
controller state
rumble
virtual SPI
player lights
Switch device state
logical link state
report scheduling policy
status counters
shutdown logical state
```

host / adapter が所有する。

```text
socket
WinUSB / libusb handle
BTstack run loop
BTstack timer source
HID transport handle
HCI power state
OS shutdown listener
log sink
HCI dump
```

### 4.5 public API

```c
swbt_app_result_t swbt_app_handle_command(
    swbt_app_t *app,
    const swbt_command_t *command,
    swbt_command_response_t *response
);

void swbt_app_dispatch(
    swbt_app_t *app,
    const swbt_event_t *event
);

swbt_app_result_t swbt_app_snapshot(
    const swbt_app_t *app,
    swbt_app_snapshot_t *snapshot
);
```

adapter は `swbt_app_t` の field を直接参照しない。

## 5. 削除対象

### 5.1 `swbt_ipc_session_t`

判定:

> 削除する。

`swbt_ipc_session_t` は、application facade、wire compatibility、rumble status、mailbox publish、lock を束ねている。owner policy が application へ移動した後も残し続けると、application の前に第二の service layer が固定化する。

置換後:

```text
ipc_server
  → connection / framing

ipc_json
  → codec

ipc_adapter
  → typed command / response mapping

application
  → owner / sequence / state / revoke
```

削除条件:

- IPC server が application handler を直接 bind する
- client disconnect を application event に変換する
- heartbeat deadline は application または host timer で扱う
- rumble status は application snapshot から返す
- mailbox publish が不要になる

削除方法:

- forwarding wrapper を残さない
- typedef alias を残さない
- deprecated header を残さない
- test fixture を新 API へ一括更新する

### 5.2 `state_mailbox`

判定:

> 原則削除する。

現在の production 経路が単一 reactor 上で IPC polling と BTstack callback を処理するなら、application state と report scheduling の間に lock-based mailbox を置く必要は薄い。

置換候補:

```text
application が current state を所有
report tick が application snapshot を取得
same reactor 上で report を生成
```

別 thread が必要なら、state mailbox ではなく typed event queue を adapter 境界に置く。

残置を認める条件:

- 実測または call graph で別 thread writer が存在する
- immutable snapshot 配送だけを行う
- authoritative state を持たない
- `state_mailbox` という名前ではなく役割に沿った snapshot queue にする

単に既存 test が依存していることは残置理由にならない。

### 5.3 `swbt_daemon_runtime_backend_t`

判定:

> 削除する。

広い backend table は、application、host、adapter の責務を再び一つに束ねる。

置換:

```text
application ports
  ├─ hid
  ├─ timer
  └─ clock

host composition
  ├─ IPC adapter lifecycle
  ├─ BTstack adapter lifecycle
  ├─ platform lifecycle
  └─ shutdown ordering
```

旧 table を新 table へ forwarding する wrapper は作らない。

### 5.4 production backend ops table

判定:

> 分解後に削除する。

production backend ops table が持つ能力を次へ分ける。

```text
host-only operations
  power on/off
  run loop
  SSP configuration
  startup / cleanup

application ports
  HID send
  request can-send
  timer
  clock
```

application が production backend 全体へ依存してはならない。

### 5.5 `swbt_daemon_runtime_t`

判定:

> 現在の形では削除する。

runtime が application state、IPC session、SPI、player lights、backend、lifecycle flag を束ねている限り、新しい application と責務が競合する。

置換:

```text
swbt_app_t
  → logical state / policy

swbt_daemon_host_t
  → resources / lifecycle / composition
```

`swbt_daemon_runtime_t` を host へ改名するだけでは不十分である。logical state field を移した後に削除する。

### 5.6 `swbt_core`

判定:

> 削除する。

`swbt_core` を compatibility aggregate として残すと、target boundary の意味が失われる。

最終 target:

```text
swbt_switch_protocol
swbt_application
swbt_ipc
swbt_btstack_adapter
swbt_daemon_host
swbt_platform_windows
swbt_platform_posix
```

production executable は必要な target を直接 link する。

禁止:

```text
add_library(swbt_core INTERFACE)
target_link_libraries(swbt_core INTERFACE ...)
```

aggregate target は移行完了後に残さない。

### 5.7 compatibility journey

判定:

> 旧 journey は削除する。

旧 runtime path を通る journey を「回帰試験」として残すと、旧構造の削除を妨げる。

新 journey が同等以上の user-visible behavior を確認した時点で置換する。

新 journey:

```text
JSON line
→ codec
→ IPC adapter
→ application command
→ fake timer
→ fake HID
→ report capture
→ client disconnect
→ neutral report
→ shutdown request
→ trailing neutral
→ transport stop
```

旧 test は rename して保存せず、削除する。

### 5.8 cutover acceptance

判定:

> 置換する。

「kept / removed / deferred」を記録するだけの acceptance は、残存構造を正当化しやすい。

新 acceptance は不在を検査する。

```text
旧 symbol がない
旧 header がない
旧 target がない
旧 test がない
禁止 include が compile できない
new journey が swbt_core なしで通る
```

## 6. 破壊的カットオーバー計画

作業を細かい compatibility phase に分けない。

三つの cutover にまとめる。

## Cutover A: application state takeover

目的:

- daemon の論理状態を application へ一本化する
- session と mailbox を削除する

同一 cutover で行う。

```text
1. swbt_app_t を opaque にする
2. rumble / SPI / player lights / device state を移す
3. report scheduling policy を移す
4. IPC server を application handler へ直結する
5. output report を typed event にする
6. report builder を application snapshot から動かす
7. swbt_ipc_session_t を削除する
8. state_mailbox を削除する
9. 旧 session / mailbox tests を削除または新 test に置換する
```

完了条件:

```sh
! grep -R "swbt_ipc_session_t" swbt tests
! grep -R "swbt_state_mailbox" swbt tests
! find swbt -iname '*ipc_session*'
! find swbt -iname '*state_mailbox*'
```

加えて:

- controller / device logical state の writer が application だけ
- report tick から一貫した snapshot を取得できる
- release / disconnect / timeout / shutdown が同じ revoke を通る
- new software journey が通る

禁止:

- session から app への forwarding wrapper を残す
- mailbox と app の dual-write
- old/new report path の runtime switch

実機試験: 不要。Switch-facing bytes または timer semantics を変えた場合だけ H1 の対象として記録する。

## Cutover B: host / BTstack takeover

目的:

- production composition を host へ一本化する
- runtime backend と production backend ops を削除する

同一 cutover で行う。

```text
1. swbt_daemon_host_t を導入する
2. startup / cleanup / power / run loop を host へ移す
3. BTstack adapter が typed event と port 実装だけを提供する
4. IPC pump lifecycle を host が所有する
5. shutdown ordering を host に固定する
6. swbt_daemon_runtime_t を削除する
7. swbt_daemon_runtime_backend_t を削除する
8. production backend ops table を削除する
9. daemon 内部を参照する BTstack include を削除する
```

完了条件:

```sh
! grep -R "swbt_daemon_runtime_t" swbt tests
! grep -R "swbt_daemon_runtime_backend_t" swbt tests
! grep -R "swbt_daemon_production_backend_ops_t" swbt tests
! grep -R '#include "daemon/' swbt/adapters/btstack swbt/btstack_bridge
! grep -R '#include "ipc/' swbt/adapters/btstack swbt/btstack_bridge
! grep -R 'btstack_' swbt/application
```

shutdown ordering test:

```text
shutdown request
→ application revoke(SHUTDOWN)
→ neutral send attempt
→ HID/timer stop
→ HCI power-off
→ run loop exit
```

neutral send failure 時:

```text
failure counter / log
→ cleanup 継続
→ power-off
```

禁止:

- runtime facade を残す
- production backend の wrapper を残す
- old/new host の選択 flag
- test-only old backend API

実機試験: Cutover C 完了後、H1 で一度だけ行う。

## Cutover C: build and test takeover

目的:

- target、include、acceptance を最終構造へ切り替える
- `swbt_core` と旧 test architecture を削除する

同一 cutover で行う。

```text
1. swbt_ipc target を作る
2. swbt_btstack_adapter target を作る
3. swbt_daemon_host target を作る
4. include path を target ごとに制限する
5. test を対象 target へ直接 link する
6. new software journey を唯一の architecture journey にする
7. old compatibility journey を削除する
8. old cutover acceptance を削除する
9. absence / boundary acceptance を追加する
10. swbt_core を削除する
```

完了条件:

```sh
! grep -R "swbt_core" CMakeLists.txt cmake swbt tests
! find . -iname '*cutover_journey*'
! find . -iname '*cutover_acceptance*'
```

CMake 上:

```text
application target
  は BTstack include path を持たない

protocol target
  は IPC / host / BTstack を link しない

IPC target
  は BTstack adapter を link しない

BTstack adapter target
  は IPC / host internal include path を持たない

test
  は aggregate target を link しない
```

完了後に Hardware Gate H1 を行う。

## 7. work unit の切り方

各 cutover は複数 commit になってよいが、一つの長寿命 compatibility 状態を `main` に置かない。

推奨:

```text
integration branch
  ├─ mechanical move
  ├─ API break
  ├─ caller migration
  ├─ old code deletion
  ├─ test replacement
  └─ cleanup

main へ merge
  → 旧経路が存在しない
```

非推奨:

```text
PR 1: 新 API 追加
PR 2: 一部 caller 移行
PR 3: wrapper 追加
PR 4: old API deprecated
PR 5: cleanup deferred
```

`main` は各 cutover の前後で buildable であればよい。cutover 内の commit すべてが independently buildable である必要はない。

merge は squash でもよい。

## 8. テスト戦略

### 8.1 試験の目的

テストは旧内部構造を保存するために使わない。

保存するのは次である。

- IPC wire behavior
- owner / sequence behavior
- controller report bytes
- neutral invariant
- startup / shutdown behavior
- production transport ordering

内部 function call、struct layout、旧 facade API は保存対象ではない。

### 8.2 削除に追従する

旧 module を削除したら、その module の unit test も削除する。

同じ behavior が新 module で必要なら、新 module の public boundary で test する。

禁止:

```text
旧 test fixture を compatibility header で生かす
旧 fake backend を新 host へ adapter する
old symbol を test-only build で残す
```

### 8.3 最小 software suite

必要な test:

```text
protocol bytes
application command / event
revoke invariant table
IPC codec / framing
BTstack event decoding
host startup / shutdown ordering
one architecture journey
include / target boundary
```

増やさない test:

```text
各 wrapper の unit test
各 forwarding function の test
old/new implementation parity test
dual-path equivalence test
```

### 8.4 revoke invariant

一つの table-driven test で確認する。

```text
release
owner disconnect
heartbeat timeout
shutdown
fatal transport error
```

各行の期待:

```text
owner cleared
controller neutral
sequence reset
neutral send requested
reason counter updated
```

各 reason を個別の integration test や実機試験にしない。

### 8.5 architecture journey

一つだけ持つ。

```text
start host
→ IPC JSON acquire
→ IPC JSON set Button A
→ report tick
→ fake HID captures non-neutral report
→ client disconnect
→ fake HID captures neutral report
→ reacquire
→ set Button A
→ shutdown
→ fake HID captures trailing neutral
→ power-off
→ stop
```

確認する順序:

```text
last non-neutral
< trailing neutral
< power-off
```

## 9. Hardware Gate H1

実機試験は、三つの cutover が完了した後に一回だけ行う。

### 9.1 内容

```text
1. 既存 bond で Switch と接続
2. Button A を送る
3. Switch UI と HCI dump で non-neutral report を確認
4. owner client を release せず切断
5. neutral report を確認
6. client を再接続
7. Button A を送る
8. daemon を停止
9. 最後の non-neutral より後、HCI power-off より前に neutral report があることを確認
10. transport error がないことを確認
```

### 9.2 除外

```text
heartbeat timeout
explicit release
stick 全方向
report period 比較
sleep / resume
USB removal / reinsertion
bonded reconnect persistence
初回 pairing 詳細
```

これらを architecture cutover の合否へ混ぜない。

### 9.3 pass manifest

```json
{
  "result": "pass",
  "commit": "<git-sha>",
  "adapter": "0a12:0001",
  "switch_model": "Switch 2",
  "firmware": "22.1.0",
  "report_period_us": 8000,
  "button_a_observed": true,
  "owner_disconnect_neutral": true,
  "shutdown_trailing_neutral": true,
  "transport_errors": 0
}
```

raw HCI dump は artifact として保持し、repository へ全文転記しない。

### 9.4 failure 時

H1 が失敗した場合、旧経路を復活させない。

対応:

```text
new path を修正
または
cutover merge を Git revert
```

禁止:

```text
旧 runtime を fallback として再導入
環境変数で旧経路へ切り替え
temporary compatibility backend を追加
```

## 10. compatibility の判定基準

残してよい compatibility は外部契約だけである。

| 対象 | 判定 |
|---|---|
| IPC wire format | 原則維持。変更するなら明示的に protocol version を上げる |
| CLI / environment variable | 利用実績があるものだけ維持 |
| Switch report bytes | 実機仕様として維持 |
| bond data | reconnect 実装時に migration 方針を決める |
| internal C API | 維持しない |
| internal struct layout | 維持しない |
| internal header path | 維持しない |
| CMake target | 維持しない |
| test fixture API | 維持しない |
| fake backend API | 維持しない |
| old runtime path | 削除 |
| aggregate target | 削除 |
| forwarding wrapper | 削除 |

## 11. 禁止事項

次を新規に追加してはならない。

```text
compat_*.c
legacy_*.c
*_v1 と *_v2 の同居
deprecated header
typedef による旧型 alias
old/new runtime switch
fallback backend
dual-write
dual-read
shadow state
aggregate target
旧 test 専用 API
旧 code path を通す smoke test
```

例外は IPC wire protocol version の並存だけである。それも明確な廃止方針を持たせる。

## 12. 完了条件

次をすべて満たしたとき、リアーキテクチャ完了とする。

```text
[ ] swbt_app_t が daemon logical state の唯一の owner
[ ] swbt_app_t が opaque
[ ] controller state に command metadata がない
[ ] release / disconnect / timeout / shutdown が同じ revoke を通る
[ ] swbt_ipc_session_t が存在しない
[ ] state_mailbox が存在しない
[ ] swbt_daemon_runtime_t が存在しない
[ ] swbt_daemon_runtime_backend_t が存在しない
[ ] production backend ops table が存在しない
[ ] swbt_core が存在しない
[ ] forwarding wrapper が存在しない
[ ] old/new runtime switch が存在しない
[ ] IPC は transport / framing / codec / client identity に限定
[ ] BTstack adapter は daemon / IPC internal type を参照しない
[ ] host が startup / shutdown / cleanup を所有
[ ] shutdown neutral send attempt が power-off より前
[ ] architecture journey が JSON から fake HID まで通る
[ ] architecture journey が aggregate target を使わない
[ ] module test が対象 target へ直接 link
[ ] 禁止依存が compile error になる
[ ] Hardware Gate H1 が pass
[ ] architecture spec と実装が一致
[ ] README / status と実装が一致
```

存在しないことを機械的に検査する。

```sh
grep -R "swbt_ipc_session_t" . && exit 1 || true
grep -R "swbt_state_mailbox" . && exit 1 || true
grep -R "swbt_daemon_runtime_t" . && exit 1 || true
grep -R "swbt_daemon_runtime_backend_t" . && exit 1 || true
grep -R "swbt_daemon_production_backend_ops_t" . && exit 1 || true
grep -R "swbt_core" . && exit 1 || true
```

vendor、historical log、Git metadata は検索対象から除外する。

## 13. 今回混ぜない作業

次は architecture cutover 後に行う。

- bonded reconnect persistence
- bond store
- adapter removal / reinsertion recovery
- sleep / resume recovery
- status / observability protocol
- parser fuzz
- Windows native CI
- release packaging
- BTstack license を含む配布方針
- 複数 controller
- Joy-Con
- NFC / IR semantic support

順序:

```text
破壊的 architecture cutover
→ Hardware Gate H1
→ bonded reconnect
→ recovery / lifecycle hardening
→ status / observability
→ IPC / platform hardening
→ release / license boundary
```

status protocol は state ownership 完了前に固定しない。

## 14. 後続作業者への開始手順

1. 最新 `main` で `just verify` を通す。
2. 本レビュー断面以降の変更を分類する。
3. 破壊的変更を許可する work unit を作る。
4. integration branch を作る。
5. Cutover A を完了し、旧 session / mailbox を削除する。
6. Cutover B を完了し、旧 runtime / backend を削除する。
7. Cutover C を完了し、`swbt_core` と旧 test を削除する。
8. Full software gate を通す。
9. Hardware Gate H1 を一回実施する。
10. pass 後に architecture spec、README、status を更新する。
11. compatibility item inventory は閉じる。新たな deferred item を作らない。
12. 次の機能開発へ進む。

最初に確認する検索:

```sh
grep -R "swbt_app_t" swbt tests
grep -R "swbt_ipc_session_t" swbt tests
grep -R "swbt_state_mailbox" swbt tests
grep -R "swbt_daemon_runtime_t" swbt tests
grep -R "swbt_daemon_runtime_backend_t" swbt tests
grep -R "swbt_daemon_production_backend_ops_t" swbt tests
grep -R '#include "daemon/' swbt/btstack_bridge
grep -R '#include "ipc/' swbt/btstack_bridge
grep -R 'btstack_' swbt/application
grep -R "swbt_core" CMakeLists.txt cmake swbt tests
```

## 15. 判断の要点

今回のリアーキテクチャでは、安全性を「旧構造を残すこと」と同一視しない。

安全性は次で確保する。

```text
small application invariant tests
one architecture journey
host shutdown ordering test
CMake dependency enforcement
one hardware gate
Git rollback
```

旧経路、互換 facade、aggregate target、dual-write は安全策ではない。将来の変更時にどちらが正しいか分からなくなるため、むしろ障害要因になる。

最終判断は次のとおりである。

> **新しい application / host / adapter 境界を唯一の実装経路にし、旧 session、mailbox、runtime、backend table、aggregate target、compatibility test を同じ cutover 内で削除する。内部互換性より、単一の依存方向と単一の状態所有を優先する。**

## 16. レビュー上の注意

このレビューは公開リポジトリに対する静的レビューである。

GitHub 上の source、spec、work unit、test、CMake、commit history は確認したが、レビュー環境からの独立 clone、local build、CTest、実機実行は行っていない。

repository record では、基準断面において `just verify` が通り、debug / ASan / clang-tidy / Windows MinGW cross build を含む software verification が成功したと記録されている。この記録は有用だが、外部レビュー側での再実行結果ではない。

## 17. 参照

- [Application boundary spec](https://github.com/niart120/swbt-daemon/blob/74508a7/spec/architecture/daemon-application-boundary-rearchitecture.md)
- [`swbt/application/app.h`](https://github.com/niart120/swbt-daemon/blob/74508a7/swbt/application/app.h)
- [`swbt/application/app.c`](https://github.com/niart120/swbt-daemon/blob/74508a7/swbt/application/app.c)
- [`swbt/ipc/ipc_session.h`](https://github.com/niart120/swbt-daemon/blob/74508a7/swbt/ipc/ipc_session.h)
- [`swbt/ipc/ipc_adapter.c`](https://github.com/niart120/swbt-daemon/blob/74508a7/swbt/ipc/ipc_adapter.c)
- [`swbt/daemon/runtime.h`](https://github.com/niart120/swbt-daemon/blob/74508a7/swbt/daemon/runtime.h)
- [`swbt/daemon/runtime.c`](https://github.com/niart120/swbt-daemon/blob/74508a7/swbt/daemon/runtime.c)
- [`swbt/daemon/production_backend_ops.h`](https://github.com/niart120/swbt-daemon/blob/74508a7/swbt/daemon/production_backend_ops.h)
- [`swbt/btstack_bridge/production_btstack.c`](https://github.com/niart120/swbt-daemon/blob/74508a7/swbt/btstack_bridge/production_btstack.c)
- [`CMakeLists.txt`](https://github.com/niart120/swbt-daemon/blob/74508a7/CMakeLists.txt)
- [`tests/daemon_cutover_journey_test.c`](https://github.com/niart120/swbt-daemon/blob/74508a7/tests/daemon_cutover_journey_test.c)
- [`tests/cmake/cutover_acceptance_test.cmake`](https://github.com/niart120/swbt-daemon/blob/74508a7/tests/cmake/cutover_acceptance_test.cmake)
