# Release README Ready

## 1. 概要

この work unit は、tag 作成前の root README から release 整備の進行中メモを外し、配布済み release の利用者が読む入口へ更新する。

完了後は、README が GitHub Release assets、Windows artifact 名、checksum 確認、source build の退避先、実機安全境界、license 境界を直接示す。開発者向けの release workflow 説明は `docs/development.md` に残す。

## 2. 起点 / ユースケース

source:

- ユーザ要求: publish 前に README に残った release 作業手順のメタ情報を外す。
- `work-units/complete/local_104/README_USER_DEVELOPMENT_DOC_SPLIT.md` の先送り事項: package artifact 実装後、README の入手手順を GitHub Release artifact 前提へ更新する。
- `spec/operations/release-build-and-publish.md` M7: tag / GitHub Release の前に release candidate の利用者向け文書を確定する。

use case:

- release asset を取得する利用者は、README から zip 名、checksum 確認、展開後の実行ファイルへ進める。
- source から build する開発者は、README から `docs/development.md` へ進める。
- reviewer は、README に release 整備中の作業計画や milestone 実行メモが残っていないことを確認できる。

## 3. 対象範囲

- root README の入手手順を GitHub Release assets 前提へ更新する。
- README から release readiness plan への進行中メタ情報を外す。
- `docs/development.md` の release 作業説明を恒久的な運用文へ更新する。
- `spec/operations/release-build-and-publish.md` の関連 work unit にこの record を追加する。

## 4. 対象外

- tag 作成、tag push、GitHub Release publish。
- release workflow の実行。
- package script、CMake、justfile の変更。
- 実機検証。

## 5. 関連 spec / docs

- `README.md`
- `docs/development.md`
- `spec/operations/release-build-and-publish.md`
- `spec/operations/release-license-boundary.md`
- `work-units/complete/local_104/README_USER_DEVELOPMENT_DOC_SPLIT.md`
- `work-units/complete/local_108/RELEASE_CANDIDATE_VALIDATION.md`

## 6. 根拠監査

not applicable。

この work unit は release README と開発者向け release 運用文の更新だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値を追加または変更しない。

## 7. 設計メモ

README は、配布後の利用者が読む情報だけに寄せる。release build / publish の内部手順は `docs/development.md` と operations spec に置く。

README は GitHub Release assets の取得形を示すが、tag push や publish の承認状態は書かない。release 実行の承認境界は operations spec と work unit record 側で扱う。

## 8. 対象ファイル

- `README.md`
- `docs/development.md`
- `spec/operations/release-build-and-publish.md`
- `work-units/complete/local_109/RELEASE_README_READY.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | README が GitHub Release assets、zip 名、checksum 確認、source build の退避先を示している。 | new | docs | no |
| green | README に release readiness plan や milestone 実行中のメタ情報が残っていない。 | regression | docs | no |
| green | `docs/development.md` の release 節が tag-driven workflow、draft release、publish 前確認の恒久的な運用文になっている。 | regression | docs | no |

## 10. 検証

- `rg -n "まだ提供|置く予定|初回 release の準備|M5 時点|Release 整備は" README.md docs\development.md`: pass。該当なし。
- `rg -n "Release Build And Publish Plan|release-build-and-publish" README.md`: pass。該当なし。
- `rg -n "GitHub Releases|swbt-daemon-v<version>-windows-x86_64.zip|Get-FileHash|docs/development.md|MIT-only" README.md`: pass。README の利用者向け入手、checksum、source build、license 境界を確認した。
- `rg -n "[ \t]+$" README.md docs\development.md spec\operations\release-build-and-publish.md work-units\complete\local_109\RELEASE_README_READY.md`: pass。行末空白なし。
- `git diff --check -- README.md docs\development.md spec\operations\release-build-and-publish.md work-units\complete\local_109\RELEASE_README_READY.md`: pass。whitespace error なし。

## 11. 実機実行条件

not applicable。

この work unit では Bluetooth adapter open、HID advertising、Switch pairing、report loop を実行しない。

## 12. 先送り事項

none。

## 13. チェックリスト

- [x] work unit の source と use case が明確である。
- [x] README から release 作業手順のメタ情報を外した。
- [x] README の入手手順を GitHub Release artifact 前提へ更新した。
- [x] 開発者向け release 運用文を `docs/development.md` に残した。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] 検証結果または未実行理由を記録した。
