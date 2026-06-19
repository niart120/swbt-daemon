# Switch HID Descriptor Core

## 1. 概要

実機前に残っている Switch Pro Controller 相当の HID descriptor core を追加するための計画 record。

BTstack HID Device registration に渡す descriptor bytes を swbt 側の read-only data として管理し、実装前に descriptor bytes、descriptor length、report ID 構成の根拠監査を完了する。

## 2. 対象範囲

- Switch Pro Controller 相当の HID descriptor bytes を swbt 側の core data として追加する。
- descriptor pointer と descriptor size を返す accessor を追加する。
- descriptor bytes、descriptor length、report ID 構成を実装前に根拠監査する。
- `swbt_btstack_hid_device_register` の caller-supplied descriptor として利用できる形にする。
- unit test で descriptor accessor の size、pointer、安定性を確認する。

## 3. 対象外

- BTstack production adapter の追加。
- output report callback の production 登録。
- daemon main からの HID advertising、pairing、report loop 実行。
- 実機 Switch での descriptor acceptability 検証。
- `vendor/btstack` の変更。
- Joy-Con L/R、NFC/IR MCU、amiibo 対応。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-hid-initial-source-audit.md`
- `spec/references/btstack-hid-device-registration.md`
- `work-units/complete/local_012/BTSTACK_HID_DEVICE_REGISTRATION.md`
- `swbt/btstack_bridge/README.md`

## 5. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| Switch Pro Controller HID descriptor bytes | 未定 | 未監査 | TBD | 実装前に根拠監査が必要 |
| HID descriptor length | 未定 | 未監査 | TBD | 実装前に根拠監査が必要 |
| report ID 構成 | 未定 | 未監査 | TBD | 実装前に根拠監査が必要 |
| BTstack registration pass-through | caller-supplied descriptor pointer/size | source fact | `spec/references/btstack-hid-device-registration.md` | recorded |
| 実機 acceptability | 未検証 | hardware fact missing | `docs/hardware-test-log.md` | 実機未実行 |

HID descriptor bytes は未監査であり、実装前に `source-audit` で根拠を記録する。

実機 Switch が descriptor を受け入れるかは未検証である。

descriptor 由来の protocol 値を推定で断定しない。

## 6. 設計メモ

- descriptor data は `swbt/switch/` 配下に置き、BTstack header へ直接依存しない。
- accessor は `const uint8_t *` と size を返すだけにし、registration 順序は local_012 の API に任せる。
- descriptor bytes は監査済み reference と対応づけ、magic byte の出所を work unit record だけに閉じ込めない。
- descriptor の実機調整が必要になった場合は別 work unit として扱う。

## 7. 対象ファイル

- `swbt/switch/switch_hid_descriptor.h`
- `swbt/switch/switch_hid_descriptor.c`
- `tests/switch_hid_descriptor_test.c`
- `spec/references/switch-hid-descriptor-core.md`
- `CMakeLists.txt`
- `work-units/wip/local_017/SWITCH_HID_DESCRIPTOR_CORE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | descriptor accessor returns a non-null read-only byte sequence and size | new | unit | no |
| todo | descriptor size matches the audited HID descriptor length | new | unit | no |
| todo | descriptor bytes match the audited source fixture | characterization | unit | no |
| todo | HID registration config can receive descriptor pointer and size without copying | regression | unit | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、実装、build、CTest、実機検証は実行していない。

実装後は `make debug CTEST_ARGS="-R switch_hid_descriptor_test"` を実行する。

変更範囲に応じて `make asan`、`make windows-cross`、`make verify` を実行する。

## 10. 実機実行条件

この work unit record 作成時点では実機を実行しない。

descriptor acceptability を実機確認する場合は、人間の明示承認を必須とする。

実機確認では専用 USB Bluetooth ドングルを使い、普段使いの内蔵 Bluetooth や常用ドングルを使わない。

実機確認では `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を必須条件にする。

実機確認結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] 根拠監査を完了した。
- [ ] HID descriptor core を実装した。
- [ ] unit test を追加した。
- [ ] 検証コマンドを実行した。
- [ ] 実機状態を記録した。
