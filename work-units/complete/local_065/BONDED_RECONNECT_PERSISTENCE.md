# Bonded Reconnect Persistence

## 1. 概要

daemon restart のたびに Switch 側で再ペアリングが必要になる状態を避けるため、passive な TLV-backed Classic link key DB / bond cache 経路を調査した work unit。

結論は不採用である。BTstack の link key DB と TLV persistence は software test で接続できたが、Switch2 `22.1.0` との実機条件では Switch が bonding を要求せず、link key material は保存されなかった。daemon restart と Switch sleep / resume では L2CAP open と Button A smoke は成立したが、再接続時にも `pairing complete, status 00` が出たため、既存 bond reconnect ではなく再 pairing と判断する。

このため、`swbt/btstack_bridge/bond_cache.*`、`tests/btstack_bond_cache_test.c`、production adapter の TLV-backed link key DB 接続、CMake の専用 build graph は削除した。調査結果は後続の active reconnect work unit の source として残す。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-24: daemon 再起動のたびに Switch 側で再ペアリングが必要になることを避けたい。成功状態は daemon 再起動後に Switch 側の再操作なしで再接続可能であること。実機検証には daemon restart、Switch sleep / resume、Switch 側の再接続操作を含める。
- user clarification, 2026-06-24: TLV file は起動時に自動生成される想定でよい。bond cache の明示設定には賛成するが、reset path を起動時の環境変数依存で与える設計にはしない。起動時設定は将来、環境変数から設定ファイルへ順次寄せる。
- user direction, 2026-06-25: 改修した実装を残すと dead code 化しやすいため、コミット履歴には残しても現在の tree には残置しない。
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md` の先送り事項: bonded reconnect。
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md` の先送り事項: bond store port。
- `docs/status.md` の未確認項目: daemon 再起動後の reconnect。

use case:

- actor: hardware operator、production daemon、Switch。
- 入力または状態: 初回 pairing 済み Switch、daemon process restart、Switch sleep / resume、BTstack link key database、dedicated USB Bluetooth dongle。
- 期待する観測結果:
  - daemon restart 後、Switch 側の再操作なしで既存 bond による reconnect が成立する。
  - Switch sleep / resume 後、再ペアリングなしで reconnect できる。
  - 未対応または環境依存の場合は、どの段階で link key が保存されないか、どの後続設計が必要かを artifact と docs に記録する。
- 制約: pairing、HID advertising、report loop、sleep / resume 操作は明示承認後だけ実行する。bond cache は秘密値を含み得るため、raw value を docs や PR に転記しない。
- 対象外: 複数 controller、adapter removal / reinsertion recovery、PC reboot、USB dongle 抜き差し、release packaging。

source から use case への変換:

この work unit は最初、Classic link key persistence による再接続を狙った。実機観測により、今回条件では Switch が bonding せず DB に保存する key material がないことが分かったため、実装済み経路を削除し、後続は joycontrol `-r` 型の active reconnect として扱う。

## 3. 対象範囲

- BTstack の link key DB / TLV persistence API と Windows port の現状を source-audit で確認する。
- swbt production path が TLV-backed link key DB を設定できるかを software test で確認する。
- daemon restart、Switch sleep / resume の実機 boundary を記録する。
- passive bond cache 経路を採用しない判断を spec / docs / work unit に記録する。
- 採用しない実装を source、test、CMake、production adapter から削除する。

## 4. 対象外

- active reconnect の実装。
- Switch 側 controller reconnect 操作の追加実機 run。
- 複数 controller 同時接続。
- Joy-Con、NFC / IR semantic support。
- adapter removal / reinsertion recovery。
- PC reboot。
- USB dongle 抜き差し。
- release artifact 作成。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `spec/archive/bond-cache-persistence.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `work-units/wip/local_072/ACTIVE_SWITCH_RECONNECT.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`

## 6. 根拠監査

実施済み。

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| Classic reconnect に必要な情報 | link key と link key type | source fact | `vendor/btstack/doc/manual/docs-template/how_to.md:1154-1157`, `vendor/btstack/doc/manual/docs-template/profiles.md:172-173` | stable source fact |
| BTstack link key DB interface | get / put / delete / iterator | source fact | `vendor/btstack/src/classic/btstack_link_key_db.h:41`, `:88`, `:96`, `:102` | stable source fact |
| TLV-backed Classic link key DB | BTstack TLV storage を使う | source fact | `vendor/btstack/src/classic/btstack_link_key_db_tlv.h:41`, `:63`, `vendor/btstack/src/classic/btstack_link_key_db_tlv.c:49-51`, `:221` | stable source fact |
| pairing 後の link key 保存条件 | bondable、bonding request、security level を満たした場合に DB へ保存 | source fact | `vendor/btstack/src/hci.c:4797-4812` | stable source fact |
| reconnect 時の link key lookup | Link Key Request で DB から取得し、あれば reply、なければ negative reply | source fact | `vendor/btstack/src/hci.c:4769-4775`, `:7950-7962` | stable source fact |
| Windows BTstack port の TLV 接続例 | `btstack_<local-bdaddr>.tlv` を開き、`hci_set_link_key_db(btstack_link_key_db_tlv_get_instance(...))` を設定 | source fact | `vendor/btstack/port/windows-h4/main.c:362-378` | stable source fact |
| 実機観測 | Switch2 `22.1.0` は今回条件で bonding を要求せず、TLV DB に link key material は保存されなかった | hardware observation | `docs/hardware-test-log.md` local_065 entries | unsupported boundary |

## 7. 設計メモ

- link key は pairing で生成される秘密値であり、local BD_ADDR、Switch address、device info profile などから毎回固定値として導出できる値とは扱わない。
- TLV file 自体は、保存先 path が決まっていれば BTstack TLV init 時に作られる。
- reset は起動時の環境変数で発火させない。必要なら明示的な operator command、将来の設定ファイル経由の管理操作、または実機検証手順内の手動 cleanup として扱う。
- passive bond cache は、今回の Switch2 `22.1.0` 条件では目的を満たさない。現行 tree に残さない。
- 後続は Switch address を既知状態として扱い、daemon 側から HID control / interrupt channel を開きに行く active reconnect を調べる。
- raw link key value はログ、docs、PR body、work unit record へ転記しない。

## 8. 対象ファイル

- `swbt/btstack_bridge/production_btstack.c`
- `swbt/btstack_bridge/bond_cache.c`（削除）
- `swbt/btstack_bridge/bond_cache.h`（削除）
- `tests/btstack_bond_cache_test.c`（削除）
- `CMakeLists.txt`
- `spec/archive/bond-cache-persistence.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_065/BONDED_RECONNECT_PERSISTENCE.md`
- `work-units/wip/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/wip/local_072/ACTIVE_SWITCH_RECONNECT.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | source audit records that Classic reconnect requires stored link key material and does not rely on deterministic derivation from public device data | characterization | docs | no |
| refactor-skipped | TLV-backed link key DB can be connected in software, but does not make Switch2 `22.1.0` persist link key material in the observed condition | characterization | unit/hardware | yes |
| refactor-skipped | daemon process restart after initial pairing records the unsupported boundary with artifact | characterization | hardware | yes |
| refactor-skipped | Switch sleep / resume records the unsupported boundary with artifact | characterization | hardware | yes |
| refactor-done | unadopted bond cache implementation is removed from source, tests, production wiring, and CMake build graph | cleanup | build | no |
| deferred | Switch-side controller reconnect operation is handled by active reconnect work instead of passive bond cache persistence | characterization | hardware | yes |

## 10. 検証

実装調査中の software gate:

- `just build-debug` green。
- `just test-debug` green: 40/40 passed。
- `just windows-cross` green。

実機 gate:

- `just windows-cross` green。
- `& .\tmp\hardware\local_065\run-daemon-restart-reconnect.ps1`: `summary.json` の `pass=false`。artifact: `tmp/hardware/local_065/20260624-234907-daemon-restart-reconnect`。
- `& .\tmp\hardware\local_065\run-sleep-resume-reconnect.ps1`: `summary.json` の `pass=false`。artifact: `tmp/hardware/local_065/20260625-000432-sleep-resume-reconnect`。

実機結果:

- daemon restart run: `initial-pairing` と `daemon-restart-reconnect` はどちらも L2CAP `0x11` / `0x13` open と Button A client exit `0` まで進んだ。restart 側にも `pairing complete, status 00` が `1` 件出た。両 run は `Remote not bonding, dropping local flag` を記録し、`swbt-bond-00-1b-dc-f9-9f-7d.tlv` は `8` bytes の空 DB のままだった。
- sleep / resume run: `final_l2cap_open_count=4` と `after_resume_client_ran=true` により、resume 後の接続復帰と Button A smoke は成立した。HCI dump は初回接続と resume 後の再接続で `pairing complete, status 00` を計 `2` 件、`Remote not bonding, dropping local flag` を計 `2` 件記録した。`swbt-bond-00-1b-dc-f9-9f-7d.tlv` は `8` bytes の空 DB のままだった。

cleanup 後の software verification:

- `scripts/format.sh`: pass。
- `git diff --check`: pass。Windows checkout の LF / CRLF warning のみ。
- `rg -n "bond_cache|btstack_bond_cache_test|hci_set_link_key_db|btstack_tlv|swbt/btstack_bridge/bond_cache" CMakeLists.txt swbt tests`: no matches。
- `just build-debug`: pass。
- `just test-debug`: pass, 39/39 tests passed。
- `just windows-cross`: pass。

## 11. 実機実行条件

実機 run は実行済み。実行時の条件は次の通り。

- approval: ユーザは 2026-06-24 に実機実験を承認済み。
- approved scope: Bluetooth adapter open、HID advertising、pairing、report loop、daemon restart、Switch sleep / resume、HCI dump / diagnostic trace 保存、cleanup 確認。
- backend: Windows native `windows-winusb`、`build/windows-mingw-debug/swbt-daemon.exe`。
- environment: `SWBT_DAEMON_BACKEND=production`, `SWBT_RUN_HARDWARE=1`, `SWBT_HARDWARE_APPROVED=1`, `SWBT_IPC_HOST=127.0.0.1`, `SWBT_IPC_PORT=37637`, `SWBT_REPORT_PERIOD_US=8000`, `SWBT_DIAGNOSTIC_TRACE_PATH`, `SWBT_CRASH_DUMP_PATH`, `SWBT_HCI_DUMP_TRACE_PATH`。
- safety: raw link key value は表示、転記、commit していない。

この cleanup では新しい実機 run を行わない。Switch-facing behavior は変更せず、未採用の passive bond cache 経路を削除するだけである。

## 12. 先送り事項

- 観測: passive TLV-backed link key DB では、今回条件の Switch2 `22.1.0` に link key material を保存させられなかった。
  先送り理由: link key DB の保存先や cleanup を詰めても、保存する key material がない。
  次の置き場: `work-units/wip/local_072/ACTIVE_SWITCH_RECONNECT.md`。
- 観測: Switch 側 controller reconnect 操作は、この work unit では実行しない。
  先送り理由: passive bond cache 経路を採用しないため、同じ設計で追加実機 run を行っても目的に対する根拠が弱い。
  次の置き場: `work-units/wip/local_072/ACTIVE_SWITCH_RECONNECT.md`。
- 観測: reconnect に必要な Switch address の取得、保存、削除、設定ファイル化は未定義である。
  先送り理由: active reconnect の接続経路と一緒に設計する必要がある。
  次の置き場: `work-units/wip/local_072/ACTIVE_SWITCH_RECONNECT.md`。

## 13. チェックリスト

- [x] source を `local_050`、`local_053`、`docs/status.md`、user request から特定した。
- [x] use case を再ペアリング回避の reconnect として定義した。
- [x] 初期 `source-audit` で BTstack link key DB と swbt 未接続状態を確認した。
- [x] passive TLV-backed link key DB 経路を software test で確認した。
- [x] hardware preflight を確認した。
- [x] daemon restart 実機結果を記録した。
- [x] Switch sleep / resume 実機結果を記録した。
- [x] passive bond cache 経路を不採用にした。
- [x] 不採用実装を source / test / CMake から削除した。
- [x] 後続 active reconnect work unit を作成する。
