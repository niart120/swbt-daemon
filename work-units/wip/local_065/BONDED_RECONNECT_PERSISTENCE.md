# Bonded Reconnect Persistence

## 1. 概要

daemon 再起動後の bonded reconnect と link key persistence を扱う work unit。

rearchitecture 中の deferred item では、bonded reconnect、bond store、adapter recovery が繰り返し残った。`docs/status.md` でも「daemon 再起動後の bonded reconnect」は未確認項目である。

この work unit では、BTstack の bond database / link key persistence を source-audit で確認し、実機手順と software boundary を分けてから実装可否を判断する。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md` の先送り事項: bonded reconnect。
- `work-units/complete/local_053/BTSTACK_PORT_EVENT_BOUNDARY.md` の先送り事項: bond store port。
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md` の先送り事項: bonded reconnect。
- `work-units/complete/local_057/ARCHITECTURE_CUTOVER_H1.md` の対象外: bonded reconnect persistence。
- `docs/status.md` の未確認項目: daemon 再起動後の bonded reconnect。
- `spec/architecture/daemon-architecture-cutover.md` の external contract: 永続化済み bond data。

use case:

- actor: hardware operator、production daemon、Switch。
- 入力または状態: 初回 pairing 済み Switch、daemon restart、BTstack link key database、dedicated USB Bluetooth dongle。
- 期待する観測結果: daemon restart 後、既存 bond に基づく reconnect が成立するか、未対応なら明確な failure と必要な persistence design が記録される。
- 制約: pairing と HID advertising は明示承認後だけ実行する。永続化済み bond data は external contract として migration 方針を決める。
- 対象外: 複数 controller、adapter removal / reinsertion recovery、sleep / resume、release packaging。

source から use case への変換:

bonded reconnect は architecture cleanup ではなく、BTstack link key persistence と実機観測を伴う feature / characterization work unit として扱う。

## 3. 対象範囲

- BTstack の link key DB / TLV persistence API と Windows port の現状を source-audit で確認する。
- swbt 側に bond store port が必要か判断する。
- 既存 bond data の保存先、migration、cleanup 方針を設計する。
- fake bond store unit test を追加する。
- 実機 reconnect 手順を `hardware-harness` に沿って定義する。
- 実機を実行する場合は `docs/hardware-test-log.md` に結果を記録する。

## 4. 対象外

- 複数 controller 同時接続。
- Joy-Con、NFC / IR semantic support。
- adapter removal / reinsertion recovery。
- sleep / resume recovery。
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

required before implementation。

BTstack link key DB、bond persistence、Windows port behavior、reconnect event handling に触れるため、実装前に `source-audit` を使う。Switch protocol byte や report period を追加する予定はないが、BTstack persistence API と実機観測値は根拠を分けて記録する。

## 7. 設計メモ

- bond data は external contract になり得るため、保存形式を軽く決めない。
- source fact、swbt implementation fact、hardware observation、推定を分ける。
- H1 の pass は初回 pairing / current connection の証跡であり、restart 後 reconnect の証明ではない。
- bond store port を入れる場合は、BTstack adapter boundary 内に留め、application state と混ぜない。

## 8. 対象ファイル

- `swbt/btstack_bridge/*`
- `swbt/daemon/production_backend.*`
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
| todo | BTstack link key DB source audit records persistence API and Windows behavior | characterization | docs | no |
| todo | fake bond store persists and reloads link key material without application ownership | new | unit | no |
| todo | production adapter initializes bond persistence before HID advertising when enabled | new | integration | no |
| todo | daemon restart with existing bond reconnects or records unsupported behavior with artifact | characterization | hardware | yes |
| todo | bond data cleanup / migration policy is documented before treating persistence as external contract | characterization | docs | no |

## 10. 検証

未実行。起票のみで、source-audit、実装、実機検証はまだ実行していない。

## 11. 実機実行条件

実機が必要である。

実行前条件:

- `hardware-harness` を読む。
- 人間の明示承認を得る。
- 専用 USB Bluetooth dongle と WinUSB driver assignment を確認する。
- `SWBT_DAEMON_BACKEND=production`、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1` を明示する。
- pairing / reconnect / cleanup の手順と artifact path を決める。
- `docs/hardware-test-log.md` へ OS、dongle VID/PID、driver、BTstack commit、swbt commit、Switch firmware、結果を記録する。

## 12. 先送り事項

none。起票時点の先送り事項は、この record の source として取り込んだ。

## 13. チェックリスト

- [x] source を `local_050`、`local_053`、`local_055`、`local_057`、`docs/status.md` から特定した。
- [x] use case を bonded reconnect persistence として定義した。
- [ ] `source-audit` を実行した。
- [ ] hardware preflight を確認した。
- [ ] red test または characterization test を追加した。
- [ ] 実装または unsupported 判断を記録した。
- [ ] 実機結果または未実行理由を記録した。
