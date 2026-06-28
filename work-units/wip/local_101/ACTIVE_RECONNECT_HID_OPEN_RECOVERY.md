# Active Reconnect HID Open Recovery

## 1. 概要

保存済み `swbt-daemon.toml` と `swbt-link-key.tlv` を使う active reconnect が、Change Grip/Order 操作なしで HID control / interrupt channel open まで戻ることを再確認し、必要なら修正する。

この work unit は `local_100` の shutdown graceful disconnect から切り出す。`local_100` の実機再確認では active reconnect が前提条件だったが、2026-06-28 の active reconnect 専用 run は HID open に到達しなかった。shutdown cleanup の実装判断と active reconnect の接続確立 failure を混ぜないため、active reconnect の復旧と実機再確認をこの work unit へ移す。

完了後の望ましい状態は、Switch HOME から sleep / resume 済み、Change Grip/Order なし、保存済み config / link key DB ありの条件で、daemon 起点の outgoing reconnect が `production: hid connection opened` と Button A smoke まで到達し、HCI dump 上で `Connection_incoming=0`、`pairing complete, status 00=0` と説明できることである。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-28: active reconnect が動かなくなっている可能性があり、`local_100` とは別 work unit で先に解消すべきかを確認した。
- `local_100` hardware observation, `tmp/hardware/local_100/20260628-194019-shutdown-active-reconnect-graceful-disconnect`: `production: active reconnect request ok` は出たが、120 秒内に `production: hid connection opened` が出なかった。HCI dump は `Create_connection` の後、`Connection_complete` へ進まなかった。
- `local_100` hardware observation, `tmp/hardware/local_100/20260628-194325-shutdown-active-reconnect-graceful-disconnect`: `Connection_complete(status=0)`、`responding to link key request`、`have link key db: 1` までは出たが、control PSM `0x11` の `L2CAP_EVENT_CHANNEL_OPENED status 0x66` で止まり、HID open に到達しなかった。
- `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`: 2026-06-26 の `20260626-202005-link-key-db-sleep-resume-existing` と `20260626-205341-link-key-db-sleep-resume-existing` では、保存済み config / link key DB から incoming pairing なしに HID open と Button A smoke へ到達していた。
- `docs/hardware-test-log.md`: `local_100` の Change Grip/Order 混入疑い run と active reconnect 専用 failed run を別 entry として記録した。
- user request, 2026-06-28: post-graceful-disconnect reconnect evaluation の reason `0x13` close は `local_101` で原因追跡する。`local_100` では local disconnect request 残りの否定までを扱い、HID open 後に Switch が初期化列へ進まず切る条件は active reconnect 側で扱う。
- `local_100` hardware observation, `tmp/hardware/local_100/20260628-210911-shutdown-active-reconnect-graceful-disconnect`: refreshed TLV から HID open したが、Switch output report が始まる前に HCI reason `0x13` で remote-side termination された。

use case:

- actor: production daemon、BTstack HID Device、Switch2、CSR8510 A10 / WinUSB adapter。
- 入力または状態:
  - `local_073` の保存済み `swbt-daemon.toml` と `swbt-link-key.tlv` を再利用する。
  - Switch は HOME から sleep / resume 済みで、Change Grip/Order 画面を開かない。
  - daemon は `--backend production --adapter-location winusb:<location-path> --config <artifact>/swbt-daemon.toml --link-key-db <artifact>/swbt-link-key.tlv` で起動する。
- 期待する観測結果:
  - trace に `btstack: link key db open ok` と `production: active reconnect request ok` が出る。
  - HCI dump に outgoing `Connection_complete(status=0)`、`responding to link key request`、`have link key db: 1` が出る。
  - HCI dump に `Connection_incoming`、`pairing started`、`pairing complete, status 00` が出ない。
  - PSM `0x11` / `0x13` の `L2CAP_EVENT_CHANNEL_OPENED status 0x0` が出て、trace に `production: hid connection opened` が出る。
  - Button A smoke が exit code `0` で通る。
  - HID open 後に remote close された場合は、Switch output report / subcommand が始まったか、local disconnect request の痕跡があるか、HCI reason が何かを分けて記録する。
- 制約:
  - Switch-side Change Grip/Order 操作で成功させない。
  - shutdown graceful disconnect の実装変更はこの work unit の対象にしない。
  - raw Switch address と raw link key value を docs / PR body へ転記しない。

source から use case への判断:

- `local_100` の shutdown disconnect 実装は、active HID connection が存在する場合の cleanup ordering を扱う。今回の failure は active HID connection の確立前に起きている。
- したがって `local_100` のリファクタリング前に、active reconnect を別 work unit で復旧または失敗境界として確定する。

## 3. 対象範囲

- `local_073` の成功条件と `local_100` の失敗条件を artifact から比較する。
- current branch の active reconnect startup path、link key DB wiring、adapter selector、HID connection open callback path を確認する。
- `L2CAP_EVENT_CHANNEL_OPENED status 0x66` の意味を BTstack source または HCI / L2CAP status definition から監査する。
- software test で固定できる boundary があれば先に TDD で固定する。
- 必要な最小修正を行う。
- CSR8510 A10 / WinUSB / Switch2 で active reconnect hardware run を再実行し、incoming pairing なしの HID open と Button A smoke を確認する。
- post-graceful-disconnect source artifact からの active reconnect で HID open 後に reason `0x13` close された条件を、Switch output report 開始前か、local disconnect request 由来か、link key / authentication 由来かに分けて追跡する。
- `local_100` の shutdown graceful disconnect final hardware run が再開可能かを判断できる状態にする。

## 4. 対象外

- shutdown graceful disconnect の sequencing、timeout、trace marker の追加修正。
- daemon IPC protocol への new command 追加。
- Switch-side Change Grip/Order 操作による incoming pairing path の成功確認。
- pair-removal、Switch 側 controller 登録情報の手動削除、初回 pairing 手順の再設計。
- 複数 controller、Joy-Con、NFC / IR、adapter removal / reinsertion recovery。
- binary release。

## 5. 関連 spec / docs

- `docs/hardware-test-log.md`
- `docs/status.md`
- `work-units/wip/local_100/SHUTDOWN_GRACEFUL_DISCONNECT.md`
- `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`
- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`

## 6. 根拠監査

`source-audit` 対象である。BTstack / L2CAP status、active reconnect request、WinUSB transport timing、HCI dump interpretation に触れるため、原因を断定する前に source fact、implementation fact、hardware observation、hypothesis を分ける。

初期状態:

| 項目 | 値 | 根拠 | status |
|---|---:|---|---|
| BTstack submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | existing repo / hardware logs | recorded |
| local_073 success | saved config / TLV DB から incoming pairing なしで HID open と Button A smoke | `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`, `docs/hardware-test-log.md` | hardware observation |
| local_100 first active reconnect failure | `Create_connection` 後に `Connection_complete` なし | `tmp/hardware/local_100/20260628-194019-shutdown-active-reconnect-graceful-disconnect` | hardware observation |
| local_100 second active reconnect failure | `Connection_complete(status=0)`, `responding to link key request`, `have link key db: 1`, `HCI_EVENT_AUTHENTICATION_COMPLETE status 0x05`, `L2CAP_EVENT_CHANNEL_OPENED status 0x66`, HID open なし | `tmp/hardware/local_100/20260628-194325-shutdown-active-reconnect-graceful-disconnect` | hardware observation |
| HCI authentication status `0x05` の意味 | `ERROR_CODE_AUTHENTICATION_FAILURE` | `vendor/btstack/src/bluetooth.h:217`, `vendor/btstack/src/btstack_defines.h:265`, `vendor/btstack/src/hci.c:5047-5070` | source fact |
| status `0x66` の意味 | `L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_SECURITY` | `vendor/btstack/src/bluetooth.h:297` | source fact |
| outgoing L2CAP security failure path | actual security level が required security level 未満の outgoing channel は、PIN/key missing 以外で `L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_SECURITY` に変換される | `vendor/btstack/src/l2cap.c:2935-2999` | source fact |
| HID control / interrupt outgoing security requirement | HID PSM `0x11` / `0x13` の outgoing `l2cap_create_channel()` は security level `2` を要求する | `vendor/btstack/src/hci.c:5556-5557`, `vendor/btstack/src/l2cap.c:2573-2575`, `vendor/btstack/src/classic/hid_device.c:855-856` | source fact |
| local_073 success security level | `responding to link key request` 後に `hci_emit_security_level 2`、`actual 2 >= required 2`、PSM `0x11` / `0x13` の `status 0x0` | `tmp/hardware/local_073/20260626-205341-link-key-db-sleep-resume-existing/sleep-resume-existing/sleep-resume-reconnect-hci-dump.txt:213-291` | hardware observation |
| local_100 failure security level | `responding to link key request` 後に authentication complete status `0x05`、`hci_emit_security_level 0`、`actual 0 >= required 2?`、PSM `0x11` の `status 0x66` | `tmp/hardware/local_100/20260628-194325-shutdown-active-reconnect-graceful-disconnect/hci-dump.txt:146-161` | hardware observation |
| local_101 active reconnect rerun | `active reconnect request ok`、`responding to link key request`、`have link key db: 1` までは到達したが、`hci_emit_security_level 0` と PSM `0x11` の `L2CAP_EVENT_CHANNEL_OPENED status 0x66` で HID open 未達 | `tmp/hardware/local_101/20260628-202504-active-reconnect-hid-open-recovery` | hardware observation |
| local_101 controlled re-pair attempt | blank config + stale TLV で incoming re-pair を待ったが、`Connection_incoming=0`、`pairing_started=0`、link key notification 保存なし、TLV 変更なし | `tmp/hardware/local_101/20260628-203457-controlled-repair-refresh-link-key-db` | hardware observation |
| local_101 controlled re-pair success | blank config + stale TLV から incoming re-pair を受け、`btstack: link key db stored notification`、`production: learned switch address save ok`、TLV `88 -> 128` bytes、hash changed | `tmp/hardware/local_101/20260628-204328-controlled-repair-refresh-link-key-db` | hardware observation |
| local_101 refreshed TLV active reconnect success | refreshed TLV から outgoing active reconnect を行い、2 回目で `Connection_incoming=0`、`pairing_complete_status_00=0` のまま HID open、Button A smoke、security level `2`、PSM `0x11` / `0x13` open status `0x0` に到達 | `tmp/hardware/local_101/20260628-204952-active-reconnect-hid-open-recovery` | hardware observation |
| HCI disconnection complete event ID | `HCI_EVENT_DISCONNECTION_COMPLETE == 0x05` | `vendor/btstack/src/btstack_defines.h:259` | source fact |
| HCI disconnection complete payload fields | BTstack event definition uses `1H1`: status, connection handle, reason | `vendor/btstack/src/hci_event.c:192-193`, `vendor/btstack/src/btstack_event.h:363-386` | source fact |
| HCI disconnection reason `0x13` | `ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION` | `vendor/btstack/src/bluetooth.h:230` | source fact |
| local_100 post-graceful active reconnect remote close | `Connection_complete(status=0)`, link key response, `hci_emit_security_level 2`, PSM `0x11` / `0x13` open status `0x0` 後、outgoing neutral `a1 30` 12 件の後に `data=050400470013` で切断された。Switch output report `a2 01` と swbt subcommand reply `a1 21` は出ていない | `tmp/hardware/local_100/20260628-210911-shutdown-active-reconnect-graceful-disconnect/hci-dump.txt:128-241`, `summary.json` | hardware observation |
| absence of local disconnect request in local_100 post-graceful reconnect | `disconnect_from_hid_host_count=0`、`shutdown_disconnect_requested_count=0`、HCI dump 内に local HCI disconnect command `data=0604...` と `Disconnect from HID Host` marker はない | `tmp/hardware/local_100/20260628-210911-shutdown-active-reconnect-graceful-disconnect/summary.json`, `hci-dump.txt`, `daemon-trace.txt:44-52` | hardware observation |
| local_101 success contrast after HID open | 同じ refreshed TLV 由来の success run は PSM open 後、neutral `a1 30` 数件の後に Switch output report `a2 01` を受け、swbt は `a1 21` subcommand replies と Button A smoke へ進んだ | `tmp/hardware/local_101/20260628-204952-active-reconnect-hid-open-recovery/hci-dump.txt:227-295`, `summary.json` | hardware observation |
| local_101 post-HID no-button observation | post-graceful source から HID open 後に Button A を送らず観測した 2 runs は、Switch output report `a2 01` と swbt subcommand reply `a1 21` が始まり、reason `0x13` は出なかった | `tmp/hardware/local_101/20260628-213316-active-reconnect-post-hid-observe`, `tmp/hardware/local_101/20260628-213730-active-reconnect-post-hid-observe` | hardware observation |
| local_101 immediate post-graceful held-shutdown rerun | no-button observation 直後の held Button A shutdown rerun 2 runs は、どちらも `Create_connection` 後に `Connection_complete` が出ず、HID open 未達だった | `tmp/hardware/local_100/20260628-213436-shutdown-active-reconnect-graceful-disconnect`, `tmp/hardware/local_100/20260628-213757-shutdown-active-reconnect-graceful-disconnect` | hardware observation |
| local_101 delayed post-graceful held-shutdown rerun | 60 秒待機後の held Button A shutdown rerun は HID open、Switch output report `a2 01`、shutdown neutral、HID disconnect closed まで到達した | `tmp/hardware/local_100/20260628-214137-shutdown-active-reconnect-graceful-disconnect` | hardware observation |
| local_101 create-connection timeout fail/retry pattern | successful graceful disconnect artifact からの first active reconnect attempt が `Create_connection` 後に `Connection_complete` 未達で fail し、同じ source artifact の retry が HID open と Switch output report `a2 01` へ進む pattern を 4 組観測した | `tmp/hardware/local_101/20260628-215231-active-reconnect-post-hid-observe`, `20260628-215529-active-reconnect-post-hid-observe`, `20260628-215603-active-reconnect-post-hid-observe`, `20260628-215829-active-reconnect-post-hid-observe`, `20260628-215902-active-reconnect-post-hid-observe`, `20260628-220124-active-reconnect-post-hid-observe`, `20260628-220300-active-reconnect-post-hid-observe`, `20260628-220515-active-reconnect-post-hid-observe` | hardware observation |
| swbt link key notification handling | configured link key DB が open の間、non-null `HCI_EVENT_LINK_KEY_NOTIFICATION` は `gap_store_link_key_for_bd_addr()` へ渡される | `swbt/btstack_bridge/production_btstack_impl.c:569-592` | implementation fact |
| BTstack TLV duplicate address handling | `btstack_link_key_db_tlv_put_link_key()` は既存 address の `tag_for_addr` を empty / oldest tag より優先して保存する | `vendor/btstack/src/classic/btstack_link_key_db_tlv.c:119-176` | source fact |
| swbt link key DB refresh characterization | 同じ address へ 2 回の `HCI_EVENT_LINK_KEY_NOTIFICATION` を emit すると、2 回目の link key / key type を `gap_get_link_key_for_bd_addr()` で読める | `tests/btstack_production_hci_dump_test.c:174-268` | software observation |

## 7. 設計メモ

この work unit は、まず regression / environment / timing のどれかを分ける。`local_073` は current tree ではなく過去 commit の observation であるため、成功条件が今の code で再現しない理由を diff だけで決めない。

最初の分岐:

```text
saved config + TLV DB available
-> active reconnect request ok
-> connection complete?
-> link key request / response?
-> L2CAP control open status?
-> HID interrupt open?
-> production hid connection opened
```

`local_100` の shutdown disconnect は active HID connection を前提にする。active reconnect が HID open へ到達しない限り、shutdown disconnect の final hardware acceptance には進めない。

Change Grip/Order で実登録が走った場合、daemon が `--link-key-db` configured で動いていれば current implementation は新しい link key notification を保存し、BTstack TLV は同一 address の entry を上書きする。このため、現時点の再発条件として疑うべきなのは、再登録が `--link-key-db` なしで行われた場合、または Switch 側の登録更新後に古い TLV artifact を再利用した場合である。実機で再登録を行う run では、DB 更新後の artifact を新しい source として扱う。

## 8. 対象ファイル

- `swbt/daemon/active_reconnect.*`
- `swbt/daemon/production_runner.*`
- `swbt/daemon/btstack_hid_session.*`
- `swbt/btstack_bridge/production_btstack_impl.*`
- `swbt/daemon/launch_options.*`
- `tests/daemon_active_reconnect_test.c`
- `tests/daemon_production_runner_test.c`
- `docs/hardware-test-log.md`
- `tmp/hardware/local_101/run-active-reconnect-hid-open-recovery.ps1`
- `tmp/hardware/local_101/run-controlled-repair-refresh-link-key-db.ps1`
- `work-units/wip/local_101/ACTIVE_RECONNECT_HID_OPEN_RECOVERY.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | source audit identifies `L2CAP_EVENT_CHANNEL_OPENED status 0x66` meaning and separates it from pairing-free reconnect success criteria | characterization | docs/source | no |
| refactor-skipped | saved config and link key DB startup path still opens link key DB before active reconnect request | regression | unit/integration | no |
| todo | active reconnect request is issued exactly once after HCI power-on when learned Switch address exists | regression | unit/integration | no |
| refactor-skipped | active reconnect failure before HID open is reported or traceable without being confused with shutdown graceful disconnect failure | characterization | unit/integration | no |
| refactor-done | re-pair with configured link key DB refreshes the stored key for an existing Switch address | characterization | integration | no |
| green | hardware run reproduces or clears current failure boundary without Change Grip/Order or incoming pairing | characterization | hardware | yes |
| green | controlled re-pair with configured link key DB refreshes TLV before active reconnect retest | characterization | hardware | yes |
| green | hardware run reaches HID open and Button A smoke with saved link key DB and no incoming pairing | regression | hardware | yes |
| green | post-HID-open reason `0x13` active reconnect close is characterized against Switch output-report start and local disconnect evidence | characterization | hardware/artifact | yes |
| green | post-graceful-disconnect immediate active reconnect timing is characterized separately from reason `0x13` remote close | characterization | hardware | yes |
| todo | Create Connection completion timeout is surfaced as an active reconnect failure and can be retried without confusing it with authentication or L2CAP failure | characterization/regression | unit/integration | no |
| deferred | local_100 shutdown graceful disconnect final hardware run resumes after active reconnect is available | characterization | hardware | yes |

## 10. 検証

起票時確認:

- `git branch --show-current`
  - result: `work/local-100-shutdown-disconnect`。
- `git status --short`
  - result: `local_100` の実装・docs 差分がある dirty worktree。`local_101` はこの状態から起票した。
- `Get-ChildItem work-units\wip,work-units\complete -Directory`
  - result: `work-units/complete/local_099` と `work-units/wip/local_100` まで存在したため、新規番号を `local_101` とした。
- `Get-Content tmp\hardware\local_100\20260628-194325-shutdown-active-reconnect-graceful-disconnect\summary.json`
  - result: `hid_connection_opened_count=0`, `connection_incoming_count=0`, `pairing_complete_status_00_count=0`, `responding_to_link_key_request_count=1`, `have_link_key_db_1_count=1`, `pass=false`。

未実行:

- software TDD
  - reason: active reconnect request の exact-once timing item と、実機結果に応じた追加修正は未実行。
- hardware rerun
  - reason: 起票のみ。実機承認と前提確認後に行う。

TDD status:
- source: `local_100` active reconnect 専用 run が `L2CAP_EVENT_CHANNEL_OPENED status 0x66` で HID open 前に停止した。
- use case: maintainer は active reconnect の失敗を shutdown graceful disconnect の失敗と混ぜず、BTstack / HCI dump の根拠からどの段階で止まったか説明できる。
- item: source audit identifies `L2CAP_EVENT_CHANNEL_OPENED status 0x66` meaning and separates it from pairing-free reconnect success criteria。
- state: refactor-skipped。
- commands:
  - `rg -n "L2CAP_CONNECTION_.*0x6|L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_SECURITY|PIN_OR_LINK_KEY|REFUSED_SECURITY" vendor\btstack\src\bluetooth.h vendor\btstack\src\l2cap.c`
  - `rg -n -C 10 "actual .*required|L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_SECURITY|security level update for handle|PIN_OR_LINK_KEY_MISSING" vendor\btstack\src\l2cap.c`
  - `rg -n "L2CAP_EVENT_CHANNEL_OPENED status 0x0|hci_emit_security_level 2|actual 2 >= required 2|gap_request_security_level requested level 2|responding to link key request" tmp\hardware\local_073\20260626-205341-link-key-db-sleep-resume-existing\sleep-resume-existing\sleep-resume-reconnect-hci-dump.txt`
  - `rg -n "L2CAP_EVENT_CHANNEL_OPENED status 0x66|hci_emit_security_level 0|actual 0 >= required 2|gap_request_security_level requested level 2|responding to link key request" tmp\hardware\local_100\20260628-194325-shutdown-active-reconnect-graceful-disconnect\hci-dump.txt`
- notes: `0x66` は BTstack source 上 `REFUSED_SECURITY` であり、link key DB open / link key response の失敗ではない。成功 run と失敗 run はどちらも level 2 を要求し link key request に応答しているが、成功 run は authentication complete status `0x00` 後に `hci_emit_security_level 2`、失敗 run は authentication complete status `0x05` 後に `hci_emit_security_level 0` で分岐している。次は保存済み config / link key DB startup path と active reconnect request timing が current tree で崩れていないかを software TDD で固定する。

Refactor status:
- decision: refactor-skipped。
- change: none。
- unchanged behavior: daemon startup、active reconnect request、link key DB wiring、shutdown graceful disconnect 実装には触れていない。
- verification: source audit と既存 artifact 比較のみ。software test と実機 rerun は未実行。

TDD status:
- source: `local_100` active reconnect 専用 run は `btstack: link key db open ok` と `production: active reconnect request ok` を順に記録していたが、HID open に到達しなかった。
- use case: maintainer は保存済み config / link key DB を指定した production startup path が、active reconnect request より前に link key DB を設定し、runner が power-on 後に active reconnect request を出すことを current tree の software test で確認できる。
- item: saved config and link key DB startup path still opens link key DB before active reconnect request。
- state: refactor-skipped。
- commands:
  - red: not applicable。既存の `production_entrypoint_boundary_cmake_test`、`daemon_launch_options_test`、`daemon_production_runner_test` がこの regression boundary を既に覆っていたため、新しい failing test は追加していない。
  - green: `$env:CTEST_ARGS='-R "production_entrypoint_boundary_cmake_test|daemon_launch_options_test|daemon_production_runner_test"'; just debug`
- result: pass。3 tests passed。
- notes: 初回の sandboxed run は Dev Container CLI の `docker ps` で失敗したため、同じ command を Docker daemon access のため昇格付きで再実行した。`production_entrypoint_boundary_cmake_test` は production entrypoint が link key DB 設定と runner handoff を持つこと、`daemon_launch_options_test` は config / link key DB launch config、`daemon_production_runner_test` は `STEP_POWER_ON` 後の active reconnect request を固定している。実機失敗の主因は startup path ではなく、link key response 後の authentication status `0x05` による security level `0` と判断する。

Refactor status:
- decision: refactor-skipped。
- change: none。
- unchanged behavior: startup path、BTstack production impl、runner sequencing には触れていない。
- verification: targeted `just debug`。

TDD status:
- source: BTstack `hid_device.c` は outgoing `L2CAP_EVENT_CHANNEL_OPENED` failure を HID connected event の nonzero status として通知する。`local_100` の failure は HCI dump では見えるが、daemon trace では `production: active reconnect request ok` の後に HID open が出ないだけだった。
- use case: maintainer は HID open 前の active reconnect failure を daemon trace 上の `production: hid connection open failed` として確認でき、shutdown graceful disconnect の cleanup trace と混同しない。
- item: active reconnect failure before HID open is reported or traceable without being confused with shutdown graceful disconnect failure。
- state: refactor-skipped。
- commands:
  - red: `$env:CTEST_ARGS='-R daemon_btstack_hid_session_test'; just debug`
  - green: `$env:CTEST_ARGS='-R daemon_btstack_hid_session_test'; just debug`
  - format: `just format`
  - check: `git diff --check`
  - green after format: `$env:CTEST_ARGS='-R daemon_btstack_hid_session_test'; just debug`
- result: red は `open failure trace: missing trace production: hid connection open failed` で失敗。green は `daemon_btstack_hid_session_test` 1 test passed。
- notes: `swbt_daemon_btstack_hid_session_handle_connection_opened()` は nonzero status の opened event で `production: hid connection open failed` を trace し、report timer は開始しない。status 値の詳細は HCI dump / BTstack log 側で確認する方針とし、daemon trace へ raw address や link key は出さない。

Refactor status:
- decision: refactor-skipped。
- change: none。
- unchanged behavior: successful HID open は従来どおり `production: hid connection opened` を trace し、report timer を開始する。connection closed / shutdown neutral / shutdown disconnect の挙動は変えない。
- verification: targeted `just debug`。

TDD status:
- source: Change Grip/Order で実登録処理が走った場合、Switch 側の登録状態が更新され、古い TLV artifact に保存された link key では authentication status `0x05` になる可能性がある。
- use case: maintainer は daemon が `--link-key-db` configured で動いている間に同じ Switch address の link key notification を受け取った場合、保存済み DB が新しい key / key type へ更新されることを software test で確認できる。
- item: re-pair with configured link key DB refreshes the stored key for an existing Switch address。
- state: refactor-done。
- commands:
  - red: not applicable。characterization item として test を追加した時点で existing implementation が成立していたため、production code の failing red は発生していない。
  - green: `$env:CTEST_ARGS='-R btstack_production_hci_dump_test'; just debug`
  - format: `just format`
  - check: `git diff --check`
  - green after refactor: `$env:CTEST_ARGS='-R btstack_production_hci_dump_test'; just debug`
- result: `btstack_production_hci_dump_test` 1 test passed。`git diff --check` は `docs/hardware-test-log.md` の CRLF 変換 warning のみ。
- notes: production code には触れていない。`link_key_db_refreshes_existing_link_key_notification()` は同一 address に first key と refreshed key を順に通知し、`gap_get_link_key_for_bd_addr()` が refreshed key と `AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192` を返すことを確認する。

Refactor status:
- decision: refactor-done。
- change: test fixture の `bd_addr_t address` から不要な const cast を除去した。
- unchanged behavior: link key DB open、link key notification handling、BTstack TLV 保存、active reconnect、shutdown graceful disconnect の挙動は変えていない。
- verification: targeted `just debug`。

TDD status:
- source: user request, 2026-06-28: 実機検証を再開し、Switch 側は sleep / resume 済み。Change Grip/Order を開かない active reconnect run で、現在の failure boundary を再確認する。
- use case: maintainer は保存済み config / TLV link key DB から active reconnect を行い、incoming pairing なしで HID open へ進むか、または前回と同じ security failure で止まるかを実機 artifact から判定できる。
- item: hardware run reproduces or clears current failure boundary without Change Grip/Order or incoming pairing。
- state: green。
- commands:
  - build: `just windows-cross`
  - hardware: `& .\tmp\hardware\local_101\run-active-reconnect-hid-open-recovery.ps1`
- result: fail as recovery、pass as characterization。artifact `tmp/hardware/local_101/20260628-202504-active-reconnect-hid-open-recovery`。summary は `active_reconnect_request_ok_count=1`、`link_key_db_open_ok_count=1`、`hid_connection_opened_count=0`、`hid_connection_open_failed_count=1`、`responding_to_link_key_request_count=1`、`have_link_key_db_1_count=1`、`security_level_2_count=0`、`security_level_0_count=1`、`connection_incoming_count=0`、`pairing_started_count=0`、`pairing_complete_status_00_count=0`、`l2cap_open_status_66_count=1`、`button_smoke_ran=false`、`pass=false`。
- notes: daemon は active reconnect request と link key DB 応答までは成功したが、保存済み key による認証後に security level `2` へ上がらず、control PSM `0x11` の open が `0x66` で拒否された。Button A smoke は HID open 未達のため未実行。cleanup は IPC neutral client exit code `0`、daemon exit code `0`、crash dump なし。`docs/hardware-test-log.md` に同じ artifact を記録した。次は `--link-key-db` configured の controlled re-pair で TLV を更新し、その artifact から active reconnect を再試験するかを判断する。

Refactor status:
- decision: refactor-skipped。
- change: none。
- unchanged behavior: 実機 run と記録のみ。production code、active reconnect request、link key DB handling、shutdown graceful disconnect の挙動は変えていない。
- verification: hardware artifact and targeted Windows cross build。

TDD status:
- source: active reconnect failure は保存済み TLV が Switch 側の登録状態に対して stale である可能性が高い。`--link-key-db` configured の daemon で controlled re-pair を行い、新しい link key notification で TLV を更新できるかを実機で確認する必要がある。
- use case: maintainer は blank config と stale TLV で daemon を起動し、Switch Change Grip/Order から incoming re-pair させることで、active reconnect を発火させずに link key DB 更新と learned address 保存を artifact 化できる。
- item: controlled re-pair with configured link key DB refreshes TLV before active reconnect retest。
- state: todo。
- commands:
  - hardware attempt: `& .\tmp\hardware\local_101\run-controlled-repair-refresh-link-key-db.ps1`
- result: attempt incomplete。artifact `tmp/hardware/local_101/20260628-203457-controlled-repair-refresh-link-key-db`。summary は `link_key_db_open_ok_count=1`、`hid_connection_opened_count=0`、`stored_link_key_notification_count=0`、`learned_switch_address_save_ok_count=0`、`connection_incoming_count=0`、`pairing_started_count=0`、`pairing_complete_status_00_count=0`、`tlv_hash_changed=false`、`pass=false`。
- notes: daemon は adapter open、HCI init、HID L2CAP service registration、HCI working state まで到達したが、Switch から incoming connection が来ていない。したがって link key refresh の成否は未判定。HID open wait の延長は根拠が薄いため、次 run 前に `180000 ms` へ戻した。次回は Switch 側を Change Grip/Order で登録待ち状態にしたうえで同じ script を再実行する。

Refactor status:
- decision: refactor-skipped。
- change: none。
- unchanged behavior: 実機 script と記録のみ。production code は変えていない。
- verification: PowerShell script parse check、hardware artifact。

TDD status:
- source: user request, 2026-06-28: Switch 側を登録待ち状態にしたため、controlled re-pair を再実行する。登録完了後に再び Change Grip/Order へ遷移しないよう注意する。re-pair wait の延長は根拠が薄いため戻す。
- use case: maintainer は `--link-key-db` configured daemon で incoming re-pair を受け、保存済み TLV と learned address が新しい登録状態へ更新された artifact を得る。
- item: controlled re-pair with configured link key DB refreshes TLV before active reconnect retest。
- state: green。
- commands:
  - script update: `tmp/hardware/local_101/run-controlled-repair-refresh-link-key-db.ps1` の HID open wait を `180000 ms` に戻した。
  - parse check: `$null = [scriptblock]::Create((Get-Content -Raw -LiteralPath 'tmp\hardware\local_101\run-controlled-repair-refresh-link-key-db.ps1'))`
  - hardware: `& .\tmp\hardware\local_101\run-controlled-repair-refresh-link-key-db.ps1`
- result: pass。artifact `tmp/hardware/local_101/20260628-204328-controlled-repair-refresh-link-key-db`。summary は `link_key_db_open_ok_count=1`、`hid_connection_opened_count=1`、`stored_link_key_notification_count=1`、`learned_switch_address_save_ok_count=1`、`connection_incoming_count=1`、`pairing_started_count=1`、`pairing_complete_status_00_count=1`、`l2cap_open_status_0_count=2`、`tlv_length_before=88`、`tlv_length_after=128`、`tlv_hash_changed=true`、`pass=true`。
- notes: controlled re-pair script は Button A smoke を送っていない。cleanup は neutral client exit code `0`、daemon exit code `0`、crash dump なし。trace は shutdown neutral send ok、shutdown HID disconnect closed、link key DB close まで到達した。この artifact は active reconnect 再試験の source として扱える。

Refactor status:
- decision: refactor-skipped。
- change: none。
- unchanged behavior: production code は変えていない。実機 script の timeout を元に戻し、artifact 記録を追加したのみ。
- verification: PowerShell script parse check、hardware artifact。

TDD status:
- source: controlled re-pair で refreshed TLV と learned address を含む artifact `tmp/hardware/local_101/20260628-204328-controlled-repair-refresh-link-key-db` が得られた。user request, 2026-06-28: Switch 側は HOME から sleep / resume 済み。
- use case: maintainer は refreshed config / TLV link key DB から active reconnect を行い、Change Grip/Order や incoming pairing なしで HID open と Button A smoke に到達できる。
- item: hardware run reaches HID open and Button A smoke with saved link key DB and no incoming pairing。
- state: green。
- commands:
  - hardware first attempt: `& .\tmp\hardware\local_101\run-active-reconnect-hid-open-recovery.ps1 -SourceArtifactPath .\tmp\hardware\local_101\20260628-204328-controlled-repair-refresh-link-key-db`
  - hardware retry: `& .\tmp\hardware\local_101\run-active-reconnect-hid-open-recovery.ps1 -SourceArtifactPath .\tmp\hardware\local_101\20260628-204328-controlled-repair-refresh-link-key-db`
- result: first attempt `tmp/hardware/local_101/20260628-204721-active-reconnect-hid-open-recovery` failed before `Connection_complete`。retry `tmp/hardware/local_101/20260628-204952-active-reconnect-hid-open-recovery` passed。retry summary は `active_reconnect_request_ok_count=1`、`link_key_db_open_ok_count=1`、`hid_connection_opened_count=1`、`button_smoke_ran=true`、`responding_to_link_key_request_count=1`、`have_link_key_db_1_count=1`、`authentication_failure_count=0`、`security_level_2_count=1`、`security_level_0_count=0`、`connection_incoming_count=0`、`pairing_started_count=0`、`pairing_complete_status_00_count=0`、`l2cap_open_status_0_count=2`、`l2cap_open_status_66_count=0`、`input_report_a130_count=117`、`pass=true`。
- notes: first attempt は `Create_connection` 後に `Connection_complete` が出ず、link key 認証の成否には到達していない。retry は refreshed TLV で security level `2` へ上がり、HID control / interrupt PSM が両方 open した。cleanup は neutral client exit code `0`、daemon exit code `0`、crash dump なし。`local_100` shutdown graceful disconnect final hardware run は、この refreshed artifact を source として再開可能。

Refactor status:
- decision: refactor-skipped。
- change: none。
- unchanged behavior: 実機 run と記録のみ。production code は変えていない。
- verification: hardware artifacts。

TDD status:
- source: user request, 2026-06-28: post-graceful-disconnect reconnect evaluation の reason `0x13` close は `local_101` で原因追跡する。`local_100` は local disconnect request 残りの否定までを扱う。
- use case: maintainer は HID open 後の remote close を、link key / authentication failure、local disconnect request carryover、Switch output report 開始前の remote-side termination に分けて説明できる。
- item: post-HID-open reason `0x13` active reconnect close is characterized against Switch output-report start and local disconnect evidence。
- state: green。
- commands:
  - `Get-Content tmp\hardware\local_100\20260628-210911-shutdown-active-reconnect-graceful-disconnect\summary.json`
  - `Get-Content tmp\hardware\local_101\20260628-204952-active-reconnect-hid-open-recovery\summary.json`
  - `rg -n "Connection_complete|responding to link key request|have link key db|hci_emit_security_level|L2CAP_EVENT_CHANNEL_OPENED|packet type=0x04.*data=050400|L2CAP_EVENT_CHANNEL_CLOSED|Connection closed|a1 30|a2 01|Button" tmp\hardware\local_100\20260628-210911-shutdown-active-reconnect-graceful-disconnect\hci-dump.txt tmp\hardware\local_100\20260628-210911-shutdown-active-reconnect-graceful-disconnect\daemon-trace.txt`
  - `rg -n "Connection_complete|responding to link key request|have link key db|hci_emit_security_level|L2CAP_EVENT_CHANNEL_OPENED|packet type=0x04.*data=050400|L2CAP_EVENT_CHANNEL_CLOSED|Connection closed|a1 30|a2 01|Button" tmp\hardware\local_101\20260628-204952-active-reconnect-hid-open-recovery\hci-dump.txt tmp\hardware\local_101\20260628-204952-active-reconnect-hid-open-recovery\daemon-trace.txt`
  - `Get-Content tmp\hardware\local_100\20260628-210911-shutdown-active-reconnect-graceful-disconnect\hci-dump.txt | Select-Object -Skip 220 -First 35`
  - `Get-Content tmp\hardware\local_101\20260628-204952-active-reconnect-hid-open-recovery\hci-dump.txt | Select-Object -Skip 220 -First 45`
  - `& .\tmp\hardware\local_101\run-active-reconnect-post-hid-observe.ps1 -SourceArtifactPath .\tmp\hardware\local_100\20260628-210030-shutdown-active-reconnect-graceful-disconnect -PostHidObserveMs 5000`
  - `& .\tmp\hardware\local_100\run-shutdown-graceful-disconnect-active-reconnect.ps1 -SourceArtifactPath .\tmp\hardware\local_101\20260628-213316-active-reconnect-post-hid-observe`
  - `& .\tmp\hardware\local_101\run-active-reconnect-post-hid-observe.ps1 -SourceArtifactPath .\tmp\hardware\local_101\20260628-213316-active-reconnect-post-hid-observe -PostHidObserveMs 5000`
  - `& .\tmp\hardware\local_100\run-shutdown-graceful-disconnect-active-reconnect.ps1 -SourceArtifactPath .\tmp\hardware\local_101\20260628-213730-active-reconnect-post-hid-observe`
  - `Start-Sleep -Seconds 60`
  - `& .\tmp\hardware\local_100\run-shutdown-graceful-disconnect-active-reconnect.ps1 -SourceArtifactPath .\tmp\hardware\local_101\20260628-213730-active-reconnect-post-hid-observe`
- result: `local_100` post-graceful reconnect close run は `Connection_complete(status=0)`、link key response、`hci_emit_security_level 2`、PSM `0x11` / `0x13` open status `0x0` までは到達しており、link key / authentication failure ではない。HCI dump は outgoing neutral `a1 30` を 12 件送った後、`data=050400470013` で remote-side termination を記録した。Switch output report `a2 01` と swbt subcommand reply `a1 21` は出ていない。`disconnect_from_hid_host_count=0`、`shutdown_disconnect_requested_count=0` で、local disconnect request carryover の痕跡もない。追加の no-button observation は `20260628-213316-active-reconnect-post-hid-observe` と `20260628-213730-active-reconnect-post-hid-observe` の 2 回とも pass し、Switch output report `a2 01` と swbt `a1 21` が始まり、reason `0x13` は `0` 件だった。
- notes: post-graceful source artifact だけで Switch output report 開始前 close へ固定的に落ちるわけではない。即時 held Button A shutdown rerun は `20260628-213436...` と `20260628-213757...` の 2 回とも `Connection_complete` 未達で fail したが、60 秒待機後の `20260628-214137...` は pass した。現時点の有力な追跡対象は local disconnect request carryover ではなく、graceful disconnect 後の短時間 active reconnect timing / Switch 側受け入れ状態である。reason `0x13` 自体は今回の追跡 run では再現していない。

Refactor status:
- decision: refactor-skipped。
- change: none。
- unchanged behavior: production code は変えていない。`tmp/hardware/local_101/run-active-reconnect-post-hid-observe.ps1` は観測用 script であり、daemon behavior は変えていない。
- verification: existing artifact comparison and hardware artifacts。

TDD status:
- source: user request, 2026-06-28: reason `0x13` 追跡の次として、graceful disconnect 後の active reconnect timing を実機で追う。
- use case: maintainer は active reconnect が `Create_connection` 後に `Connection_complete` 未達で止まる failure を、reason `0x13`、link key / authentication failure、L2CAP security failure と分けて説明できる。
- item: post-graceful-disconnect immediate active reconnect timing is characterized separately from reason `0x13` remote close。
- state: green。
- commands:
  - `& .\tmp\hardware\local_101\run-active-reconnect-post-hid-observe.ps1 -SourceArtifactPath .\tmp\hardware\local_100\20260628-214137-shutdown-active-reconnect-graceful-disconnect -PostHidObserveMs 5000`
  - `& .\tmp\hardware\local_101\run-active-reconnect-post-hid-observe.ps1 -SourceArtifactPath .\tmp\hardware\local_100\20260628-214137-shutdown-active-reconnect-graceful-disconnect -PostHidObserveMs 5000`
  - `& .\tmp\hardware\local_101\run-active-reconnect-post-hid-observe.ps1 -SourceArtifactPath .\tmp\hardware\local_101\20260628-215529-active-reconnect-post-hid-observe -PostHidObserveMs 5000`
  - `& .\tmp\hardware\local_101\run-active-reconnect-post-hid-observe.ps1 -SourceArtifactPath .\tmp\hardware\local_101\20260628-215529-active-reconnect-post-hid-observe -PostHidObserveMs 5000`
  - `& .\tmp\hardware\local_101\run-active-reconnect-post-hid-observe.ps1 -SourceArtifactPath .\tmp\hardware\local_101\20260628-215829-active-reconnect-post-hid-observe -PostHidObserveMs 5000`
  - `& .\tmp\hardware\local_101\run-active-reconnect-post-hid-observe.ps1 -SourceArtifactPath .\tmp\hardware\local_101\20260628-215829-active-reconnect-post-hid-observe -PostHidObserveMs 5000`
  - `Start-Sleep -Seconds 60`
  - `& .\tmp\hardware\local_101\run-active-reconnect-post-hid-observe.ps1 -SourceArtifactPath .\tmp\hardware\local_101\20260628-220124-active-reconnect-post-hid-observe -PostHidObserveMs 5000`
  - `& .\tmp\hardware\local_101\run-active-reconnect-post-hid-observe.ps1 -SourceArtifactPath .\tmp\hardware\local_101\20260628-220124-active-reconnect-post-hid-observe -PostHidObserveMs 5000`
- result: `20260628-215231...`、`20260628-215603...`、`20260628-215902...`、`20260628-220300...` は fail。いずれも `active_reconnect_request_ok_count=1`、`hid_connection_opened_count=0`、`responding_to_link_key_request_count=0`、`l2cap_open_status_0_count=0` で、HCI dump は `Create_connection` を記録したが `Connection_complete` へ進んでいない。同じ source artifact の retry `20260628-215529...`、`20260628-215829...`、`20260628-220124...`、`20260628-220515...` は pass し、HID open、Switch output report `a2 01`、swbt subcommand reply `a1 21` まで進んだ。
- notes: failure は reason `0x13` remote close より前の段階であり、link key request、authentication、L2CAP open へ到達していない。待機時間だけでは pass / fail を説明できなかった。次の software item は、Create Connection completion timeout を active reconnect failure として trace / retry policy に載せるか、少なくとも authentication / L2CAP failure と混同しない形で観測できるようにすること。

Refactor status:
- decision: refactor-skipped。
- change: none。
- unchanged behavior: production code は変えていない。実機 artifact と記録のみ。
- verification: hardware artifacts。

## 11. 実機実行条件

実機が必要である。ただしこの record 起票時点では実行しない。

必要な承認:

- CSR8510 A10 / WinUSB adapter open。
- 保存済み `swbt-daemon.toml` / `swbt-link-key.tlv` の再利用。
- Switch HOME から sleep / resume 済み状態での active reconnect request。
- HID open 後の Button A smoke。
- HCI dump / diagnostic trace / crash dump path 保存。
- cleanup 確認。

adapter assumptions:

- 専用 CSR8510 A10、InstanceId `USB\VID_0A12&PID_0001\9&12127A34&0&1`。
- WinUSB service。
- 内蔵 Bluetooth と常用 Bluetooth device は対象外。

environment variables:

- `SWBT_RUN_HARDWARE=1`
- `SWBT_HARDWARE_APPROVED=1`
- `SWBT_IPC_HOST=127.0.0.1`
- `SWBT_IPC_PORT=37637`
- `SWBT_REPORT_PERIOD_US=8000`

expected log target:

- `tmp/hardware/local_101/<timestamp>-active-reconnect-hid-open-recovery`
- `docs/hardware-test-log.md`

cleanup requirements:

- Button pressed state が残らないよう、HID open 後は neutral send または shutdown neutral を確認する。
- daemon exit code、forced stop 有無、crash dump 有無を記録する。
- `Get-Process swbt-daemon` が空であることを確認する。

## 12. 先送り事項

- 観測: `local_100` shutdown graceful disconnect final hardware run は、この work unit で active reconnect が復旧するまで再実行しない。
  先送り理由: active HID connection が確立しない run では shutdown disconnect behavior を評価できない。
  次の置き場: `work-units/wip/local_100/SHUTDOWN_GRACEFUL_DISCONNECT.md` の active reconnect hardware item。

## 13. チェックリスト

- [x] source を `local_100` 実機観測と `local_073` 既存成功記録から特定した。
- [x] use case を active reconnect HID open recovery として定義した。
- [x] `local_100` の shutdown graceful disconnect 実装変更を対象外にした。
- [x] `L2CAP_EVENT_CHANNEL_OPENED status 0x66` の根拠監査を行う。
- [ ] software TDD item を 1 つずつ red / green / refactor で進める。
- [x] 必要な実装修正を行う、または不要と判断した理由を記録する。
- [x] active reconnect hardware run を実行し、incoming pairing なしの HID open と Button A smoke を確認する。
- [x] `local_100` へ再開条件を返す。
- [x] post-HID-open reason `0x13` close の原因追跡を `local_101` の TDD item として開始した。
