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
| simple ACK reply | ACK `0x80`, reply-to subcommand ID, no reply data | source fact / implementation contract | `spec/references/switch-subcommand-reply-core.md`; `swbt/switch/switch_subcommand_reply.h` | used for simple handled subcommands |
| SPI read request shape | little-endian address + size | source fact | `spec/references/switch-subcommand-core.md` | dispatcher parses request only |
| SPI read reply shape | ACK `0x90`, echoed address/size, read data | source fact | `spec/references/switch-subcommand-reply-core.md` | dispatcher supplies reply data |
| SPI read boundary validation | address / size validation delegated to virtual SPI | implementation fact | `spec/references/switch-spi-core.md`; `swbt/switch/switch_spi.h` | dispatcher does not duplicate validation |
| implemented simple subcommands | `SET_REPORT_MODE`, `ENABLE_IMU`, `ENABLE_VIBRATION` | source fact for IDs; policy decision for first dispatcher slice | `spec/references/switch-subcommand-core.md` | no session state stored yet |
| implemented player lights subcommands | `SET_PLAYER_LIGHTS`, `GET_PLAYER_LIGHTS` | source fact / implementation contract | `spec/references/switch-player-lights-policy.md`; `swbt/switch/switch_subcommand_dispatcher.c` | connected to player lights state |
| unsupported subcommands | explicit unsupported result | design policy | this work unit | no reply bytes |

## 4. 実装判断

- dispatcher は BTstack header に依存しない。
- dispatcher は existing reply builder を使い、input report `0x21` の byte layout を重複実装しない。
- SPI read の address / size validation は `swbt_switch_spi_read` に委譲する。
- `SET_REPORT_MODE`、`ENABLE_IMU`、`ENABLE_VIBRATION` はこの work unit では session state を保持せず simple ACK を返す。
- `SET_PLAYER_LIGHTS` は player lights state を更新して simple ACK を返す。
- `GET_PLAYER_LIGHTS` は player lights state の current raw byte を reply data として返す。
- `REQUEST_DEVICE_INFO`、manual pairing、regulated voltage などは explicit unsupported として後続に残す。

## 5. 未解決事項

- 実機 Switch が simple ACK だけで対象 subcommand を受け入れるかは未検証である。
- report mode、IMU、vibration の session state 保存は未実装である。
- device info、regulated voltage の reply data は未実装である。
