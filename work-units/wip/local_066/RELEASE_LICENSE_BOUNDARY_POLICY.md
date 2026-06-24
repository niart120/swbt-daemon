# Release License Boundary Policy

## 1. 概要

BTstack を含む配布表現、release artifact 条件、license notice の境界を整理する docs / policy work unit。

rearchitecture 中の deferred item では、release / license boundary が cutover 完了条件から分離された。初期範囲では binary release は対象外だが、BTstack を含む artifact を MIT-only と表現してはいけないという repository policy は既にある。

この work unit では release artifact を作らず、将来の release 作業が誤った license 表現や不十分な notice で進まないよう、方針と gate を固定する。

## 2. 起点 / ユースケース

source:

- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md` の先送り事項: release / license boundary。
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md` の先送り事項: release / license boundary。
- `spec/architecture/daemon-architecture-cutover.md` の対象外: release packaging、BTstack license を含む配布方針。
- repository policy: BTstack を含む binary / release を MIT-only artifact と表現しない。license / notice に触れる変更では `THIRD_PARTY_NOTICES.md` を確認する。

use case:

- actor: maintainer、release operator、reviewer。
- 入力または状態: BTstack submodule を含む source tree、将来の binary artifact、docs / README / status 表記、third-party notices。
- 期待する観測結果: release 作業前に、BTstack を含む配布物の license 表現、notice 更新、対象外 artifact が明確になっている。
- 制約: この work unit では binary release を作らない。legal advice ではなく repository policy と確認手順を記録する。
- 対象外: version tag push、package upload、installer 作成、license 変更。

source から use case への変換:

release / license boundary は implementation cutover の完了条件ではない。後続では、release 作業を始める前の policy / gate として docs に固定する。

## 3. 対象範囲

- 現行 `THIRD_PARTY_NOTICES.md` と BTstack submodule の license 表記を確認する。
- README / docs / spec で MIT-only と誤解される表現がないか確認する。
- release artifact を作る前に確認する checklist を docs または spec に追加する。
- binary release が対象外である現状を docs/status と矛盾しない形で記録する。
- BTstack を含む artifact の表現に必要な follow-up があれば先送り事項として残す。

## 4. 対象外

- binary release の作成。
- tag push、GitHub release publish。
- legal review の代替。
- BTstack 本体の license 変更。
- package manager 配布。

## 5. 関連 spec / docs

- `THIRD_PARTY_NOTICES.md`
- `README.md`
- `docs/status.md`
- `spec/operations/release-license-boundary.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`

## 6. 根拠監査

not applicable。

この work unit は release / license policy と docs gate を扱う。Switch protocol byte、BTstack source selection、report period、WinUSB/libusb fact を追加または変更しない。BTstack source selection や vendor content を変更する場合は別途 source-audit を使う。

## 7. 設計メモ

- binary release は現時点の対象外として扱う。
- notice と license 表記は、release 作業の直前ではなく、policy と checklist として先に固定する。
- `THIRD_PARTY_NOTICES.md` の確認結果と、変更しない判断も record に残す。
- 「MIT-only artifact」という表現は、BTstack を含む配布物に使わない。

## 8. 対象ファイル

- `THIRD_PARTY_NOTICES.md`
- `README.md`
- `docs/status.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `spec/operations/*`
- `work-units/wip/local_066/RELEASE_LICENSE_BOUNDARY_POLICY.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | docs state that binary release is out of current scope or list exact release gate prerequisites | characterization | docs | no |
| green | BTstack-containing artifacts are not described as MIT-only | regression | docs | no |
| green | release checklist points to `THIRD_PARTY_NOTICES.md` before binary artifact creation | new | docs | no |
| green | docs distinguish source repository license text from bundled third-party notices | regression | docs | no |

## 10. 検証

- red: `rg -n "binary release は現時点の対象外|binary release は現在の scope に含めない|binary release を現在の対象範囲に含めない" spec\operations README.md docs\status.md` -> no match。
- green: `spec/operations/release-license-boundary.md` に binary release が現時点の対象外であり、release artifact 作成は別 work unit で scope 化する判断を記録する。
- characterization: `rg -n "MIT-only|MIT only|MIT License" README.md docs spec THIRD_PARTY_NOTICES.md LICENSE` -> 一致箇所は自前 license の記述、または BTstack を含む artifact の MIT-only 表記を禁止する否定形 policy である。
- red: `rg -n "BTstack を含む.*MIT-only|MIT-only artifact|MIT-only binary|MIT-only と表現しない" spec\operations\release-license-boundary.md` -> no match。
- green: `spec/operations/release-license-boundary.md` に BTstack を含む source 配布物、binary artifact、release artifact を MIT-only artifact と表現しない方針を記録する。
- red: `rg -n "release checklist|binary artifact 作成前|THIRD_PARTY_NOTICES.md.*binary artifact|release 前 checklist" spec\operations\release-license-boundary.md` -> no match。
- green: `spec/operations/release-license-boundary.md` に release 前 checklist を追加し、binary artifact 作成前に `THIRD_PARTY_NOTICES.md`、`vendor/btstack/LICENSE`、`vendor/btstack/3rd-party/README.md` を確認する手順を記録する。
- red: `rg -n "root LICENSE.*自前|third-party notice|bundled third-party notices|source repository license text|source repository の license" spec\operations\release-license-boundary.md` -> no match。
- green: `spec/operations/release-license-boundary.md` と `README.md` に、root `LICENSE` は自前 project files の license text、`THIRD_PARTY_NOTICES.md` は bundled third-party notices の入口として別に確認する方針を記録する。

## 11. 実機実行条件

実機不要。release / license docs policy の work unit であり、Bluetooth adapter、Switch pairing、HID advertising、report loop を実行しない。

## 12. 先送り事項

none。起票時点の先送り事項は、この record の source として取り込んだ。

## 13. チェックリスト

- [x] source を `local_050`、`local_055`、architecture spec、repository policy から特定した。
- [x] use case を release / license policy gate として定義した。
- [ ] `THIRD_PARTY_NOTICES.md` を確認した。
- [ ] docs の license / release 表現を確認した。
- [ ] 必要な docs / spec を更新した。
- [ ] docs-only verification を実行した。
