# Switch Subcommand Dispatcher Core Source Audit

## 1. 状態

recorded。

この reference は `swbt_switch_output_report_t` から subcommand reply または no-reply action を選ぶ dispatcher core の根拠監査である。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| swbt output parser reference | current swbt implementation | `spec/references/switch-subcommand-core.md` |
| swbt subcommand reply reference | current swbt implementation | `spec/references/switch-subcommand-reply-core.md` |
| swbt virtual SPI reference | current swbt implementation | `spec/references/switch-spi-core.md` |
| swbt player lights reference | current swbt implementation | `spec/references/switch-player-lights-policy.md` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| parsed subcommand input | `swbt_switch_output_report_t` with `has_subcommand`, `subcommand_id`, `subcommand_data` | implementation fact | `swbt/switch/switch_subcommand.h`; `spec/references/switch-subcommand-core.md` | existing parser contract |
| simple ACK reply | ACK `0x80`, reply-to subcommand ID, no reply data | source fact / implementation contract / hardware observation | `spec/references/switch-subcommand-reply-core.md`; `swbt/switch/switch_subcommand_reply.h`; `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | used for simple handled subcommands; observed in limited bring-up |
| SPI read request shape | little-endian address + size | source fact | `spec/references/switch-subcommand-core.md` | dispatcher parses request only |
| SPI read reply shape | ACK `0x90`, echoed address/size, read data | source fact | `spec/references/switch-subcommand-reply-core.md` | dispatcher supplies reply data |
| SPI read boundary validation | address / size validation delegated to virtual SPI | implementation fact | `spec/references/switch-spi-core.md`; `swbt/switch/switch_spi.h` | dispatcher does not duplicate validation |
| implemented simple subcommands | `LOW_POWER_MODE`, `SET_REPORT_MODE`, `ENABLE_IMU`, `ENABLE_VIBRATION` | source fact for IDs; policy decision for current dispatcher slice | `spec/references/switch-subcommand-core.md`; dekuNukem `bluetooth_hid_subcommands_notes.md`; joycontrol `protocol.py` | no low-power or session state stored yet |
| implemented player lights subcommands | `SET_PLAYER_LIGHTS`, `GET_PLAYER_LIGHTS` | source fact / implementation contract | `spec/references/switch-player-lights-policy.md`; `swbt/switch/switch_subcommand_dispatcher.c` | connected to player lights state |
| implemented device info subcommand | `REQUEST_DEVICE_INFO` replies with ACK `0x82`, subcommand `0x02`, and 12 bytes of Pro Controller identity data | source fact / implementation contract / hardware observation | dekuNukem `bluetooth_hid_subcommands_notes.md`; joycontrol `report.py` / `protocol.py`; `swbt/switch/switch_device_info.h`; `swbt/switch/switch_subcommand_dispatcher.c`; `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | controller address is supplied by runtime identity; observed in limited bring-up |
| `swbt-pro` device info profile | firmware `04 00`, controller type `03`, marker `02`, Bluetooth MAC, tail `01 01` | implementation policy backed by existing implementation contract | `swbt/switch/switch_device_info.h`; `swbt/switch/switch_device_info.c`; `swbt/daemon/config.c`; `work-units/wip/local_048/SWBT_DEVICE_INFO_PROFILE_DEFINITION.md` | daemon default; selectable with `SWBT_DEVICE_INFO_PROFILE=swbt-pro` |
| unsupported subcommands | explicit unsupported result | design policy | this work unit | no reply bytes |

## 4. 実装判断

- dispatcher は BTstack header に依存しない。
- dispatcher は existing reply builder を使い、input report `0x21` の byte layout を重複実装しない。
- SPI read の address / size validation は `swbt_switch_spi_read` に委譲する。
- `LOW_POWER_MODE` は ACK `0x80` の simple ACK を返す。shipment / low-power state の永続化は current dispatcher の責務に含めない。
- `SET_REPORT_MODE`、`ENABLE_IMU`、`ENABLE_VIBRATION` はこの work unit では session state を保持せず simple ACK を返す。
- `SET_PLAYER_LIGHTS` は player lights state を更新して simple ACK を返す。
- `GET_PLAYER_LIGHTS` は player lights state の current raw byte を reply data として返す。
- `REQUEST_DEVICE_INFO` は Pro Controller identity の reply data を返す。Bluetooth address は dispatcher 内で固定せず、runtime から渡された device info を使う。
- `swbt-pro` は daemon の規定 device info profile であり、未指定時の既定値でもある。
- `SWBT_DEVICE_INFO_PROFILE=mizuyoukanao-pro` は削除済みであり、互換 selector として残さない。
- manual pairing、regulated voltage などは explicit unsupported として後続に残す。

## 5. 未解決事項

- `local_037` では CSR8510 A10、WinUSB、Switch2 firmware `22.1.0`、当時の実験 profile `mizuyoukanao-pro` の条件で、`REQUEST_DEVICE_INFO`、`LOW_POWER_MODE`、`SET_REPORT_MODE`、`ENABLE_IMU`、`ENABLE_VIBRATION`、`SET_PLAYER_LIGHTS`、`GET_TRIGGER_BUTTONS_ELAPSED_TIME`、`NFC_IR_MCU_CONFIG` を含む reply sequence と Switch UI での input 反映を観測した。
- `mizuyoukanao-pro` 削除後の `swbt-pro` profile では、同じ実機経路を未実行である。
- 別 adapter / firmware での subcommand acceptability は未検証である。
- report mode、IMU、vibration の session state 保存は未実装である。
- regulated voltage の reply data は未実装である。
