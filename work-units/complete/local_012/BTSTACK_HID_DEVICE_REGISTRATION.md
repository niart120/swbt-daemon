# BTstack HID Device Registration

## 1. 概要

Phase 4: BTstack bridge の HID Device registration 境界を追加する work unit。

BTstack の Classic HID Device 登録手順を swbt 側の小さな API に閉じ込め、unit test では fake backend で呼び出し順序と parameter pass-through を確認する。

## 2. 対象範囲

- HID SDP record 登録に必要な configuration struct を追加する。
- BTstack HID registration backend の function table を追加する。
- `sdp_init`、HID SDP record 作成、`sdp_register_service`、`hid_device_init`、packet handler registration の順序を固定する。
- SDP record length が service buffer を超える場合に error を返す。
- Phase 4 TODO の HID Device registration を完了状態にする。

## 3. 対象外

- Switch Pro Controller HID descriptor bytes の実装。
- BTstack 本体の変更。
- daemon main からの実機 advertising / pairing 実行。
- Output Report parser 接続。
- Subcommand `0x21` reply。
- Periodic `0x30` input report。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/references/btstack-hid-device-registration.md`
- `spec/references/btstack-source-selection.md`
- `swbt/btstack_bridge/README.md`

## 5. 根拠監査

### 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| BTstack HID SDP record params | `hid_sdp_record_t` field set | source fact | `vendor/btstack/src/classic/hid_device.h:59-74` | stable BTstack API at pinned commit |
| BTstack HID Device setup API | `hid_create_sdp_record`, `hid_device_init`, `hid_device_register_packet_handler` | source fact | `vendor/btstack/src/classic/hid_device.h:76-98` | stable BTstack API at pinned commit |
| Example registration order | SDP init, HID record create/register, HID device init, packet handler registration | source fact | `vendor/btstack/example/hid_keyboard_demo.c:470-510` | example-driven registration sequence |

### 未解決事項

- Switch Pro Controller HID descriptor bytes は未監査であり、この work unit では caller-supplied data として扱う。
- 実機 Switch での acceptability は未検証である。

## 6. 設計メモ

- `swbt_core` から vendor header に直接依存させず、backend function table で登録順序を unit test 可能にする。
- production BTstack adapter は後続 work unit で追加する。
- service buffer は caller が所有する。backend が record length を返した後、buffer size を超える場合は service registration を実行しない。

## 7. 対象ファイル

- `swbt/btstack_bridge/hid_device_registration.h`
- `swbt/btstack_bridge/hid_device_registration.c`
- `tests/btstack_hid_device_registration_test.c`
- `spec/references/btstack-hid-device-registration.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/complete/local_012/BTSTACK_HID_DEVICE_REGISTRATION.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | fake backend observes HID SDP and HID Device registration in BTstack example order | new | unit | no |
| refactor-done | invalid arguments are rejected before backend calls | edge | unit | no |
| refactor-done | oversized SDP record is rejected before service registration | edge | unit | no |
| refactor-done | SDP registration failure is propagated as registration error | edge | unit | no |

## 9. 検証

- red: `make debug CTEST_ARGS="-R btstack_hid_device_registration_test"` は missing `btstack_bridge/hid_device_registration.h` のため compile で失敗した。
- green: `make debug CTEST_ARGS="-R btstack_hid_device_registration_test"` は 1/1 passed。
- standard verification: `make verify` は pass。
  - format-check pass。
  - clang-tidy preset build pass。
  - linux-debug CTest 10/10 passed。
  - linux-asan CTest 10/10 passed。
  - windows-mingw-debug cross build pass。

## 10. 実機実行条件

実機検証は不要。

この work unit は fake backend による unit test のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 11. チェックリスト

- [x] red を確認した。
- [x] HID Device registration core を追加した。
- [x] unit test を追加した。
- [x] `make debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] 実機状態を記録した。
