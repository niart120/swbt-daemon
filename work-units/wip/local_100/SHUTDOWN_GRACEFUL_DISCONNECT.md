# Shutdown Graceful Disconnect

## 1. 概要

daemon shutdown で trailing neutral report を送った後、可能なら Bluetooth HID の graceful disconnect request を投げ、connection closed event または timeout を確認してから HCI power-off へ進むかを検証する。

この work unit は、daemon IPC protocol に新しい disconnect command を追加しない。対象は production runtime の shutdown cleanup であり、入力安全のための neutral fail-safe と、Bluetooth link を明示的に閉じる cleanup 品質を分けて扱う。

完了後の望ましい状態は、shutdown 中の順序が `neutral report`、`HID disconnect request`、`closed event or bounded timeout`、`HCI power-off` として software test と実機ログで説明できることである。disconnect request が失敗または未確認の場合でも、daemon は停止を継続する。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-28: daemon 終了時に neutral report を送ったうえで disconnect request を投げて切断確認する仕様拡張の是非を検討し、work unit として進める。
- `docs/status.md`: owner disconnect、heartbeat timeout、shutdown の neutral fail-safe は確認済みだが、Bluetooth HID disconnect request は現行の確認済み項目ではない。
- `docs/hardware-test-log.md`: `local_037` owner disconnect neutral rerun は「Bluetooth L2CAP close は手動停止時に記録されており、owner disconnect と同義ではない」と明記している。
- `docs/hardware-test-log.md`: shutdown trailing neutral は HCI power-off 前に観測済みである。
- `spec/references/btstack-daemon-entrypoint.md`: HCI power-off と `hci_close()` は cleanup boundary として監査済みだが、HID graceful disconnect request は同 reference の既存表にまだない。
- `work-units/complete/local_079/DEVICE_LIFECYCLE_API.md`: `swbt_btstack_device_close()` は HID stop / platform stop を閉じる internal cleanup であり、HID Host への disconnect request ではない。
- `work-units/complete/local_080/DEVICE_API_PRODUCTION_PATH.md`: shutdown neutral send は device send path を通るが、disconnect request の経路はまだない。

use case:

- actor: production daemon shutdown path、BTstack HID Device、Switch。
- 入力または状態: Switch と HID control / interrupt L2CAP channel が開いている。owner が non-neutral state を保持したまま process shutdown が要求される。
- 期待する観測結果:
  - shutdown request は既存どおり neutral state を保存し、HCI power-off 前に Switch-facing neutral input report を 1 件以上送る。
  - active HID connection が残っている場合、neutral report の後に HID disconnect request を投げる。
  - `HID_SUBEVENT_CONNECTION_CLOSED` を観測した場合、closed confirmation として記録してから HCI power-off / run loop exit へ進む。
  - closed event が来ない、connection がすでに閉じている、または request が投げられない場合でも、bounded wait または即時 fallback により HCI power-off / `hci_close()` へ進む。
  - 実機では HCI dump / trace で trailing neutral、L2CAP disconnect または closed event、HCI power-off の順序を分けて記録できる。
- 制約:
  - disconnect request は shutdown の必須成功条件にしない。
  - daemon IPC に Bluetooth disconnect command を公開しない。
  - Switch-facing input report bytes、report period、subcommand payload は変更しない。
  - 実機検証は専用 USB Bluetooth dongle、明示承認、artifact 記録を前提にする。

source から use case への判断:

- 現行の neutral fail-safe は入力安全を満たしている。今回追加候補の価値は、HCI power-off に頼る前に HID / L2CAP connection を明示的に閉じることである。
- shutdown cleanup は daemon 停止を妨げてはならないため、disconnect request の確認は bounded cleanup として扱う。
- まず software boundary と source audit を固定し、実機では追加の効果と副作用を観測する。

## 3. 対象範囲

- BTstack `hid_device_disconnect()` と `HID_SUBEVENT_CONNECTION_CLOSED` の source audit を record に残す。
- 現行 shutdown neutral、pending retry、HCI power-off / close の境界を既存 docs から整理する。
- internal device API または production port に HID disconnect request boundary を追加するかを実装で決める。
- shutdown sequence が trailing neutral 後に disconnect request を扱うことを software test で固定する。
- disconnect request 後の connection closed event、timeout、already closed の各 cleanup path を software test で固定する。
- 実機実行条件を定義し、承認が得られた場合だけ HCI dump / trace で順序を確認する。
- 結果に応じて `docs/status.md`、`spec/references/btstack-daemon-entrypoint.md`、または architecture spec の current state を更新する。

## 4. 対象外

- daemon IPC protocol への `disconnect` command 追加。
- client-side `release` の意味変更。
- owner disconnect、heartbeat timeout、explicit release を Bluetooth link disconnect として扱うこと。
- active reconnect、link key DB、pairing-free reconnect の挙動変更。
- Switch-facing input report bytes、subcommand bytes、SPI、rumble packet、report period の変更。
- 複数 controller、Joy-Con、NFC / IR、adapter removal / reinsertion recovery。
- HCI power-off / `hci_close()` の最終 cleanup を廃止すること。
- binary release。

## 5. 関連 spec / docs

- `docs/status.md`
- `docs/hardware-test-log.md`
- `spec/references/btstack-daemon-entrypoint.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`
- `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md`
- `work-units/complete/local_079/DEVICE_LIFECYCLE_API.md`
- `work-units/complete/local_080/DEVICE_API_PRODUCTION_PATH.md`

## 6. 根拠監査

`source-audit` 対象である。BTstack HID Device API、HID connection closed event、shutdown HCI cleanup、実機 HCI dump の解釈に触れるため、source fact、implementation fact、hardware observation、hypothesis を分ける。

### 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| BTstack submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | repository fact | `spec/references/btstack-daemon-entrypoint.md` | recorded |
| current shutdown neutral | shutdown request calls `swbt_daemon_process_send_neutral_now()` before finish shutdown | implementation fact | `swbt/daemon/shutdown_sequence.c:59-71` | existing behavior |
| pending shutdown neutral | pending neutral completes on can-send-now, failed retry still finishes shutdown | implementation fact | `swbt/daemon/btstack_hid_session.c:78-87`, `work-units/complete/local_058/SHUTDOWN_NEUTRAL_RETRY_FAILURE.md` | existing behavior |
| current HCI cleanup | production finish shutdown powers off then triggers run loop exit; final cleanup still calls power off and destroys host | implementation fact | `swbt/daemon/production_runner.c:65-71`, `swbt/daemon/production_runner.c:154-158` | existing behavior |
| HCI power-off | transport close is called during HCI power-off | source fact | `vendor/btstack/src/hci.c:5867-5884`, `spec/references/btstack-daemon-entrypoint.md` | stable at pinned commit |
| HCI close | `hci_close()` closes link key DB, discards connections through BTstack cleanup, and can call HCI power-off | source fact | `vendor/btstack/src/hci.c:5705-5720`, `vendor/btstack/src/hci.c:6331-6339`, `spec/references/btstack-daemon-entrypoint.md` | stable at pinned commit |
| BTstack HID disconnect API | `hid_device_disconnect(hid_cid)` disconnects interrupt and control L2CAP CIDs if present | source fact | `vendor/btstack/src/classic/hid_device.h:126-130`, `vendor/btstack/src/classic/hid_device.c:995-1006` | implementation candidate |
| BTstack HID channel-specific disconnect APIs | interrupt-only and control-only disconnect helpers exist for PTS testing | source fact | `vendor/btstack/src/classic/hid_device.h:171-173`, `vendor/btstack/src/classic/hid_device.c:971-991` | not selected initially |
| BTstack closed event emission | `HID_SUBEVENT_CONNECTION_CLOSED` is emitted after both interrupt and control CIDs are cleared | source fact | `vendor/btstack/src/classic/hid_device.c:737-749` | stable at pinned commit |
| swbt closed event decode | swbt decodes `HID_SUBEVENT_CONNECTION_CLOSED` and extracts `hid_cid` | implementation fact | `swbt/btstack_bridge/hid_event.c:71-77` | existing behavior |
| owner disconnect neutral hardware observation | owner socket close produced neutral reports, but Bluetooth L2CAP close was not equivalent to owner disconnect | hardware observation | `docs/hardware-test-log.md:722-742` | existing observation |
| shutdown trailing neutral hardware observation | shutdown sent trailing neutral before HCI power-off on CSR8510 A10 / WinUSB / Switch2 `22.1.0` | hardware observation | `docs/hardware-test-log.md:788-808`, `docs/hardware-test-log.md:855-876` | existing observation |
| graceful disconnect benefit | explicit HID disconnect before HCI power-off may produce cleaner remote-side close behavior | unverified hypothesis | user discussion, 2026-06-28 | must be tested |

### 未解決事項

- `hid_device_disconnect()` は `void` で、request 失敗を戻り値で判定できない。swbt 側では active HID cid の有無、closed event、timeout を別々に扱う必要がある。
- `HID_SUBEVENT_CONNECTION_CLOSED` が shutdown 中に常に来るとは扱わない。Switch 側状態、BTstack run loop timing、HCI power-off timing に依存する可能性がある。
- graceful disconnect が次回 active reconnect、link key DB、Switch UI の controller state に良い影響を持つかは未実機検証である。

## 7. 設計メモ

採用候補の shutdown cleanup order:

```text
shutdown request
→ neutral state save
→ trailing neutral report send or pending retry
→ HID disconnect request if active HID cid exists
→ HID_SUBEVENT_CONNECTION_CLOSED or bounded timeout
→ HCI power-off
→ run loop exit
→ runtime stop / hci_close / run loop deinit
```

`disconnect request` は shutdown の必須成功条件にしない。閉じたことを確認できれば trace / metrics / hardware log で扱い、確認できない場合は bounded timeout 後に既存 cleanup へ進む。

初期実装では BTstack の channel-specific PTS helper ではなく、`hid_device_disconnect(hid_cid)` 相当の両 channel disconnect を優先する。理由は、Switch HID connection は control / interrupt の両 channel が揃って connection として扱われ、BTstack closed event も両 CID が cleared になってから出るためである。

device API の既存 `close` は HID registration と platform cleanup の終了処理であり、remote HID Host への disconnect request ではない。命名を混同しないため、追加する場合は `disconnect` または `request_disconnect` のような link-level API として分ける。

timeout 値はこの record 起票時点では未確定である。最初の software boundary では fake clock / synthetic event で「待ちすぎない」ことを固定し、実機で必要な余裕を観測してから stable value へ格上げする。

## 8. 対象ファイル

- `swbt/btstack_bridge/device.*`
- `swbt/btstack_bridge/production_btstack_impl.c`
- `swbt/btstack_bridge/production_ports.*`
- `swbt/daemon/btstack_hid_session.*`
- `swbt/daemon/shutdown_sequence.*`
- `swbt/daemon/production_runner.*`
- `tests/btstack_device_test.c`
- `tests/daemon_production_runner_test.c`
- `tests/btstack_hid_event_test.c`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `spec/references/btstack-daemon-entrypoint.md`
- `work-units/wip/local_100/SHUTDOWN_GRACEFUL_DISCONNECT.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | source audit records BTstack HID disconnect API, closed event emission, and current shutdown neutral / HCI cleanup boundary | characterization | docs | no |
| refactor-skipped | device API can request HID disconnect for an active HID cid without treating device close as disconnect | new | unit | no |
| refactor-skipped | shutdown with an active connection sends trailing neutral before HID disconnect request and before HCI power-off | regression | integration | no |
| refactor-skipped | pending neutral send completes or fails before disconnect request is attempted | edge | integration | no |
| refactor-skipped | HID connection closed event after shutdown disconnect request finishes cleanup and triggers power-off / run loop exit once | new | integration | no |
| refactor-skipped | disconnect request unavailable, already-closed connection, or closed-event timeout still proceeds to HCI power-off / run loop exit | edge | integration | no |
| refactor-skipped | graceful disconnect trace or status distinguishes requested, closed, timed out, and unavailable states | new | unit/integration | no |
| green | hardware run observes trailing neutral, HID/L2CAP disconnect or closed event, and HCI power-off ordering on CSR8510 A10 / WinUSB / Switch2 | characterization | hardware | yes |
| todo | active reconnect hardware run observes shutdown graceful disconnect without Change Grip/Order or incoming pairing path | characterization | hardware | yes |
| deferred | hardware run checks whether graceful disconnect changes next active reconnect or Switch-side controller state | characterization | hardware | yes |

TDD status:

- source: user request, 2026-06-28。
- use case: shutdown 中に trailing neutral の後で HID disconnect request を扱い、closed event が来る場合と来ない場合をどちらも bounded cleanup として記録する。
- item: source audit records BTstack HID disconnect API, closed event emission, and current shutdown neutral / HCI cleanup boundary。
- state: refactor-skipped。
- commands:
  - `rg -n "shutdown neutral|trailing neutral|disconnect|HID_SUBEVENT_CONNECTION_CLOSED|hid_device_disconnect|hci_close|power-off|power off|active reconnect|link key" spec docs work-units swbt tests -g "!vendor/**"`。
  - `rg -n "hid_device_disconnect|HID_SUBEVENT_CONNECTION_CLOSED|l2cap_disconnect|gap_disconnect|hci_close|hci_power_control" vendor\btstack\src\classic\hid_device.c vendor\btstack\src\classic\hid_device.h vendor\btstack\src\hci.c vendor\btstack\src\btstack_defines.h vendor\btstack\src\btstack_event.h`。
  - `rg -n -C 12 "HID_SUBEVENT_CONNECTION_CLOSED|hid_device_disconnect\(|hid_device_disconnect_interrupt_channel|hid_device_disconnect_control_channel" vendor\btstack\src\classic\hid_device.c vendor\btstack\src\classic\hid_device.h`。
- notes: initial source audit and work unit launch only。production code はまだ変更していない。実機コマンドは実行していない。

次に扱う item:

- item: device API can request HID disconnect for an active HID cid without treating device close as disconnect。
- state: refactor-skipped。
- rationale: BTstack request boundary を小さく固定してから shutdown ordering へ接続する方が、neutral fail-safe と cleanup sequencing を分けて検証できる。
- red attempt:
  - `just build-debug`
  - result: Dev Container 起動前に失敗。`docker ps` が `npipe:////./pipe/dockerDesktopLinuxEngine` へ接続できず、Docker Desktop Linux engine が起動していないため。これは test failure ではない。
- implementation:
  - `swbt_btstack_device_port_t` に `disconnect(context, hid_cid)` を追加した。
  - `swbt_btstack_device_disconnect()` を追加し、device close / HID stop / platform stop とは別に port へ HID cid を転送する境界を作った。
  - production port では `hid_device_disconnect(hid_cid)` を呼ぶ。
  - fake port と production port validation を追随した。
- verification:
  - `just build-debug`: pass。
  - `just test-debug`: pass。59/59 passed。`btstack_device_test` と `btstack_production_ports_test` を含む。
  - `just format-check`: pass。
  - `git diff --check`: pass。
- refactor:
  - decision: refactor-skipped。
  - reason: 今の item で必要な責務分離は `disconnect` port と public wrapper の追加で足りている。shutdown ordering / timeout state は次の TDD item で扱うため、ここで抽象化を増やさない。

次に扱う item:

- item: shutdown with an active connection sends trailing neutral before HID disconnect request and before HCI power-off。
- state: refactor-skipped。
- expectation: shutdown listener 経路で HID connection が開いている場合、`STEP_TIMER_SEND_NEUTRAL_NOW` の後、`STEP_POWER_OFF` の前に `STEP_DEVICE_DISCONNECT` が 1 回記録され、`hid_cid` は active connection の `0x0042` になる。
- red:
  - `just build-debug`: pass。
  - `just test-debug`: expected failure。`daemon_production_runner_test` のみ failed。failure reason は `disconnect calls: expected 1, got 0` と expected step `21` が power-off step `8` の前に存在しないこと。
- implementation:
  - shutdown finish で active `report_timer.running` を確認し、`report_timer.hid_cid` を使って `swbt_btstack_device_disconnect()` を呼ぶ。
  - disconnect request の戻り値は shutdown の成功条件にしない。
- green:
  - `just build-debug`: pass。
  - `just test-debug`: pass。59/59 passed。red だった `daemon_production_runner_test` を含む。
  - `just format-check`: pass。
  - `git diff --check`: pass。
- refactor:
  - decision: refactor-skipped。
  - reason: 現時点の設計は `neutral completed -> disconnect request -> power-off` の順序固定に限定する。closed event wait / timeout state をまだ持たない段階で shutdown helper を増やしても、次 item の形を先取りするだけになる。
- notes: pending neutral success / failure の既存 shutdown integration tests でも、`STEP_TIMER_CAN_SEND_NOW` の後、`STEP_DEVICE_DISCONNECT` が power-off 前に来ることを確認している。

次に扱う item:

- item: pending neutral send completes or fails before disconnect request is attempted。
- state: refactor-skipped。
- evidence:
  - `pending_stop_request_finishes_after_can_send_event` は `STEP_TIMER_SEND_NEUTRAL_NOW`、`STEP_TIMER_CAN_SEND_NOW`、`STEP_DEVICE_DISCONNECT` の順序を確認する。
  - `pending_stop_request_finishes_after_failed_can_send_event` は failed can-send 後も `STEP_TIMER_CAN_SEND_NOW` の後に `STEP_DEVICE_DISCONNECT` が来ることを確認する。
- commands:
  - `just test-debug`: pass。59/59 passed。
- refactor:
  - decision: refactor-skipped。
  - reason: 既存 integration test がこの item の観測をすでに固定している。今回の fallback 実装でも同じ ordering test が green のため、追加の構造変更は不要。

hardware item:

- item: hardware run observes trailing neutral, HID/L2CAP disconnect or closed event, and HCI power-off ordering on CSR8510 A10 / WinUSB / Switch2。
- state: green。
- approval: user approved, 2026-06-28。承認範囲は CSR8510 A10 / WinUSB adapter open、HID advertising、Switch pairing or reconnect、`8000 us` report loop、Button A held owner 中の daemon shutdown graceful disconnect、HCI dump / diagnostic trace / crash dump path 保存、cleanup 確認。
- command:
  - `just windows-cross`: pass。current diff 入り `build/windows-mingw-debug/swbt-daemon.exe` を生成。
  - `powershell -NoProfile -ExecutionPolicy Bypass -File .\tmp\hardware\local_100\run-shutdown-graceful-disconnect.ps1`: pass。
- artifact: `tmp/hardware/local_100/20260628-184530-shutdown-graceful-disconnect`。
- result:
  - run log: L2CAP open count `2`、seq1 / seq2 debug client exit code `0`、seq3 held owner `state_accepted`、daemon exit code `0`。
  - trace order: `production: shutdown neutral send ok`、`production: shutdown hid disconnect request`、`btstack: hid disconnect request`、`btstack: hid disconnect requested`、`btstack: hci power off`。
  - HCI dump: `pairing complete, status 00` `1` 件、`L2CAP_EVENT_CHANNEL_OPENED status 0x0` `2` 件、standard full input report `a1 30` `306` 件、neutral `000000` `208` 件、Button A `080000` `98` 件、`Disconnect from HID Host` `1` 件、`L2CAP_EVENT_CHANNEL_CLOSED` `2` 件、`invalid size` `0` 件、`non-registered handle` `0` 件。
  - HCI dump tail order: line `914` Button A `080000`、line `916` trailing neutral `000000`、line `917` `Disconnect from HID Host`、line `918` `hci_power_control: 0`、line `922` / `927` `L2CAP_EVENT_CHANNEL_CLOSED`。
- interpretation:
  - trailing neutral と HID disconnect request は HCI power-off 前に観測できた。
  - `L2CAP_EVENT_CHANNEL_CLOSED` は最初の `hci_power_control: 0` の後に観測されたため、`closed event を確認してから HCI power-off` はこの run では未達。
  - `HID connection closed event after shutdown disconnect request finishes cleanup and triggers power-off / run loop exit once` は後続 TDD で software 固定済みである。timeout fallback は未対応である。
- hardware log: `docs/hardware-test-log.md` に `2026-06-28: local_100 shutdown graceful disconnect first run on Switch2` として記録した。

active reconnect hardware item:

- item: active reconnect hardware run observes shutdown graceful disconnect without Change Grip/Order or incoming pairing path。
- state: todo。`local_101` で active reconnect 前提は復旧済みだが、この work unit の shutdown graceful disconnect run はまだ再実行していない。
- approval: user approved, 2026-06-28。承認範囲は CSR8510 A10 / WinUSB、保存済み `swbt-daemon.toml` / `swbt-link-key.tlv` の再利用、Switch HOME から sleep / resume 済み状態での active reconnect request、Change Grip/Order 操作なし、HCI dump / diagnostic trace / crash dump path 保存、cleanup 確認。HID open 後に held Button A shutdown graceful disconnect を行う計画だった。
- command:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File .\tmp\hardware\local_100\run-shutdown-graceful-disconnect-active-reconnect.ps1`: failed before HID open。
- artifacts:
  - `tmp/hardware/local_100/20260628-194019-shutdown-active-reconnect-graceful-disconnect`
  - `tmp/hardware/local_100/20260628-194325-shutdown-active-reconnect-graceful-disconnect`
- result:
  - 1 回目は `production: active reconnect request ok` を記録したが、120 秒内に `production: hid connection opened` は出なかった。HCI dump は `Create_connection` を記録し、`Connection_complete`、`Connection_incoming`、`pairing started`、`pairing complete, status 00`、`L2CAP_EVENT_CHANNEL_OPENED`、`responding to link key request` は出なかった。script 改善前の failure cleanup は emergency `Stop-Process` で止めた。
  - 2 回目は failure cleanup を通常 `CTRL_BREAK_EVENT` 優先に直した後の再試行である。`active reconnect request ok`、`Connection_complete(status=0)`、`responding to link key request`、`have link key db: 1` までは進んだが、control PSM `0x11` の `L2CAP_EVENT_CHANNEL_OPENED status 0x66` で止まり、`production: hid connection opened` は出なかった。`Connection_incoming`、`pairing started`、`pairing complete, status 00` は `0` 件だった。
- interpretation:
  - Change Grip/Order / incoming pairing の混入は除外できた。
  - shutdown graceful disconnect の観測対象である active HID connection へ到達していないため、この item は未完了のまま残す。
  - 2 回目の cleanup は daemon exit code `0`、crash dump なし。trace は shutdown neutral send failed、HCI power-off、run loop returned、host stop、device close、HCI close、link key DB close、run loop deinit、HCI dump close、host stop done まで到達した。
  - `L2CAP_EVENT_CHANNEL_OPENED status 0x66` の意味と再現条件は未解釈であり、この work unit の shutdown cleanup 実装判断とは分ける。
- follow-up result:
  - `work-units/wip/local_101/ACTIVE_RECONNECT_HID_OPEN_RECOVERY.md` で controlled re-pair により link key DB を更新した。
  - refreshed source artifact は `tmp/hardware/local_101/20260628-204328-controlled-repair-refresh-link-key-db`。
  - refreshed TLV での active reconnect retest は `tmp/hardware/local_101/20260628-204952-active-reconnect-hid-open-recovery` で成功した。`Connection_incoming=0`、`pairing complete, status 00=0`、`responding to link key request>=1`、`have link key db: 1>=1`、`security level 2>=1`、`L2CAP_EVENT_CHANNEL_OPENED status 0x0=2`、Button A smoke を確認した。
  - local_100 の再実行では古い `local_073` TLV ではなく refreshed source artifact を使う。
- hardware log: `docs/hardware-test-log.md` に `2026-06-28: local_100 active reconnect shutdown graceful disconnect attempts without Grip Order` として記録した。

次に扱う item:

- item: HID connection closed event after shutdown disconnect request finishes cleanup and triggers power-off / run loop exit once。
- state: refactor-skipped。
- expectation: shutdown disconnect request 後、`HID_SUBEVENT_CONNECTION_CLOSED` を受けるまでは HCI power-off と run loop exit を実行しない。closed event 後に power-off / run loop exit を 1 回だけ実行する。
- red:
  - `just build-debug`: pass。
  - `just test-debug`: expected failure。`daemon_production_runner_test` で `STEP_POWER_OFF` / `STEP_RUN_LOOP_TRIGGER_EXIT` が `STEP_HID_CONNECTION_CLOSED` より前に発生し、`waits for hid close: expected 0, got 1` で失敗した。
- implementation:
  - `swbt_daemon_shutdown_sequence_t` に `disconnect_pending` を追加し、shutdown prepare / init で初期化する。
  - shutdown finish は active report timer が残っている場合、HID disconnect request を一度だけ投げ、HCI power-off / run loop exit へ進まず closed event を待つ。
  - HID session の connection closed handler は `shutdown_disconnect_pending` を解消し、既存の finish callback へ戻す。timer stop 後なので finish 側は power-off / run loop exit へ進む。
  - production runner integration fake に synthetic `HID_SUBEVENT_CONNECTION_CLOSED` を追加し、closed event 前に power-off / run loop exit が出ないことを assertion で固定した。
- green:
  - `just build-debug`: pass。
  - `just test-debug`: pass。59/59 passed。
  - `just format-check`: pass。
  - `git diff --check`: pass。`docs/hardware-test-log.md` の LF/CRLF 警告のみ。
- refactor:
  - decision: refactor-skipped。
  - reason: 今の item で必要な状態は `disconnect_pending` と既存 HID closed callback の接続だけで足りている。timeout fallback、already-closed、unavailable state は次 item の振る舞いであり、ここで抽象化を増やさない。

次に扱う item:

- item: disconnect request unavailable, already-closed connection, or closed-event timeout still proceeds to HCI power-off / run loop exit。
- state: refactor-skipped。
- expectation:
  - disconnect request 後に closed event が来ない場合、one-shot timeout が発火し、report timer stop、HCI power-off、run loop exit へ進む。
  - disconnect request が失敗した場合、timeout を待たず report timer stop、HCI power-off、run loop exit へ進む。
  - active HID connection がない場合、disconnect request を投げず HCI power-off、run loop exit へ進む。
- red:
  - `just build-debug`: pass。
  - `just test-debug`: expected failure。`daemon_production_runner_test` で timeout timer が arm されず、`timer add calls: expected 1, got 0`、`timeout is bounded` failure、`trigger exit calls: expected 1, got 0` が出た。disconnect failure case も `trigger exit calls: expected 1, got 0` で失敗した。
- implementation:
  - production run loop port に one-shot timer boundary を追加した。
  - disconnect request 成功時だけ shutdown disconnect timeout timer を arm する。
  - closed event path では timer を cancel してから HCI power-off / run loop exit へ進む。
  - closed event が来ない場合は timeout callback が `shutdown.disconnect_pending` を解消し、active report timer を止めて HCI power-off / run loop exit へ進む。
  - disconnect request が失敗した場合は timeout を待たず active report timer を止めて HCI power-off / run loop exit へ進む。
  - active HID connection がない場合は disconnect request と timeout timer を使わず HCI power-off / run loop exit へ進む。
- green:
  - `just build-debug`: pass。
  - `just test-debug`: pass。59/59 passed。
  - `just format-check`: pass。
  - `git diff --check`: pass。`docs/hardware-test-log.md` の LF/CRLF 警告のみ。
- refactor:
  - decision: refactor-skipped。
  - reason: timeout timer boundary、request failure fallback、already-closed fallback は production runner helper に閉じている。trace/status の観測を次 item で扱うため、ここで追加の状態型や public API を増やさない。

次に扱う item:

- item: graceful disconnect trace or status distinguishes requested, closed, timed out, and unavailable states。
- state: refactor-skipped。
- expectation: production trace file で successful request、closed、timeout、unavailable を別々の marker として読める。
- red:
  - `just build-debug`: pass。
  - `just test-debug`: expected failure。`daemon_production_runner_test` で `requested trace: missing production: shutdown hid disconnect requested` が出た。closed / timeout / unavailable marker は既存 trace で観測できている。
- implementation:
  - successful disconnect request 後に `production: shutdown hid disconnect requested` を trace へ出す。
  - `daemon_production_runner_test` で trace file を使い、requested / closed / timeout / unavailable marker を別々に確認する。
- green:
  - `just build-debug`: pass。
  - `just test-debug`: pass。59/59 passed。
- refactor:
  - decision: refactor-skipped。
  - reason: trace marker 追加と trace file assertion だけで item を満たす。status API や metrics に新しい state を増やすと実機観測前の surface 拡張になるため、この work unit では trace に限定する。

## 10. 検証

起票時確認:

- `git status --short`
  - result: clean。
- `git branch --show-current`
  - result: `main`。編集前に `work/local-100-shutdown-disconnect` を作成した。
- `Get-ChildItem work-units\wip,work-units\complete -Directory`
  - result: `work-units/complete/local_099` まで存在し、`work-units/wip` は空。新規番号は `local_100` とした。
- `rg` による docs / source 調査
  - result: 現行 shutdown neutral、owner disconnect neutral、HCI power-off / close、BTstack HID disconnect API、closed event decode を確認した。

追加確認:

- `git diff --check`
  - result: pass。
- `rg -n "swbt_btstack_device_port_t" swbt tests apps api`
  - result: 既存の `swbt_btstack_device_port_t` 初期化箇所は production port と test fake port であり、追加した `.disconnect` を反映済み。
- `just build-debug`
  - result: pass。
- `just test-debug`
  - result: pass。59/59 passed。
- `just format-check`
  - result: pass。
- `just test-debug` red observation during shutdown ordering TDD
  - result: expected failure。`daemon_production_runner_test` failed because disconnect was not called before power-off。実装後は pass。
- `docker ps`
  - result: after Docker Desktop restart, pass。Dev Container `53ae85bce273` was running.
- `just windows-cross`
  - result: pass。実機 run 前に current diff 入り Windows build を生成した。
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\tmp\hardware\local_100\run-shutdown-graceful-disconnect.ps1`
  - result: pass。CSR8510 A10 / WinUSB / Switch2 で trailing neutral、HID disconnect request、L2CAP channel closed、HCI power-off を観測した。ただし L2CAP closed は最初の HCI power-off request 後に出た。

実機未完了:

- active reconnect 前提の shutdown graceful disconnect 再確認
  - reason: closed event wait と timeout fallback は software test で固定した。`20260628-193109-shutdown-graceful-disconnect` では trace 上 `shutdown hid disconnect closed` が `hci power off` より前に出たが、HCI dump に `Connection_incoming=3` と `pairing complete, status 00=1` が残り、operator observation として connection 確立後に Change Grip/Order 画面へ遷移していた可能性がある。`local_101` で refreshed TLV を作成し、active reconnect は Change Grip/Order / incoming pairing なしで HID open と Button A smoke まで復旧した。この work unit では、その refreshed artifact を使った shutdown graceful disconnect run が未実行である。

## 11. 実機実行条件

実機が必要である。ただし起票時点では実行しない。

実行前条件:

- `hardware-harness` を読む。
- 人間の明示承認を得る。承認範囲には Bluetooth adapter open、Switch pairing or active reconnect、HID advertising、periodic input report loop、shutdown graceful disconnect、HCI dump / diagnostic trace 保存、cleanup 確認を含める。
- 専用 USB Bluetooth dongle を使い、内蔵 Bluetooth と常用 Bluetooth device は対象外にする。
- Windows native では WinUSB driver assignment と `--adapter-location winusb:<location-path>` を記録する。
- daemon 起動には `--backend production` または既定 production、`--adapter-location`、必要に応じて `--config` / `--link-key-db`、`--trace-path`、`--hci-dump-path`、`--crash-dump-path` を明示する。
- 実機ログには OS、dongle VID/PID、driver、backend、BTstack commit、swbt commit、Switch firmware、report period、artifact root を記録する。
- raw Switch address と raw link key value は docs / PR body へ転記しない。

想定 hardware run:

1. `tmp/hardware/local_100/<timestamp>-shutdown-graceful-disconnect` を artifact root にする。
2. production daemon を起動し、既存 pairing または active reconnect で HID control / interrupt channel open まで進める。
3. IPC client で Button A などの non-neutral state を保持する。
4. shutdown request を出し、trace で `shutdown neutral send ok`、disconnect request、closed event or timeout、HCI power-off、run loop exit を確認する。
5. HCI dump で末尾の `a1 30` neutral report、L2CAP disconnect / channel closed event、HCI power-off の順序を確認する。
6. daemon exit code、crash dump 有無、後続 reconnect への影響を記録する。

改訂後の active reconnect hardware run:

1. `tmp/hardware/local_101/20260628-204328-controlled-repair-refresh-link-key-db` の `swbt-daemon.toml` と `swbt-link-key.tlv` を新しい local_100 artifact へコピーする。
2. Switch は HOME へ戻し、sleep / resume 後に待機する。Change Grip/Order 画面は開かない。
3. daemon は `--backend production --adapter-location winusb:<location-path> --config <artifact>/swbt-daemon.toml --link-key-db <artifact>/swbt-link-key.tlv --trace-path ... --hci-dump-path ... --crash-dump-path ...` で起動する。
4. `production: active reconnect request ok` と `production: hid connection opened` を待つ。
5. shutdown 前に `Connection_incoming=0`、`pairing complete, status 00=0`、`responding to link key request>=1`、`have link key db: 1>=1`、`security level 2>=1` を確認できる run だけを active reconnect observation として扱う。
6. IPC owner を保持したまま Button A state を送信し、TCP connection を閉じずに `CTRL_BREAK_EVENT` で daemon shutdown を要求する。
7. trace で `shutdown neutral send ok`、`shutdown hid disconnect requested`、`shutdown hid disconnect closed` または `shutdown hid disconnect timeout`、`hci power off`、`run loop returned` を確認する。
8. HCI dump と summary で L2CAP closed / HCI power-off order、crash dump absence、daemon exit code `0` を記録する。

## 12. 先送り事項

- 観測: graceful disconnect が次回 active reconnect または Switch UI の controller state に与える影響は未検証である。
  先送り理由: `local_101` で active reconnect は復旧したが、local_100 の shutdown graceful disconnect を refreshed TLV でまだ再実行していない。graceful disconnect の効果は、その run と次回 reconnect の観測を分けて評価する。
  次の置き場: この work unit の active reconnect hardware item。

- 観測: disconnect request timeout の具体値は未確定である。
  先送り理由: source fact だけでは適切な待機時間を決められない。software では bounded であることを先に固定し、実機観測で値を調整する。
  次の置き場: この work unit の設計メモ、必要なら `spec/references/btstack-daemon-entrypoint.md` の追加監査。

## 13. チェックリスト

- [x] source を user request、既存 docs、BTstack source から特定した。
- [x] use case を shutdown graceful disconnect として定義した。
- [x] daemon IPC protocol 変更を対象外にした。
- [x] 初期 source audit を記録した。
- [x] TDD Test List を作成した。
- [x] device API の disconnect request boundary を software test で固定する。
- [x] shutdown ordering を software test で固定する。
- [x] timeout / already closed / unavailable の cleanup path を software test で固定する。
- [x] 必要な実装を追加する。
- [x] targeted CTest を実行し、結果を記録する。
- [x] 実機実行条件を満たす場合だけ hardware run を行い、結果を記録する。
- [x] active reconnect 専用手順で Change Grip/Order / incoming pairing を除外した hardware run を行い、HID open 未達を `local_101` へ切り出す。
- [ ] `local_101` の refreshed artifact で active reconnect 前提の shutdown graceful disconnect run を再実行する。
- [x] 関連 docs / spec の current state を必要に応じて更新する。
