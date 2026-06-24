# Bond Cache Persistence

## 1. 状態

current。

この spec は、`local_065` 時点の bond cache 境界を定義する。software boundary は current とする。daemon restart の実機 characterization では、Switch2 `22.1.0` が bonding を要求せず、現行実装では既存 bond reconnect ではなく再 pairing になる境界を確認した。Switch sleep / resume、Switch 側 reconnect 操作で既存 bond が使われることはまだ実機未検証である。

現在の `swbt-bond-<local-bdaddr>.tlv` は swbt 内部の運用 cache であり、release 互換を約束するユーザ向け永続データ契約ではない。

## 2. 目的

daemon restart のたびに Switch 側で再ペアリングが必要になる状態を避けるため、BTstack Classic link key DB に pairing 済み link key material を保持させる。

目的は既存 bond による reconnect であり、汎用 key-value store、公開保存形式、起動時 reset 環境変数を増やすことではない。

## 3. 適用範囲

- `swbt/btstack_bridge/bond_cache.*` の path 決定、TLV-backed link key DB 接続、明示 cleanup API。
- `swbt/btstack_bridge/production_btstack.c` の `HCI_STATE_WORKING` 後の bond cache 接続。
- BTstack TLV file を swbt 内部 cache として扱う判断。
- bond cache を release 互換の外部契約へ格上げする前の保存先、削除、破損時復旧、migration 条件。

次はこの spec の対象外である。

- Switch sleep / resume と Switch 側 reconnect 操作での実機 reconnect 成否の確定。
- daemon restart 後の既存 bond reconnect を成立させるための追加設計。`local_065` の実機観測では `Remote not bonding, dropping local flag` により link key material が TLV DB に保存されなかった。
- 複数 controller 向け bond store。
- PC reboot、USB dongle 抜き差し、adapter removal / reinsertion recovery。
- 設定ファイル format と探索 path の完成。
- operator 向け CLI / IPC cleanup command の完成。

## 4. 決定事項

- Classic reconnect は、pairing で生成された link key と link key type を BTstack link key DB へ保存し、Link Key Request 時に取得できることに依存する。local BD_ADDR、Switch address、device info profile などの公開情報から毎回決定論的に導出できる固定値として扱わない。
- production adapter は、`HCI_STATE_WORKING` event 後に `gap_local_bd_addr` で得た local address から `swbt-bond-<local-bdaddr>.tlv` を作り、BTstack TLV singleton と Classic link key DB を設定する。byte order は、現行実装で `gap_local_bd_addr` から受け取った配列順とする。
- TLV file は BTstack TLV init が作る。swbt は path 決定、TLV / link key DB 接続、終了時 deinit、明示 cleanup 境界を所有する。親 directory 作成と保存先設定は、後続の daemon config work unit で扱う。
- 現行の file name は process working directory 相対である。これは内部運用 cache の暫定実装であり、release 互換の保存先 contract ではない。
- reset / cleanup は daemon 起動時の環境変数で発火させない。必要な場合は、明示 operator command、将来の設定ファイル経由の管理操作、または実機検証手順内の手動 cleanup として扱う。
- bond cache path や storage root を設定可能にする場合は、daemon config abstraction に載せる。設定ファイル対応は `default -> config file -> environment override` の順序へ寄せる。環境変数を追加する場合も、恒久的な reset path ではなく一時 override として削除条件を同じ work unit に記録する。
- raw link key value はログ、docs、PR body、work unit record に転記しない。artifact に残る可能性がある場合は、保存範囲と cleanup を記録する。
- bond cache を外部契約へ格上げするには、保存 root、file naming、削除手段、破損時の復旧、migration note、実機 reconnect 結果、既存 cache を破壊する変更時の扱いを同じ PR または同じ work unit で固定する。

## 5. 根拠

- BTstack source fact: Classic reconnect には link key と link key type が必要であり、Link Key Request では link key DB を参照する。詳細は `work-units/wip/local_065/BONDED_RECONNECT_PERSISTENCE.md` の「初期根拠監査」に記録している。
- BTstack source fact: TLV-backed Classic link key DB は `btstack_link_key_db_tlv_get_instance` で取得する。Windows / POSIX TLV 実装は init 時に file を開き、存在しない場合は作成する。
- swbt implementation fact: `swbt_btstack_bond_cache_link_key_db_from_tlv`、`swbt_btstack_bond_cache_configure_for_local_address`、`swbt_btstack_bond_cache_cleanup_for_local_address` は `swbt/btstack_bridge/bond_cache.*` にある。
- swbt implementation fact: `production_btstack.c` は `HCI_STATE_WORKING` event で bond cache を設定し、`HCI_STATE_OFF` または stop path で TLV / link key DB を外す。
- test evidence: `tests/btstack_bond_cache_test.c` は fake TLV backend の put / reload / get / delete、production path 用の path 決定、起動時環境変数に依存しない cleanup callback を確認する。
- hardware observation: `docs/hardware-test-log.md` の `2026-06-24: local_065 daemon restart reconnect boundary on Switch2` では、`btstack: bond cache configured` は各 run で出たが、HCI dump は `Remote not bonding, dropping local flag` と再起動後の `pairing complete, status 00` を記録した。TLV file は `8` bytes の空 DB のままだった。

## 6. 関連 work units

- `work-units/wip/local_065/BONDED_RECONNECT_PERSISTENCE.md`
- `work-units/wip/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`

## 7. 未解決事項

- Switch sleep / resume と Switch 側 reconnect 操作で既存 bond が使われるか。
- daemon restart 後の既存 bond reconnect を成立させる場合、BTstack vendor patch、Link Key Notification interception、専用 bonding mode のどれを採用できるか。
- release 互換の保存 root、設定ファイル key、operator cleanup command。
- TLV file 破損時の復旧手順。現時点では BTstack TLV init の再作成挙動以上の swbt policy を持たない。
- local BD_ADDR が変わる adapter replacement、PC reboot、USB dongle 抜き差し時の扱い。
