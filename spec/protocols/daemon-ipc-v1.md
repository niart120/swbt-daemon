# Daemon IPC Wire Protocol V1

## 1. 状態

current。

## 2. 目的

この spec は、daemon IPC v1 の通信仕様を定める。

daemon IPC は、local client が controller state snapshot を daemon へ渡し、daemon の owner / state / rumble status を取得するための JSON Lines protocol である。daemon は最新 state を保持し、BTstack report scheduler がその state を Switch HID input report に反映する。

## 3. 適用範囲

この spec は次を扱う。

- TCP loopback transport。
- JSON Lines framing。
- request / response envelope。
- connection ごとの `client_id` と owner model。
- `hello`、`acquire`、`release`、`set_state`、`get_status`。
- controller state object。
- status response の `owner`、`state`、`rumble`。
- error response。
- owner disconnect と heartbeat timeout の neutral fail-safe。

次は対象外である。

- daemon protocol としての `tap`、`duration_ms`、`sequence`、`at_ms`。
- authentication token。
- subscribe / event delivery。
- stable metrics protocol。
- Switch connection state の公開 schema。
- Bluetooth adapter access、Switch pairing、HID advertising、report loop。

## 4. 決定事項

### 4.1 Transport

transport は TCP over IPv4 loopback とする。

server は `127.0.0.1` だけに bind する。`0.0.0.0` などの wildcard bind は拒否する。

port は daemon config が決める。test では port `0` を指定し、OS が割り当てた port を使える。

server は accepted connection ごとに internal `client_id` を割り当てる。`client_id` は daemon process 内の connection identifier であり、daemon restart をまたいで安定しない。

### 4.2 Framing

1 message は 1 行の JSON object とする。

```text
{...}\n
```

line は `\n` で終端する。server は newline まで読み、1 line を 1 request として処理する。

newline 前の partial line は request として処理しない。server は connection ごとに未完了 line を保持し、response は complete line を処理した場合だけ返す。

complete line がない connection は、request 処理なしで server loop に戻る。これにより、slow client の partial line は heartbeat timeout の確認を塞がない。

message は `SWBT_IPC_JSON_LINE_MAX`、現行値 `8192` bytes に収まる必要がある。これを超える line は transport layer で message too long として扱い、server はその connection を閉じる。閉じる connection が active owner の場合、owner は解除され、latest state は neutral に戻る。

response がある場合、server は 1 JSON object と trailing newline を返す。

`set_state` は `request_id` がない成功時だけ response を返さない。error response は `request_id` の有無に関係なく返す。

### 4.3 JSON Envelope

request は JSON object でなければならない。

必須 field:

| field | type | value |
|---|---|---|
| `v` | integer | `1` |
| `type` | string | command name |

任意 field:

| field | type | rule |
|---|---|---|
| `request_id` | string | response correlation 用。現行 parser は escape sequence と control character を拒否する。 |

`request_id` がある request に対する response は、可能な限り同じ `request_id` を含める。`request_id` 自体が不正な場合、server は `invalid_json` を返し、`request_id` は echo しない。

現行 parser は unknown field を拒否しない。ただし client は、互換性のため、この spec にある field だけに意味を持たせる。

### 4.4 Identifiers

JSON 上の `client_id` と `owner_id` は 8 桁 lowercase hexadecimal string で表す。

例:

```json
"000003e9"
```

`hello_ok` は connection の `client_id` を返す。

`acquired` は active owner になった connection の `owner_id` を返す。

`release` と `set_state` の `owner_id` は、送信元 connection の `client_id` と一致しなければならない。一致しない場合は `not_owner` を返す。

### 4.5 Command: `hello`

目的: connection の `client_id` を取得する。

request:

```json
{"v":1,"type":"hello","request_id":"h1"}
```

`client_name` を送ってもよいが、current implementation は保持しない。

response:

```json
{"v":1,"type":"hello_ok","request_id":"h1","client_id":"00000001"}
```

### 4.6 Command: `acquire`

目的: active owner を取得する。

request:

```json
{"v":1,"type":"acquire","mode":"exclusive","request_id":"a1"}
```

current implementation は常に exclusive owner model で動く。`mode` は将来拡張用の field であり、現時点では owner mode の分岐に使わない。

success response:

```json
{"v":1,"type":"acquired","request_id":"a1","owner_id":"00000001"}
```

既に別 connection が owner の場合:

```json
{"v":1,"type":"error","request_id":"a1","code":"owner_busy","message":"another client owns the controller"}
```

### 4.7 Command: `release`

目的: active owner を解放し、latest state を neutral に戻す。

request:

```json
{"v":1,"type":"release","owner_id":"00000001","request_id":"r1"}
```

success response:

```json
{"v":1,"type":"released","request_id":"r1"}
```

`owner_id` が送信元 connection と一致しない場合、または送信元が active owner ではない場合は `not_owner` を返す。

### 4.8 Command: `set_state`

目的: active owner が latest controller state snapshot を更新する。

request:

```json
{
  "v": 1,
  "type": "set_state",
  "owner_id": "00000001",
  "seq": 77,
  "request_id": "s1",
  "state": {
    "buttons": 8,
    "lx": 1234,
    "ly": 2345,
    "rx": 2048,
    "ry": 2048,
    "accel_x": 1,
    "accel_y": 2,
    "accel_z": 3,
    "gyro_x": 4,
    "gyro_y": 5,
    "gyro_z": 6
  }
}
```

success response with `request_id`:

```json
{"v":1,"type":"state_accepted","request_id":"s1","seq":77}
```

success response without `request_id`:

```text
<no response>
```

`set_state` は full snapshot だけを受け付ける。partial update、button tap、hold duration、future scheduling は daemon protocol に入れない。

`seq` は任意の non-negative integer である。省略時は `0` として扱う。daemon は `seq` を Switch HID report に埋め込まず、status と診断用に保持する。

### 4.9 State Object

`state` は JSON object でなければならない。
次の必須 field はすべて `state` object の member として送る。
top-level field として送った controller state 値は protocol contract ではない。

必須 field:

| field | type | range | meaning |
|---|---|---:|---|
| `buttons` | integer | defined button mask only | canonical button bitset |
| `lx` | integer | `0..4095` | left stick X |
| `ly` | integer | `0..4095` | left stick Y |
| `rx` | integer | `0..4095` | right stick X |
| `ry` | integer | `0..4095` | right stick Y |
| `accel_x` | integer | `int16` | accelerometer X |
| `accel_y` | integer | `int16` | accelerometer Y |
| `accel_z` | integer | `int16` | accelerometer Z |
| `gyro_x` | integer | `int16` | gyroscope X |
| `gyro_y` | integer | `int16` | gyroscope Y |
| `gyro_z` | integer | `int16` | gyroscope Z |

neutral state:

```json
{
  "buttons": 0,
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
```

`buttons` の bit layout:

| bit | mask | name |
|---:|---:|---|
| 0 | `0x00000001` | `Y` |
| 1 | `0x00000002` | `X` |
| 2 | `0x00000004` | `B` |
| 3 | `0x00000008` | `A` |
| 4 | `0x00000010` | `SR_R` |
| 5 | `0x00000020` | `SL_R` |
| 6 | `0x00000040` | `R` |
| 7 | `0x00000080` | `ZR` |
| 8 | `0x00000100` | `MINUS` |
| 9 | `0x00000200` | `PLUS` |
| 10 | `0x00000400` | `R_STICK` |
| 11 | `0x00000800` | `L_STICK` |
| 12 | `0x00001000` | `HOME` |
| 13 | `0x00002000` | `CAPTURE` |
| 16 | `0x00010000` | `DOWN` |
| 17 | `0x00020000` | `UP` |
| 18 | `0x00040000` | `RIGHT` |
| 19 | `0x00080000` | `LEFT` |
| 20 | `0x00100000` | `SR_L` |
| 21 | `0x00200000` | `SL_L` |
| 22 | `0x00400000` | `L` |
| 23 | `0x00800000` | `ZL` |

未定義 bit が立っている場合は `invalid_state` を返し、latest state は変更しない。

### 4.10 Command: `get_status`

目的: owner、latest state、rumble raw status を取得する。

request:

```json
{"v":1,"type":"get_status","request_id":"g1"}
```

response:

```json
{
  "v": 1,
  "type": "status",
  "request_id": "g1",
  "daemon": {
    "protocol_version": 1,
    "daemon_version": "0.1.0-dev",
    "backend": "production",
    "lifecycle_state": "running",
    "hardware_approval": "approved"
  },
  "metrics": {
    "hardware_status": "unavailable",
    "report_ticks_total": 0,
    "reports_sent_total": 0,
    "send_failures_total": 0,
    "report_interval_average_us": 0,
    "report_interval_max_us": 0,
    "ipc_state_accepted_total": 0,
    "ipc_state_rejected_total": 0,
    "ipc_state_coalesced_total": 0,
    "actual_report_rate_hz": 0,
    "jitter_max_us": 0
  },
  "hardware": {
    "adapter_state": "unavailable",
    "switch_connection_state": "unavailable",
    "hid_channel_state": "unavailable"
  },
  "owner": {
    "present": true,
    "owner_id": "00000001",
    "last_seq": 77
  },
  "state": {
    "buttons": 8,
    "lx": 1234,
    "ly": 2345,
    "rx": 2048,
    "ry": 2048,
    "accel_x": 1,
    "accel_y": 2,
    "accel_z": 3,
    "gyro_x": 4,
    "gyro_y": 5,
    "gyro_z": 6
  },
  "rumble": {
    "updated": true,
    "last_update_ms": 4242,
    "raw": "0401804108018042"
  }
}
```

`daemon.protocol_version` は JSON envelope の `v` と同じ daemon IPC protocol version である。`daemon.daemon_version` は running daemon の version string であり、現行実装では `swbt_get_version_string()` を返す。

`daemon.backend` は `"unknown"`、`"noop"`、`"production"` のいずれかである。`daemon.lifecycle_state` は `"unavailable"`、`"stopped"`、`"running"` のいずれかである。`daemon.hardware_approval` は現行 schema では `"unavailable"` または `"approved"` である。`"approved"` は production backend が hardware gate を通過していることだけを示し、report rate や jitter の実機測定値が存在することは意味しない。

`metrics` は low-frequency diagnostic counters であり、input report ごとの event stream ではない。`*_us` は microseconds、`*_hz` は hertz、`*_total` は daemon process 内の累積 counter である。report tick counters と interval fields は scheduler または test timestamp から増える場合があるが、それだけでは hardware observation を意味しない。`hardware_status` が `"unavailable"` の場合、`actual_report_rate_hz` と `jitter_max_us` は実機観測値ではない。現行 status schema は未観測値を measured value として返さない。

production backend では、periodic scheduler tick が input report send を試行したときに `report_ticks_total` を増やす。send が成功した場合は `reports_sent_total`、send callback が失敗した場合は `send_failures_total` を増やす。subcommand reply と shutdown neutral retry の send failure は、現時点では report tick metrics に含めない。

`ipc_state_accepted_total` は app command path で `set_state` を accepted とした回数である。`ipc_state_rejected_total` は `set_state` が owner mismatch または stale sequence で拒否された回数であり、state は変わらない。invalid JSON と invalid state decode error は app command path に到達しないため、この counter に含めない。現行 app command path は state update queue を持たないため、`ipc_state_coalesced_total` は 0 のまま返す。

`hardware.adapter_state`、`hardware.switch_connection_state`、`hardware.hid_channel_state` は adapter、Switch connection、HID channel の低頻度診断 state である。現行実装では noop backend と未観測状態を `"unavailable"` として返す。実機で観測した state 名は、この schema に追加する前に実機ログと対応する work unit で根拠を残す。

`owner.present` が `false` の場合、`owner.owner_id` は `"00000000"` である。

`owner.last_seq` は latest state の `seq` である。owner がない場合も field は存在し、neutral state の `seq` を返す。

`rumble.raw` は 8 bytes の raw rumble payload を 16 桁 lowercase hexadecimal string にした値である。`rumble.updated` が `false` の場合、payload は未更新状態である。

### 4.11 Error Response

error response:

```json
{"v":1,"type":"error","request_id":"s1","code":"invalid_state","message":"state field is invalid"}
```

`request_id` が request から正常に parse できた場合、error response は `request_id` を含む。

current error codes:

| code | condition |
|---|---|
| `invalid_json` | line が complete JSON object ではない。`request_id` または `type` string が不正。 |
| `invalid_version` | `v` が存在しない、または `1` ではない。 |
| `unsupported_command` | `type` がない、または未対応 command。 |
| `owner_busy` | 別 connection が active owner。 |
| `not_owner` | `owner_id` が送信元 connection と一致しない、または送信元が active owner ではない。 |
| `invalid_state` | `state` が object ではない、必須 field がない、range が不正、未定義 button bit が立っている。 |
| `internal_error` | application command mapping が内部 error を返した。 |

`invalid_json` response を返せる complete line では、server は connection を継続し、次の valid line を同じ connection で処理する。`SWBT_IPC_JSON_LINE_MAX` を超える line は response を返さず、message too long として connection を閉じる。

### 4.12 Neutral Fail-Safe

次の場合、daemon は owner を解除し、latest state を neutral に戻す。

- active owner が `release` に成功した。
- owner connection が disconnected になった。
- owner connection が heartbeat timeout になった。

heartbeat timeout は input timing 機能ではない。connection health 用の safety mechanism として扱う。

## 5. 根拠

この spec は IPC wire protocol を扱う。Switch HID report bytes、BTstack source selection、report period、WinUSB/libusb facts を追加しないため、根拠監査は not applicable とする。

| 項目 | 根拠 | source | status |
|---|---|---|---|
| JSON line size and response size | implementation fact | `swbt/ipc/ipc_json.h` | current |
| JSON envelope and command dispatch | implementation fact | `swbt/ipc/ipc_json.c`, `tests/ipc_json_test.c` | current |
| state object field scope | protocol contract / implementation fact | `swbt/ipc/ipc_json.c`, `tests/ipc_json_test.c`, `work-units/complete/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md` | current after `local_052` |
| status daemon fields | protocol contract / implementation fact | `swbt/ipc/ipc_json.c`, `tests/ipc_json_test.c`, `work-units/complete/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md` | current after `local_039` |
| status backend and hardware approval fields | protocol contract / implementation fact | `swbt/application/app.*`, `swbt/daemon/host.c`, `swbt/daemon/production_backend.c`, `swbt/ipc/ipc_adapter.c`, `tests/daemon_production_backend_test.c`, `work-units/complete/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`, `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md` | current after `local_056` |
| status metrics field names and units | protocol contract / implementation fact | `swbt/core/metrics.h`, `swbt/ipc/ipc_json.c`, `swbt/btstack_bridge/input_report_timer_adapter.c`, `swbt/daemon/production_backend.c`, `tests/ipc_json_test.c`, `tests/report_metrics_test.c`, `tests/daemon_production_backend_test.c`, `tests/application_command_test.c`, `work-units/complete/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`, `work-units/complete/local_064/PRODUCTION_STATUS_METRICS_CONNECTION.md` | current after `local_064` |
| status hardware channel unavailable fields | protocol contract / implementation fact | `swbt/application/status.h`, `swbt/ipc/ipc_status.h`, `swbt/ipc/ipc_json.c`, `swbt/daemon/host.c`, `tests/daemon_host_test.c`, `tests/ipc_json_test.c`, `work-units/complete/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`, `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md` | current after `local_056` |
| owner/application model | implementation fact | `swbt/application/app.*`, `swbt/ipc/ipc_adapter.*`, `tests/application_command_test.c`, `tests/ipc_server_test.c` | current after `local_056` |
| loopback TCP transport | implementation fact | `swbt/ipc/ipc_server.*`, `tests/ipc_server_test.c` | current |
| heartbeat timeout neutral | implementation fact | `swbt/ipc/ipc_server.*`, `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md` | current |
| state bit layout | implementation fact | `swbt/switch/switch_controller_state.h` | current |
| no daemon timing macro | design policy | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`, AGENTS.md | current |

## 6. 関連 work units

- `work-units/complete/local_008/IPC_SESSION_CORE.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`
- `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md`
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`
- `work-units/complete/local_042/PRODUCTION_IPC_RUNNER_AND_STATE_SYNC.md`
- `work-units/complete/local_036/SPEC_WORK_UNIT_INVENTORY.md`
- `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`
- `work-units/complete/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md`
- `work-units/complete/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`

## 7. 未解決事項

- authentication token は未実装であり、この spec では stable contract にしない。
- subscribe / event delivery は未実装であり、この spec では stable contract にしない。
- 実機で観測した adapter / driver / report rate / jitter を measured value として追加公開する contract は未定義である。後続 work unit では `work-units/complete/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md` の explicit unavailable contract を前提にする。
