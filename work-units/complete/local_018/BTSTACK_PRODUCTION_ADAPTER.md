# BTstack Production Adapter

## 1. 概要

`local_012` で追加した fake backend で testable な registration core を、
pinned BTstack の Classic HID Device API に接続する production adapter の work unit。

BTstack production adapter は `swbt/btstack_bridge/` に閉じ込め、BTstack header と license 境界を Switch protocol core へ漏らさない。

## 2. 起点 / ユースケース

起点は `spec/initial/REPOSITORY_INITIALIZATION_TODO.md` の BTstack integration 後続項目である。
`local_012` は caller-supplied backend table を使う registration core までを完了し、実 BTstack API への接続は対象外にした。

ユースケース:

- daemon runtime integration が、BTstack HID Device registration core へ production backend table を渡せる。
- production adapter が使う BTstack headers と entry point を source-audited reference で追跡できる。
- Linux `libusb` build と Windows `windows-winusb` cross build で adapter boundary が compile/link できる。

## 3. 対象範囲

- `swbt_btstack_hid_registration_backend_t` を実 BTstack API へ接続する production adapter を追加する。
- adapter で使う BTstack entry point、header、link 境界を根拠監査する。
- `vendor/btstack` を直接編集せず、`swbt/btstack_bridge/` 側で統合する。
- `sdp_init`、HID SDP record 作成、service 登録、`hid_device_init`、packet handler registration を production backend table から呼べるようにする。
- Linux `libusb` build と Windows `windows-winusb` cross build の compile/link 境界を確認する。

## 4. 対象外

- Switch HID descriptor bytes の新規定義。
- daemon main からの run loop 起動、HID advertising、pairing、report loop 実行。
- output report data callback の production 登録。
- SET_REPORT callback 判断。
- BTstack source selection の広範囲な再設計。
- `vendor/btstack` の変更。
- binary release と配布物作成。

## 5. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/btstack-hid-device-registration.md`
- `spec/references/btstack-source-selection.md`
- `spec/references/btstack-backend-build-matrix.md`
- `spec/references/btstack-production-adapter.md`
- `work-units/complete/local_012/BTSTACK_HID_DEVICE_REGISTRATION.md`
- `work-units/complete/local_017/SWITCH_HID_DESCRIPTOR_CORE.md`
- `swbt/btstack_bridge/README.md`
- `THIRD_PARTY_NOTICES.md`

## 6. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| BTstack HID Device setup API | `hid_create_sdp_record`, `hid_device_init`, `hid_device_register_packet_handler` | source fact | `spec/references/btstack-production-adapter.md` | recorded |
| production adapter include/link set | `classic/hid_device.h`, `classic/sdp_server.h`, `classic/sdp_util.h`; selected BTstack include dirs only | source fact / implementation fact | `spec/references/btstack-production-adapter.md` | recorded |
| SDP record length function | `de_get_len(const uint8_t *)` | source fact | `spec/references/btstack-production-adapter.md` | recorded |
| BTstack run loop ownership | BTstack-owning thread only | design policy | `swbt/btstack_bridge/README.md` | adapter must preserve boundary |
| backend build matrix | `libusb` / `windows-winusb` build success | verification fact | `spec/references/btstack-backend-build-matrix.md` | prior record |
| 実機 advertising / pairing | 未実行 | 実機根拠なし | `docs/hardware-test-log.md` | hardware-gated |

BTstack を link する成果物の license notice 境界は `THIRD_PARTY_NOTICES.md` で確認する。

Windows native runtime 挙動は cross build だけでは証明しない。

## 7. 設計メモ

- adapter は `swbt/btstack_bridge/` に閉じ込め、Switch protocol core へ BTstack header を漏らさない。
- registration core は既存の fake backend test を維持し、production adapter は最小の translation layer にする。
- daemon main はこの work unit では stub のままとし、run loop と実機 advertising は後続 work unit に残す。
- BTstack API は BTstack-owning thread から呼ぶ前提を崩さない。
- BTstack を含む build artifact を MIT-only artifact と表現しない。

## 8. 対象ファイル

- `swbt/btstack_bridge/hid_device_btstack_adapter.h`
- `swbt/btstack_bridge/hid_device_btstack_adapter.c`
- `tests/btstack_hid_device_btstack_adapter_test.c`
- `CMakeLists.txt`
- `spec/references/btstack-production-adapter.md`
- `work-units/complete/local_018/BTSTACK_PRODUCTION_ADAPTER.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | production adapter exposes a backend table compatible with registration core | new | unit | no |
| refactor-done | production adapter forwards SDP and HID calls to BTstack symbols through registration core | new | unit | no |
| refactor-done | adapter rejects missing descriptor/config before production registration is attempted | regression | unit | no |
| refactor-done | Linux `libusb` build compiles and links the adapter boundary | verification | build | no |
| refactor-done | Windows `windows-winusb` cross build compiles and links the adapter boundary | verification | build | no |
| refactor-done | vendor BTstack files remain unmodified | regression | build | no |

## 10. 検証

TDD red / green:

- red: `just build-debug` は `btstack_bridge/hid_device_btstack_adapter.h` missing で compile 失敗した。
- green attempt: adapter 実装後の `just build-debug` は `swbt_core` から `swbt_btstack_source_selection` の backend link libs が全 executable に伝播し、`-llibusb-1.0` が見つからず link 失敗した。
- green: CMake を selected BTstack include dirs のみへ変更し、`just build-debug` は成功した。
- green: `just test-debug` は 16/16 passed。`btstack_hid_device_btstack_adapter_test` を含む。
- refactor: `just verify` の初回実行は `btstack_hid_device_btstack_adapter_test.c` の fake callback と reset helper が clang-tidy `bugprone-easily-swappable-parameters` に該当して失敗した。BTstack callback ABI の箇所は `NOLINTNEXTLINE` で明示し、reset helper は options struct に変更した。
- refactor: clang static analyzer が test fake state の `memset` に警告を出したため、compound literal assignment に変更した。

最終検証:

- `just verify` は成功した。内容は format-check、clang-tidy、linux-debug build/test 16/16、linux-asan build/test 16/16、windows-mingw cross build。
- `vendor/btstack` は変更していない。

Test Desiderata review:

- 価値: production adapter が BTstack symbols へ渡す引数と registration core との互換性を unit test で固定できる。
- 独立性: fake BTstack symbols を test binary 内に置くため、Bluetooth adapter、Switch 実機、BTstack run loop に依存しない。
- 残るリスク: 実 BTstack run loop、HID advertising、Switch pairing、output report callback はこの test では証明しない。

## 11. 実機実行条件

この work unit では実機を実行しない。

この work unit は production adapter の compile/link 境界までを主対象にし、Switch pairing、HID advertising、report loop は実行しない。

実機 advertising または pairing を実行する場合は、人間の明示承認を必須とする。

実機実行では専用 USB Bluetooth ドングルを使い、普段使いの内蔵 Bluetooth や常用ドングルを使わない。

実機実行では `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を必須条件にする。

実機結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を記録する。

## 12. 先送り事項

- daemon main からの production registration 呼び出し。
- output report data callback の production 登録。
- 実機 advertising、pairing、HID connection、report loop。

## 13. チェックリスト

- [x] work unit record を新形式へ更新した。
- [x] 根拠監査を記録した。
- [x] BTstack production adapter を実装した。
- [x] unit test を追加した。
- [x] build 検証を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] 実機状態を記録した。
