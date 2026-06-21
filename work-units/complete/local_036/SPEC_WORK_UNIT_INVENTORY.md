# Spec Work Unit Inventory

## 1. 概要

`spec/` と `work-units/` を棚卸し、reference や完了済み work unit に残っていた current contract を安定 spec へ昇格した work unit。

この work unit では、architecture spec 1 件、protocol spec 2 件を作成し、後続で追跡すべき work unit record を 3 件作成した。既存 wip の `local_028` は、現行の source / use case / 13 セクション構成へ更新した。

## 2. 起点 / ユースケース

source:

- ユーザ要求。`spec` と `work-units` の棚卸を行い、spec に昇格できるもの・すべきものを洗い出して作成し、新規に払い出すべき work unit record を作成する。
- `spec/README.md`。実装が従う文書は `architecture/`、`protocols/`、`operations/` に置き、`references/` は規範として扱わない。
- `spec/operations/work-unit-spec-tdd-flow.md`。source から use case を作り、work unit record と spec の責務を分ける。
- `spec/references/*.md` と完了済み work unit record。Switch HID、daemon IPC、BTstack bridge、runtime lifecycle の current contract が reference や individual record に分散していた。
- `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`。当時は現行 required sections に合っておらず、source / use case が不足していた。

use case:

- actor: maintainer または agent。
- 入力または状態: `spec/`、`work-units/complete/`、`work-units/wip/`、`spec/dev-journal.md` の current files。
- 期待する観測結果: 実装が従う current contract は `architecture/` または `protocols/` の spec から辿れる。未実装・未検証の follow-up は wip work unit record として source / use case / scope / non-goals を持つ。
- 制約: reference を規範として扱わない。実機未検証、根拠監査未完了、BTstack license boundary を確認済み事実として書かない。
- 対象外: code implementation、実機コマンド、BTstack submodule の変更。
- source から use case へ変換した判断: 今回の完了条件は、全 deferred item の実装ではなく、current contract の置き場を作り、後続作業を work unit record として起こせる状態にすること。

## 3. 対象範囲

- `spec/architecture/`、`spec/protocols/`、`spec/operations/`、`spec/references/`、`spec/dev-journal.md` を棚卸する。
- `work-units/complete/` と `work-units/wip/` の未解決事項、先送り事項、wip record を棚卸する。
- daemon runtime / BTstack bridge boundary を current architecture spec へ昇格する。
- daemon IPC v1 contract を current protocol spec へ昇格する。
- Switch HID protocol core contract を current protocol spec へ昇格する。
- `local_028` の wip record を現行構成へ更新する。
- 実機 bring-up、BTstack send-ready integration、daemon status / observability protocol を後続 wip work unit record として作成する。
- spec README と references README を更新し、current spec から辿れるようにする。

## 4. 対象外

- Switch protocol 値の新規監査。
- code implementation。
- CMake / CTest の build 実行。
- Windows native 実機検証。
- `vendor/btstack` の変更。
- `spec/initial/` の historical docs の全面移行。
- すべての future client library の record 化。

## 5. 関連 spec / docs

- `spec/README.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/protocols/daemon-ipc-v1.md`
- `spec/protocols/switch-hid-core.md`
- `spec/operations/work-unit-spec-tdd-flow.md`
- `spec/operations/windows-native-preflight.md`
- `spec/references/README.md`
- `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md`
- `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`

## 6. 根拠監査

`source-audit` を確認した。

この work unit では新しい Switch HID report bytes、subcommand ID、SPI address、rumble packet、report timing、BTstack source selection、WinUSB/libusb facts を追加しない。既存の `spec/references/` に記録済みの根拠監査と完了済み work unit record を current spec から参照する。

local_036 時点の実機未検証項目は `spec/protocols/switch-hid-core.md` と `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` に未解決事項として残した。

## 7. 設計メモ

棚卸結果:

| source | 判断 | 出力 |
|---|---|---|
| daemon runtime、mailbox、BTstack bridge の完了済み record | 複数 work unit から参照される境界であり architecture spec に昇格する | `spec/architecture/daemon-runtime-boundaries.md` |
| IPC session / JSON / TCP / heartbeat の完了済み record | daemon IPC wire protocol であり通信仕様として protocol spec に昇格する | `spec/protocols/daemon-ipc-v1.md` |
| Switch report / subcommand / SPI / rumble / player lights / scheduler references | 実装が従う protocol contract であり protocol spec に昇格する | `spec/protocols/switch-hid-core.md` |
| Phase 5、hardware acceptability、report period comparison | local_036 時点では実機承認が必要な follow-up として wip work unit にした。現在は complete | `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` |
| subcommand reply queue と periodic scheduler の exact integration | software integration と実機確認を分ける follow-up であり local_036 当時は wip work unit にした | `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md` |
| stable IPC metrics / status protocol | `get_status` contract とは別に定義すべき follow-up であり wip work unit にする | `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md` |
| `work-units/complete/local_028` | local_036 当時は wip として有効だが旧構成のため source / use case を追記する対象だった | `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md` |
| `spec/dev-journal.md` の Dev Container 導線観測 | `development-tooling` と `windows-native-preflight`、`local_032` で吸収済み | 新規 record なし |
| archived `just-task-runner-migration` の未解決事項 | current tooling spec に同じ未解決事項が残るため archive は編集しない | `spec/operations/development-tooling.md` の未解決事項として維持 |

## 8. 対象ファイル

- `spec/README.md`
- `spec/architecture/README.md`
- `spec/architecture/daemon-runtime-boundaries.md`
- `spec/protocols/README.md`
- `spec/protocols/daemon-ipc-v1.md`
- `spec/protocols/switch-hid-core.md`
- `spec/references/README.md`
- `work-units/complete/local_036/SPEC_WORK_UNIT_INVENTORY.md`
- `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_038/BTSTACK_SEND_READY_INTEGRATION.md`
- `work-units/wip/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | current runtime / BTstack boundary is represented by an architecture spec instead of only references and records | new | docs | no |
| refactor-done | daemon IPC wire protocol is represented by a communication spec | new | docs | no |
| refactor-done | Switch HID core current contract is represented by a protocol spec with unresolved hardware items separated | new | docs | no |
| refactor-done | follow-up sources are converted into wip work unit records with source and use case | new | docs | no |
| refactor-done | existing wip debug IPC client record has the current required sections | regression | docs | no |

## 10. 検証

- `git branch --show-current`: `main` を確認した後、`docs/spec-workunit-inventory` へ切り替えた。
- `git status --short`: 開始時点で clean。
- `rg --files spec`: spec 配下の current files を確認した。
- `rg --files work-units`: work unit record 一覧を確認した。
- `rg -n "未解決事項|先送り事項|deferred|昇格|journal|spec" spec work-units docs`: 未解決事項、先送り事項、wip source を抽出した。
- `Get-Content -Raw spec/dev-journal.md`: journal entry を確認した。
- 当時の `Get-Content -Raw` による `local_028` record 確認: 唯一の wip record を確認した。現在の file path は `work-units/complete/local_028/MINIMAL_DEBUG_IPC_CLIENT.md`。
- required section check: 新規 spec 3 件は 7 required sections、更新または新規 work unit record 5 件は 13 required sections をすべて含む。欠落出力なし。
- placeholder pattern search on new and updated files: no matches。
- `rg --files spec work-units | Sort-Object`: 新規 spec 3 件、`local_036`、`local_037`、`local_038`、`local_039`、更新済み `local_028` が一覧に含まれることを確認した。

文書変更のみのため、CMake / CTest / 実機コマンドは実行していない。

## 11. 実機実行条件

実機検証は不要。

この work unit は棚卸と文書作成だけを扱い、Bluetooth adapter、Switch pairing、HID advertising、report loop を実行しない。

実機作業が必要な follow-up は `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` に分けた。

## 12. 先送り事項

none。

棚卸で見つかった follow-up は、`local_037`、`local_038`、`local_039`、更新済み `local_028` に source と use case を持たせた。

## 13. チェックリスト

- [x] `spec/` の current / reference / journal を棚卸した。
- [x] `work-units/complete/` と `work-units/wip/` を棚卸した。
- [x] spec 昇格対象を分類した。
- [x] current architecture spec を作成した。
- [x] current protocol spec を作成した。
- [x] 新規 wip work unit record を作成した。
- [x] 既存 wip work unit record を現行構成に更新した。
- [x] 根拠監査の状態を記録した。
- [x] 実機未実行理由を記録した。
