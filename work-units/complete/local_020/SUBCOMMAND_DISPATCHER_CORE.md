# Subcommand Dispatcher Core

## 1. 概要

Switch output report parser と subcommand reply builder の間に、subcommand ID ごとの処理を選ぶ dispatcher core を追加する work unit。

この work unit は BTstack send path に依存しない core 部分だけを扱い、返信が必要な場合は 50 byte の `0x21` reply report を出力する。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_005/SWITCH_SUBCOMMAND_CORE.md` は output report parser と subcommand ID classifier を追加した。
- `work-units/complete/local_006/SWITCH_SPI_CORE.md` は virtual SPI read boundary を追加した。
- `work-units/complete/local_014/SWITCH_SUBCOMMAND_REPLY_CORE.md` は `0x21` subcommand reply builder を追加し、dispatcher と ACK policy を後続に残した。

use case:

- actor: daemon runtime または output report callback handler。
- 入力または状態: parsed `swbt_switch_output_report_t`、controller state、report options、virtual SPI storage。
- 期待する観測結果: handled subcommand は reply action を返し、rumble-only report は no-reply、unsupported subcommand と malformed payload は explicit result を返す。
- 制約: BTstack send-ready queue、HID interrupt send、実機 report loop は扱わない。
- 対象外: 実機 Switch が simple ACK だけで受け入れることの断定。

source から use case への変換:

dispatcher は existing parser / reply builder / SPI core を接続する。`SET_REPORT_MODE`、`ENABLE_IMU`、`ENABLE_VIBRATION` は simple ACK を返し、SPI read は request の address / size を `switch_spi` に渡して reply data を作る。その他の subcommand は explicit unsupported として後続に残す。

## 3. 対象範囲

- `swbt_switch_output_report_t` を入力として subcommand handler を選ぶ。
- `SET_REPORT_MODE`、`ENABLE_IMU`、`ENABLE_VIBRATION` を simple ACK reply に dispatch する。
- SPI flash read は virtual SPI read へ委譲し、echoed address / size と read data を reply payload にする。
- dispatcher は reply report または no-reply action を caller へ返す。
- malformed SPI read data、unsupported subcommand、reply build failure を explicit result で返す。
- dispatcher core を unit test で固定する。

## 4. 対象外

- BTstack send-ready queue。
- HID interrupt send の直接呼び出し。
- virtual SPI seed data の実体値。
- player lights の bit policy。
- request device info、manual pairing、regulated voltage の reply data。
- report mode、IMU、vibration の session state 保存。
- 実機 pairing、HID advertising、report loop。

## 5. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/references/switch-subcommand-core.md`
- `spec/references/switch-subcommand-reply-core.md`
- `spec/references/switch-spi-core.md`
- `spec/references/switch-subcommand-dispatcher-core.md`
- `work-units/complete/local_005/SWITCH_SUBCOMMAND_CORE.md`
- `work-units/complete/local_006/SWITCH_SPI_CORE.md`
- `work-units/complete/local_014/SWITCH_SUBCOMMAND_REPLY_CORE.md`

## 6. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| parsed subcommand input | `swbt_switch_output_report_t` | implementation fact | `spec/references/switch-subcommand-dispatcher-core.md` | recorded |
| simple ACK reply | ACK `0x80`, no reply data | source fact / implementation contract | `spec/references/switch-subcommand-dispatcher-core.md` | recorded |
| SPI read request shape | little-endian address + size | source fact | `spec/references/switch-subcommand-core.md`; `spec/references/switch-subcommand-dispatcher-core.md` | recorded |
| SPI read reply shape | ACK `0x90`, echoed address/size, read data | source fact | `spec/references/switch-subcommand-reply-core.md`; `spec/references/switch-subcommand-dispatcher-core.md` | recorded |
| SPI read validation | delegated to `swbt_switch_spi_read` | implementation fact | `spec/references/switch-spi-core.md` | recorded |
| implemented simple subcommands | `SET_REPORT_MODE`, `ENABLE_IMU`, `ENABLE_VIBRATION` | source fact for IDs; implementation policy | `spec/references/switch-subcommand-dispatcher-core.md` | recorded |
| 実機 acceptability | 未検証 | 実機根拠なし | `docs/hardware-test-log.md` | hardware-gated |

実機 Switch での要求順序と acceptability は未検証である。

## 7. 設計メモ

- dispatcher は BTstack header に依存しない。
- dispatcher は parsed report の `subcommand_data` を同期的に読み、必要な reply data は出力構造へ copy する。
- reply report の byte layout は `swbt_switch_build_subcommand_reply` に任せ、dispatcher では重複実装しない。
- SPI flash read は address と size の検証を `switch_spi.*` に寄せる。
- report mode、IMU、vibration の状態を扱う場合は session state に分け、IPC の controller state へ混ぜない。
- player lights は後続の `PLAYER_LIGHTS_POLICY` に分ける。

## 8. 対象ファイル

- `CMakeLists.txt`
- `swbt/switch/switch_subcommand_dispatcher.h`
- `swbt/switch/switch_subcommand_dispatcher.c`
- `tests/switch_subcommand_dispatcher_test.c`
- `spec/references/switch-subcommand-dispatcher-core.md`
- `work-units/complete/local_020/SUBCOMMAND_DISPATCHER_CORE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | source-audited simple subcommand is dispatched to a reply action without BTstack send | new | unit | no |
| refactor-done | SPI flash read delegates address and size validation to virtual SPI | new | unit | no |
| refactor-done | unsupported or unknown subcommand returns explicit unsupported result | edge | unit | no |
| refactor-done | malformed subcommand payload does not build reply bytes | edge | unit | no |
| refactor-done | rumble-only report returns no-reply action | edge | unit | no |

## 10. 検証

TDD red / green:

- red: `just build-debug` は `switch/switch_subcommand_dispatcher.h` missing で compile 失敗した。
- green: `just build-debug` は成功した。
- green: `just test-debug` は 18/18 passed。`switch_subcommand_dispatcher_test` を含む。
- refactor: `just verify` の初回実行は test の enum action 比較で clang-tidy sign-conversion により失敗した。action 専用 helper に変更した。
- refactor: `just format` は成功した。

最終検証:

- `just verify` は成功した。内容は format-check、clang-tidy、linux-debug build/test 18/18、linux-asan build/test 18/18、windows-mingw cross build。
- `vendor/btstack` は変更していない。

Test Desiderata review:

- 価値: parser、reply builder、SPI core の接続を deterministic unit test で固定できる。
- 独立性: BTstack send queue、Bluetooth adapter、Switch 実機に依存しない。
- 残るリスク: 実機 Switch が simple ACK を受け入れること、send-ready queue で返信を送ること、session state の更新はこの test では証明しない。

## 11. 実機実行条件

この work unit では実機を実行しない。

この work unit の unit test 実装では実機検証は不要である。

Switch が dispatcher 応答を受け入れるかを確認する段階では実機検証が必要である。

実機検証を行う場合は、人間の明示承認を得る。

実機検証を行う場合は、専用 USB Bluetooth ドングルだけを使う。

実機検証を行う場合は、`SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。

実機検証を行う場合は、OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を `docs/hardware-test-log.md` に記録する。

## 12. 先送り事項

- request device info、manual pairing、regulated voltage の reply data。
- player lights の bit policy。
- report mode、IMU、vibration の session state 保存。
- BTstack send-ready queue への接続。
- 実機 Switch での dispatcher reply acceptability 確認。

## 13. チェックリスト

- [x] work unit record を新形式へ更新した。
- [x] 根拠監査を完了した。
- [x] Subcommand dispatcher core を実装した。
- [x] テストを追加した。
- [x] 検証を実行した。
- [x] 実機状態を記録した。
