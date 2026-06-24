# IPC Fuzz And Slow Client Hardening

## 1. 概要

IPC JSON parser と local socket server の hardening を行う work unit。

`local_052` は IPC codec / adapter boundary を分け、malformed JSON が application state を変更しないことを固定した。その時点で parser fuzz と slow client handling は後続候補として残った。`local_055` でも parser fuzz は cutover 後の roadmap として扱われた。

この work unit では、wire protocol v1 を変えずに、malformed / oversized / fragmented input と slow client が daemon state を壊さないことを観測できる test を追加する。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md` の先送り事項: parser fuzz、slow client。
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md` の先送り事項: parser fuzz。
- `spec/protocols/daemon-ipc-v1.md` の current wire contract。
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md` の IPC session 削除後の application context 経路。

use case:

- actor: local IPC client、debug client、malformed client、slow client。
- 入力または状態: malformed JSON、oversized line、partial line、valid command after invalid input、slow receive / send。
- 期待する観測結果: invalid input は typed error response または connection close で扱われ、application state、owner、metrics の整合性を壊さない。slow client は他の connection の owner policy を直接壊さない。
- 制約: IPC wire protocol v1 の互換性を維持する。authentication や subscription は追加しない。
- 対象外: Windows native CI、external fuzz service、network exposure hardening。

source から use case への変換:

fuzz infrastructure 全体を先に入れるのではなく、既存 codec / server boundary で観測できる入力クラスを test corpus として固定する。必要なら後続で libFuzzer などを検討する。

## 3. 対象範囲

- IPC JSON codec の malformed / oversized input corpus を追加する。
- line framing と partial input の server behavior を test で固定する。
- slow client が heartbeat / disconnect / owner state に与える影響を確認する。
- invalid input 後に valid command が扱えるか、または明確に connection close されるかを決める。
- rejected / accepted / coalesced metrics への影響を既存 schema に沿って確認する。

## 4. 対象外

- authentication token。
- subscribe / event stream。
- external telemetry backend。
- Windows native CI job の追加。
- Switch HID、BTstack、実機経路の変更。

## 5. 関連 spec / docs

- `spec/protocols/daemon-ipc-v1.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/complete/local_052/IPC_ADAPTER_COMMAND_CODEC_BOUNDARY.md`
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`
- `work-units/complete/local_056/ARCHITECTURE_CUTOVER.md`

## 6. 根拠監査

not applicable。

この work unit は local IPC parsing と socket handling を扱う。Switch HID report bytes、BTstack source selection、report period、WinUSB/libusb facts を追加または変更しない。

## 7. 設計メモ

- fuzz という名前でも、初期範囲は再現可能な deterministic corpus にする。
- corpus は protocol v1 の互換性を壊さない error handling を固定する。
- slow client handling は daemon の local IPC 前提で評価し、remote network security として過大に扱わない。
- state mutation の有無は `get_status` または application snapshot で確認する。

## 8. 対象ファイル

- `swbt/ipc/ipc_json.*`
- `swbt/ipc/ipc_server.*`
- `swbt/ipc/ipc_adapter.*`
- `tests/ipc_json_test.c`
- `tests/ipc_server_test.c`
- `tests/debug_ipc_client_test.c`
- `spec/protocols/daemon-ipc-v1.md`
- `work-units/wip/local_062/IPC_FUZZ_AND_SLOW_CLIENT_HARDENING.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | malformed JSON corpus leaves application state unchanged | regression | unit | no |
| refactor-skipped | oversized IPC line is rejected or closed without buffer overrun and with documented behavior | edge | integration | no |
| refactor-skipped | fragmented valid command is handled according to line framing contract | regression | integration | no |
| refactor-skipped | invalid input followed by valid input has explicit recovery or close semantics | edge | integration | no |
| todo | slow client does not block unrelated owner disconnect / heartbeat processing in the tested loopback model | characterization | integration | no |

## 10. 検証

進行中。

TDD status:

- source: `local_052` と `local_055` の parser fuzz 先送り事項。
- use case: malformed JSON を送った client が、既存 owner / state / IPC state metrics を壊さない。
- item: malformed JSON corpus leaves application state unchanged。
- state: refactor-skipped。
- commands:
  - red: `just build-debug` -> pass。
  - red: `$env:CTEST_ARGS='-R ipc_json_test'; just test-debug` -> fail。`ipc_json_test` が malformed JSON corpus を `invalid_json` として扱わない既存挙動で失敗した。
  - green: `just build-debug` -> pass。
  - green: `$env:CTEST_ARGS='-R ipc_json_test'; just test-debug` -> pass。
  - format: `just format` -> pass。
  - after format: `just build-debug` -> pass。
  - after format: `$env:CTEST_ARGS='-R ipc_json_test'; just test-debug` -> pass。
- notes: `tdd-one-cycle` と `refactor-after-green` を使用。追加の構造変更は、今回の item で必要な parser 終端確認と corpus test の範囲を超えるため `refactor-skipped` とした。

TDD status:

- source: `spec/protocols/daemon-ipc-v1.md` の line size contract と、この record の oversized input item。
- use case: active owner connection が oversized line を送った場合、buffer overrun せず、message-too-long として接続を閉じ、owner / state を neutral に戻す。
- item: oversized IPC line is rejected or closed without buffer overrun and with documented behavior。
- state: refactor-skipped。
- commands:
  - red: `just build-debug` -> pass。
  - red: `$env:CTEST_ARGS='-R ipc_server_test'; just test-debug` -> fail。oversized line 後に connection が閉じられず、owner state が残る既存挙動で失敗した。
  - format: `just format` -> pass。
  - green: `just build-debug` -> pass。
  - green: `$env:CTEST_ARGS='-R "ipc_server_test|daemon_ipc_runner_test"'; just test-debug` -> pass。
- notes: message-too-long は typed JSON error response ではなく transport close として固定した。`spec/protocols/daemon-ipc-v1.md` に connection close と owner neutral 化を追記した。runner の `has_connection` も message-too-long 後に clear する。追加の構造変更は不要なため `refactor-skipped` とした。

TDD status:

- source: `spec/protocols/daemon-ipc-v1.md` の JSON Lines framing と、この record の fragmented input item。
- use case: valid command が複数 fragment に分かれて届いても、newline 前には request として処理されず、newline 到着後に 1 request として処理される。
- item: fragmented valid command is handled according to line framing contract。
- state: refactor-skipped。
- commands:
  - red: `just build-debug` -> pass。
  - red: `$env:CTEST_ARGS='-R ipc_server_test --timeout 2'; just test-debug` -> fail。partial line だけで `serve_connection_once` が newline 待ちになり timeout した。
  - format: `just format` -> pass。
  - green: `just build-debug` -> pass。
  - green: `$env:CTEST_ARGS='-R "ipc_server_test|daemon_ipc_runner_test" --timeout 5'; just test-debug` -> pass。
- notes: connection ごとの未完了 line buffer を追加し、complete line がない場合は response なしで `SWBT_IPC_SERVER_OK` を返す。`spec/protocols/daemon-ipc-v1.md` に partial line の保持と complete line 処理を追記した。green 後の追加構造変更は不要なため `refactor-skipped` とした。

TDD status:

- source: malformed / oversized input の recovery or close semantics item。
- use case: `invalid_json` response を返せる complete line は同じ connection で次の valid command を処理し、oversized line は connection close として扱う。
- item: invalid input followed by valid input has explicit recovery or close semantics。
- state: refactor-skipped。
- commands:
  - red: `rg -n "malformed complete JSON|malformed JSON.*connection|invalid_json.*connection" spec\protocols\daemon-ipc-v1.md` -> fail。recovery / close semantics が spec に明記されていなかった。
  - format: `just format` -> pass。
  - green: `just build-debug` -> pass。
  - green: `$env:CTEST_ARGS='-R ipc_server_test --timeout 5'; just test-debug` -> pass。
  - docs check: `rg -n "invalid_json.*connection|message too long.*connection|malformed complete JSON" spec\protocols\daemon-ipc-v1.md` -> pass。
- notes: `ipc_server_test` に malformed line 後の valid `hello` recovery を追加した。oversized line の close semantics は item 2 の test と spec を参照する。追加の構造変更は不要なため `refactor-skipped` とした。

## 11. 実機実行条件

実機不要。local IPC と fake application state で閉じる。

## 12. 先送り事項

none。起票時点の先送り事項は、この record の source として取り込んだ。

## 13. チェックリスト

- [x] source を `local_052` と `local_055` から特定した。
- [x] use case を IPC malformed / slow-client behavior として定義した。
- [x] input corpus を決めた。
- [x] red test を追加した。
- [x] green 実装を行った。
- [x] targeted CTest を実行した。
- [x] protocol docs の更新要否を判定した。
