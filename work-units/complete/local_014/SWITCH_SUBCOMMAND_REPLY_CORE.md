# Switch Subcommand Reply Core

## 1. 概要

Phase 4: BTstack bridge の Subcommand `0x21` reply を構築する work unit。

input report `0x21` の standard prefix、ACK byte、reply-to subcommand ID、reply data を組み立てる builder を追加する。

## 2. 対象範囲

- input report `0x21` subcommand reply の builder を追加する。
- standard input report prefix に state と report options を反映する。
- ACK byte、reply-to subcommand ID、reply data を offset `13` 以降へ書く。
- reply data 最大長 `35` bytes と output report size `50` bytes を検証する。
- Phase 4 TODO の Subcommand `0x21` reply を完了状態にする。

## 3. 対象外

- Subcommand dispatcher。
- BTstack send path。
- SPI read data の生成。
- Subcommand ID ごとの ACK policy。
- Periodic `0x30` input report。
- 実機 pairing、HID advertising、report loop。

## 4. 関連 spec / docs

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/references/switch-subcommand-reply-core.md`
- `spec/references/switch-report-core.md`
- `spec/references/switch-subcommand-core.md`
- `work-units/complete/local_013/BTSTACK_OUTPUT_REPORT_PARSER_BRIDGE.md`

## 5. 根拠監査

### 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| input report ID | `0x21` | source fact | dekuNukem `bluetooth_hid_notes.md:108-110`; Linux `hid-nintendo.c:84` | stable for implementation |
| ACK byte | byte `13`; simple ACK `0x80`; SPI read ACK `0x90` | source fact | dekuNukem `bluetooth_hid_notes.md:149,156`; dekuNukem `bluetooth_hid_subcommands_notes.md:146-162` | stable for implementation |
| reply-to subcommand ID | byte `14` | source fact | dekuNukem `bluetooth_hid_notes.md:150`; Linux `hid-nintendo.c:511-514` | stable for implementation |
| reply data | bytes `15..49`, max `35` bytes | source fact | dekuNukem `bluetooth_hid_notes.md:151`; Linux `hid-nintendo.c:511-514`, `hid-nintendo.c:542` | stable for implementation |

### 未解決事項

- 実機 Switch での acceptability は未検証である。
- ACK policy と subcommand dispatcher は後続 work unit に残す。

## 6. 設計メモ

- builder は full 50-byte report を返し、未使用 reply data 領域は zero のままにする。
- reply data の内容は caller-provided とし、SPI read echo/data などの subcommand 固有 payload 生成はこの work unit では行わない。
- state/options prefix の packing は `switch_report.*` と同じ layout を使う。

## 7. 対象ファイル

- `swbt/switch/switch_subcommand_reply.h`
- `swbt/switch/switch_subcommand_reply.c`
- `tests/switch_subcommand_reply_test.c`
- `spec/references/switch-subcommand-reply-core.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `work-units/complete/local_014/SWITCH_SUBCOMMAND_REPLY_CORE.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | builder writes 50-byte `0x21` report with state prefix, ACK, subcommand ID, and reply data | new | unit | no |
| refactor-done | builder reports required size for too-small output buffer | edge | unit | no |
| refactor-done | invalid arguments and oversized reply data are rejected | edge | unit | no |

## 9. 検証

- red: `make debug CTEST_ARGS="-R switch_subcommand_reply_test"` は missing `switch/switch_subcommand_reply.h` のため compile で失敗した。
- green: `make debug CTEST_ARGS="-R switch_subcommand_reply_test"` は 1/1 passed。
- standard verification: `make verify` は pass。
  - format-check pass。
  - clang-tidy preset build pass。
  - linux-debug CTest 12/12 passed。
  - linux-asan CTest 12/12 passed。
  - windows-mingw-debug cross build pass。

## 10. 実機実行条件

実機検証は不要。

この work unit は packet builder unit test のみを扱い、Switch pairing、HID advertising、report loop、Bluetooth adapter 操作を実行しない。

## 11. チェックリスト

- [x] red を確認した。
- [x] Subcommand `0x21` reply builder を追加した。
- [x] unit test を追加した。
- [x] `make debug` を実行した。
- [x] sanitizer または cross build の結果を記録した。
- [x] 実機状態を記録した。
