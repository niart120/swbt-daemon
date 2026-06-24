# Bonded Reconnect Persistence

## 1. 概要

daemon 再起動のたびに Switch 側で再ペアリングが必要になる状態を解消する work unit。

目標は、初回 pairing 済みの Switch が、daemon process restart 後に既存 bond を使って再接続できる状態である。主成功条件は、daemon 再起動後に Switch 側の再操作なしで再接続できることとする。追加の characterization として、Switch sleep / resume と Switch 側の controller reconnect 操作でも、再ペアリングではなく既存 bond による再接続になるかを確認する。

BTstack の source と文書では、Bluetooth Classic の reconnect は pairing 時に生成された link key と link key type を保持する前提である。固定値として毎回決定論的に導出できる値として扱う方針は、この work unit の実装方針にしない。daemon が扱う保存データは当面 swbt 内部の bond cache とし、release 互換を約束するユーザ向けデータ契約へ格上げする前に保存先、削除方法、破損時の復旧、migration 方針を決める。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-24: daemon 再起動のたびに Switch 側で再ペアリングが必要になることを避けたい。成功状態は daemon 再起動後に Switch 側の再操作なしで再接続可能であること。実機検証には daemon restart、Switch sleep / resume、Switch 側の再接続操作を含める。
- user clarification, 2026-06-24: TLV file は起動時に自動生成される想定でよい。bond cache の明示設定には賛成するが、reset path を起動時の環境変数依存で与える設計にはしない。起動時設定は将来、環境変数から設定ファイルへ順次寄せる。
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md` の先送り事項: bonded reconnect。
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md` の先送り事項: bond store port。
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md` の先送り事項: bonded reconnect。
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md` の対象外: bonded reconnect persistence。
- `docs/status.md` の未確認項目: daemon 再起動後の bonded reconnect。
- `spec/architecture/daemon-architecture-cutover.md` の external contract: 永続化済み bond data。

use case:

- actor: hardware operator、production daemon、Switch。
- 入力または状態: 初回 pairing 済み Switch、daemon process restart、Switch sleep / resume、Switch 側の controller reconnect 操作、BTstack link key database、dedicated USB Bluetooth dongle。
- 期待する観測結果:
  - daemon restart 後、Switch 側の再操作なしで既存 bond による reconnect が成立する。
  - Switch sleep / resume 後、再ペアリングなしで reconnect できる。
  - Switch 側で controller reconnect 操作をした場合も、再ペアリングではなく既存 bond を使う。
  - 未対応または環境依存の場合は、どの段階で link key が失われるか、どの persistence design が必要かを artifact と docs に記録する。
- 制約: pairing、HID advertising、report loop、sleep / resume 操作は明示承認後だけ実行する。bond cache は秘密値を含み得るため、raw value を docs や PR に転記しない。
- 対象外: 複数 controller、adapter removal / reinsertion recovery、PC reboot、USB dongle 抜き差し、release packaging。

source から use case への変換:

bonded reconnect は architecture cleanup ではなく、BTstack link key persistence と実機観測を伴う feature / characterization work unit として扱う。ユーザ価値は「daemon restart のたびに Switch UI で再ペアリングしないこと」であり、保存形式の抽象化や汎用 key-value store の導入自体は目的ではない。

## 3. 対象範囲

- BTstack の link key DB / TLV persistence API と Windows port の現状を source-audit で確認する。
- swbt production path が TLV-backed link key DB を設定する責務境界を決める。
- bond cache の保存先、明示的な削除操作、破損時の復旧、release 互換を約束する契約へ格上げする条件を設計する。
- fake bond store / fake link key DB unit test を追加する。
- 実機 reconnect 手順を `hardware-harness` に沿って定義し、daemon restart、Switch sleep / resume、Switch 側 reconnect 操作を含める。
- 実機を実行する場合は `docs/hardware-test-log.md` に結果を記録する。

## 4. 対象外

- 複数 controller 同時接続。
- Joy-Con、NFC / IR semantic support。
- adapter removal / reinsertion recovery。
- PC reboot。
- USB dongle 抜き差し。
- release artifact 作成。

## 5. 関連 spec / docs

- `spec/architecture/daemon-architecture-cutover.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`

## 6. 根拠監査

実装前に必須。

BTstack link key DB、bond persistence、Windows port behavior、reconnect event handling に触れるため、実装前に `source-audit` を使う。Switch protocol byte や report period を追加する予定はないが、BTstack persistence API と実機観測値は根拠を分けて記録する。

### 初期根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| Classic reconnect に必要な情報 | link key と link key type | source fact | `vendor/btstack/doc/manual/docs-template/how_to.md:1154-1157`, `vendor/btstack/doc/manual/docs-template/profiles.md:172-173` | stable source fact |
| BTstack link key DB interface | get / put / delete / iterator | source fact | `vendor/btstack/src/classic/btstack_link_key_db.h:41`, `:88`, `:96`, `:102` | stable source fact |
| TLV-backed Classic link key DB | BTstack TLV storage を使う | source fact | `vendor/btstack/src/classic/btstack_link_key_db_tlv.h:41`, `:63`, `vendor/btstack/src/classic/btstack_link_key_db_tlv.c:49-51`, `:221` | stable source fact |
| pairing 後の link key 保存条件 | bondable、bonding request、security level を満たした場合に DB へ保存 | source fact | `vendor/btstack/src/hci.c:4797-4812` | stable source fact |
| reconnect 時の link key lookup | Link Key Request で DB から取得し、あれば reply、なければ negative reply | source fact | `vendor/btstack/src/hci.c:4769-4775`, `:7950-7962` | stable source fact |
| Windows BTstack port の TLV 接続例 | `btstack_<local-bdaddr>.tlv` を開き、`hci_set_link_key_db(btstack_link_key_db_tlv_get_instance(...))` を設定 | source fact | `vendor/btstack/port/windows-h4/main.c:362-378` | stable source fact |
| Windows TLV file creation | init 時に既存 file を開き、存在しないか不正なら `CREATE_ALWAYS` で作る | source fact | `vendor/btstack/platform/windows/btstack_tlv_windows.c:196`, `:274`, `:314` | stable source fact |
| POSIX TLV file creation | init 時に既存 file を `r+` で開き、存在しないか不正なら `w+` で作る | source fact | `vendor/btstack/platform/posix/btstack_tlv_posix.c:182`, `:251`, `:279` | stable source fact |
| swbt production path の現状 | `hci_init` と discovery / L2CAP 初期化はあるが、link key DB 設定はない | implementation fact | `swbt/btstack_bridge/production_btstack.c:139`, `:156`, `:168`; repo-wide `rg hci_set_link_key_db` は swbt 側 0 件 | missing implementation |
| source inclusion | BTstack Classic source は CMake 選択に含まれ、TLV platform source は swbt link source に追加済み | implementation fact | `cmake/btstack_sources.cmake:60`, `CMakeLists.txt` の `btstack_tlv_posix.c` / `btstack_tlv_windows.c` 追加 | wired for build |
| production link key DB wiring | `HCI_STATE_WORKING` event で local BD_ADDR から `swbt-bond-<bdaddr>.tlv` を決め、BTstack TLV singleton と Classic link key DB を設定する | implementation fact | `swbt/btstack_bridge/production_btstack.c`, `swbt/btstack_bridge/bond_cache.c`, `tests/btstack_bond_cache_test.c` | software-verified, hardware unverified |

### 未解決事項

- BTstack の desktop port 例は HCI state working 後、local BD_ADDR から TLV path を決める。swbt production path では、どの event boundary で local BD_ADDR を得て link key DB を設定するかを実装前に決める。
- TLV file は BTstack TLV init が作るが、親 directory の作成は swbt 側の責務である。
- controller 内部の link key storage に依存できるかは未検証であり、この work unit の対応済み挙動にはしない。対応済み挙動は swbt / BTstack 側の link key DB で説明できる必要がある。

## 7. 設計メモ

- bond data は external contract になり得るため、保存形式を軽く決めない。
- source fact、swbt implementation fact、hardware observation、推定を分ける。
- H1 の pass は初回 pairing / current connection の証跡であり、restart 後 reconnect の証明ではない。
- bond store port を入れる場合は、BTstack adapter boundary 内に留め、application state と混ぜない。
- link key は pairing で生成される秘密値として扱う。local BD_ADDR、Switch address、device info profile などから毎回固定値として導出できる値とは扱わない。
- bond cache はまず swbt 内部運用データとする。ユーザ向け永続データ契約に格上げする場合は、保存先、削除手段、互換性、破損時の復旧、migration note を同じ work unit または後続 work unit で固定する。
- TLV file 自体は、保存先 path が決まっていれば BTstack TLV init 時に作られる。swbt は保存先 directory の作成、path 決定、DB 接続、終了時 deinit を所有する。
- reset は起動時の環境変数で発火させない。必要なら明示的な operator command、将来の設定ファイル経由の管理操作、または実機検証手順内の手動 cleanup として扱う。
- bond cache の保存先設定は daemon config abstraction に載せる。新しい起動時環境変数を増やす場合は、一時的な互換入力であることと設定ファイル移行の削除条件を同時に記録する。
- raw link key value はログ、docs、PR body、work unit record へ転記しない。artifact に残す場合も access scope と cleanup を記録する。

## 8. 対象ファイル

- `swbt/btstack_bridge/*`
- `swbt/daemon/production_backend.*`
- `swbt/daemon/config.*`
- `cmake/btstack_sources.cmake`
- `tests/btstack_*`
- `tests/daemon_production_backend_test.c`
- `spec/references/*`
- `spec/architecture/daemon-architecture-cutover.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `work-units/wip/local_065/BONDED_RECONNECT_PERSISTENCE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | source audit records that Classic reconnect requires stored link key material and does not rely on deterministic derivation from public device data | characterization | docs | no |
| refactor-skipped | fake link key DB stores, reloads, and deletes link key material without application ownership | new | unit | no |
| refactor-skipped | production BTstack adapter wires TLV-backed link key DB at the chosen HCI / local-address boundary before link-key request handling | new | integration | no |
| todo | explicit bond cache cleanup removes swbt-owned bond data without relying on startup environment variables | new | unit | no |
| todo | daemon process restart after initial pairing reconnects without Switch-side operation, or records the exact unsupported boundary with artifact | characterization | hardware | yes |
| todo | Switch sleep / resume reconnects without re-pairing, or records the exact unsupported boundary with artifact | characterization | hardware | yes |
| todo | Switch-side controller reconnect operation uses existing bond without full re-pairing | characterization | hardware | yes |
| todo | bond cache storage / cleanup / migration policy is documented before treating persistence as a release-compatible external contract | characterization | docs | no |

## 10. 検証

2026-06-24 時点では、BTstack source と swbt implementation の初期根拠監査を実施した。fake TLV backend による link key DB unit test と、production adapter が `HCI_STATE_WORKING` 後に TLV-backed link key DB を設定する software test を追加し、software test は通過した。実機検証はまだ実行していない。

TDD status:

- source: user request, 2026-06-24; `docs/status.md` の daemon 再起動後 bonded reconnect 未確認項目。
- use case: daemon restart 後に Switch 側の再操作なしで既存 bond による reconnect が成立する。
- item: source audit records that Classic reconnect requires stored link key material and does not rely on deterministic derivation from public device data。
- state: refactor-skipped。
- commands:
  - `rg -n "link key|deterministic|固定値|決定論的" work-units/wip/local_065/BONDED_RECONNECT_PERSISTENCE.md`
  - `git diff --check`
- notes: `source-audit` により BTstack link key DB、TLV persistence、swbt production path の未接続状態を記録済み。docs characterization item であり CMake / CTest は対象外。構造変更は不要のため refactor は行わない。

TDD status:

- source: user request, 2026-06-24; `local_053` の bond store port 先送り事項。
- use case: daemon ではなく BTstack link key DB が link key material を保存、再読込、削除する。
- item: fake link key DB stores, reloads, and deletes link key material without application ownership。
- state: refactor-skipped。
- commands:
  - `just configure-debug`
  - `just build-debug` red: `btstack_bridge/bond_cache.h` が未実装のため `btstack_bond_cache_test` compile failure。
  - `just build-debug` green。
  - `just format`
  - `git diff --check`
  - `just test-debug` green: 40/40 passed。
- notes: `swbt_btstack_bond_cache_link_key_db_from_tlv` は BTstack の `btstack_link_key_db_tlv_get_instance` に TLV backend を渡すだけにし、swbt application state では raw link key を保持しない。`CTEST_ARGS='-R btstack_bond_cache_test' just test-debug` は Windows 側 Dev Container setup で失敗したため、対象テストを含む full `just test-debug` を検証根拠にした。

Refactor status:

- decision: refactor-skipped。
- change: なし。`just format` による機械的整形のみ。
- unchanged behavior: fake TLV backend 経由の put / reload / get / delete、invalid argument rejection。
- verification: `just build-debug`、`just test-debug`。
- notes: この cycle では production wiring、path 決定、cleanup policy へ進めない。

TDD status:

- source: `local_053` の bond store port 先送り事項、`docs/status.md` の daemon 再起動後 bonded reconnect 未確認項目。
- use case: HCI が working になった後、link key request が来る前に production adapter が TLV-backed Classic link key DB を BTstack へ渡す。
- item: production BTstack adapter wires TLV-backed link key DB at the chosen HCI / local-address boundary before link-key request handling。
- state: refactor-skipped。
- commands:
  - `just build-debug` red: `swbt_btstack_bond_cache_configure_for_local_address` などの production wiring API が未定義。
  - `just build-debug` green。
  - `just format`
  - `git diff --check`
  - `just test-debug` green: 40/40 passed。
  - `just windows-cross` green。
- notes: production adapter は `HCI_STATE_WORKING` event で `gap_local_bd_addr` を読み、`swbt-bond-<bdaddr>.tlv` を開いて BTstack TLV singleton と Classic link key DB を設定する。実機で Link Key Request より前にこの event ordering になることは未検証。

Refactor status:

- decision: refactor-skipped。
- change: `HCI_STATE_WORKING` event の重複 configure を避ける guard のみ。
- unchanged behavior: platform start、HID registration、report loop、HCI dump start の既存 test は green。
- verification: `just build-debug`、`just test-debug`、`just windows-cross`。
- notes: 保存先の外部契約化、設定ファイル連携、明示 cleanup は別 item として残す。

## 11. 実機実行条件

実機が必要である。

実行前条件:

- `hardware-harness` を読む。
- 人間の明示承認を得る。
- 専用 USB Bluetooth dongle と WinUSB driver assignment を確認する。
- `SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1` を明示する。
- pairing / daemon restart / Switch sleep-resume / Switch 側 reconnect / cleanup の手順と artifact path を決める。
- `docs/hardware-test-log.md` へ OS、dongle VID/PID、driver、BTstack commit、swbt commit、Switch firmware、結果を記録する。
- raw link key value は記録しない。bond cache の path、存在有無、reset 結果だけを記録する。

## 12. 先送り事項

none。起票時点の先送り事項は、この record の source として取り込んだ。

## 13. チェックリスト

- [x] source を `local_050`、`local_053`、`local_055`、`local_057`、`docs/status.md` から特定した。
- [x] use case を再ペアリング回避の bonded reconnect persistence として定義した。
- [x] 初期 `source-audit` で BTstack link key DB と swbt 未接続状態を確認した。
- [x] 実装前の詳細 `source-audit` を完了した。
- [ ] hardware preflight を確認した。
- [x] red test または characterization test を追加した。
- [ ] link key DB / bond cache 実装または unsupported 判断を記録した。
- [ ] 実機結果または未実行理由を記録した。
