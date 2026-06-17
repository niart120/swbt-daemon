# swbt-daemon: Switch Pro Controller daemon IPC 設計仕様

- 版: 0.3
- 作成日: 2026-06-15
- 更新日: 2026-06-17
- 対象: `vendor/btstack` submodule の BTstack `v1.8.2` を利用する swbt-daemon と daemon IPC 実装

## 0. 設計の位置づけ

clean な `swbt-daemon` repository を root とし、BTstack は `vendor/btstack` の Git submodule として利用する。現在の pin は `v1.8.2` / `075a0780f0fad7ff67d58ac19f46e8953656a752` である。

Nintendo Switch に Bluetooth Classic HID Device として Pro Controller 相当に見える daemon を実装する。実行形態は **daemon IPC を主経路** とする。daemon は Bluetooth アダプタ、BTstack run loop、Switch Pro Controller プロトコル、HID report 送信周期を所有する。CLI debug client、将来の Python / C# / GUI などのクライアントは、daemon に対して現在のコントローラー状態を送る。

C ABI は daemon 内部、単体テスト、将来の代替組み込み経路として残す。ただし、外部クライアントの通常利用では daemon IPC を使う。

root `LICENSE` は swbt-daemon 自前コードだけに適用する MIT License とする。BTstack は別ライセンスであり、BTstack を含む binary / release は MIT-only artifact として扱わない。詳細は `THIRD_PARTY_NOTICES.md` に記録する。

daemon に時間指定コマンドは実装しない。

- daemon は `tap A 80ms`、`sequence`、`at_ms`、`duration_ms` を解釈しない。
- クライアントは、押下状態・スティック状態・IMU 状態を **状態スナップショット** として送る。
- daemon は最新状態を保持し、固定周期で Switch へ HID input report を送る。
- ボタンを 80ms 押す、300ms スティックを倒す、といった操作はクライアント側の責務とする。

目的は、Bluetooth と Switch 固有プロトコルを daemon に閉じ込め、クライアント側を安全で薄い制御層にすることである。

## 1. 判断の要約

### 1.1 採用方針

採用する構成は次の通り。

```text
Client application
  CLI / test runner / future Python, C#, GUI
  │
  │ JSON Lines over local IPC
  ▼
swbt-daemon
  │
  ├─ IPC server
  ├─ client ownership / validation
  ├─ latest controller state
  ├─ report scheduler
  ├─ Switch Pro Controller protocol
  ├─ BTstack HID Device bridge
  └─ WinUSB / libusb Bluetooth adapter access
      │
      ▼
Nintendo Switch
```

daemon は「現在の状態を入力 report として出し続ける装置」として扱う。クライアントは必要なタイミングで状態を更新するだけでよい。

### 1.2 C ABI の扱い

C ABI は外部クライアントの第一経路にはしない。

C ABI は次の用途に限定する。

- daemon 内部から `libswbt_core` を呼ぶ境界
- 単体テスト・結合テスト
- 将来、組み込み先が daemon を使えない場合の代替経路

Python などの通常利用では、daemon IPC を使う。

理由は次の通り。

- BTstack run loop の所有権を daemon に固定できる。
- native 側クラッシュが Python プロセスを巻き込む範囲を減らせる。
- Python、C#、Node.js、CLI から同じインターフェースを使える。
- Bluetooth アダプタの専有、再接続、ログ、状態診断を daemon に集約できる。
- latency の支配項は IPC ではなく HID report tick と Bluetooth 側である可能性が高い。

## 2. daemon IPC 案のセルフレビュー

### 2.1 良い点

#### BTstack の単一 run loop と相性が良い

BTstack は run loop と timer を中心に動く。daemon 化すると、BTstack run loop を 1 プロセス内で所有できる。クライアント側の Python event loop、GUI event loop、C# thread、REPL などを BTstack 側に混ぜずに済む。

#### 言語非依存になる

IPC はローカルソケット上の JSON Lines とする。Python からは標準ライブラリだけで接続できる。C#、Go、Node.js でも実装が容易である。

#### 障害範囲を分離できる

C ABI 直呼びでは、native 側の不正メモリアクセスや DLL unload 問題がホストアプリを巻き込む。daemon 方式では、少なくともクライアントプロセスとは分離できる。

#### latency の増加が限定的

125Hz report であれば、次 tick までの待ち時間は平均約 4ms、最大約 8ms である。ローカル IPC の追加遅延は、設計が重くなければこの tick 待ちに比べて小さい見込みである。

### 2.2 悪い点と対策

#### クライアントのタイミング揺れがそのまま入力に出る

時間指定コマンドを daemon に持たせないため、`A を 80ms 押す` という動作はクライアントが次の 2 回の状態更新で表現する。

```text
1. buttons に A を含む state を送る
2. クライアント側で 80ms 待つ
3. buttons から A を外した state を送る
```

この方式では、クライアントの sleep 精度、GC、GUI thread の詰まり、OS scheduling が押下時間に影響する。

対策:

- daemon の仕様としては、時間指定コマンドを入れない。
- 将来の client library には便宜関数 `tap()` を実装してよいが、これは daemon protocol ではなく client-side helper とする。
- 高精度マクロが必要になった場合は、別プロジェクトまたは将来仕様として扱う。v0 には入れない。

#### 複数クライアントが同時に状態を送ると競合する

複数のクライアントが同時に `set_state` を送ると、最後に届いた状態で上書きされる。これは予測不能な操作につながる。

対策:

- v0 では単一 writer モデルにする。
- `acquire` で active owner を取得したクライアントだけが `set_state` できる。
- owner 以外のクライアントは `get_status` と `subscribe` のみ許可する。
- owner が切断した場合、daemon はコントローラー状態を neutral に戻し、owner を解放する。

#### state が古いまま残る危険がある

クライアントがボタン押下状態を送ったあと異常終了すると、daemon がその状態を保持し続ける可能性がある。

対策:

- owner の IPC 接続が切れたら、daemon は即座に neutral state を適用する。
- 追加で heartbeat timeout を設定できるようにする。ただしこれは安全機構であり、入力時間指定機能ではない。
- `--fail-safe-neutral-on-disconnect=true` を既定値にする。

#### JSON parse が送信周期に干渉する可能性がある

daemon が BTstack run loop と同じ thread で JSON parse を行うと、125Hz の report tick に揺れが出る。

対策:

- IPC accept/read/parse は IPC thread で行う。
- BTstack thread には検証済みの固定サイズ `swbt_state_t` を渡す。
- BTstack thread から IPC socket へ直接 write しない。
- IPC thread と BTstack thread の境界は、single-producer/single-consumer queue または mutex 付き latest-state buffer に限定する。

#### report rate の正解が固定できない

Pro Controller の report rate については資料上の表現が揺れている。dekuNukem の HID notes は Standard full mode の input report について「60Hz、Pro Controller では120Hz」と記述している。一方、Linux `hid-nintendo.c` のコメントは、標準 reporting mode で Pro Controller Bluetooth は 8ms 間隔としつつ、実測では 11ms または 15ms になる場合があるとも述べている。

対策:

- 既定値は `report_period_us = 8000`、つまり 125Hz 相当とする。
- `8333us`、`10000us`、`11000us`、`15000us`、`16667us` へ設定変更できるようにする。
- 実機検証時は送信間隔、Switch からの subcommand、切断有無、入力反映の安定性をログに残す。
- 125Hz で不安定なら 120Hz または 66.7Hz/60Hz へ落とす。

## 3. 参考情報と report rate 方針

### 3.1 参照した事実

| 参照元 | 内容 | 設計への反映 |
|---|---|---|
| dekuNukem Nintendo Switch Reverse Engineering `bluetooth_hid_notes.md` | `INPUT 0x30` は Standard full mode。現在状態を 60Hz、Pro Controller では 120Hz で push すると記述。 | 60Hz 固定は低すぎる可能性がある。Pro Controller モードでは 120Hz 近辺を候補にする。 |
| Linux `hid-nintendo.c` | 標準 reporting mode で Pro Controller Bluetooth は 8ms 間隔とコメント。8ms は 125Hz 相当。ただし実測では 11ms または 15ms もあり得ると書かれている。 | 既定値は 8ms とし、互換性のため可変にする。 |
| `joycontrol` | full input report mode の送信遅延を `0.015` 秒としている。 | 66.7Hz 相当でも動作する実装例があるため、fallback rate として残す。 |
| BTstack documentation | run loop は data source と timer を処理する。timer は周期処理に使える。 | HID report 送信は BTstack run loop timer で行う。 |

### 3.2 既定 report rate

v0 の既定値は次とする。

```text
controller_type: pro_controller
transport: bluetooth
input_report_mode: 0x30
report_period_us: 8000
report_rate_hz: 125.0
```

ただし、この値は仕様として固定しない。実機検証により変更できるよう、設定値として外に出す。

設定例:

```toml
[report]
default_period_us = 8000
fallback_period_us = 15000
max_jitter_warn_us = 2000
mode = "fixed_period"
```

CLI 例:

```bash
swbt-daemon --report-period-us 8000
swbt-daemon --report-period-us 8333
swbt-daemon --report-period-us 15000
```

### 3.3 latency 見積もり

これは未検証の設計見積もりである。実測値ではない。

| report rate | period | 次 tick までの平均待ち | 次 tick までの最大待ち |
|---:|---:|---:|---:|
| 125Hz | 8.000ms | 4.000ms | 8.000ms |
| 120Hz | 8.333ms | 4.167ms | 8.333ms |
| 100Hz | 10.000ms | 5.000ms | 10.000ms |
| 66.7Hz | 15.000ms | 7.500ms | 15.000ms |
| 60Hz | 16.667ms | 8.333ms | 16.667ms |

C ABI と daemon IPC の差は、ここでは主支配項にしない。設計上の優先順位は次である。

```text
1. report period
2. BTstack / USB Bluetooth adapter / Bluetooth radio
3. Switch 側の入力処理
4. クライアントの状態更新タイミング
5. IPC 処理時間
```

125Hz を採用すると、60Hz 固定より tick 待ちの最大値は約半分になる。daemon IPC を採用しても、report scheduler が native 側で固定周期を維持できれば、IPC の追加遅延は相対的に小さくなる見込みである。

## 4. スコープ

### 4.1 初期版で実装するもの

- daemon 起動とローカル IPC 待ち受け
- 単一 active owner の取得・解放
- クライアントからの full controller state 受信
- 最新 state の保持
- BTstack HID Device としての Pro Controller 接続
- Output Report parser
- Subcommand ACK
- Device info 応答
- SPI flash read 応答
- Input report mode `0x30`
- 125Hz 既定の periodic input report
- button / stick report 生成
- 最低限の IMU report 生成
- rumble packet 受信と status 反映
- player lights / pad color の応答
- 状態・統計情報取得
- client disconnect 時の neutral state 適用

### 4.2 初期版で実装しないもの

- daemon protocol としての `tap`
- daemon protocol としての `hold duration`
- daemon protocol としての `sequence`
- daemon protocol としての `at_ms`
- daemon 内 scheduler による future input
- Joy-Con L/R の完全再現
- 複数 controller 同時接続
- NFC/IR MCU の完全実装
- amiibo データ読み込み・送信
- 任天堂サービスやゲーム固有仕様への依存機能

client library 側に `tap()` などの便宜関数を置くことは許可する。ただし、その関数は単に `set_state` を複数回送るだけであり、daemon protocol には時間指定情報を送らない。

## 5. 目標アーキテクチャ

### 5.1 プロセス構成

```text
+----------------------+        local IPC        +----------------------+
| CLI debug client      |  ───────────────────▶  | swbt-daemon          |
| test runner           |                         |                      |
| future Python/C#/GUI  |  ◀───────────────────  | - IPC server         |
|                       |        status/event     | - owner manager      |
+----------------------+                         | - latest state       |
                                                 | - report scheduler   |
                                                 | - Switch protocol    |
                                                 | - BTstack bridge     |
                                                 +----------+-----------+
                                                            |
                                                            | HCI / L2CAP / HID
                                                            ▼
                                                 +----------------------+
                                                 | USB Bluetooth dongle |
                                                 +----------------------+
                                                            |
                                                            ▼
                                                 +----------------------+
                                                 | Nintendo Switch      |
                                                 +----------------------+
```

### 5.2 thread 構成

```text
main thread
  - 設定読み込み
  - daemon lifecycle
  - signal handling

ipc thread
  - socket accept
  - client message read
  - JSON parse
  - command validation
  - owner check
  - latest-state buffer への投入

btstack thread
  - BTstack run loop
  - HID Device callback
  - subcommand parser
  - report timer
  - hid_device_send_interrupt_message
```

BTstack thread 以外から BTstack API を直接呼ばない。IPC thread は検証済み state を queue に入れるだけにする。

### 5.3 モジュール分割

```text
swbt/switch/
  switch_types.h
  switch_controller_state.c
  switch_controller_state.h
  switch_report.c
  switch_report.h
  switch_subcommand.c
  switch_subcommand.h
  switch_spi.c
  switch_spi.h
  switch_rumble.c
  switch_rumble.h
  switch_btstack_bridge.c
  switch_btstack_bridge.h

swbt/core/
  swbt_core.c
  swbt_core.h
  swbt_error.c
  swbt_error.h

apps/swbt-daemon/
  main.c
  daemon_config.c
  daemon_config.h
  ipc_server.c
  ipc_server.h
  ipc_json.c
  ipc_json.h
  owner_manager.c
  owner_manager.h
  state_mailbox.c
  state_mailbox.h
  metrics.c
  metrics.h
```

BTstack 本体の `vendor/btstack/src/classic/hid_device.c` などは、原則として改変しない。Switch 固有の例外は `switch_btstack_bridge.c` に閉じ込める。

## 6. daemon IPC 仕様

### 6.1 transport

v0 は TCP loopback を既定とする。

```text
host: 127.0.0.1
port: 59731
encoding: UTF-8
framing: JSON Lines
```

理由:

- Windows / Linux / macOS のクライアントから扱いやすい。
- Python 標準ライブラリだけで実装できる。
- デバッグ時に `nc` や簡易スクリプトで観察できる。

将来の選択肢:

- Linux: Unix domain socket
- Windows: Named Pipe
- 開発用: stdin/stdout bridge

TCP loopback は外部 network interface に bind しない。`0.0.0.0` bind は禁止する。

### 6.2 framing

1 行 1 JSON object とする。

```text
{...}\n
```

制限:

- 1 message 最大 8192 bytes
- UTF-8 のみ
- JSON object 以外は拒否
- unknown field は原則拒否。ただし `meta` object 内は将来拡張用として無視可能。

### 6.3 protocol version

全 message は `v` を持つ。

```json
{"v":1,"type":"hello","client_name":"example"}
```

`v` が未指定または未対応の場合、daemon は error を返す。

### 6.4 認証

v0 の開発中は `--no-auth` を許可する。本番運用では per-run token を使う。

起動時の例:

```bash
swbt-daemon --auth-token-file "$XDG_RUNTIME_DIR/swbt/token"
```

handshake 例:

```json
{"v":1,"type":"hello","client_name":"python-macro","token":"..."}
```

認証失敗時:

```json
{"v":1,"type":"error","code":"auth_failed","message":"invalid token"}
```

### 6.5 owner model

`set_state` は active owner のみ許可する。

owner 取得:

```json
{"v":1,"type":"acquire","mode":"exclusive","request_id":"r1"}
```

成功:

```json
{"v":1,"type":"acquired","request_id":"r1","owner_id":"c7a2f7"}
```

失敗:

```json
{"v":1,"type":"error","request_id":"r1","code":"owner_busy","message":"another client owns the controller"}
```

owner 解放:

```json
{"v":1,"type":"release","owner_id":"c7a2f7"}
```

owner 接続が切れた場合、daemon は次を行う。

1. latest state を neutral に戻す。
2. owner を解放する。
3. `owner_disconnected` event を observer に通知する。

### 6.6 state 更新

`set_state` は full snapshot を送る。partial update は v0 では実装しない。

例:

```json
{
  "v": 1,
  "type": "set_state",
  "owner_id": "c7a2f7",
  "seq": 42,
  "state": {
    "buttons": 1,
    "lx": 2048,
    "ly": 2048,
    "rx": 2048,
    "ry": 2048,
    "accel_x": 0,
    "accel_y": 0,
    "accel_z": 0,
    "gyro_x": 0,
    "gyro_y": 0,
    "gyro_z": 0
  }
}
```

`seq` は任意だが、クライアント側では単調増加させることを推奨する。daemon は `seq` を report には埋め込まない。診断ログと status にのみ使う。

`set_state` に対する ack は既定で返さない。低頻度デバッグ時に ack が必要な場合は、`request_id` を付ける。

```json
{"v":1,"type":"set_state","owner_id":"c7a2f7","seq":43,"request_id":"r43","state":{...}}
```

ack 例:

```json
{"v":1,"type":"state_accepted","request_id":"r43","seq":43}
```

### 6.7 status 取得

```json
{"v":1,"type":"get_status","request_id":"s1"}
```

応答例:

```json
{
  "v": 1,
  "type": "status",
  "request_id": "s1",
  "daemon": {
    "state": "running",
    "uptime_ms": 123456,
    "report_period_us": 8000,
    "actual_report_rate_hz": 124.8,
    "report_jitter_p95_us": 420
  },
  "switch": {
    "connected": true,
    "paired": true,
    "input_report_mode": 48,
    "player_index": 1
  },
  "owner": {
    "present": true,
    "owner_id": "c7a2f7",
    "client_name": "python-macro",
    "last_seq": 42
  },
  "rumble": {
    "updated": true,
    "last_update_ms": 123400,
    "raw": "0001404000014040"
  }
}
```

### 6.8 ping

```json
{"v":1,"type":"ping","request_id":"p1"}
```

```json
{"v":1,"type":"pong","request_id":"p1"}
```

ping は connection health 用であり、入力時間指定には使わない。

### 6.9 subscribe

observer は status event を購読できる。

```json
{"v":1,"type":"subscribe","events":["status","connection","owner","rumble"],"request_id":"sub1"}
```

v0 では event 配信は低頻度に制限する。入力 report ごとの event は出さない。

## 7. controller state 仕様

### 7.1 C struct

```c
typedef struct {
    uint32_t buttons;

    uint16_t lx;
    uint16_t ly;
    uint16_t rx;
    uint16_t ry;

    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;

    uint64_t client_seq;
} swbt_state_t;
```

### 7.2 neutral state

```text
buttons = 0
lx = 2048
ly = 2048
rx = 2048
ry = 2048
accel_x = 0
accel_y = 0
accel_z = 0
gyro_x = 0
gyro_y = 0
gyro_z = 0
```

`accel_*` と `gyro_*` は初期版では固定値または最新クライアント値をそのまま report に詰める。物理的に妥当な IMU シミュレーションは v0 の対象外とする。

### 7.3 stick 範囲

```text
minimum: 0
center: 2048
maximum: 4095
```

IPC では範囲外の値を拒否する。daemon 側で黙って clamp しない。client library 側で clamp helper を提供してよい。

論理座標系:

```text
x: 右方向を増加
y: 上方向を増加
```

Switch HID report への packing 時に必要な反転・変換は `switch_report.c` に閉じ込める。実機テストで座標方向を確認し、必要なら mapping table を更新する。

### 7.4 button bit layout

IPC と C API では、HID report byte layout とは別の canonical bit layout を使う。report builder が canonical bit layout を Switch の input report へ変換する。

| bit | name | Pro Controller での扱い |
|---:|---|---|
| 0 | A | 使用 |
| 1 | B | 使用 |
| 2 | X | 使用 |
| 3 | Y | 使用 |
| 4 | L | 使用 |
| 5 | R | 使用 |
| 6 | ZL | 使用 |
| 7 | ZR | 使用 |
| 8 | PLUS | 使用 |
| 9 | MINUS | 使用 |
| 10 | HOME | 使用 |
| 11 | CAPTURE | 使用 |
| 12 | L_STICK | 使用 |
| 13 | R_STICK | 使用 |
| 14 | DPAD_UP | 使用 |
| 15 | DPAD_DOWN | 使用 |
| 16 | DPAD_LEFT | 使用 |
| 17 | DPAD_RIGHT | 使用 |
| 18 | SL | Pro Controller では未使用 |
| 19 | SR | Pro Controller では未使用 |

未定義 bit が立っている場合、daemon は `invalid_state` error を返す。

## 8. report scheduler 仕様

### 8.1 基本動作

- Switch から input report mode `0x30` が設定されたら、daemon は periodic full input report を開始する。
- 既定周期は 8000us とする。
- timer は monotonic clock を使う。
- `next_deadline += period` で次回期限を進め、処理時間に応じた drift を抑える。
- 送信処理が遅れて deadline を複数回過ぎた場合、古い tick を追いかけて連続送信しない。次の deadline を現在時刻基準で再設定する。

擬似コード:

```c
static void report_timer_cb(btstack_timer_source_t *ts) {
    swbt_state_t state = state_mailbox_load_latest();

    switch_input_report_t report;
    switch_report_build_0x30(&state, &controller_session, &report);

    switch_btstack_send_input_report(&report);

    metrics_record_report_tick(monotonic_now_us());

    schedule_next_report(ts);
}
```

### 8.2 0x21 subcommand reply との関係

Subcommand reply は `0x21` input report として送る。これは periodic `0x30` report とは別の優先キューで扱う。

方針:

- Switch から subcommand が来たら parser が検証する。
- 応答が必要な場合、`switch_subcommand_build_reply()` で `0x21` を生成する。
- `0x21` は可能な限り早く送る。
- `0x21` 送信直後に periodic `0x30` が来る場合、BTstack の send-ready 制約に従い、どちらかを次 tick にずらす。
- periodic `0x30` は 1 tick 落としてもよいが、subcommand reply の欠落は避ける。

### 8.3 immediate send は v0 では入れない

入力状態が変化した瞬間に tick を待たず即時 report を送る案は、latency を下げる可能性がある。しかし v0 では入れない。

理由:

- Switch 側が期待する push rate を超える可能性がある。
- BTstack send-ready 管理が複雑になる。
- periodic 125Hz で最大待ちが 8ms 程度まで下がるため、初期版では固定周期の安定性を優先する。

将来検討:

- `send_on_change = true`
- `min_interval_us = 8000`
- `immediate_first_press_only = true`

## 9. Switch protocol 層

### 9.1 Output Report parser

生の `uint8_t *report` を直接読む処理を callback に置かない。必ず parser を通す。

```c
typedef enum {
    SW_PARSE_OK = 0,
    SW_PARSE_TOO_SHORT,
    SW_PARSE_UNSUPPORTED_REPORT_ID,
    SW_PARSE_UNSUPPORTED_REPORT_TYPE,
    SW_PARSE_UNKNOWN_SUBCOMMAND,
    SW_PARSE_INVALID_ARGUMENT,
} sw_parse_result_t;

typedef struct {
    uint8_t output_report_id;
    uint8_t packet_counter;
    uint8_t rumble[8];
    uint8_t subcommand_id;
    uint8_t subcommand_data[38];
    size_t subcommand_data_len;
    bool has_subcommand;
} switch_output_report_t;

sw_parse_result_t switch_parse_output_report(
    const uint8_t *data,
    size_t len,
    switch_output_report_t *out
);
```

parser の責務:

- `data != NULL` を確認する。
- `len` を確認してから field を読む。
- output report id を判定する。
- `0x01` の場合、rumble + subcommand として処理する。
- `0x10` の場合、rumble only として処理する。
- 未対応 report は error code で返す。

### 9.2 subcommand dispatcher

```c
typedef enum {
    SW_SUBCMD_STATE                = 0x00,
    SW_SUBCMD_MANUAL_BT_PAIRING    = 0x01,
    SW_SUBCMD_REQUEST_DEVICE_INFO  = 0x02,
    SW_SUBCMD_SET_REPORT_MODE      = 0x03,
    SW_SUBCMD_TRIGGER_BUTTONS      = 0x04,
    SW_SUBCMD_SPI_FLASH_READ       = 0x10,
    SW_SUBCMD_SET_PLAYER_LIGHTS    = 0x30,
    SW_SUBCMD_ENABLE_IMU           = 0x40,
    SW_SUBCMD_ENABLE_VIBRATION     = 0x48,
    SW_SUBCMD_GET_REGULATED_VOLTAGE= 0x50,
} switch_subcommand_id_t;
```

`pairing_state = 13` のような整数状態は使わない。subcommand ID と session state を分ける。

### 9.3 仮想 SPI

固定 byte 配列を subcommand ごとに返すのではなく、仮想 SPI 領域を持つ。

```c
typedef struct {
    uint8_t data[0x10000];
    bool initialized;
} switch_virtual_spi_t;
```

初期版では Switch が実際に読む範囲だけを埋める。

- device type
- serial number
- MAC address
- factory stick calibration
- factory IMU calibration
- controller color
- user calibration magic

SPI read は、要求された address と size に基づいて応答を組み立てる。

```c
bool switch_spi_read(
    const switch_virtual_spi_t *spi,
    uint32_t address,
    uint8_t size,
    uint8_t *out,
    size_t out_capacity
);
```

### 9.4 rumble

Switch からの rumble data は入力状態とは分離する。

```c
typedef struct {
    uint8_t raw[8];
    uint64_t updated_at_ms;
    bool updated;
} switch_rumble_state_t;
```

rumble の内容を button state に混ぜない。`get_status` または `subscribe` event でクライアントに通知する。

### 9.5 NFC/IR と amiibo

初期版では未実装とする。

方針:

- NFC/IR subcommand が来た場合は、安全な ACK または未対応応答を返す。
- amiibo file loader は daemon core に入れない。
- 将来実装する場合は `switch_nfc_ir.c` として独立させる。

## 10. IPC と BTstack の境界

### 10.1 latest-state mailbox

`set_state` は queue に全件積まない。複数の state が report tick 間に届いた場合、最新のみを使う。

```c
typedef struct {
    swbt_state_t state;
    uint64_t generation;
    bool has_update;
} swbt_state_mailbox_t;
```

操作:

```c
void state_mailbox_store(swbt_state_mailbox_t *mb, const swbt_state_t *state);
swbt_state_t state_mailbox_load_latest(swbt_state_mailbox_t *mb);
```

実装は mutex 付きでよい。最初から lock-free にしない。125Hz では mutex の負荷より正しさを優先する。

### 10.2 BTstack thread への通知

状態更新のたびに BTstack thread を起こす必要はない。periodic timer が最新状態を読む。

ただし、次は BTstack thread へ通知する。

- daemon stop
- Bluetooth adapter reset
- report period 変更
- controller color 変更
- virtual SPI 更新

BTstack に main thread callback 実行機構が使える場合は、それを使う。使えない port では pipe/eventfd/condition variable などで run loop を起こす。

## 11. daemon lifecycle

### 11.1 起動

```bash
swbt-daemon \
  --adapter auto \
  --ipc tcp:127.0.0.1:59731 \
  --report-period-us 8000 \
  --log-level info
```

起動手順:

1. 設定読み込み
2. IPC server 初期化
3. BTstack platform 初期化
4. HID Device 登録
5. Bluetooth adapter open
6. Switch pairing 待ち
7. IPC accept 開始
8. report mode 設定待ち
9. periodic report 開始

### 11.2 終了

終了条件:

- SIGINT / SIGTERM
- Windows console control event
- IPC admin command `shutdown`
- 致命的な adapter error

終了手順:

1. owner を無効化
2. neutral state を適用
3. periodic report timer を停止
4. HID connection を切断
5. BTstack run loop を停止
6. IPC server を閉じる
7. adapter を close
8. process を終了

ライブラリ内で `exit()` を呼ぶ設計は避ける。daemon の `main()` だけが process exit code を決める。

## 12. 将来の client library の形

初期 repository には `bindings/python/` を置かない。daemon IPC と C protocol core が固まった後、Python / C# / CLI などの client library を追加する。

daemon protocol は状態更新のみだが、将来の Python client library には便宜関数を置いてよい。

```python
from switchbt import SwitchBtClient, Buttons, State

with SwitchBtClient() as pad:
    pad.acquire()

    state = State.neutral()
    state.buttons |= Buttons.A
    pad.set_state(state)

    # 時間管理は client 側の責務
    pad.sleep_ms(80)

    state.buttons &= ~Buttons.A
    pad.set_state(state)
```

`tap()` helper を置く場合:

```python
def tap(self, button: int, ms: int = 80) -> None:
    state = self.state.copy()
    state.buttons |= button
    self.set_state(state)
    self.sleep_ms(ms)
    state.buttons &= ~button
    self.set_state(state)
```

この helper は daemon に `duration_ms` を送らない。

## 13. C ABI 案

C ABI は daemon IPC の下層またはテスト用に残す。

```c
typedef struct swbt_handle swbt_handle_t;

typedef struct {
    uint32_t buttons;
    uint16_t lx;
    uint16_t ly;
    uint16_t rx;
    uint16_t ry;
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    uint64_t client_seq;
} swbt_state_t;

int swbt_create(const swbt_config_t *config, swbt_handle_t **out);
int swbt_start(swbt_handle_t *handle);
int swbt_stop(swbt_handle_t *handle);
void swbt_destroy(swbt_handle_t *handle);

int swbt_set_state(swbt_handle_t *handle, const swbt_state_t *state);
int swbt_get_status(swbt_handle_t *handle, swbt_status_t *out);
int swbt_get_rumble(swbt_handle_t *handle, swbt_rumble_t *out);
```

C ABI でも時間指定コマンドは入れない。`swbt_set_state()` のみを基本単位にする。

## 14. ログとメトリクス

daemon は次を記録する。

### 14.1 起動時ログ

- build version
- git commit
- BTstack submodule commit
- platform
- IPC endpoint
- report period
- selected Bluetooth adapter
- HID descriptor version
- auth enabled/disabled

### 14.2 実行時メトリクス

- `actual_report_rate_hz`
- `report_interval_avg_us`
- `report_jitter_p50_us`
- `report_jitter_p95_us`
- `report_jitter_max_us`
- `state_updates_received`
- `state_updates_rejected`
- `state_updates_coalesced`
- `last_client_seq`
- `subcommands_received`
- `subcommands_unknown`
- `rumble_updates_received`
- `disconnect_count`
- `reconnect_count`

### 14.3 latency 計測

v0 では HID report 内に計測用データを埋め込まない。

内部計測として次をログに残す。

```text
client set_state parse complete time
state mailbox store time
report build time
BTstack send request time
BTstack send complete / can-send event time
```

`seq` により、どの client state がどの report tick で使われたか追えるようにする。

## 15. エラーハンドリング

### 15.1 IPC error code

| code | 意味 |
|---|---|
| `invalid_json` | JSON として読めない |
| `invalid_version` | protocol version が未対応 |
| `auth_failed` | token 不一致 |
| `owner_busy` | active owner が別にいる |
| `not_owner` | owner ではない client が `set_state` した |
| `invalid_state` | state field が不正 |
| `unsupported_command` | 未対応 command |
| `internal_error` | daemon 内部 error |
| `not_connected` | Switch 未接続 |

### 15.2 state validation

daemon は不正な state を拒否する。

- `buttons` に未定義 bit がある
- stick が `0..4095` 外
- IMU が int16 範囲外
- 必須 field が欠けている
- `state` が object ではない

拒否時、latest state は変更しない。

## 16. テスト計画

### 16.1 parser unit test

- 空 buffer
- 1 byte buffer
- `report[9]` に届かない長さ
- 未知 output report id
- `0x01` だが subcommand 部分が短い
- rumble only
- SPI read の address / size 境界
- 未知 subcommand

### 16.2 report builder unit test

- neutral state の report
- A/B/X/Y bit mapping
- L/R/ZL/ZR bit mapping
- d-pad bit mapping
- stick `0`, `2048`, `4095`
- stick packing の 12-bit 境界
- IMU 正値・負値の little endian 化
- report length が常に期待値になる

### 16.3 IPC test

- hello / acquire / release
- owner なし set_state 拒否
- owner あり set_state 成功
- owner 以外 set_state 拒否
- invalid JSON
- 8192 bytes 超過 message
- client disconnect で neutral 適用
- `set_state` burst で latest のみ採用

### 16.4 scheduler test

- `report_period_us=8000` で 10,000 tick 測定
- jitter p95 / max を記録
- subcommand reply が periodic report に埋もれないこと
- report build が deadline を超えた場合の recovery
- state update burst 中でも report timer が止まらないこと

### 16.5 実機 test

最低限、次を matrix として記録する。

| 項目 | 記録内容 |
|---|---|
| Switch | model, system version |
| Bluetooth adapter | vendor/product id, chipset |
| OS | Windows/Linux version |
| driver | WinUSB/libusb/udev 設定 |
| BTstack | upstream base commit, submodule commit |
| report period | 8000 / 8333 / 15000 / 16667 us |
| result | pairing, reconnect, button, stick, rumble, disconnect |

125Hz が不安定な場合は、120Hz、100Hz、66.7Hz、60Hz の順で比較する。

## 17. 実装順序

### Phase 1: daemon skeleton

- `swbt-daemon` 起動
- TCP loopback JSON Lines server
- hello / acquire / release / get_status
- owner disconnect handling
- neutral state
- debug IPC client prototype

### Phase 2: protocol core without real Bluetooth

- state validation
- mailbox
- report scheduler mock
- report builder unit test
- output report parser unit test
- virtual SPI unit test

### Phase 3: BTstack bridge

- HID Device 登録
- Switch pairing
- subcommand parser 接続
- `0x21` reply
- `0x30` periodic report
- 125Hz send loop

### Phase 4: integration

- button / stick 実機確認
- rumble 受信
- player lights
- reconnect
- report period 比較

### Phase 5: hardening

- fuzz test
- ASan / UBSan
- malformed IPC test
- logging / metrics
- Windows WinUSB path
- packaging

## 18. 未検証事項

- Pro Controller Bluetooth report rate の最適値。資料上は 120Hz と 125Hz 相当の記述が混在する。
- Switch OS バージョンごとの subcommand 要求順序。
- 125Hz 送信時の BTstack / WinUSB / libusb / Bluetooth adapter ごとの安定性。
- Switch 側が periodic 0x30 report の jitter をどの程度許容するか。
- IMU 値を固定または重複サンプルで送った場合のゲーム側挙動。
- NFC/IR 未実装時のゲームごとの挙動。

## 19. 設計上の禁止事項

- BTstack 本体の HID validation を無条件に無効化しない。
- `hid_report_data_callback()` で長さ検証前に固定 offset を読まない。
- daemon protocol に `duration_ms`、`at_ms`、`sequence steps` を入れない。
- 複数 client の state を merge しない。
- owner disconnect 後に押下 state を保持し続けない。
- BTstack thread 以外から BTstack API を直接呼ばない。
- daemon 内のライブラリ層から `exit()` を呼ばない。
- rumble state を button state に混ぜない。
- 範囲外 stick 値を黙って clamp しない。

## 20. Definition of Done

v0 完了条件:

- daemon が起動し、local IPC で hello / acquire / set_state / get_status が動く。
- client disconnect 時に neutral state へ戻る。
- Switch に Pro Controller として pairing できる。
- report period 8000us で `0x30` input report を継続送信できる。
- A/B/X/Y、L/R/ZL/ZR、d-pad、左右 stick が実機で確認できる。
- subcommand reply が固定配列ではなく parser + builder 経由で処理される。
- SPI read が address / size に応じて仮想 SPI から返る。
- 125Hz、120Hz、66.7Hz、60Hz の比較ログが残せる。
- malformed IPC と malformed output report で crash しない。
- ASan / UBSan 付き unit test が通る。
- `vendor/btstack/src/classic/hid_device.c` の validation 無効化を submodule 差分に含めない。

## 21. 参考リンク

- BTstack: https://github.com/bluekitchen/btstack
- BTstack run loop / configuration: https://bluekitchen-gmbh.com/btstack/v1.0/how_to/
- BTstack manual PDF: https://bluekitchen-gmbh.com/btstack.pdf
- mizuyoukanao/btstack: https://github.com/mizuyoukanao/btstack
- `btkeyLib.c`: https://github.com/mizuyoukanao/btstack/blob/master/example/btkeyLib.c
- `hid_device.c` validation 変更 commit: https://github.com/mizuyoukanao/btstack/commit/cac30c0
- dekuNukem Nintendo Switch Reverse Engineering HID notes: https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/bluetooth_hid_notes.md
- Linux `hid-nintendo.c`: https://android.googlesource.com/kernel/common/+/9f6af9a6c2cc38/drivers/hid/hid-nintendo.c
- joycontrol: https://github.com/mart1nro/joycontrol
- nxbt: https://github.com/Brikwerk/nxbt
