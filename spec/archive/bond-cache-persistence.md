# Bond Cache Persistence

## 1. 状態

superseded。

この spec は、`local_065` で検討した TLV-backed Classic link key DB / bond cache 経路の記録である。software implementation は `local_065` の後で削除した。現行アーキテクチャは `swbt-bond-<local-bdaddr>.tlv`、BTstack TLV singleton、Classic link key DB の永続化経路を持たない。

削除理由は、実機観測で Switch2 `22.1.0` が bonding を要求せず、link key material が保存されなかったためである。このまま残すと、daemon restart / sleep-resume reconnect に効かない dead code になりやすい。

## 2. 当時の目的

daemon restart のたびに Switch 側で再ペアリングが必要になる状態を避けるため、BTstack Classic link key DB に pairing 済み link key material を保持させる設計だった。

目的は既存 bond による reconnect であり、汎用 key-value store、公開保存形式、起動時 reset 環境変数を増やすことではなかった。

## 3. 当時の設計範囲

- `swbt/btstack_bridge/bond_cache.*` の path 決定、TLV-backed link key DB 接続、明示 cleanup API。
- `swbt/btstack_bridge/production_btstack.c` の `HCI_STATE_WORKING` 後の bond cache 接続。
- BTstack TLV file を swbt 内部 cache として扱う判断。
- bond cache を release 互換の外部契約へ格上げする前の保存先、削除、破損時復旧、migration 条件。

次は当時も対象外だった。

- Switch 側 reconnect 操作での実機 reconnect 成否の確定。
- daemon restart と Switch sleep / resume 後の既存 bond reconnect を成立させるための追加設計。
- 複数 controller 向け bond store。
- PC reboot、USB dongle 抜き差し、adapter removal / reinsertion recovery。
- 設定ファイル format と探索 path の完成。
- operator 向け CLI / IPC cleanup command の完成。

## 4. 得られた判断

- Classic reconnect には、pairing で生成された link key と link key type が必要である。local BD_ADDR、Switch address、device info profile などの公開情報から毎回決定論的に導出できる値としては扱えない。
- BTstack の TLV-backed link key DB は software test では接続できるが、Switch2 `22.1.0` との今回条件では link key material が保存されなかった。
- daemon restart と Switch sleep / resume では L2CAP open と Button A smoke は成立した。ただし HCI dump には再接続時にも `pairing complete, status 00` が出たため、既存 bond reconnect ではなく再 pairing と判断する。
- `swbt-bond-00-1b-dc-f9-9f-7d.tlv` は実機 run 後も `8` bytes の空 DB のままだった。
- raw link key value は docs、PR body、work unit record に転記しない方針を維持する。
- 今後 reconnect を扱う場合は、passive な link key DB 接続ではなく、Switch address を既知状態として扱い、daemon 側から HID control / interrupt channel を開きに行く active reconnect 経路を別 work unit で定義する。

## 5. 根拠

- source fact: Classic reconnect は link key と link key type を必要とし、Link Key Request で link key DB を参照する。詳細は `work-units/complete/local_065/BONDED_RECONNECT_PERSISTENCE.md` の根拠監査を正本にする。
- source fact: TLV-backed Classic link key DB は `btstack_link_key_db_tlv_get_instance` で取得する。Windows / POSIX TLV 実装は init 時に file を開き、存在しない場合は作成する。
- removed implementation fact: `swbt_btstack_bond_cache_link_key_db_from_tlv`、`swbt_btstack_bond_cache_configure_for_local_address`、`swbt_btstack_bond_cache_cleanup_for_local_address`、`tests/btstack_bond_cache_test.c` は `local_065` の調査中に存在したが、現行 tree から削除済みである。
- hardware observation: `docs/hardware-test-log.md` の `2026-06-24: local_065 daemon restart reconnect boundary on Switch2` では、`Remote not bonding, dropping local flag` と再起動後の `pairing complete, status 00` を記録した。
- hardware observation: `docs/hardware-test-log.md` の `2026-06-25: local_065 Switch sleep/resume reconnect boundary on Switch2` では、sleep / resume 後の L2CAP open と Button A smoke は成立したが、初回接続と resume 後の再接続で `pairing complete, status 00` を計 `2` 件記録した。

## 6. 関連 work units

- `work-units/complete/local_065/BONDED_RECONNECT_PERSISTENCE.md`
- `work-units/wip/local_072/ACTIVE_SWITCH_RECONNECT.md`
- `work-units/wip/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md`
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md`

## 7. 未解決事項

- Switch 側 reconnect 操作で既存 bond が使われるかは `local_065` では未実行のまま閉じた。
- active reconnect に必要な Switch Bluetooth address の取得、保存、設定、削除の境界。
- BTstack で daemon 側から HID control PSM `0x11` / interrupt PSM `0x13` を開く実装境界。
- release 互換の保存 root、設定ファイル key、operator cleanup command。
