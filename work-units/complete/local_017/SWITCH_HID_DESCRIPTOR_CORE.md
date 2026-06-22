# Switch HID Descriptor Core

## 1. 概要

Switch Pro Controller 相当の HID report descriptor を `swbt/switch/` の
read-only data として追加する work unit。

BTstack HID Device registration に渡す descriptor pointer と size を swbt 側で提供し、
descriptor bytes、length、report ID 構成の根拠監査を実装前に記録する。

## 2. 起点 / ユースケース

起点は `spec/initial/REPOSITORY_INITIALIZATION_TODO.md` の HID Device registration 後続項目である。
`local_012` は BTstack registration 境界を追加したが、Switch Pro Controller 固有の HID descriptor bytes は caller-supplied data として対象外にした。

ユースケース:

- production adapter が BTstack registration に渡す HID report descriptor pointer と size を取得する。
- unit test で descriptor bytes と length が source-audited fixture から drift していないことを確認する。
- 実機前に descriptor 由来の protocol 値を推定ではなく根拠付きで固定する。

## 3. 対象範囲

- Switch Pro Controller 相当の HID report descriptor bytes を core data として追加する。
- descriptor pointer と descriptor size を返す accessor を追加する。
- descriptor bytes、descriptor length、report ID 構成の根拠監査を `spec/references/` に記録する。
- `swbt_btstack_hid_device_register` の caller-supplied descriptor として使える API shape にする。
- unit test で descriptor accessor の size、pointer、安定性、byte fixture を確認する。

## 4. 対象外

- BTstack production adapter の追加。
- output report callback の production 登録。
- daemon main からの HID advertising、pairing、report loop 実行。
- 実機 Switch での descriptor acceptability 検証。
- USB device / configuration / endpoint descriptor の実装。
- `vendor/btstack` の変更。
- Joy-Con L/R、NFC/IR MCU、amiibo 対応。

## 5. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/references/switch-hid-initial-source-audit.md`
- `spec/references/btstack-hid-device-registration.md`
- `spec/references/switch-hid-descriptor-core.md`
- `work-units/complete/local_012/BTSTACK_HID_DEVICE_REGISTRATION.md`
- `swbt/btstack_bridge/README.md`

## 6. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| Switch Pro Controller HID report descriptor bytes | ToadKing gist の 203 bytes | source fact | `spec/references/switch-hid-descriptor-core.md` | recorded |
| HID report descriptor length | `203` bytes / `0x00CB` | source fact corroborated by real-device descriptor dump | `spec/references/switch-hid-descriptor-core.md` | recorded |
| report ID 構成 | input `0x30`, `0x21`, `0x81`; output `0x01`, `0x10`, `0x80`, `0x82` | source fact | `spec/references/switch-hid-descriptor-core.md` | recorded |
| BTstack registration pass-through | caller-supplied descriptor pointer/size | source fact | `spec/references/btstack-hid-device-registration.md` | recorded |
| 実機 acceptability | 未検証 | 実機根拠なし | `docs/hardware-test-log.md` | hardware-gated |

実機 Switch が descriptor を受け入れるかは未検証である。この work unit では software unit test までを完了条件にする。

## 7. 設計メモ

- descriptor data は `swbt/switch/` 配下に置き、BTstack header へ直接依存しない。
- accessor は `const uint8_t *` と `size_t` を返すだけにし、registration 順序は local_012 の API に任せる。
- BTstack に渡すのは HID report descriptor だけであり、USB configuration descriptor は含めない。
- descriptor の実機調整が必要になった場合は別 work unit として扱う。

## 8. 対象ファイル

- `swbt/switch/switch_hid_descriptor.h`
- `swbt/switch/switch_hid_descriptor.c`
- `tests/switch_hid_descriptor_test.c`
- `spec/references/switch-hid-descriptor-core.md`
- `CMakeLists.txt`
- `work-units/complete/local_017/SWITCH_HID_DESCRIPTOR_CORE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | descriptor accessor returns a non-null read-only byte sequence and size | new | unit | no |
| refactor-done | descriptor size matches the source-audited HID descriptor length | new | unit | no |
| refactor-done | descriptor bytes match the source-audited fixture | characterization | unit | no |
| refactor-done | descriptor exposes the expected report ID markers | characterization | unit | no |
| refactor-done | HID registration config can receive descriptor pointer and size without copying | regression | unit | no |

## 10. 検証

TDD red / green:

- red: `just build-debug` は `switch/switch_hid_descriptor.h` missing で compile 失敗した。
- green: `just build-debug` は成功した。
- green: `just test-debug` は 15/15 passed。`switch_hid_descriptor_test` を含む。
- refactor: `just verify` の初回実行は `switch_hid_descriptor_test.c` の test helper が clang-tidy `bugprone-easily-swappable-parameters` に該当して失敗した。`descriptor_view_t` に変更して修正した。
- CI follow-up: PR #24 の初回 CI は format-check で失敗した。原因は新規 C files が `just format` 実行時点で untracked だったため、local format script の対象外だったこと。新規 files が tracked になった後に `just format` を再実行して修正した。

実行経路の誤り:

- `ctest --preset linux-debug -R switch_hid_descriptor_test --output-on-failure` は Windows host から直接実行したため、CTest 内の Dev Container 側 executable path と合わず Not Run になった。この失敗は実装の red として扱わない。

整形:

- `just format` は成功した。

最終検証:

- `just verify` は成功した。内容は format-check、clang-tidy、linux-debug build/test 15/15、linux-asan build/test 15/15、windows-mingw cross build。
- CI format 修正後の `just verify` も成功した。

Test Desiderata review:

- 価値: descriptor accessor の API contract と 203 byte fixture の drift を unit test で検出できる。
- 独立性: Bluetooth adapter、BTstack runtime、Switch 実機に依存しない。
- 残るリスク: 実機 Switch が descriptor を受け入れることはこの test では証明しない。

## 11. 実機実行条件

この work unit では実機を実行しない。

descriptor acceptability を実機確認する場合は、人間の明示承認を必須とする。

実機確認では専用 USB Bluetooth ドングルを使い、普段使いの内蔵 Bluetooth や常用ドングルを使わない。

実機確認では `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を必須条件にする。

実機確認結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、ドライバー、BTstack commit、swbt commit、Switch firmware、report period、結果を記録する。

## 12. 先送り事項

- Switch acceptability の実機確認。
- Bluetooth SDP parameters と class-of-device の実機調整。
- descriptor が `0x81`、`0x80`、`0x82` を含む理由の protocol-level 解析。

## 13. チェックリスト

- [x] work unit record を新形式へ更新した。
- [x] 根拠監査を記録した。
- [x] HID descriptor core を実装した。
- [x] unit test を追加した。
- [x] 検証コマンドを実行した。
- [x] 実機状態を記録した。
