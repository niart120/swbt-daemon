# Switch HID Core

## 1. 状態

current。

## 2. 目的

この spec は、現在の swbt が実装している Switch Pro Controller 相当の HID protocol core を定める。

protocol 値の詳細根拠は `spec/references/` に置く。この spec は実装が従う current contract をまとめ、実機未検証の項目を完了済み扱いにしない。

## 3. 適用範囲

- standard full input report `0x30`。
- subcommand reply input report `0x21`。
- output report parser。
- subcommand dispatcher。
- virtual SPI read。
- rumble raw payload。
- player lights policy。
- HID descriptor。
- periodic input report scheduler と reply queue の protocol priority。

次は対象外である。

- NFC/IR MCU と amiibo。
- 複数 controller 同時接続。
- rumble frequency / amplitude semantic decode。
- 実機 acceptability の成功判定。

## 4. 決定事項

standard full input report は report ID `0x30` を使う。report size は report ID を含めて 49 bytes とする。

standard full input report は timer、battery/connection、3 button bytes、left stick、right stick、vibrator report、6-Axis payload を持つ。

report builder は battery/connection と vibrator report の値を caller から受け取る。production daemon default は battery/connection `0x8e`、vibrator report `0x80` を使う。

button state は swbt の canonical bit layout から Switch report bytes へ変換する。IPC と C API は HID byte layout を直接公開しない。

stick value は 12-bit raw value として report に pack する。factory / user calibration による補正は current contract に含めない。

IMU payload は `int16le` の accel x/y/z と gyro x/y/z を 3 frames 分詰める。物理的に妥当な IMU simulation は current contract に含めない。

output report parser は report ID `0x01` を rumble + subcommand、report ID `0x10` を rumble only として扱う。unsupported report ID は explicit error にする。

subcommand reply は input report `0x21` で返す。simple ACK は ACK `0x80`、SPI read reply は ACK `0x90` を使う。

dispatcher は BTstack header に依存しない。dispatcher は parser が作った `swbt_switch_output_report_t` を受け取り、reply builder と virtual SPI を使って response を作る。

current dispatcher は `LOW_POWER_MODE`、`SET_REPORT_MODE`、`ENABLE_IMU`、`ENABLE_VIBRATION` に simple ACK を返し、`SPI_FLASH_READ` に virtual SPI read reply を返し、`SET_PLAYER_LIGHTS` / `GET_PLAYER_LIGHTS` を player lights state に接続する。

request device info の既定 profile は firmware `04 00`、controller type `03`、marker `02`、BTstack local BD_ADDR、tail `01 01` を返す。実験用に `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` を指定した場合は firmware `03 48`、controller type `03`、marker `02`、BTstack local BD_ADDR、tail `03 02` を返す。

virtual SPI は caller-seeded storage を読む。実機相当の factory data generator、serial number、MAC / Bluetooth address persistence は current contract に含めない。

rumble は controller input state と分けて raw 8 bytes として保持する。frequency / amplitude semantic decode と actuator-safe conversion は current contract に含めない。

Subcommand reply `0x21` は periodic `0x30` より優先する方針とする。現行実装では reply queue core が already-built report bytes を保持し、send failure では head item を残して retry できる。BTstack can-send event 上では `swbt_btstack_input_report_timer_adapter` が queued reply を periodic scheduler より先に送る。Switch 実機がこの優先送信を受け入れるかは未検証である。

periodic input report scheduler の default period は `8000us` とする。ただしこれは configurable default であり、実機で最適値として確認済みの値ではない。

## 5. 根拠

この spec は既存の根拠監査と実装済み work unit を current contract として束ねる。新しい Switch protocol 値、report period、BTstack source selection は追加しない。

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| standard full input report ID | `0x30` | source fact | `spec/references/switch-report-core.md` | stable for implementation |
| standard full report size | `49` bytes | implementation contract | `spec/references/switch-report-core.md` | stable for implementation |
| production report prefix defaults | `0x8e`, `0x80` | source fact / implementation policy | `spec/references/switch-report-core.md`, `swbt/daemon/config.c` | production daemon default; hardware not proven |
| subcommand reply report ID | `0x21` | source fact / implementation contract | `spec/references/switch-subcommand-reply-core.md` | stable for implementation |
| simple ACK | `0x80` | source fact / implementation contract | `spec/references/switch-subcommand-reply-core.md`, `spec/references/switch-subcommand-dispatcher-core.md` | stable for current dispatcher |
| SPI read ACK | `0x90` | source fact | `spec/references/switch-subcommand-reply-core.md` | stable for current dispatcher |
| output reports | `0x01`, `0x10` | source fact | `spec/references/switch-subcommand-core.md` | stable for parser |
| rumble raw payload | `8` bytes | source fact | `spec/references/switch-rumble-core.md` | stable raw storage |
| player lights `0x30` / `0x31` | subcommand IDs | source fact / implementation contract | `spec/references/switch-player-lights-policy.md` | stable for current policy |
| default report period | `8000us` | design policy | `spec/references/btstack-periodic-input-report-core.md`, `CMakeLists.txt` | configurable; hardware not proven |
| HID descriptor bytes | implementation contract | `spec/references/switch-hid-descriptor-core.md` | stable for current build |

## 6. 関連 work units

- `work-units/complete/local_004/SWITCH_REPORT_CORE.md`
- `work-units/complete/local_005/SWITCH_SUBCOMMAND_CORE.md`
- `work-units/complete/local_006/SWITCH_SPI_CORE.md`
- `work-units/complete/local_007/SWITCH_RUMBLE_CORE.md`
- `work-units/complete/local_014/SWITCH_SUBCOMMAND_REPLY_CORE.md`
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md`
- `work-units/complete/local_017/SWITCH_HID_DESCRIPTOR_CORE.md`
- `work-units/complete/local_020/SUBCOMMAND_DISPATCHER_CORE.md`
- `work-units/complete/local_021/VIRTUAL_SPI_SEED_DATA.md`
- `work-units/complete/local_022/SUBCOMMAND_REPLY_SEND_QUEUE.md`
- `work-units/complete/local_029/RUMBLE_STATUS_EXPOSURE.md`
- `work-units/complete/local_030/PLAYER_LIGHTS_POLICY.md`
- `work-units/complete/local_036/SPEC_WORK_UNIT_INVENTORY.md`
- `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md`

## 7. 未解決事項

- Switch 実機が current HID descriptor、subcommand replies、player lights replies、virtual SPI seed、rumble output handling を受け入れることは未検証である。`work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `docs/hardware-test-log.md` で扱う。
- BTstack DATA callback と SET_REPORT callback のどちらで Switch 実機が output report を渡すかは未検証である。
- BTstack can-send event と subcommand reply queue / periodic scheduler の software integration は `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md` で完了している。Switch 実機が prioritized `0x21` reply を受け入れるかは `work-units/wip/local_037/WINDOWS_HARDWARE_BRINGUP.md` で扱う。
- request device info は ACK `0x82` と subcommand `0x02` の `0x21` reply を返す実装になっている。production では BTstack の local BD_ADDR を controller address として使う。Switch2 実機がこの reply を受け入れて初期化列を進めることは未検証である。
- `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` は `0x08` 反復の原因を切り分ける実験条件であり、Switch2 22.1.0 で正しい identity だとは未検証である。
- low power mode は ACK `0x80` と subcommand `0x08` の `0x21` reply を返す実装になっている。shipment / low-power state の永続化は current contract に含めない。Switch2 実機がこの reply を受け入れて次の初期化 subcommand へ進むことは未検証である。
- production daemon default の battery/connection `0x8e`、vibrator report `0x80` が Switch2 実機の初期化列を進めることは未検証である。
- report mode、IMU、vibration の session state 保存は未実装である。
- manual pairing、regulated voltage の reply data は未実装である。
- serial number と MAC / Bluetooth address の SPI storage 位置、長さ、永続化責務は未監査である。
- rumble frequency / amplitude semantic decode と actuator-safe conversion は未実装である。
