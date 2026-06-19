# Subcommand Dispatcher Core

## 1. 概要

Switch output report parser と subcommand reply builder の間に、subcommand ID ごとの処理を選ぶ dispatcher core を追加するための計画 record。

local_014 で後続に残した ACK policy と subcommand dispatcher のうち、BTstack send path に依存しない core 部分を扱う。

## 2. 対象範囲

- `swbt_switch_output_report_t` を入力として subcommand handler を選ぶ。
- state、manual pairing、request device info、set report mode、SPI flash read、enable IMU、enable vibration、get regulated voltage を初期候補として整理し、実装対象と延期対象を根拠監査後に分ける。
- SPI flash read は virtual SPI read へ委譲し、reply payload 候補を返す。
- dispatcher は reply report または no-reply action を caller へ返す。
- malformed subcommand data、unsupported subcommand、handler failure を explicit result で返す。

## 3. 対象外

- BTstack send-ready queue。
- HID interrupt send の直接呼び出し。
- virtual SPI seed data の実体値。
- player lights の bit policy。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/references/switch-subcommand-core.md`
- `spec/references/switch-subcommand-reply-core.md`
- `spec/references/switch-spi-core.md`
- `work-units/complete/local_005/SWITCH_SUBCOMMAND_CORE.md`
- `work-units/complete/local_006/SWITCH_SPI_CORE.md`
- `work-units/complete/local_014/SWITCH_SUBCOMMAND_REPLY_CORE.md`

## 5. 根拠監査

Switch subcommand ID ごとの dispatcher policy は実装前に根拠監査が必要である。

ACK byte と reply bytes は実装前に根拠監査が必要である。

既存の parser と reply builder の根拠監査は参照できるが、subcommand ごとの応答内容としては断定しない。

実機 Switch での要求順序と acceptability は未検証である。

## 6. 設計メモ

- dispatcher は BTstack header に依存しない。
- dispatcher は parsed report の `subcommand_data` を同期的に読み、必要な reply data は出力構造へ copy する。
- report mode、IMU、vibration の状態を扱う場合は session state に分け、IPC の controller state へ混ぜない。
- SPI flash read は address と size の検証を `switch_spi.*` に寄せる。
- player lights は後続の `PLAYER_LIGHTS_POLICY` に分ける。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/switch/switch_subcommand_dispatcher.h`
- `swbt/switch/switch_subcommand_dispatcher.c`
- `tests/switch_subcommand_dispatcher_test.c`
- `spec/references/switch-subcommand-dispatcher-core.md`
- `work-units/wip/local_020/SUBCOMMAND_DISPATCHER_CORE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | source-audited simple subcommand is dispatched to a reply action without BTstack send | new | unit | no |
| todo | SPI flash read delegates address and size validation to virtual SPI | new | unit | no |
| todo | unsupported or unknown subcommand returns explicit unsupported result | edge | unit | no |
| todo | malformed subcommand payload does not build reply bytes | edge | unit | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、red、green、refactor、実機検証は実行していない。

実装後は `make debug CTEST_ARGS="-R switch_subcommand_dispatcher_test"` を実行する。

変更範囲に応じて `make asan` または `make windows-cross` を実行する。

## 10. 実機実行条件

この work unit の unit test 実装では実機検証は不要である。

Switch が dispatcher 応答を受け入れるかを確認する段階では実機検証が必要である。

実機検証を行う場合は、人間の明示承認を得る。

実機検証を行う場合は、専用 USB Bluetooth ドングルだけを使う。

実機検証を行う場合は、`SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定する。

実機検証を行う場合は、OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を `docs/hardware-test-log.md` に記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] 根拠監査を完了した。
- [ ] Subcommand dispatcher core を実装した。
- [ ] テストを追加した。
- [ ] 検証を実行した。
- [ ] 実機状態を記録した。
