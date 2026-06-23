# Daemon Application Boundary Rearchitecture Before Architecture Cutover

## 1. 状態

superseded。

この文書は `spec/architecture/daemon-architecture-cutover.md` に置き換えられた。段階移行、互換 wrapper、`swbt_core` aggregate target の残置を前提にした判断は、破壊的 architecture cutover の方針としては採用しない。

この spec は、現行の `spec/architecture/daemon-runtime-boundaries.md` を置き換えない。現行実装の責務境界を保ちながら、application 所有の状態モデルへ段階的に寄せるための設計候補である。

一時提案文書の内容は、この spec と関連 work unit record に正規化して取り込む。`tmp/` 配下の文書は恒久的な根拠として扱わない。

`local_055` cutover acceptance では、残る中継点を current spec の責務定義付き item として棚卸しした。理由不明の compatibility wrapper は cutover 完了条件を満たさない。

## 2. 目的

daemon の振る舞い、入力所有権、controller state、neutral 化の責務を、IPC や BTstack adapter から切り離す。

現行実装は実機 bring-up まで到達しており、全面書き換えの対象ではない。この spec の目的は、動作済み経路を残したまま、次の境界をコードとビルド構成で確認できる形にすることである。

- controller state と配送メタデータを分ける。
- owner policy と neutral 化を IPC transport から独立させる。
- BTstack callback と daemon state の接続点を明示する。
- production backend の広い ops table を、実際に差し替える能力単位へ縮小する。
- CMake target と include path で禁止依存を検出できる状態に近づける。

## 3. 適用範囲

- `swbt/switch/` の controller state、report builder、subcommand reply との関係。
- `swbt/ipc/` の owner、sequence、status、JSON codec、session boundary。
- `swbt/daemon/` の runtime lifecycle、mailbox、production backend。
- `swbt/btstack_bridge/` の BTstack callback、timer、HID send、IPC pump。
- unit test、fake backend、Windows production 実機 gate。
- CMake target と include boundary。

次は対象外である。

- Switch HID report bytes、subcommand bytes、SPI data、rumble packet の新規値。
- bonded reconnect、bond store、adapter recovery の実装。
- status protocol v1 の追加 field 固定。これは `work-units/complete/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md` で完了済みであり、この rearchitecture では拡張しない。
- 実機受入試験手順の詳細。これは operations spec または別 work unit で扱う。
- BTstack 本体の変更。

## 4. 決定事項

この spec の状態は draft である。以下は採用候補であり、実装 PR ごとに検証して current spec へ昇格する。

### 4.1 現行境界は維持しながら移行する

現行の `daemon-runtime-boundaries.md` は current spec として維持する。rearchitecture は、既存の noop backend、production opt-in、実機承認条件、Switch2 / CSR8510 A10 / WinUSB の観測済み経路を壊さない範囲で進める。

Phase や PR の単位ごとに実機試験を要求しない。実機が必要になるのは、BTstack callback registration、HID send、timer / scheduling、production composition、shutdown 呼び出し順、Switch-facing bytes を変えた場合である。

### 4.2 controller state から配送メタデータを外す

作業開始時の `client_seq` は Switch へ送る controller state ではなく、IPC command の順序と診断に使う配送メタデータだった。

`local_050` 完了後の controller state は report 生成に必要な値だけを持つ。`seq` は set-state command metadata として受け取り、`swbt_control_lease_t` と `swbt_ipc_status_t.last_seq` が保持する。既存 IPC status の `last_seq` field は互換性を保つ。

これは `seq` を daemon protocol から削除する作業ではない。`seq` は wire protocol と debug client option として残り、Switch report bytes には埋め込まない。

### 4.3 owner policy と neutral 化は application 側へ寄せる

現行の `ipc_session` は owner、controller state、rumble status、mailbox publish、release / disconnect / heartbeat timeout の neutral 化を持つ。これは初期実装としては動作しているが、IPC transport の責務を超えている。

採用候補は、IPC adapter が JSON と socket を扱い、owner policy と neutral 化は application command handler が扱う構造である。

`local_050` では、`control_lease` 相当の小さな型を IPC 非依存に抽出し、`ipc_session` は互換 wrapper として残す。release、owner disconnect、heartbeat timeout は session-side の neutral publish helper を共有する。shutdown を含む application revoke policy への移動は `local_051` 以降で扱う。

`local_051` 完了後は、`swbt_app_t` が owner、last accepted sequence、controller state、revoke policy を所有する。`ipc_session` は lock、mailbox publish、rumble status、IPC 互換 result mapping を持つ forwarding wrapper として残る。stale `seq` は application API では stale として観測できるが、IPC wire では既存 client 互換のため `state_accepted` を返し、status の `last_seq` と controller state は更新しない。

### 4.4 application boundary は typed command / event で閉じる

IPC から application へは、JSON line や socket state ではなく typed command を渡す。BTstack から application へは、BTstack packet そのものではなく typed event を渡す。

この境界では、BTstack の vendor header、socket 型、WinUSB/libusb の detail を application public API に出さない。

`local_052` 完了後は、`swbt/ipc/ipc_json.*` が typed command / response codec を持ち、`swbt/ipc/ipc_adapter.*` が codec と `swbt_ipc_session_t` / application command API を接続する。JSON codec は session、application、mailbox、socket を受け取らない。IPC server と daemon IPC runner は disconnect、heartbeat timeout、shutdown を adapter event entrypoint へ渡す。

### 4.5 port interface は実際に差し替える能力だけにする

production backend の ops table は、IPC、platform、HID、timer、subcommand reply、SSP、address、clock、power、run loop を横断している。

小さな port interface へ分ける候補はあるが、機械的に関数ポインタを増やさない。差し替える理由がある能力だけを port にする。

初期候補は次である。

- HID send / can-send。
- timer。
- clock。
- bond store。ただし bonded reconnect work unit が source になった時点で扱う。

`local_053` 完了後は、HID send / can-send と BTstack run loop timer が `swbt/btstack_bridge/*_port.*` に分かれる。`input_report_timer_adapter` は直接 `hid_device_*` や `btstack_run_loop_*` を呼ばず、port 経由で report send と timer 操作を行う。

`timer_port.h` は `btstack_timer_source_t` を扱うため BTstack header を含む。これは `btstack_bridge` 内の port API であり、application public API ではない。application 側へ vendor header を広げない制約は維持する。

### 4.6 BTstack adapter は daemon 内部型を減らす

`local_054` 完了後の `production_btstack` は、BTstack run loop 上の IPC pump schedule を持つが、daemon IPC runner 型を直接参照しない。IPC runner の start / stop と poll callback の作成は `production_backend` が担当する。

移行後は、BTstack adapter が daemon の IPC runner や production backend 内部型を直接所有しない構造を目標にする。composition root が IPC adapter、BTstack adapter、application、platform shutdown listener を束ねる。

BTstack run loop を system reactor として使う方針は残してよい。ただし application state を更新できる実行主体を一つに絞り、callback から直接複数の状態所有者を変更しない。

`local_053` 完了後は、BTstack HID callback packet の raw offset 判定を `hid_event` decoder に閉じ込める。`production_backend` は typed event を受けて report timer と SSP confirmation へ dispatch する。

`local_054` 完了後は、`production_btstack` は `daemon/ipc_runner.h` と `daemon/production_backend.h` を include しない。`swbt_daemon_production_backend_ops_t` 返却は残るため、これは実機済み production composition の互換 glue として扱い、`local_055` で削除または current spec 上の責務定義を要求する。

`local_055` 完了後の `production_btstack` IPC pump は、BTstack run loop 上で generic IPC pump callback を schedule する current port adapter として残す。production backend ops table は production backend の hardware-facing 能力を composition root へ渡す current port table として残す。どちらも削除条件は `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md` に記録する。

### 4.7 CMake target 分割は後半の検出手段にする

現行の `swbt_core` は core、daemon、IPC、BTstack bridge、Switch protocol をまとめている。この状態では include 依存の禁止をビルドで検出しにくい。

target 分割は最初の作業にしない。先に source の所有者を狭め、compatibility wrapper を薄くしてから、controller / protocol / application / adapter / host の単位へ target を分ける。

`local_054` 完了後は、`swbt_switch_protocol` と `swbt_application` target を分け、protocol / application test は `swbt_core` aggregate target を経由しない。`swbt_core` は互換用 aggregate target として残るが、`local_055` で残置理由と削除条件を棚卸しする。

`local_055` 完了後の `swbt_core` aggregate target は、daemon executable、public C ABI、IPC、BTstack bridge を結合する current integration target として残す。protocol と application の単体 target は分離済みであり、aggregate target だけで禁止依存を見逃す範囲は `tests/cmake/include_boundaries_test.cmake` と `tests/cmake/cutover_acceptance_test.cmake` で補う。

移行中は grep または小さな script で禁止 include を確認してよい。最終的には target include directory の制約で検出する。

### 4.8 roadmap 文書の番号は採用しない

一時 roadmap 文書にある `local_046` 以降の番号は、現在の repository では別 work unit として使用済みである。番号つき roadmap としては採用しない。

有用な項目は次の置き場へ分ける。

- current state / support matrix: 完了済みの `work-units/complete/local_047` と `docs/status.md`。
- status / observability: `work-units/complete/local_039`。
- bonded reconnect: `docs/status.md` の未確認項目から別 work unit を起こす。
- acceptance harness、lifecycle hardening、IPC / platform hardening、release / license boundary: 後続 work unit の source 候補として扱う。

### 4.9 最終状態を明示する

移行の最終状態では、互換 wrapper、旧 runtime backend、旧 `ipc_session` application logic、state mailbox、旧 production backend glue を「動くから残す」扱いにしない。残す場合は current spec に責務を定義し、削除しない理由を別の work unit record に書く。

最終状態は次を満たす。

- application state の authoritative owner が一つである。
- IPC は transport、framing、JSON encode / decode、client connection ID だけを扱う。
- BTstack adapter は BTstack callback と port 実装だけを扱う。
- controller state に command metadata が含まれない。
- release、owner disconnect、heartbeat timeout、shutdown は同じ revoke policy を通る。
- application public API は BTstack vendor header、socket 型、WinUSB/libusb の detail を公開しない。
- adapter は application 内部 field を直接参照しない。
- CMake target と include path が禁止依存を検出する。
- production executable は新しい composition root だけを使う。
- compatibility wrapper が残る場合は、期限、削除条件、後続 work unit を持つ。

### 4.10 work unit roadmap

rearchitecture は次の work unit record に分ける。番号は現在の repository の未使用番号へ割り当てる。

| work unit | 目的 | 完了時の状態 |
|---|---|---|
| `local_050` | controller state と command metadata を分離し、IPC 非依存の control lease を作る | owner policy と latest sequence は control lease へ移る |
| `local_051` | application command API と revoke policy を導入する | owner、sequence、neutral 化の authoritative owner が application になる |
| `local_052` | IPC adapter と JSON codec を application command API へ接続する | JSON codec は副作用を持たず、IPC は transport / codec に閉じる |
| `local_053` | BTstack port / typed event 境界を導入する | HID event decode、HID send / can-send、timer 操作は port / typed event 境界へ移り、残る production IPC pump glue は `local_054` / `local_055` の入力になる |
| `local_054` | daemon host と CMake include boundary を分ける | composition root と target 境界で禁止依存を検出できる |
| `local_055` | cutover、acceptance、互換層削除を完了する | 旧 glue / wrapper / aggregate dependency に未分類の残置がない |

`local_039` は status / observability の既存 work unit として並行して扱う。status field の追加は rearchitecture の必須前提ではないが、application が authoritative state を持つ設計と矛盾させない。

### 4.11 テストと実機 gate

移行中の検証は、実機でなければ分からない事項だけを実機へ残す。構造変更ごとに毎回実機試験を要求しない。

自動試験で閉じる範囲:

- controller state と metadata の分離。
- owner lease、authorization、sequence handling。
- release、owner disconnect、heartbeat timeout、shutdown request の revoke invariant。
- JSON parse / serialize と IPC response 互換性。
- fake HID、fake timer、fake clock を使う synthetic journey。
- startup failure と cleanup ordering。
- include boundary / forbidden include check。

実機試験を要求する変更:

- BTstack callback registration。
- HID send または can-send の順序。
- timer / send scheduling。
- production executable の composition。
- shutdown 時の HCI power-off と neutral send の呼び出し順。
- Switch-facing report bytes、subcommand bytes、descriptor data。
- GAP、SSP、SDP、device identity、bond store。

rearchitecture の production cutover では、`local_053` と `local_054` の統合候補を一度の hardware gate に通す。`local_055` が削除と CMake boundary だけなら追加実機は不要とし、production composition、BTstack 初期化、report bytes、timer / send scheduling、shutdown 順序を変えた場合だけ再度実機 gate を立てる。

### 4.12 互換層の扱い

互換層は移行のためだけに置く。work unit record には、互換層の所有者、呼び出し元、削除条件、削除予定 work unit を書く。

次の状態を禁止する。

- 新旧両方が同じ state を authoritative に書く。
- compatibility wrapper が新しい application API より広い責務を持つ。
- target 分割後も古い aggregate target だけで test が通るため、禁止依存を検出できない。
- 実機 pass を理由に、責務不明な旧 glue を current spec なしで残す。

互換層を残す必要がある場合は、`local_055` の checklist に未削除理由と削除先を残す。理由を説明できない互換層は、cutover 完了条件を満たさない。

## 5. 根拠

この spec は新しい Switch protocol 値、BTstack source selection、report period、WinUSB/libusb 実測値を追加しない。根拠監査は not applicable とする。

| 項目 | 根拠 | source | status |
|---|---|---|---|
| controller state は command sequence metadata を含まない | implementation fact | `swbt/switch/switch_controller_state.h` | current after `local_050` |
| set-state `seq` は `swbt_control_lease_t` と `swbt_ipc_status_t.last_seq` が保持する | implementation fact | `swbt/application/control_lease.h`, `swbt/ipc/ipc_session.h` | current after `local_050` |
| `swbt_app_t` は owner、latest sequence、controller state、revoke policy を持つ | implementation fact | `swbt/application/app.h`, `swbt/application/app.c` | current after `local_051` |
| `ipc_session` は application、rumble、mailbox を持つ互換 wrapper である | implementation fact | `swbt/ipc/ipc_session.h`, `swbt/ipc/ipc_session.c` | current after `local_051` |
| release / disconnect / heartbeat timeout / shutdown は application revoke policy を通る | implementation fact | `swbt/application/app.c`, `swbt/ipc/ipc_session.c` | current after `local_051` |
| JSON codec は typed command / response の decode / encode を行い、session や socket を受け取らない | implementation fact | `swbt/ipc/ipc_json.h`, `swbt/ipc/ipc_json.c`, `tests/ipc_json_test.c` | current after `local_052` |
| IPC adapter が JSON codec と session / application command API を接続する | implementation fact | `swbt/ipc/ipc_adapter.h`, `swbt/ipc/ipc_adapter.c`, `swbt/ipc/ipc_server.c`, `swbt/daemon/ipc_runner.c` | current after `local_052` |
| BTstack HID raw packet decode は typed event に変換される | implementation fact / source audit | `swbt/btstack_bridge/hid_event.h`, `swbt/btstack_bridge/hid_event.c`, `spec/references/btstack-hid-event-port-boundary.md` | current after `local_053` |
| HID send / can-send と timer 操作は BTstack bridge port を通る | implementation fact / source audit | `swbt/btstack_bridge/hid_port.h`, `swbt/btstack_bridge/hid_port.c`, `swbt/btstack_bridge/timer_port.h`, `swbt/btstack_bridge/timer_port.c`, `swbt/btstack_bridge/input_report_timer_adapter.c`, `spec/references/btstack-hid-event-port-boundary.md` | current after `local_053` |
| daemon runtime output handler は rumble status を application boundary 経由で記録する | implementation fact | `swbt/daemon/runtime.c`, `tests/daemon_runtime_test.c` | current after `local_053` |
| daemon runtime backend は IPC、HID、timer、reply queue、device info を横断する | implementation fact | `swbt/daemon/runtime.h` | current |
| production backend ops は platform、HID、timer、SSP、clock、power、run loop と IPC pump port を横断する | implementation fact | `swbt/daemon/production_backend_ops.h` | current after `local_054` |
| BTstack production adapter は daemon IPC runner 型を参照せず、generic IPC pump callback を BTstack run loop へ登録する | implementation fact | `swbt/btstack_bridge/production_btstack.h`, `swbt/btstack_bridge/production_btstack.c`, `swbt/daemon/production_backend.c` | current after `local_054` |
| `swbt_core` は互換用 aggregate target として残り、protocol / application は個別 target として分かれる | implementation fact | `CMakeLists.txt`, `tests/cmake/include_boundaries_test.cmake` | current after `local_054`; aggregate cleanup tracked by `local_055` |
| `local_055` は remaining compatibility inventory を分類し、synthetic journey を自動試験へ固定した | implementation fact / verification | `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`, `tests/cmake/cutover_acceptance_test.cmake`, `tests/daemon_cutover_journey_test.c` | current after `local_055` |
| current state / support matrix は更新済みである | implementation fact / completed work unit | `docs/status.md`, `work-units/complete/local_047/CURRENT_STATE_AND_SUPPORT_MATRIX.md` | current |

## 6. 関連 work units

- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`
- `work-units/complete/local_051/DAEMON_APPLICATION_COMMAND_API.md`
- `work-units/complete/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md`
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`
- `work-units/complete/local_054/DAEMON_HOST_AND_BUILD_BOUNDARIES.md`
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`
- `work-units/complete/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`
- `work-units/complete/local_047/CURRENT_STATE_AND_SUPPORT_MATRIX.md`
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`
- `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md`
- `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`
- `work-units/complete/local_044/PRODUCTION_DAEMON_SHUTDOWN_PATH.md`

## 7. 未解決事項

- 最新 `main` の include graph と call graph を機械的に取得し、禁止依存の候補を確定する。
- BTstack callback、IPC poll、shutdown listener が application state を変更する実行コンテキストを確認する。
- `state_mailbox` が必要になった競合条件を、`local_024` と現行 code から再確認する。
- shutdown 直前の neutral send を application revoke 処理へ寄せた場合に、HCI power-off 前の送信保証が維持されるか確認する。
- `swbt_ipc_session_t`、`state_mailbox`、`production_btstack` IPC pump、production backend ops table、`swbt_core` aggregate target は `local_055` で current responsibility と削除条件を棚卸し済みである。これらを小さくする場合は、対象 item と置換先を source にした後続 work unit record を起こす。
- bonded reconnect と bond store port は別 work unit の source から起こす。
