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

request device info の規定 profile は `swbt-pro` である。`swbt-pro` は daemon の既定値であり、firmware `04 00`、controller type `03`、marker `02`、BTstack local BD_ADDR、tail `01 01` を返す。`SWBT_DEVICE_INFO_PROFILE=swbt-pro` を明示しても同じ profile を使う。`mizuyoukanao-pro` は削除済みであり、互換 selector として残さない。

virtual SPI は caller-seeded storage を読む。実機相当の factory data generator、serial number、MAC / Bluetooth address persistence は current contract に含めない。

rumble は controller input state と分けて raw 8 bytes として保持する。frequency / amplitude semantic decode と actuator-safe conversion は current contract に含めない。

Subcommand reply `0x21` は periodic `0x30` より優先する方針とする。現行実装では reply queue core が already-built report bytes を保持し、send failure では head item を残して retry できる。BTstack can-send event 上では `swbt_btstack_input_report_timer_adapter` が queued reply を periodic scheduler より先に送る。`local_037` では CSR8510 A10、WinUSB、Switch2 firmware `22.1.0` の条件で、prioritized `0x21` reply、後続の `0x30` report、Switch UI 上の Button A 入力反映を観測した。`local_049` では `SWBT_DEVICE_INFO_PROFILE` 未指定の `swbt-pro` default run で、同じ subcommand reply sequence と Switch UI 上の Button A 入力反映を観測した。これらの観測は当該構成の hardware observation であり、別 adapter / firmware での一般互換性ではない。

periodic input report scheduler の default period は `8000us` とする。`local_037` では `8000 / 8333 / 15000 / 16667 us` の各 run で画面遷移までの粗い受理を観測した。ただし、これは report jitter、入力遅延、取りこぼし率の厳密測定ではない。`8000us` は current configurable default であり、最適値として固定した値ではない。

## 5. 根拠

この spec は既存の根拠監査と実装済み work unit を current contract として束ねる。新しい Switch protocol 値、report period、BTstack source selection は追加しない。

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| standard full input report ID | `0x30` | source fact | `spec/references/switch-report-core.md` | stable for implementation |
| standard full report size | `49` bytes | implementation contract | `spec/references/switch-report-core.md` | stable for implementation |
| production report prefix defaults | `0x8e`, `0x80` | source fact / implementation policy / hardware observation | `spec/references/switch-report-core.md`, `swbt/daemon/config.c`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | production daemon default; observed on CSR8510 A10 / Switch2 22.1.0 |
| subcommand reply report ID | `0x21` | source fact / implementation contract | `spec/references/switch-subcommand-reply-core.md` | stable for implementation |
| simple ACK | `0x80` | source fact / implementation contract | `spec/references/switch-subcommand-reply-core.md`, `spec/references/switch-subcommand-dispatcher-core.md` | stable for current dispatcher |
| SPI read ACK | `0x90` | source fact | `spec/references/switch-subcommand-reply-core.md` | stable for current dispatcher |
| output reports | `0x01`, `0x10` | source fact | `spec/references/switch-subcommand-core.md` | stable for parser |
| request device info profile | `swbt-pro` | implementation policy backed by existing implementation contract / hardware observation | `spec/references/switch-subcommand-dispatcher-core.md`, `swbt/switch/switch_device_info.c`, `work-units/complete/local_048/SWBT_DEVICE_INFO_PROFILE_DEFINITION.md`, `work-units/complete/local_049/SWBT_PRO_HARDWARE_VERIFICATION.md` | daemon の既定値; `mizuyoukanao-pro` removed; observed on CSR8510 A10 / Switch2 |
| rumble raw payload | `8` bytes | source fact | `spec/references/switch-rumble-core.md` | stable raw storage |
| player lights `0x30` / `0x31` | subcommand IDs | source fact / implementation contract | `spec/references/switch-player-lights-policy.md` | stable for current policy |
| default report period | `8000us` | design policy / hardware observation | `spec/references/btstack-periodic-input-report-core.md`, `CMakeLists.txt`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | configurable; coarse acceptance observed, not optimized |
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
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md`
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`
- `work-units/complete/local_049/SWBT_PRO_HARDWARE_VERIFICATION.md`

## 7. 未解決事項

- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` では、CSR8510 A10、WinUSB、Switch2 firmware `22.1.0` の条件で current HID descriptor、observed subcommand reply sequence、prioritized `0x21` reply、periodic input report loop、Button A の Switch UI 反映を観測した。`work-units/complete/local_049/SWBT_PRO_HARDWARE_VERIFICATION.md` では、`SWBT_DEVICE_INFO_PROFILE` 未指定の `swbt-pro` default run で同じ bring-up 経路を観測した。これは hardware observation であり、他 adapter / firmware の一般互換性保証ではない。
- `local_037` では Switch output report が `a2 01` として来ること、truncated HID report acceptance 後に invalid-size drop が消えることを観測した。BTstack DATA callback と SET_REPORT callback のどちらを実機が使うかの広範な互換性は未監査である。
- player lights replies、virtual SPI seed 全域、rumble output handling の実機上の完全な受理範囲は未検証である。
- report mode、IMU、vibration の session state 保存は未実装である。
- manual pairing、regulated voltage の reply data は未実装である。
- serial number と MAC / Bluetooth address の SPI storage 位置、長さ、永続化責務は未監査である。
- rumble frequency / amplitude semantic decode と actuator-safe conversion は未実装である。
