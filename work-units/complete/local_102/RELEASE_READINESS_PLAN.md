# Release Readiness Plan

## 1. 概要

この work unit は、`swbt-daemon` の初回 binary release 整備を複数 milestone に分解し、最初に着手する順序と gate を固定する。

完了後は、release 作業の全体計画が `spec/operations/release-build-and-publish.md` にあり、後続 work unit が M1 以降の source として使える状態になる。

## 2. 起点 / ユースケース

source:

- ユーザ要求: README を利用者向けへ整理し、開発者向け記述を別ファイルへ退避する。
- ユーザ要求: `justfile` / CMake に Release build 経路を追加する。
- ユーザ要求: release 用 GitHub Actions を整備する。
- ユーザ要求: tag を切って release する前に、他に必要な作業を洗い出す。
- ユーザ要求: milestone と作業計画を建てて順番に進める。
- 既存 spec の未解決事項: `spec/operations/release-license-boundary.md` は初回 binary release の artifact 名、同梱ファイル、publish 手順を未定としている。

use case:

- release operator は、初回 release までの作業を M0 から M7 の順に辿れる。
- reviewer は、artifact、license / notice、実機状態、tag publish の承認境界を確認できる。
- 後続 work unit は、対象 milestone を source として scope を狭く立てられる。

## 3. 対象範囲

- release 整備全体の milestone を operations spec に記録する。
- M1 以降の作業順序、完了条件、承認 gate を記録する。
- この計画 work unit record を作成する。
- `spec/operations/README.md` から新しい spec へ辿れるようにする。

## 4. 対象外

- README の書き換え。
- 開発者向け docs の作成。
- CMake / `justfile` の Release build 実装。
- package artifact 作成。
- GitHub Actions release workflow 実装。
- tag 作成、tag push、GitHub Release publish。
- 実機検証。

## 5. 関連 spec / docs

- `spec/operations/release-build-and-publish.md`
- `spec/operations/release-license-boundary.md`
- `THIRD_PARTY_NOTICES.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `CMakeLists.txt`
- `CMakePresets.json`
- `justfile`
- `.github/workflows/ci.yml`

## 6. 根拠監査

not applicable。

この work unit は release 作業順序と docs の計画だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値を追加または変更しない。

BTstack / license については artifact policy の範囲として扱い、M1 で `spec/operations/release-license-boundary.md` と `THIRD_PARTY_NOTICES.md` を更新または確認する。

## 7. 設計メモ

全体計画は複数 work unit から参照されるため、work unit record ではなく operations spec に置く。

tag push と GitHub Release publish は release 実行そのものであるため、計画と workflow 実装が終わっても自動実行しない。M7 でユーザの明示承認を必要条件にする。

## 8. 対象ファイル

- `spec/operations/release-build-and-publish.md`
- `spec/operations/README.md`
- `work-units/complete/local_102/RELEASE_READINESS_PLAN.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | release 整備の milestone が M0 から M7 まで順序付きで記録されている。 | new | docs | no |
| green | tag push / GitHub Release publish / 実機実行が明示承認なしに進まない gate として記録されている。 | new | docs | no |
| green | 初回 artifact、license / notice、version、package smoke、release workflow が後続 milestone の source として分離されている。 | new | docs | no |
| green | `spec/operations/README.md` から release plan spec へ辿れる。 | regression | docs | no |

## 10. 検証

- `rg -n '[ \t]+$' spec\operations\release-build-and-publish.md spec\operations\README.md work-units\complete\local_102\RELEASE_READINESS_PLAN.md`: pass。行末空白なし。`rg` は該当なしのため exit code `1`、stdout なし。
- `git diff --check -- spec\operations\README.md spec\operations\release-build-and-publish.md work-units\complete\local_102\RELEASE_READINESS_PLAN.md`: pass。whitespace error なし。PowerShell checkout の CRLF 変換警告だけを出した。

## 11. 実機実行条件

not applicable。

この work unit では Bluetooth adapter open、HID advertising、Switch pairing、report loop を実行しない。

## 12. 先送り事項

- M1: `spec/operations/release-license-boundary.md` を初回 binary artifact 対応へ更新する。
- M2: README を利用者向けへ書き換え、開発者向け手順を `docs/development.md` などへ退避する。
- M3: CMake / `justfile` に Release build 経路を追加し、version 正本を決める。
- M4: package artifact、checksum、manifest、artifact smoke を実装する。
- M5: tag-driven release workflow を追加する。
- M6: release candidate を検証し、実機状態または未実行理由を記録する。
- M7: ユーザ承認後に tag と GitHub Release を実行する。

## 13. チェックリスト

- [x] work unit の source と use case が明確である。
- [x] release milestone が operations spec に記録されている。
- [x] 後続 milestone が source として使える粒度で記録されている。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] 検証結果または未実行理由を記録した。
