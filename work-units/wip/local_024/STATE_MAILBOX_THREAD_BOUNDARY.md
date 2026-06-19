# State Mailbox Thread Boundary

## 1. 概要

IPC owner state と BTstack-owning thread の間に latest-state mailbox 境界を置くための計画 record。

IPC 側は full snapshot を保存し、report scheduler 側は report tick ごとに最新 snapshot を読む。

## 2. 対象範囲

- latest-state mailbox core を追加する。
- mailbox 初期状態を neutral state にする。
- accepted state update を latest snapshot として保存する。
- 複数 update が report tick 間に届いた場合は最新だけを読む挙動を unit test で固定する。
- owner disconnect と heartbeat timeout の neutral state を mailbox に反映する。
- thread boundary の実装方針を characterization test で固定する。

## 3. 対象外

- BTstack run loop を起こす production queue。
- BTstack timer adapter。
- daemon lifecycle への thread 起動統合。
- metrics と structured logging。
- Switch pairing、HID advertising、report loop の実機実行。

## 4. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `swbt/btstack_bridge/README.md`
- `work-units/complete/local_008/IPC_SESSION_CORE.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`
- `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md`
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md`

## 5. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| latest-state mailbox | 未実装 | design policy | `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` | pending |
| BTstack-owning thread rule | 未監査 | local project note | `swbt/btstack_bridge/README.md` | not upstream-audited |
| BTstack wake mechanism | 未確定 | source audit required if implemented | `vendor/btstack` | pending |
| real thread behavior | 未記録 | characterization test または実機観測が必要 | tests and `docs/hardware-test-log.md` | not run |

mailbox の lock 方式は未確定である。

BTstack thread を起こす mechanism はこの work unit では固定しない。

thread boundary の backend fact は根拠監査または characterization test の後に記録する。

## 6. 設計メモ

- IPC parser と session は validated full snapshot を mailbox に保存する。
- report scheduler は mailbox から copy を取得し、shared state pointer を保持しない。
- mailbox は generation を持ち、reader が coalesced update を検出できる形にする。
- lock-free 実装は初期範囲に入れない。
- metrics 用の coalesced count は後続 work unit で読み取れるようにするが、この work unit では logging schema を固定しない。

## 7. 対象ファイル

- `CMakeLists.txt`
- `swbt/core/state_mailbox.h`
- `swbt/core/state_mailbox.c`
- `swbt/ipc/ipc_session.h`
- `swbt/ipc/ipc_session.c`
- `swbt/btstack_bridge/input_report_scheduler.h`
- `tests/state_mailbox_test.c`
- `work-units/wip/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | mailbox init returns neutral state and generation zero | new | unit | no |
| todo | store then load returns a copied latest state without exposing mutable storage | new | unit | no |
| todo | multiple stores before load keep latest state and report coalescing metadata | new | unit | no |
| todo | owner disconnect and heartbeat timeout store neutral state through the mailbox boundary | regression | integration | no |

## 9. 検証

未実行。

この record では計画を作成しただけで、実装、build、CTest、実機検証は実行していない。

実装後は `make debug CTEST_ARGS="-R state_mailbox_test"` を実行する。

IPC integration に影響する場合は `make debug CTEST_ARGS="-R ipc"` を実行する。

thread abstraction または platform code に影響する場合は `make windows-cross` を実行する。

## 10. 実機実行条件

通常の unit test と loopback IPC integration test では実機検証は不要である。

BTstack-owning thread と実 adapter を使う daemon run は実機作業として扱う。

実機作業はユーザの明示承認を必要とする。

実機作業は専用 USB Bluetooth ドングルを使う。

実機作業は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定して実行する。

実機結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、driver、BTstack commit、swbt commit、Switch firmware、report period、結果、cleanup を記録する。

## 11. チェックリスト

- [x] work unit record を作成した。
- [ ] red を確認した。
- [ ] state mailbox core を追加した。
- [ ] thread boundary test を追加した。
- [ ] `make debug` を実行した。
- [ ] sanitizer または cross build の必要性を判断した。
- [ ] 実機状態を記録した。
