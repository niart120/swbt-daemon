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
- `work-units/wip/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`

## 6. 根拠監査

未実施。実装前に必須。

この work unit は BTstack outbound L2CAP、HID device reconnect、Switch controller reconnect 操作、実機 HCI event を扱う。Switch-facing report bytes そのものを変える予定はないが、connection lifecycle と実機挙動に触れるため `source-audit` と `hardware-harness` を使う。

初期 source-audit 起点:

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| HID control PSM | `0x11` | BTstack source fact | `vendor/btstack/src/l2cap.h:463`, `vendor/btstack/src/classic/hid_device.c:855-856` | audit seed |
| HID interrupt PSM | `0x13` | BTstack source fact | `vendor/btstack/src/l2cap.h:464`, `vendor/btstack/src/classic/hid_device.c:855-856` | audit seed |
| Classic outbound L2CAP API | `l2cap_create_channel(...)` | BTstack source fact | `vendor/btstack/src/l2cap.h:507`, `vendor/btstack/src/l2cap.c:2569-2577` | audit seed |
| BTstack HID device outbound connect precedent | control PSM を開き、control open 後に interrupt PSM を開く | BTstack source fact | `vendor/btstack/src/classic/hid_device.c:721-722`, `:960-962` | audit seed |
| local_065 hardware boundary | passive link key DB では link key material が保存されなかった | hardware observation | `docs/hardware-test-log.md`, `work-units/complete/local_065/BONDED_RECONNECT_PERSISTENCE.md` | source for new design |

## 7. 設計メモ

- active reconnect の最小保存状態は、raw link key ではなく Switch Bluetooth address 候補である。
- Switch address は自動取得する。取得タイミングの候補は pairing complete 時点ではなく、少なくとも HID connection opened、できれば report smoke 成立後にする。理由は、単に address が見えただけの失敗 pairing を learned reconnect target として固定しないためである。
- Switch address は設定ファイル layer に永続化する。`local_071` では、手書きの explicit address と daemon-managed learned address を同じ TOML file 内の別 key として扱う方針に寄せる。
- 削除境界は設定ファイル上の値の除去とする。起動時環境変数で reset path や reset flag を与える設計にはしない。
- pairing で新しい Switch address を確認した場合、daemon-managed learned address は上書きしてよい。ただし手書きの explicit address を自動上書きするかは別扱いにし、初期案では上書きしない。
- effective reconnect address の優先順位は、手書き explicit address、daemon-managed learned address、address なしの順にする。将来 address 用の環境変数 override を追加する場合は `local_071` の全体方針に従い、一時 override として最優先に置く。
- Switch Bluetooth address は raw link key ではないが、実機固有情報である。trace log と hardware artifact では検証用に明示してよい。リポジトリへ残す `docs/hardware-test-log.md` や work unit record では必要に応じて scrub する。
- active reconnect は既存 incoming pairing / advertising 経路を壊してはならない。address が不明または reconnect が失敗した場合は、現行の pairing-capable path に戻れる必要がある。
- production adapter に直接 joycontrol 互換ロジックを混ぜ込まない。BTstack outbound connect、address source、operator intent、diagnostics を分ける。
- 設定ファイルへの取り込みは、この work unit で Switch address / policy の boundary を決めてから `local_071` へ渡す。
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
| todo | source audit records joycontrol-style reconnect assumptions and BTstack outbound L2CAP / HID device connect boundaries | characterization | docs | no |
| todo | daemon config/state can represent a known Switch Bluetooth address without raw link key storage | new | unit | no |
| todo | production adapter exposes an active reconnect request boundary for HID control PSM `0x11` and interrupt PSM `0x13` | new | unit/integration | no |
| todo | learned Switch address is captured from a successful pairing / connection path and persisted through the config layer without overwriting explicit address | new | integration | no |
| todo | active reconnect failure paths report unavailable / failed states without breaking incoming pairing path | edge | integration | no |
| todo | active reconnect hardware preflight defines initial address capture, daemon restart, active reconnect, optional Switch-side reconnect operation, and Button A smoke | characterization | docs | yes |
| todo | daemon restart active reconnect reaches L2CAP open and Button A smoke without Change Grip / Order re-pairing, or records the exact unsupported boundary | characterization | hardware | yes |
| deferred | config file key for active reconnect state / policy is added after the reconnect boundary is frozen | new | unit | no |

## 10. 検証

起票のみ。実装、software test、実機検証はまだ実行していない。

起票時確認:

- `rg -n "joycontrol|active reconnect|HID control|PSM 0x11|PSM 0x13|outbound L2CAP" spec docs work-units swbt tests`: local_065 / local_071 / docs/status の新方針と既存 joycontrol references を確認。
- `rg -n "l2cap_create_channel|l2cap_disconnect|L2CAP_EVENT_CHANNEL_OPENED|PSM_HID" vendor\btstack\src\l2cap.h vendor\btstack\src\l2cap.c vendor\btstack\src\classic\hid_device.c vendor\btstack\src\classic\hid_host.c`: BTstack source-audit 起点を確認。

2026-06-25 address persistence policy discussion:

- user decision: Switch address は自動取得し、設定ファイル layer に永続化する。
- user decision: 削除境界は設定ファイル上の値の除去とする。
- user decision: trace log では検証用に address を明示してよい。リポジトリ上の記録では scrub する。
- design decision: pairing / connection success で learned address を更新する。手書き explicit address と daemon-managed learned address は分ける。
- design decision: 設定形式は `local_071` で確定する。現時点の第一候補は TOML である。
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

- 観測: Switch address は設定ファイル layer に永続化する方針としたが、key 名、同一ファイル更新時の atomic write、comment / unknown key preservation は未定義である。
  先送り理由: これらは TOML parser / serializer の選定に依存する。active reconnect の意味はこの work unit で固定し、具体的な設定 layer 実装は `local_071` と接続する。
  次の置き場: `work-units/wip/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md` への後続更新。
- 観測: Switch 側 controller reconnect 操作が必須かどうかは未確認である。
  先送り理由: daemon restart active reconnect の最小 run を先に設計し、操作の要否を artifact で分ける。
  次の置き場: この work unit の hardware item。

## 13. チェックリスト

- [x] source を user discussion、`local_065`、`docs/status.md` から特定した。
- [x] use case を active reconnect として定義した。
- [x] passive bond cache persistence を対象外にした。
- [ ] joycontrol `-r` 型 reconnect の source audit を完了した。
- [ ] BTstack outbound L2CAP / HID device connect boundary を source audit で固定した。
- [x] Switch address の取得 / 保存 / 削除 boundary を決めた。
- [ ] red test または characterization test を追加した。
- [ ] active reconnect boundary を実装した。
- [ ] hardware preflight を作成した。
- [ ] 実機結果または未実行理由を記録した。
