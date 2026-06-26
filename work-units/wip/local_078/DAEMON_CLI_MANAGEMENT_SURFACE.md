# Daemon CLI Management Surface

## 1. 概要

`show-config`、`validate-config`、reconnect state cleanup など、daemon の管理用 CLI surface を整理する work unit。

`local_074` は daemon 起動引数に限定し、起動 mode と診断 path を CLI flag へ移した。管理 command は daemon を production run loop へ進める起動とは性質が違うため、この record で分離する。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`: `show-config` / `validate-config` / reconnect state cleanup などの CLI subcommand は有用だが、daemon 起動引数の work unit から分離した先送り事項。
- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`: TOML config file schema と validation 境界。
- `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`: learned Switch address target、`--link-key-db`、pairing-free active reconnect。

use case:

- actor: maintainer、hardware operator、CI。
- 入力または状態: TOML config file、link key DB path、learned Switch address、active reconnect state。
- 期待する観測結果:
  - production run loop に入らず config を表示または検証できる。
  - reconnect state cleanup を daemon 起動と分けて実行できる。
  - CI / smoke は実機 adapter を開かずに management command を検証できる。
- 制約: daemon protocol に `tap`、`duration_ms`、`sequence`、`at_ms` を追加しない。client-side helper の責務は維持する。

## 3. 対象範囲

- `swbt-daemon show-config`、`validate-config`、reconnect state cleanup の必要性と command shape を決める。
- management command が production backend を開かないことを test で固定する。
- config file と environment override の表示範囲を決める。
- learned Switch address と link key DB の削除または確認 command を実装するか判断する。

## 4. 対象外

- GUI client、Python client、C# client。
- daemon IPC protocol への macro executor 追加。
- 複数 controller 同時接続。
- binary release、installer、service manager。
- adapter selector / identity guard。これは `work-units/wip/local_077/ADAPTER_SELECTOR_GUARD.md` で扱う。

## 5. 関連 spec / docs

- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`
- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`
- `docs/status.md`
- `spec/operations/windows-native-preflight.md`

## 6. 根拠監査

not applicable for initial CLI management design if this work unit only reads / validates local config and daemon state files.

source-audit is required if a cleanup command changes Switch protocol behavior, BTstack source selection, or link key handling assumptions beyond the existing local file boundary.

## 7. 設計メモ

- 起動 flag と management subcommand は同じ parser に入る可能性があるが、実行責務は分ける。
- `show-config` / `validate-config` は production backend を初期化しない。
- reconnect state cleanup は、削除対象と復旧手段が明確でない限り destructive command にしない。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `swbt/daemon/launch_options.*`
- `swbt/daemon/config.*`
- `swbt/daemon/config_file.*`
- `tests/daemon_*`
- `docs/status.md`
- `work-units/wip/local_078/DAEMON_CLI_MANAGEMENT_SURFACE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | `validate-config` rejects invalid config without opening production backend | new | unit/integration | no |
| todo | `show-config` reports effective config without requiring hardware | new | unit/integration | no |
| todo | reconnect state cleanup command has an explicit dry-run or confirmation boundary | edge | unit/docs | no |
| todo | management command syntax does not conflict with daemon startup flags | regression | unit | no |

## 10. 検証

not run. This record only captures a deferred work unit source from `local_074`.

## 11. 実機実行条件

初期設計では実機不要。management command は production backend を開かない前提で設計する。将来の command が adapter open、Switch pairing、HID advertising、report loop を含む場合は別 work unit とし、`hardware-harness` の承認境界に従う。

## 12. 先送り事項

none.

## 13. チェックリスト

- [x] source を `local_074` の先送り事項から特定した。
- [x] use case を daemon management command surface として定義した。
- [x] `local_074` の範囲外として分離した。
- [ ] command shape を決めた。
- [ ] TDD Test List を実装前に再確認した。
- [ ] software verification または未実行理由を記録した。
