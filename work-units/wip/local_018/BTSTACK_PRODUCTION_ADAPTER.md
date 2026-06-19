# BTstack Production Adapter

## 1. 概要

local_012 で追加した fake-testable registration core を、pinned BTstack の Classic HID Device API に接続するための計画 record。

BTstack production adapter は `swbt/btstack_bridge/` に閉じ込め、BTstack header と license 境界を Switch protocol core へ漏らさない。

## 2. 対象範囲

- `swbt_btstack_hid_registration_backend_t` を実 BTstack API へ接続する production adapter を追加する。
- adapter で使う BTstack entry point、header、link 境界を実装前に根拠監査する。
- `vendor/btstack` を直接編集せず、`swbt/btstack_bridge/` 側で統合する。
- `sdp_init`、HID SDP record 作成、service 登録、`hid_device_init`、packet handler registration を production path で呼べるようにする。
- Linux `libusb` build と Windows `windows-winusb` cross build の compile/link 境界を確認する。

## 3. 対象外

- Switch HID descriptor bytes の新規定義。
- daemon main からの run loop 起動、HID advertising、pairing、report loop 実行。
- output report data callback の production 登録。
- SET_REPORT callback 判断。
- BTstack source selection の広範囲な再設計。
- `vendor/btstack` の変更。
- binary release と配布物作成。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/btstack-hid-device-registration.md`
- `spec/references/btstack-source-selection.md`
- `spec/references/btstack-backend-build-matrix.md`
- `work-units/complete/local_012/BTSTACK_HID_DEVICE_REGISTRATION.md`
- `work-units/wip/local_017/SWITCH_HID_DESCRIPTOR_CORE.md`
- `swbt/btstack_bridge/README.md`
- `THIRD_PARTY_NOTICES.md`

## 5. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| BTstack HID Device setup API | `hid_create_sdp_record`, `hid_device_init`, `hid_device_register_packet_handler` | source fact | `spec/references/btstack-hid-device-registration.md` | recorded for core registration |
| production adapter include/link set | 未定 | 未監査 | TBD | 実装前に根拠監査が必要 |
| SDP record length function | 未定 | 未監査 for production adapter | TBD | 実装前に根拠監査が必要 |
| BTstack run loop ownership | BTstack-owning thread only | design policy | `swbt/btstack_bridge/README.md` | adapter must preserve boundary |
| backend build matrix | `libusb` / `windows-winusb` build success | verification fact | `spec/references/btstack-backend-build-matrix.md` | recorded before adapter |
| 実機 advertising / pairing | 未実行 | hardware fact missing | `docs/hardware-test-log.md` | 実機未実行 |

production adapter が直接参照する BTstack functions と headers は実装前に `source-audit` で確認する。

BTstack を link する成果物の license notice 境界は `THIRD_PARTY_NOTICES.md` を確認してから記録する。

Windows native runtime behavior は cross build だけでは証明しない。

## 6. 設計メモ

- adapter は `swbt/btstack_bridge/` に閉じ込め、Switch protocol core へ BTstack header を漏らさない。
- registration core は既存の fake backend test を維持し、production adapter は最小の translation layer にする。
- daemon main はこの work unit では stub のままとし、run loop と実機 advertising は後続 work unit に残す。
- BTstack API は BTstack-owning thread から呼ぶ前提を崩さない。
- BTstack を含む build artifact を MIT-only artifact と表現しない。

## 7. 対象ファイル

- `swbt/btstack_bridge/hid_device_btstack_adapter.h`
- `swbt/btstack_bridge/hid_device_btstack_adapter.c`
- `tests/btstack_hid_device_btstack_adapter_test.c`
- `cmake/btstack_sources.cmake`
- `CMakeLists.txt`
- `spec/references/btstack-production-adapter.md`
- `work-units/wip/local_018/BTSTACK_PRODUCTION_ADAPTER.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | production adapter exposes a backend table compatible with registration core | new | unit | no |
| todo | adapter rejects missing descriptor/config before production registration is attempted | regression | unit | no |
| todo | Linux `libusb` build compiles and links the adapter boundary | verification | build | no |
| todo | Windows `windows-winusb` cross build compiles and links the adapter boundary | verification | build | no |
| todo | vendor BTstack files remain unmodified | regression | build | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、実装、build、CTest、実機検証は実行していない。

実装後は `make debug CTEST_ARGS="-R btstack_hid_device_btstack_adapter_test"` を実行する。

実装後は `make debug` と `make windows-cross` を実行する。

変更範囲に応じて `make verify` を実行する。

## 10. 実機実行条件

この work unit record 作成時点では実機を実行しない。

この work unit は production adapter の compile/link 境界までを主対象にし、Switch pairing、HID advertising、report loop は実行しない。

実機 advertising または pairing を実行する場合は、人間の明示承認を必須とする。

実機実行では専用 USB Bluetooth ドングルを使い、普段使いの内蔵 Bluetooth や常用ドングルを使わない。

実機実行では `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を必須条件にする。

実機結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] 根拠監査を完了した。
- [ ] BTstack production adapter を実装した。
- [ ] unit test を追加した。
- [ ] build 検証を実行した。
- [ ] sanitizer または cross build の結果を記録した。
- [ ] 実機状態を記録した。
