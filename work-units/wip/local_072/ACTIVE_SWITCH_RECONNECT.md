# Active Switch Reconnect

## 1. 概要

daemon restart 後に、Switch 側の Change Grip / Order 画面や再ペアリング操作へ戻らず、daemon 側から既知の Switch Bluetooth address へ HID control / interrupt L2CAP channel を開きに行く reconnect 経路を設計、実装、実機確認する work unit。

`local_065` の passive TLV-backed link key DB / bond cache 経路は、Switch2 `22.1.0` が bonding を要求せず link key material が保存されないため不採用にした。この work unit では、保存済み link key DB を主経路にせず、joycontrol `-r` 型の active reconnect を調査対象にする。

## 2. 起点 / ユースケース

source:

- user discussion, 2026-06-25: bond による reconnect と repairing の違いを整理した結果、joycontrol `-r` 型の経路を持つべきという判断になった。
- `work-units/complete/local_065/BONDED_RECONNECT_PERSISTENCE.md`: passive TLV-backed link key DB 経路では Switch2 `22.1.0` が bonding せず、daemon restart / sleep-resume の復帰は既存 bond reconnect ではなく再 pairing だった。
- `spec/archive/bond-cache-persistence.md`: passive bond cache は現行設計ではなく、active reconnect に必要な Switch address と outbound L2CAP 境界が未解決事項である。
- `docs/status.md`: active reconnect は未実装である。
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`: bond store / reconnect 候補は BTstack port boundary の後続候補である。
- user discussion, 2026-06-25: Switch address は自動取得し、設定ファイル layer に永続化する。削除は設定ファイル上の値の除去で扱う。trace log では検証用に address を明示してよいが、リポジトリに残す記録では scrub する。

use case:

- actor: hardware operator、production daemon、Switch。
- 入力または状態: 初回 pairing または一度成功した接続から自動取得した Switch Bluetooth address、設定ファイル layer に残る learned address、daemon process restart、dedicated USB Bluetooth dongle、Switch が controller reconnect を受け付ける状態。
- 期待する観測結果:
  - daemon restart 後、Switch 側の再ペアリング操作なしで daemon が既知 Switch address へ active reconnect を試行する。
  - L2CAP PSM `0x11` / `0x13` が open し、既存の HID report loop と Button A smoke へ戻る。
  - Switch 側操作が必要な場合でも、Change Grip / Order での再ペアリングではなく、controller reconnect 操作として切り分けて記録する。
  - 失敗時は、address 不明、ACL 接続不可、control channel open 失敗、interrupt channel open 失敗、HID report 採用失敗を分けて診断できる。
- 制約: pairing、HID advertising、active outbound L2CAP、report loop は明示承認後だけ実行する。Switch address は raw link key ではないが、実機 artifact では扱いと削除手順を記録する。
- 対象外: passive TLV-backed link key DB / bond cache persistence の復活、raw link key storage、複数 controller、PC reboot / USB dongle 抜き差し recovery、release packaging。

source から use case への変換:

`local_065` は「保存済み link key があれば reconnect できる」という前提を確認したが、今回の Switch 条件では保存される key material がなかった。ユーザ価値は保存形式ではなく「daemon restart 後に再ペアリングへ戻らないこと」なので、次は Switch address を既知状態として扱い、daemon 側から接続を開始する active reconnect を独立した work unit にする。

## 3. 対象範囲

- joycontrol `-r` 型 reconnect の source audit を実施し、必要な address、channel open 順、pairing / reconnect の違いを記録する。
- BTstack の outbound L2CAP / HID device connect 境界を source-audit で確認する。
- Switch Bluetooth address の取得、保存、設定、削除、ログ露出の方針を決める。
- daemon-managed learned Switch address を同一 TOML file へ書き戻す config file boundary を追加する。
- production adapter に active reconnect request を表現する testable boundary を追加する。
- active reconnect 失敗時に既存 incoming pairing / advertising 経路を壊さないことを test で固定する。
- 実機手順を `hardware-harness` に沿って作り、daemon restart、Switch sleep / resume、Switch 側 controller reconnect 操作のどれを要求するか分けて記録する。
- 実機結果を `docs/hardware-test-log.md` とこの work unit に記録する。

## 4. 対象外

- `local_065` で削除した `swbt-bond-<local-bdaddr>.tlv` 経路の復活。
- raw link key value の保存、表示、docs 転記。
- 複数 controller 同時接続。
- Joy-Con、NFC / IR semantic support。
- adapter removal / reinsertion recovery。
- PC reboot。
- USB dongle 抜き差し。
- release artifact 作成。

## 5. 関連 spec / docs

- `work-units/complete/local_065/BONDED_RECONNECT_PERSISTENCE.md`
- `spec/archive/bond-cache-persistence.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`

## 6. 根拠監査

実施中。

この work unit は BTstack outbound L2CAP、HID device reconnect、Switch controller reconnect 操作、実機 HCI event を扱う。Switch-facing report bytes そのものを変える予定はないが、connection lifecycle と実機挙動に触れるため `source-audit` と `hardware-harness` を使う。

source-audit 結果:

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| BTstack submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | repository fact | `git ls-tree HEAD vendor/btstack` | recorded |
| HID control PSM | `0x11` | BTstack source fact | `vendor/btstack/src/l2cap.h:463`, `vendor/btstack/src/classic/hid_device.c:855-856` | stable source fact |
| HID interrupt PSM | `0x13` | BTstack source fact | `vendor/btstack/src/l2cap.h:464`, `vendor/btstack/src/classic/hid_device.c:855-856` | stable source fact |
| Classic outbound L2CAP API | `l2cap_create_channel(...)` | BTstack source fact | `vendor/btstack/src/l2cap.h:507`, `vendor/btstack/src/l2cap.c:2569-2577` | stable source fact |
| BTstack HID device outbound connect precedent | `hid_device_connect()` は HID control PSM を開く | BTstack source fact | `vendor/btstack/src/classic/hid_device.c:949-963` | implementation candidate |
| BTstack HID outbound channel order | outgoing control channel open 後に interrupt PSM を開く | BTstack source fact | `vendor/btstack/src/classic/hid_device.c:720-722` | implementation candidate |
| BTstack incoming channel handling | incoming control / interrupt connection は `L2CAP_EVENT_INCOMING_CONNECTION` で accept される | BTstack source fact | `vendor/btstack/src/classic/hid_device.c:651-681` | current incoming path precedent |
| BTstack HID connection opened event address | `HID_SUBEVENT_CONNECTION_OPENED` は `bd_addr` を含み、BTstack accessor は event offset `6` から reverse して取得する | BTstack source fact | `vendor/btstack/src/btstack_defines.h:4198-4203`, `vendor/btstack/src/btstack_event.h:15090-15095`, `vendor/btstack/src/classic/hid_device.c:329-334` | stable source fact |
| joycontrol commit | `18a09da1a04306534ff9e1df8a1a69c0192a3244` | upstream source fact | `https://github.com/mart1nro/joycontrol/commits/master/`, `spec/references/switch-hid-initial-source-audit.md` | recorded |
| joycontrol `-r` input | `--reconnect_bt_addr` is a Switch console Bluetooth address for reconnecting as an already paired controller | upstream implementation fact | `https://github.com/mart1nro/joycontrol/blob/18a09da1a04306534ff9e1df8a1a69c0192a3244/run_controller_cli.py` | implementation precedent |
| joycontrol reconnect PSM values | control `17`, interrupt `19` | upstream implementation fact | `https://github.com/mart1nro/joycontrol/blob/18a09da1a04306534ff9e1df8a1a69c0192a3244/run_controller_cli.py` | matches BTstack PSM values |
| joycontrol reconnect behavior | with reconnect address, create control / interrupt L2CAP sockets and `connect()` to the address / PSM pairs | upstream implementation fact | `https://github.com/mart1nro/joycontrol/blob/18a09da1a04306534ff9e1df8a1a69c0192a3244/joycontrol/server.py` | implementation precedent |
| joycontrol initial pairing behavior | without reconnect address, start HID server, advertise, and wait for Switch in Change Grip / Order | upstream implementation fact | `https://github.com/mart1nro/joycontrol/blob/18a09da1a04306534ff9e1df8a1a69c0192a3244/joycontrol/server.py` | contrast with reconnect path |
| joycontrol reconnect startup behavior | after reconnect sockets are established, send empty input reports until the Switch replies | upstream implementation fact | `https://github.com/mart1nro/joycontrol/blob/18a09da1a04306534ff9e1df8a1a69c0192a3244/joycontrol/server.py` | implementation precedent, not yet swbt policy |
| joycontrol README reconnect note | already connected controller can use reconnect option with Switch Bluetooth MAC, without opening Change Grip / Order | upstream documentation fact | `https://github.com/mart1nro/joycontrol/blob/18a09da1a04306534ff9e1df8a1a69c0192a3244/README.md` | user-facing precedent |
| local_065 hardware boundary | passive link key DB では link key material が保存されなかった | hardware observation | `docs/hardware-test-log.md`, `work-units/complete/local_065/BONDED_RECONNECT_PERSISTENCE.md`, `spec/archive/bond-cache-persistence.md` | source for active reconnect design |

未解決事項:

- joycontrol の empty input report 送信は reconnect startup precedent だが、swbt で必要かは未検証である。採用する場合は hardware item の観測対象に含める。
- BTstack outbound connect が Switch2 `22.1.0` / CSR8510 A10 / WinUSB の今回条件で Change Grip / Order なしに成立するかは未実機検証である。

## 7. 設計メモ

- active reconnect の最小保存状態は、raw link key ではなく Switch Bluetooth address 候補である。
- Switch address は自動取得する。取得タイミングの候補は pairing complete 時点ではなく、少なくとも HID connection opened、できれば report smoke 成立後にする。理由は、単に address が見えただけの失敗 pairing を learned reconnect target として固定しないためである。
- Switch address は設定ファイル layer に永続化する。`local_071` は TOML 入力境界と初期 runtime schema を固定済みであり、手書きの explicit address は `[active_reconnect] switch_address`、daemon-managed learned address は `[active_reconnect.learned] switch_address` として分ける。
- 削除境界は設定ファイル上の値の除去とする。起動時環境変数で reset path や reset flag を与える設計にはしない。
- pairing で新しい Switch address を確認した場合、daemon-managed learned address は上書きしてよい。ただし手書きの explicit address を自動上書きするかは別扱いにし、初期案では上書きしない。
- effective reconnect address の優先順位は、手書き explicit address、daemon-managed learned address、address なしの順にする。将来 address 用の環境変数 override を追加する場合は `local_071` の全体方針に従い、一時 override として最優先に置く。
- Switch Bluetooth address は raw link key ではないが、実機固有情報である。trace log と hardware artifact では検証用に明示してよい。リポジトリへ残す `docs/hardware-test-log.md` や work unit record では必要に応じて scrub する。
- active reconnect は既存 incoming pairing / advertising 経路を壊してはならない。address が不明または reconnect が失敗した場合は、現行の pairing-capable path に戻れる必要がある。
- production adapter に直接 joycontrol 互換ロジックを混ぜ込まない。BTstack outbound connect、address source、operator intent、diagnostics を分ける。
- production active reconnect boundary は `swbt_btstack_production_active_reconnect_request_t` で Switch address、HID control PSM、HID interrupt PSM を表す。production backend は effective reconnect address がある場合だけ power on 後にこの request を出す。現時点では request failure は trace に記録して run loop へ進み、既存 incoming pairing path を止めない。
- `swbt_btstack_hid_event_decode()` は `HID_SUBEVENT_CONNECTION_OPENED` から remote Bluetooth address を decode する。これは learned address capture の入力境界であり、保存の成否や成功判定はまだ扱わない。
- learned address 書き戻しは `swbt_daemon_config_save_active_reconnect_learned_switch_address()` が同一 TOML file を読み、現行 schema で検証してから `[active_reconnect.learned] switch_address` だけを追加または更新する。手書き explicit address は上書きしない。
- learned address 書き戻し API は、設定ファイルが存在しない場合に最小 TOML file を作る。親ディレクトリの自動作成、atomic replace、format / comment preservation の保証はまだ持たない。
- learned address capture の software 成功境界は `HID_SUBEVENT_CONNECTION_OPENED` の `status == 0` とし、report timer が初期化済みの production host 上でだけ保存へ進む。Switch UI が input report を採用したかは software だけでは確認できないため、Button A smoke は実機 item に残す。
- production backend は保存先 target が設定されている場合だけ learned address を同一 TOML file へ渡す。daemon main が config path を収集して target を注入する入口は `local_073` の起動引数 / config path 契約へ残す。
- 設定ファイルへの取り込みは、`local_071` で固定した TOML 入力境界に reconnect 用 key / writer を追加する形で扱う。daemon 起動時の config path 収集は `local_073` へ送る。
- 実機 pass 条件は、L2CAP open だけでは足りない。Button A smoke または同等の input report 採用まで確認する。
- failure は再 pairing と reconnect を分けて記録する。`pairing complete, status 00` が再度出る場合は active reconnect 成功として扱わない。

## 8. 対象ファイル

- `swbt/btstack_bridge/*`
- `swbt/daemon/config.*`
- `swbt/daemon/production_backend.*`
- `apps/swbt-daemon/main.c`
- `tests/btstack_*`
- `tests/daemon_*`
- `spec/references/*`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `work-units/wip/local_072/ACTIVE_SWITCH_RECONNECT.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | source audit records joycontrol-style reconnect assumptions and BTstack outbound L2CAP / HID device connect boundaries | characterization | docs | no |
| refactor-done | daemon config/state can represent explicit and learned Switch Bluetooth addresses from TOML without raw link key storage | new | unit | no |
| refactor-done | production adapter exposes an active reconnect request boundary for HID control PSM `0x11` and interrupt PSM `0x13` | new | unit/integration | no |
| refactor-done | learned Switch address can be persisted to the same TOML config file without overwriting explicit address | new | unit | no |
| refactor-done | HID connection opened event exposes remote Bluetooth address for learned Switch address capture | new | unit | no |
| refactor-skipped | learned Switch address is captured from a successful pairing / connection path and handed to config persistence after the chosen success boundary | new | integration | no |
| refactor-skipped | invalid active reconnect Switch address in TOML is rejected without partially mutating config | edge | unit | no |
| todo | active reconnect failure paths report unavailable / failed states without breaking incoming pairing path | edge | integration | no |
| todo | active reconnect hardware preflight defines initial address capture, daemon restart, active reconnect, optional Switch-side reconnect operation, and Button A smoke | characterization | docs | yes |
| todo | daemon restart active reconnect reaches L2CAP open and Button A smoke without Change Grip / Order re-pairing, or records the exact unsupported boundary | characterization | hardware | yes |

## 10. 検証

一部 software 実装を開始した。active reconnect address の TOML 読み取り、learned address 書き戻し boundary、production adapter の active reconnect request boundary、HID connection opened event からの remote address decode、connection opened 成功 event から保存先 target への learned address 永続化呼び出しは実装済みである。daemon main からの config path 注入、実機 reconnect、request failure の状態分類はまだ実装していない。

起票時確認:

- `rg -n "joycontrol|active reconnect|HID control|PSM 0x11|PSM 0x13|outbound L2CAP" spec docs work-units swbt tests`: local_065 / local_071 / docs/status の新方針と既存 joycontrol references を確認。
- `rg -n "l2cap_create_channel|l2cap_disconnect|L2CAP_EVENT_CHANNEL_OPENED|PSM_HID" vendor\btstack\src\l2cap.h vendor\btstack\src\l2cap.c vendor\btstack\src\classic\hid_device.c vendor\btstack\src\classic\hid_host.c`: BTstack source-audit 起点を確認。

TDD status:
- source: joycontrol `-r` 型 reconnect は、既知 Switch Bluetooth address へ daemon 側から HID control / interrupt L2CAP channel を開きに行く設計候補である。BTstack には Classic outbound L2CAP と HID device outbound connect precedent がある。
- use case: 実装前に、必要な address、PSM、channel open 順、incoming pairing path との差分、local_065 の passive bond cache 不採用理由を work unit record の根拠監査として分類する。
- item: source audit records joycontrol-style reconnect assumptions and BTstack outbound L2CAP / HID device connect boundaries。
- state: refactor-skipped
- commands:
  - `rg -n "l2cap_create_channel|PSM_HID|L2CAP_EVENT_CHANNEL_OPENED|reconnect" vendor\btstack\src\classic\hid_device.c vendor\btstack\src\l2cap.h vendor\btstack\src\l2cap.c`
  - `git ls-tree HEAD vendor/btstack`
  - `rg -n "remote not bonding|pairing complete|bond|daemon restart|sleep|local_065|swbt-bond" docs\hardware-test-log.md work-units\complete\local_065\BONDED_RECONNECT_PERSISTENCE.md spec\archive\bond-cache-persistence.md`
  - upstream joycontrol source review: `mart1nro/joycontrol` `run_controller_cli.py`, `joycontrol/server.py`, README。
- notes: `git -C vendor\btstack rev-parse HEAD` は Windows sandbox user と submodule owner が異なるため `safe.directory` error になった。submodule commit は parent repository の gitlink から `075a0780f0fad7ff67d58ac19f46e8953656a752` と確認した。実機は不要。

Refactor status:
- decision: refactor-skipped
- change: work unit record の根拠監査だけを更新した。production code と test は変更しない。
- unchanged behavior: active reconnect address config、daemon runtime、BTstack bridge、実機挙動は変更しない。
- verification: `git diff --check`
- notes: source facts は implementation candidate であり、outbound connect の swbt 実装方針と実機 pass 条件は後続 item に残す。

2026-06-25 address persistence policy discussion:

- user decision: Switch address は自動取得し、設定ファイル layer に永続化する。
- user decision: 削除境界は設定ファイル上の値の除去とする。
- user decision: trace log では検証用に address を明示してよい。リポジトリ上の記録では scrub する。
- design decision: pairing / connection success で learned address を更新する。手書き explicit address と daemon-managed learned address は分ける。
- design decision: 設定形式は `local_071` で TOML に確定した。

TDD status:
- source: Switch address は自動取得し、設定ファイル layer に永続化する。手書き explicit address と daemon-managed learned address は分け、effective reconnect address は explicit を learned より優先する。
- use case: TOML file に `[active_reconnect] switch_address` と `[active_reconnect.learned] switch_address` がある場合、daemon config は raw link key を持たずに両方の address 文字列を保持し、effective address として explicit address を返す。
- item: daemon config/state can represent explicit and learned Switch Bluetooth addresses from TOML without raw link key storage。
- state: refactor-done
- commands:
  - red: `just build-debug`
  - green: `just build-debug`; `just test-debug`
  - refactor: `just build-debug`; `just test-debug`; `just format`
- notes: red は `swbt_daemon_config_t` に active reconnect address field がなく、`swbt_daemon_config_effective_reconnect_switch_address()` も未実装だったため `daemon_config_file_test` の compile failure になった。green では `[active_reconnect] switch_address` を手書き explicit address、`[active_reconnect.learned] switch_address` を daemon-managed learned address として受け付け、`AA:BB:CC:DD:EE:FF` 形式へ正規化して保持する。effective address は explicit を優先し、explicit がない場合だけ learned を返す。raw link key は保存しない。実機は不要。

Refactor status:
- decision: refactor-done
- change: 手書き address の field / setter 名に `explicit` を入れ、learned address と同じ型の別状態として読めるようにした。
- unchanged behavior: 既存 `report`、`ipc`、`device` TOML key、unknown key reject、environment override precedence、daemon main の起動順序は変更しない。
- verification: `just build-debug`, `just test-debug`
- notes: address の自動取得、同一 TOML file への learned address 書き戻し、invalid address rollback は後続 item に残す。

TDD status:
- source: Switch address は設定ファイル layer に永続化するが、typo や不正形式を有効な reconnect target として保存してはならない。
- use case: TOML file に有効な runtime key と不正な `[active_reconnect] switch_address` が混在する場合、`SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE` で失敗し、既存 config を部分更新しない。
- item: invalid active reconnect Switch address in TOML is rejected without partially mutating config。
- state: refactor-skipped
- commands:
  - characterization: `just build-debug`; `just test-debug`; `just format`
- notes: explicit address と learned address の不正値 test は現行実装で通った。address validation は各 setter に閉じており、config file apply は `next = *config` へ適用してから成功時だけ commit するため、先行する `report.period_us` も部分反映されない。実機は不要。

Refactor status:
- decision: refactor-skipped
- change: test だけを追加した。production code は変更しない。
- unchanged behavior: active reconnect address の正常 TOML 読み取り、explicit 優先、learned fallback、既存 runtime config は維持する。
- verification: `just build-debug`, `just test-debug`
- notes: learned 側の不正値 rollback も characterization test で固定した。production code は変更していない。

TDD status:
- source: 取得した Switch address は同一設定ファイルに daemon-managed learned address として永続化し、手書き explicit address は operator の明示設定として残す。
- use case: 既存 TOML file に `[active_reconnect] switch_address` がある状態で learned address を保存しても explicit address は上書きされず、再読み込み後も effective reconnect address は explicit を優先する。設定ファイルが存在しない場合は、learned address だけを含む最小 TOML file を作れる。
- item: learned Switch address can be persisted to the same TOML config file without overwriting explicit address。
- state: refactor-done
- commands:
  - red: `just build-debug`
  - green: `just build-debug`; `just test-debug`
  - refactor: `just build-debug`; `just test-debug`; `just format`; `just format-check`; `just tidy`; `git diff --check`
- notes: red は `swbt_daemon_config_save_active_reconnect_learned_switch_address()` が未定義だったため `daemon_config_file_test` の compile failure になった。green では `swbt_daemon_config_file_target_t` と保存 API を追加し、既存 TOML を現行 schema で検証してから `[active_reconnect.learned] switch_address` だけを追加または更新する。保存時に address は大文字 `AA:BB:CC:DD:EE:FF` 形式へ正規化する。既存 file の unknown key / 不正値は書き戻し前に失敗させる。実機は不要。

Refactor status:
- decision: refactor-done
- change: 読み取り用の `swbt_daemon_config_file_source_t` と書き込み用の `swbt_daemon_config_file_target_t` を分けた。`required` は read policy であり、write API には持ち込まない。
- unchanged behavior: daemon main の起動順序、config path discovery、environment override precedence、backend selection、hardware approval、diagnostic env、BTstack reconnect は変更しない。
- verification: `just build-debug`, `just test-debug`, `just format-check`, `just tidy`
- notes: writer は同一 path への direct rewrite であり、atomic replace、parent directory auto-create、format / comment preservation guarantee はまだ持たない。

TDD status:
- source: joycontrol `-r` 型 reconnect と BTstack `hid_device_connect()` precedent から、既知 Switch address がある場合に daemon 側から HID reconnect を要求する境界が必要である。
- use case: effective reconnect address が設定されている production daemon は、hardware approval 後に既存 host / HID setup を壊さず、power on 後に Switch address、HID control PSM `0x11`、HID interrupt PSM `0x13` を production adapter の active reconnect port へ渡す。
- item: production adapter exposes an active reconnect request boundary for HID control PSM `0x11` and interrupt PSM `0x13`。
- state: refactor-done
- commands:
  - red: `just build-debug`
  - green: `just build-debug`; `just test-debug`
  - refactor: `just format`; `just build-debug`; `just test-debug`; `just format-check`; `just tidy`; `git diff --check`
- notes: red は `swbt_btstack_production_active_reconnect_request_t`、`swbt_btstack_production_active_reconnect_port_t`、adapter field、PSM 定数が未定義だったため `daemon_production_backend_test` の compile failure になった。green では production adapter に active reconnect port を追加し、production backend が effective address を byte array に変換して power on 後に request するようにした。BTstack 実装は `hid_device_connect()` を使う。実機は不要。

Refactor status:
- decision: refactor-done
- change: active reconnect address parser に長さ確認を追加し、短い文字列で byte parser が範囲外を読まないようにした。BTstack wrapper の fixed-size address copy は `memcpy` ではなく for copy にし、clang-tidy の insecureAPI warning を避けた。
- unchanged behavior: address 未設定時の production startup、hardware approval gate、incoming pairing / advertising setup、report timer lifecycle、shutdown neutral path は変更しない。active reconnect request failure は現時点では fatal にせず、trace 後に run loop へ進む。
- verification: `just build-debug`, `just test-debug`, `just format-check`, `just tidy`
- notes: active reconnect の実機成功、failure state の IPC / diagnostics 反映、pairing / connection success からの learned address capture は後続 item に残す。

TDD status:
- source: BTstack `HID_SUBEVENT_CONNECTION_OPENED` には remote `bd_addr` が含まれる。learned Switch address capture は、この event から address を取れることを前提にする。
- use case: HID connection opened event を decode した typed event は、`hid_cid` と `status` だけでなく、BTstack event 内の remote Bluetooth address を human-order byte array として保持する。
- item: HID connection opened event exposes remote Bluetooth address for learned Switch address capture。
- state: refactor-done
- commands:
  - red: `just build-debug`; `just test-debug`
  - green: `just build-debug`; `just test-debug`
  - refactor: `just format`; `just build-debug`; `just test-debug`; `just format-check`; `just tidy`
- notes: red は `btstack_hid_event_test` の connection opened address assertion だけが失敗した。green では reversed address copy helper を source offset 指定で使える形にし、user confirmation request と connection opened の両方で同じ helper を使うようにした。実機は不要。

Refactor status:
- decision: refactor-done
- change: user confirmation request 専用だった reversed address copy helper を、address byte sequence の先頭を受け取る helper に整理した。
- unchanged behavior: user confirmation request decode、connection opened の `hid_cid` / `status` decode、connection closed / can-send decode は維持する。
- verification: `just build-debug`, `just test-debug`, `just format-check`, `just tidy`
- notes: address を設定ファイルへ保存する trigger と保存先 path はまだ実装していない。daemon 起動時の config path discovery は `local_073` に切り出したままにする。

TDD status:
- source: Switch address は自動取得し、設定ファイル layer に永続化する。単に address が見えただけの失敗 pairing を learned reconnect target として固定しないため、少なくとも HID connection opened 成功後に保存へ渡す。
- use case: production backend が保存先 target を持つ状態で `HID_SUBEVENT_CONNECTION_OPENED` の `status == 0` を受け取った場合、event の remote Bluetooth address を `AA:BB:CC:DD:EE:FF` 形式へ変換し、同一 TOML file の `[active_reconnect.learned] switch_address` へ渡す。target 未設定時は保存しない。
- item: learned Switch address is captured from a successful pairing / connection path and handed to config persistence after the chosen success boundary。
- state: refactor-skipped
- commands:
  - red: `just build-debug`
  - green: `just build-debug`; `just test-debug`; `just format`; `just build-debug`; `just test-debug`; `just format-check`; `just tidy`; `git diff --check`
- notes: red は最初に test helper 不足も検出したため helper を追加し、最終的に `swbt_daemon_production_backend_set_learned_switch_address_target()` 未定義の compile failure に絞った。green では production backend に保存先 target setter を追加し、connection opened 成功 event で `swbt_daemon_config_save_active_reconnect_learned_switch_address()` を呼ぶようにした。`$env:CTEST_ARGS='-R daemon_production_backend_test'; just test-debug` は Dev Container 起動前の `docker ps` で失敗したため test result として扱わず、通常の `just test-debug` で 40/40 pass を確認した。実機は不要。

Refactor status:
- decision: refactor-skipped
- change: none
- unchanged behavior: target 未設定時の connection opened、active reconnect request、hardware approval gate、incoming pairing / advertising、report timer start / stop は維持する。
- verification: `just build-debug`, `just test-debug`, `just format-check`, `just tidy`, `git diff --check`
- notes: config path の収集、CLI / service 起動契約、保存失敗時の operator-facing 状態分類はこの cycle に混ぜず後続 item に残す。
## 11. 実機実行条件

実機が必要である。ただし起票時点では実行しない。

実行前条件:

- `hardware-harness` を読む。
- 人間の明示承認を得る。
- 専用 USB Bluetooth dongle と WinUSB driver assignment を確認する。
- `SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1` を明示する。
- Switch address の取得元、artifact root、診断 trace、HCI dump、cleanup、停止条件を決める。
- `docs/hardware-test-log.md` へ OS、dongle VID/PID、driver、BTstack commit、swbt commit、Switch firmware、結果を記録する。
- raw link key value は記録しない。

想定 hardware run:

- initial connection or address capture run: Switch Bluetooth address を取得し、保存または明示入力の候補として記録する。
- daemon restart active reconnect run: daemon を restart し、Switch 側の再ペアリング画面を使わず active reconnect を試す。
- Switch-side reconnect operation run: 必要な場合だけ Switch 側 controller reconnect 操作を行い、re-pairing と active reconnect を区別する。
- smoke: L2CAP PSM `0x11` / `0x13` open 後、Button A または同等の input report が Switch UI に反映されることを確認する。

## 12. 先送り事項

- 観測: learned address の同一 TOML file 書き戻しは実装したが、atomic replace、parent directory auto-create、format / comment preservation guarantee は持たない。
  先送り理由: 今回は daemon-managed learned address を保存できる最小 config boundary を固定した。耐障害性の高い file update policy は、設定ファイル path / CLI / service 運用と合わせて決める方がよい。
  次の置き場: 後続の設定ファイル運用 spec または CLI launch mode work。
- 観測: learned address を pairing / connection success から自動取得して保存 API へ渡す経路は未実装である。
  先送り理由: 取得タイミングは HID connection opened、report smoke、Switch 側 reconnect 操作の観測と結び付くため、production adapter boundary と実機 preflight で決める必要がある。
  次の置き場: この work unit の integration / hardware item。
- 観測: Switch 側 controller reconnect 操作が必須かどうかは未確認である。
  先送り理由: daemon restart active reconnect の最小 run を先に設計し、操作の要否を artifact で分ける。
  次の置き場: この work unit の hardware item。

## 13. チェックリスト

- [x] source を user discussion、`local_065`、`docs/status.md` から特定した。
- [x] use case を active reconnect として定義した。
- [x] passive bond cache persistence を対象外にした。
- [x] joycontrol `-r` 型 reconnect の source audit を完了した。
- [x] BTstack outbound L2CAP / HID device connect boundary を source audit で固定した。
- [x] Switch address の取得 / 保存 / 削除 boundary を決めた。
- [x] red test または characterization test を追加した。
- [x] learned Switch address の同一 TOML file 書き戻し boundary を実装した。
- [x] HID connection opened 成功 event から learned address を config persistence target へ渡す boundary を実装した。
- [x] active reconnect boundary を実装した。
- [ ] hardware preflight を作成した。
- [ ] 実機結果または未実行理由を記録した。
