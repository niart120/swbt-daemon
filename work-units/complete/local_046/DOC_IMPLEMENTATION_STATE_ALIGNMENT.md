# Documentation Implementation State Alignment

## 1. 概要

README と active spec に残っている古い状態記述を、現在の実装、完了済み work unit、実機ログに合わせて修正する work unit。

この work unit は実装を変更しない。production backend、Windows native 実機観測、`0x21` reply、report period comparison、neutral fail-safe の現在状態を、README と current spec の未解決事項に反映する。

## 2. 起点 / ユースケース

source:

- ユーザ要求: README の状態記述や spec の先送り事項・未実施事項などと履歴・実装にずれがあるため、ドキュメントと実装の乖離を確認してドキュメント側を修正する。
- `README.md` が、daemon は Switch pairing 未対応、実機未検証、現行実行ファイルは noop backend だけと書いていた。
- `spec/protocols/switch-hid-core.md` が、prioritized `0x21` reply と report period の実機受理を未観測扱いにしていた。
- `spec/operations/windows-native-preflight.md` が、Windows native execution、WinUSB driver assignment、Switch pairing、report period comparison、neutral fail-safe を未解決事項として残していた。
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`、`work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`、`work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md` は、上記より新しい実装状態と実機観測を記録している。

use case:

- actor: maintainer。
- 入力または状態: current `main` 実装、完了済み work unit record、`docs/hardware-test-log.md`、README と current spec。
- 期待する観測結果: README と active spec が、default no-op、production opt-in、限定済み hardware observation、未検証の残り範囲を分けて説明する。
- 制約: 新しい Switch protocol 値、BTstack source selection、report period default、実機手順は追加しない。実機観測は OS、adapter、driver、firmware の限定条件付きで扱う。
- 対象外: 実装変更、追加実機 run、production backend 既定化、device info profile の正規化、stable metrics protocol。
- source から use case へ変換した判断: historical work unit record の source 欄は当時の未検証状態として残し、current README / spec の未解決事項だけを現在状態へ直す。

## 3. 対象範囲

- `README.md` の状態記述と repository structure を現在の実装へ合わせる。
- `spec/architecture/daemon-runtime-boundaries.md` の entrypoint 根拠と関連 work unit を更新する。
- `spec/protocols/switch-hid-core.md` の `0x21` reply、report period、production report prefix の実機観測状態を更新する。
- `spec/operations/windows-native-preflight.md` の根拠と未解決事項を、`local_037` / `local_045` 後の状態へ更新する。
- `spec/operations/windows-hardware-bringup-sequence.md` を、初回 bring-up 完了後も使える再実行順序として表現する。
- spec index の説明を更新する。

## 4. 対象外

- C source、CMake、test code の変更。
- `vendor/btstack` の変更。
- 実機コマンド、Switch pairing、HID advertising、report loop の再実行。
- report period default の変更。
- `SWBT_DAEMON_BACKEND=production` の既定化。

## 5. 関連 spec / docs

- `README.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/protocols/switch-hid-core.md`
- `spec/protocols/daemon-ipc-v1.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_043/PRODUCTION_DAEMON_BTSTACK_ENTRYPOINT.md`
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`

## 6. 根拠監査

`source-audit` を使用した。

この work unit は新しい protocol 値や report period default を追加しない。既存の implementation fact と hardware observation の分類を README / spec へ反映する。実機観測は `docs/hardware-test-log.md` と `local_037` / `local_045` に記録済みの CSR8510 A10、WinUSB、Switch2 firmware `22.1.0` の範囲に限定する。

### 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| default daemon backend | no-op | implementation fact | `apps/swbt-daemon/main.c` | current default |
| production backend selection | `SWBT_DAEMON_BACKEND=production` | implementation fact | `apps/swbt-daemon/main.c`, `swbt/daemon/production_backend.c` | opt-in |
| hardware approval gate | `SWBT_RUN_HARDWARE=1` and `SWBT_HARDWARE_APPROVED=1` | implementation fact | `swbt/daemon/production_backend.c`, `tests/daemon_production_backend_test.c` | required before adapter open |
| Switch2 hardware observation | CSR8510 A10 / WinUSB / Switch2 firmware `22.1.0` | hardware observation | `docs/hardware-test-log.md`, `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | limited observation |
| report period comparison | `8000 / 8333 / 15000 / 16667 us` | hardware observation | `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` | coarse acceptance, not optimized |

### 未解決事項

- 別 adapter / firmware の一般互換性、長時間安定性、report jitter / latency / drop-rate は、この work unit では証明しない。

## 7. 設計メモ

- current spec では「未検証」と「限定済み実機観測」を分ける。
- completed work unit record の過去の source には、当時の未検証状態が残ってよい。current state の読者が見る README と active spec だけを更新対象にする。
- `8000us` は current default として維持する。今回の修正で最適値や source-audited default に昇格しない。

## 8. 対象ファイル

- `README.md`
- `spec/README.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/protocols/README.md`
- `spec/protocols/switch-hid-core.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `spec/references/README.md`
- `spec/references/btstack-backend-build-matrix.md`
- `spec/references/btstack-periodic-input-report-core.md`
- `spec/references/btstack-production-ports.md`
- `spec/references/btstack-subcommand-reply-send-queue.md`
- `spec/references/switch-hid-descriptor-core.md`
- `spec/references/switch-report-core.md`
- `spec/references/switch-subcommand-dispatcher-core.md`
- `work-units/complete/local_046/DOC_IMPLEMENTATION_STATE_ALIGNMENT.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| pass | README state distinguishes default no-op backend from production opt-in hardware run | regression | docs | no |
| pass | active specs no longer claim Windows native pairing / report period / neutral fail-safe are unobserved current state | regression | docs | no |
| pass | Switch HID spec keeps report period and hardware observations limited to recorded adapter / firmware conditions | regression | docs | no |
| pass | spec indexes and reference summaries describe current responsibilities without stale production-send-path or hardware-not-proven wording | regression | docs | no |

## 10. 検証

- `git diff --check`: pass。Windows checkout の LF to CRLF warning のみ。
- stale current-state marker search over `README.md`, `spec/`, and this work unit record: no matches。
- build / CTest: not run。docs-only 変更で C source、CMake、test code を変更していないため。

## 11. 実機実行条件

実機実行は不要である。

理由: この work unit は既存の実装、完了済み work unit record、`docs/hardware-test-log.md` の記録を文書へ反映する。Bluetooth adapter open、Switch pairing、HID advertising、report loop を実行しない。

## 12. 先送り事項

none。

## 13. チェックリスト

- [x] branch と worktree status を確認した。
- [x] README の状態記述を現在の実装へ合わせた。
- [x] active spec の未解決事項を現在の実機観測へ合わせた。
- [x] reference summary のうち後続実機観測で状態が変わった項目を更新した。
- [x] 根拠監査と実機未実行理由を記録した。
- [x] docs-only 検証を実行した。
- [x] work unit record を complete へ移した。

## 14. 完了判定

complete。

理由: README、active spec、reference summary の stale current-state 記述を、現在の実装と `local_037` / `local_045` の限定観測へ合わせた。残した未検証事項は、別 adapter / firmware 互換性、jitter / latency / drop-rate、DATA / SET_REPORT callback 選択、virtual SPI 全域、rumble effect、LED 実動作など、この work unit で証明していない範囲に限定している。
