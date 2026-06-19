# State Mailbox Thread Boundary

## 1. 概要

IPC owner state と BTstack-owning thread の間に latest-state mailbox 境界を置く work unit。

IPC 側は validated full snapshot を保存し、report scheduler 側は report tick ごとに最新 snapshot の copy を読む。複数 update が tick 間に届いた場合は最新 snapshot だけを読むが、coalesced update の数を後続 metrics が参照できる形で残す。

この work unit は mailbox core と IPC neutral path の接続を単体テストで固定する。BTstack run loop wake mechanism、production queue、実機 report loop は扱わない。

## 2. 起点 / ユースケース

source:

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md` は daemon IPC が controller state snapshot を受け取り、daemon 側が最新状態を report loop へ渡す方針を示している。
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md` と `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md` で、IPC session は accepted state、owner disconnect、heartbeat timeout neutral を持つようになった。
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md` で、report scheduler は tick ごとに `swbt_state_t` から input report を build する境界を持つようになった。
- `local_025` の daemon runtime integration では、IPC と BTstack bridge を同じ runtime に接続する必要がある。

use case:

- actor: daemon runtime。
- 入力または状態: IPC thread が accepted full snapshot、owner disconnect、heartbeat timeout neutral を発生させる。BTstack-owning thread は report tick で最新 snapshot を読む。
- 期待する観測結果: mailbox は初期 neutral state と generation zero を返し、store された snapshot を copy として返し、複数 store の coalescing metadata を返す。
- 制約: lock-free 実装、BTstack wake mechanism、OS thread 起動、実機 report loop はこの work unit では扱わない。
- 対象外: metrics schema、structured logging、BTstack timer adapter、daemon lifecycle 統合。

source から use case への変換:

既存 IPC と scheduler の間に共有 pointer を渡すのではなく、latest-state mailbox の copy boundary と generation metadata を実装する。これにより後続 runtime integration は IPC state mutation と BTstack report build を直接結合せずに接続できる。

## 3. 対象範囲

- latest-state mailbox core を追加する。
- mailbox 初期状態を neutral state にする。
- accepted state update を latest snapshot として保存する。
- 複数 update が report tick 間に届いた場合は最新だけを読む挙動を unit test で固定する。
- owner disconnect と heartbeat timeout の neutral state を mailbox に反映する。
- thread boundary の実装方針を unit test で固定する。

## 4. 対象外

- BTstack run loop を起こす production queue。
- BTstack timer adapter。
- daemon lifecycle への thread 起動統合。
- metrics と structured logging。
- Switch pairing、HID advertising、report loop の実機実行。

## 5. 関連 spec / docs

- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `swbt/btstack_bridge/README.md`
- `work-units/complete/local_008/IPC_SESSION_CORE.md`
- `work-units/complete/local_009/IPC_JSON_PROTOCOL_CORE.md`
- `work-units/complete/local_010/IPC_TCP_SERVER_CORE.md`
- `work-units/complete/local_011/IPC_HEARTBEAT_CORE.md`
- `work-units/complete/local_015/PERIODIC_INPUT_REPORT_CORE.md`

## 6. 根拠監査

この work unit は Switch protocol bytes、BTstack source selection、report period、WinUSB/libusb backend facts を追加しない。

| 項目 | 状態 | 扱い |
|---|---|---|
| latest-state mailbox | implementation fact pending | この work unit の C unit test で固定する。 |
| BTstack-owning thread rule | project boundary | `swbt/btstack_bridge/README.md` と daemon architecture の境界として扱う。BTstack wake mechanism は固定しない。 |
| BTstack wake mechanism | out of scope | 実装しない。必要になった場合は `source-audit` を使う。 |
| real thread behavior | not run | OS thread と実 adapter は使わない。 |

## 7. 設計メモ

- IPC parser と session は validated full snapshot を mailbox に保存する。
- report scheduler は mailbox から copy を取得し、shared state pointer を保持しない。
- mailbox は generation を持ち、reader が coalesced update を検出できる形にする。
- lock-free 実装は初期範囲に入れない。
- metrics 用の coalesced count は後続 work unit で読み取れるようにするが、この work unit では logging schema を固定しない。

## 8. 対象ファイル

- `CMakeLists.txt`
- `swbt/core/state_mailbox.h`
- `swbt/core/state_mailbox.c`
- `swbt/ipc/ipc_session.h`
- `swbt/ipc/ipc_session.c`
- `tests/state_mailbox_test.c`
- `work-units/complete/local_024/STATE_MAILBOX_THREAD_BOUNDARY.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | mailbox init returns neutral state and generation zero | new | unit | no |
| refactor-done | store then load returns a copied latest state without exposing mutable storage | new | unit | no |
| refactor-done | multiple stores before load keep latest state and report coalescing metadata | new | unit | no |
| refactor-done | owner disconnect and heartbeat timeout store neutral state through the mailbox boundary | regression | integration | no |

TDD status:

- source: IPC snapshot state と periodic input report scheduler の境界。
- use case: IPC thread が更新した latest snapshot を BTstack-owning thread が copy と generation metadata として読む。
- item: owner disconnect and heartbeat timeout store neutral state through the mailbox boundary。
- state: refactor-done。
- commands:
  - red: `just build-debug` は missing `core/state_mailbox.h` のため compile 失敗。
  - green: `just build-debug` と `just test-debug` は initial neutral / generation zero item で pass。
  - red: `just build-debug` は `swbt_state_mailbox_store` 未宣言で compile 失敗。
  - green: `just build-debug` は store / copied latest state item で pass。
  - red: `just build-debug` 後の `just test-debug` は `state_mailbox_test` 失敗。`just test-debug` だけを先に実行した結果は stale binary のため red 証拠から除外した。
  - green: `just build-debug` と `just test-debug` は coalesced metadata item で pass。
  - red: `just build-debug` は `swbt_ipc_session_bind_mailbox` 未宣言で compile 失敗。
  - green: `just build-debug` と `just test-debug` は IPC neutral mailbox item で pass。
  - refactor: `just format` で整形し、result enum comparison を `expect_eq_int` へ整理した。
  - final: `just format-check` pass。
  - final: `just debug` pass、CTest 14/14。
  - final: `just asan` pass、CTest 14/14。
  - final: `just windows-cross` pass。
  - final: `just verify` pass。
- notes: `tdd-workflow`、`tdd-test-list`、`tdd-one-cycle`、`work-unit-record`、`test-desiderata-review` を読んだ。根拠監査は new protocol / BTstack / backend fact を追加しないため not applicable。

Test desiderata:

- purpose: mailbox core の new behavior と、IPC neutral path の regression を deterministic unit / integration test で固定する。
- key trade-offs: fast / deterministic / behavioral を優先した。OS thread と BTstack wake mechanism は predictive ではないため、この work unit では test に入れていない。
- risks: 実 thread scheduling、BTstack run loop wake、実 adapter での report loop は未検証である。
- action: 未検証領域は `local_023`、`local_025`、実機 work unit に先送りする。

## 10. 検証

実行済み:

- `just format-check`: pass。
- `just debug`: pass、CTest 14/14。
- `just asan`: pass、CTest 14/14。
- `just windows-cross`: pass。
- `just verify`: pass。

red / green の詳細は TDD status に記録した。

## 11. 実機実行条件

通常の unit test と loopback IPC integration test では実機検証は不要である。

BTstack-owning thread と実 adapter を使う daemon run は実機作業として扱う。

実機作業はユーザの明示承認を必要とする。

実機作業は専用 USB Bluetooth ドングルを使う。

実機作業は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` を設定して実行する。

実機結果は `docs/hardware-test-log.md` に OS、ドングル VID/PID、driver、BTstack commit、swbt commit、Switch firmware、report period、結果、cleanup を記録する。

この work unit では実機コマンドを実行していない。理由は、mailbox core と IPC session の software boundary だけを扱い、Bluetooth adapter、Switch pairing、HID advertising、report loop を開始していないためである。

## 12. 先送り事項

- 観測: BTstack wake mechanism と production queue は mailbox core だけでは決まらない。
  先送り理由: この work unit は copy boundary と IPC neutral path だけを固定する。
  次の置き場: `local_023` または `local_025`。
- 観測: coalesced count を metrics と structured logging に出す schema は未定である。
  先送り理由: mailbox は metadata を返すだけにし、logging schema はこの work unit の対象外にする。
  次の置き場: `local_026`。

## 13. チェックリスト

- [x] work unit record を作成した。
- [x] work unit record を新形式へ更新した。
- [x] red を確認した。
- [x] state mailbox core を追加した。
- [x] thread boundary test を追加した。
- [x] `just` 経由の検証を実行した。
- [x] sanitizer または cross build の必要性を判断した。
- [x] 実機状態を記録した。
