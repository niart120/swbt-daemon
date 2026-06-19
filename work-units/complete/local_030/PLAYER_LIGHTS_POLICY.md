# Player Lights Policy

## 1. 概要

Switch の `SET_PLAYER_LIGHTS` request を受けたときに、virtual Pro Controller として保持する player lights state と reply policy を定義する work unit。

local_020 の dispatcher から呼べる policy core として分離し、`GET_PLAYER_LIGHTS` には現在の raw policy byte を返す。

この work unit は物理 LED 制御、複数 controller の slot assignment、実機での点灯確認を扱わない。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_020/SUBCOMMAND_DISPATCHER_CORE.md` は player lights を explicit unsupported として後続に残した。
- `spec/references/switch-subcommand-core.md` は `SET_PLAYER_LIGHTS = 0x30` と `GET_PLAYER_LIGHTS = 0x31` を classifier の既知 subcommand ID として記録している。
- `spec/references/switch-subcommand-reply-core.md` は `0x21` reply report、ACK byte、reply data offset を記録している。
- `spec/references/switch-player-lights-policy.md` は player lights payload と reply policy を記録する。

use case:

- actor: subcommand dispatcher と daemon runtime。
- 入力または状態: parsed `SET_PLAYER_LIGHTS` / `GET_PLAYER_LIGHTS` output report、daemon-owned player lights state。
- 期待する観測結果: `SET_PLAYER_LIGHTS` は 1-byte bitfield を保持して simple ACK を返し、`GET_PLAYER_LIGHTS` は ACK `0xB0` と current raw bitfield を返す。
- 制約: default state は player slot を暗黙に割り当てない。実機 acceptability と LED 表示は断定しない。
- 対象外: 物理 LED 制御、slot assignment、USB-specific flash behavior、実機 LED 観測。

source から use case への変換:

dekuNukem の bitfield 定義と Linux / joycontrol の実装 precedent を使い、low nibble を on mask、high nibble を flash mask として保持する。on overrides flashing は software effective state に反映する。dispatcher では `SET_PLAYER_LIGHTS` を state update + simple ACK、`GET_PLAYER_LIGHTS` を state read + one-byte reply として扱う。

## 3. 対象範囲

- player lights の policy state を追加する。
- `SET_PLAYER_LIGHTS` payload を検証し、state を更新する。
- `GET_PLAYER_LIGHTS` reply data を current state から生成する。
- subcommand dispatcher から player lights policy core を呼ぶ。
- invalid payload と missing state dependency を explicit result で返す。
- default state が player slot を暗黙に示さないことを unit test で固定する。

## 4. 対象外

- 物理 LED の点灯制御。
- 複数 controller の slot assignment。
- GUI 表示。
- game-specific player indicator policy。
- USB-specific flash behavior の実装。
- 実機 pairing、HID advertising、report loop。
- 実機での LED 表示と subcommand acceptability の確認。
- `vendor/btstack` の変更。

## 5. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-subcommand-core.md`
- `spec/references/switch-subcommand-reply-core.md`
- `spec/references/switch-subcommand-dispatcher-core.md`
- `spec/references/switch-player-lights-policy.md`
- `work-units/complete/local_014/SWITCH_SUBCOMMAND_REPLY_CORE.md`
- `work-units/complete/local_020/SUBCOMMAND_DISPATCHER_CORE.md`

## 6. 根拠監査

`source-audit` を使い、`spec/references/switch-player-lights-policy.md` を追加した。

player lights payload は 1-byte bitfield で、low nibble が on bits、high nibble が flash bits である。

`GET_PLAYER_LIGHTS` は ACK `0xB0` と `0x31`、同じ bitfield の 1 byte reply data を返す。

実機 Switch の acceptability、LED 点灯、USB-specific flash behavior は未検証である。

| 項目 | 状態 | 扱い |
|---|---|---|
| subcommand ID `0x30` / `0x31` | recorded | `switch-subcommand-core` と player lights reference の根拠を使う。 |
| payload bitfield | recorded | low nibble を on mask、high nibble を flash mask として保持する。 |
| on overrides flashing | recorded | effective flash mask は `flash & ~on` とする。 |
| `GET_PLAYER_LIGHTS` reply | recorded | ACK `0xB0`、reply data 1 byte として実装する。 |
| real LED behavior | pending | 実機未検証として記録する。 |

## 7. 設計メモ

- player lights state は controller input state と分ける。
- default state は raw `0x00`、updated `false` とし、player 1 などの slot を暗黙に選ばない。
- policy core は BTstack header に依存しない。
- `SET_PLAYER_LIGHTS` は last-request-wins とする。
- `SET_PLAYER_LIGHTS` の simple ACK は `0x80` を使う。
- `GET_PLAYER_LIGHTS` は current raw byte を reply data として返す。
- invalid payload は state を変更しない。

## 8. 対象ファイル

- `CMakeLists.txt`
- `swbt/switch/switch_player_lights.h`
- `swbt/switch/switch_player_lights.c`
- `swbt/switch/switch_subcommand_reply.h`
- `swbt/switch/switch_subcommand_dispatcher.h`
- `swbt/switch/switch_subcommand_dispatcher.c`
- `tests/switch_player_lights_test.c`
- `tests/switch_subcommand_dispatcher_test.c`
- `spec/references/switch-player-lights-policy.md`
- `work-units/complete/local_030/PLAYER_LIGHTS_POLICY.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | default player lights state is explicit and does not imply an unaudited slot | new | unit | no |
| done | source-audited set player lights payload updates policy state | new | unit | no |
| done | request requiring a reply returns payload from current policy state | new | unit | no |
| done | invalid payload leaves state unchanged and returns explicit error | edge | unit | no |
| done | dispatcher updates player lights state and builds `0x80` simple ACK for `SET_PLAYER_LIGHTS` | new | integration | no |
| done | dispatcher builds `0xB0` reply with current raw state for `GET_PLAYER_LIGHTS` | new | integration | no |

TDD status:

- source: `SET_PLAYER_LIGHTS` / `GET_PLAYER_LIGHTS` payload policy と dispatcher unsupported gap。
- use case: Switch subcommand から daemon-owned player lights state を更新し、status request に reply する。
- item: dispatcher builds `0xB0` reply with current raw state for `GET_PLAYER_LIGHTS`。
- state: done。
- commands:
  - red: `just build-debug` failed as expected because `switch/switch_player_lights.h` was missing.
  - green: `just debug` pass。CTest 24/24。
  - final: `just verify` pass。format check、clang-tidy、linux debug tests、ASan/UBSan tests、Windows MinGW cross build が通った。

Test desiderata:

- purpose: player lights bitfield の保持、reply byte、dispatcher integration を software unit test で固定する。
- key trade-offs: deterministic / fast / behavioral を優先する。実機 LED 表示は predictive ではないため、この work unit の unit test に入れない。
- risks: 実機 acceptability、LED 表示、USB-specific flash behavior は未検証である。
- action: 実機 LED 観測は hardware gate 付きの後続 work unit または `docs/hardware-test-log.md` に残す。

## 10. 検証

実行済み:

- red: `just build-debug` failed as expected because `switch/switch_player_lights.h` was missing.
- green: `just debug` pass、CTest 24/24。
- final verify: `just verify` pass。format check、clang-tidy、linux debug tests、ASan/UBSan tests、Windows MinGW cross build が通った。

実機で player lights subcommand が Switch に受け入れられること、LED が実際に点灯または点滅することは、この software verification だけでは証明しない。

## 11. 実機実行条件

policy core と dispatcher unit test だけなら実機検証は不要である。

Switch 上の player lights 表示と subcommand acceptability を確認する段階では実機検証が必要である。

実機検証を行う場合は、人間が pairing、HID advertising、report loop、LED observation の範囲を明示承認する。

実機検証では専用 USB Bluetooth ドングルを使い、内蔵 Bluetooth と常用ドングルは使わない。

Windows native では Zadig による WinUSB assignment、USB VID/PID、driver state を記録する。

実行時は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。

結果は `docs/hardware-test-log.md` に OS、dongle、driver、backend、BTstack commit、swbt commit、Switch firmware、report period、player lights result、cleanup を記録する。

この work unit では実機コマンドを実行していない。理由は、software unit test だけで player lights policy core と dispatcher reply を検証し、Switch pairing、HID advertising、report loop、LED observation を開始していないためである。

## 12. 先送り事項

- 観測: 実機 Switch が swbt の `0x30` ACK と `0x31` reply を受け入れることは、この work unit では証明しない。
  先送り理由: Switch pairing、HID advertising、report loop、LED observation が必要である。
  次の置き場: `docs/hardware-test-log.md` または実機 bring-up work unit。
- 観測: 複数 controller の player slot assignment は未実装である。
  先送り理由: 初期範囲は単一 controller であり、slot assignment は daemon policy の別問題である。
  次の置き場: 後続 player assignment work unit。

## 13. チェックリスト

- [x] work unit record を作成した。
- [x] work unit record を新形式へ更新した。
- [x] red を確認した。
- [x] player lights policy core を実装した。
- [x] dispatcher integration を実装した。
- [x] TDD テストを追加した。
- [x] 根拠監査を完了した。
- [x] `just` 経由の検証を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] 実機状態または未実行理由を記録した。
