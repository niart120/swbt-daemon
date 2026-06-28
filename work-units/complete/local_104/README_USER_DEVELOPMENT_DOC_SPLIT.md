# README User Development Doc Split

## 1. 概要

この work unit は、root README を利用者向けの入口へ整理し、開発者向けの build / test / hook / architecture / repository layout を `docs/development.md` へ移す。

完了後は、利用者は root README から現状、実機安全境界、管理コマンド、IPC、license を確認できる。開発者は `docs/development.md` から Dev Container、`just` recipe、hooks、内部構成へ進める。

## 2. 起点 / ユースケース

source:

- ユーザ要求: README を利用者向けへ整理し、開発者向け記述を別ファイルへ退避する。
- `spec/operations/release-build-and-publish.md` M2: README を利用者向けへ整理し、開発者向け手順を別 docs へ移す。
- `work-units/complete/local_102/RELEASE_READINESS_PLAN.md` の先送り事項 M2。
- `work-units/complete/local_103/RELEASE_ARTIFACT_LICENSE_BOUNDARY.md` の先送り事項 M2。

use case:

- 利用者は、binary release 未提供、確認済み実機範囲、安全境界、管理コマンド、license を root README で確認できる。
- 開発者は、build / test / hook / architecture / repository layout を `docs/development.md` で確認できる。
- release docs は、artifact と license wording を README から参照できる。

## 3. 対象範囲

- root README の構成を利用者向けに書き換える。
- `docs/development.md` を作成する。
- release artifact / license boundary へのリンクを README に残す。
- developer commands と hooks を README から移す。

## 4. 対象外

- CMake / `justfile` の Release build 実装。
- package artifact 作成。
- GitHub Actions release workflow 実装。
- tag 作成、tag push、GitHub Release publish。
- 実機検証。

## 5. 関連 spec / docs

- `README.md`
- `docs/development.md`
- `docs/status.md`
- `docs/hardware-test-log.md`
- `spec/operations/release-build-and-publish.md`
- `spec/operations/release-license-boundary.md`
- `spec/protocols/daemon-ipc-v1.md`
- `spec/architecture/daemon-architecture-cutover.md`

## 6. 根拠監査

not applicable。

この work unit は documentation split だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値を追加または変更しない。

## 7. 設計メモ

README では実機対応状況を短く示し、詳細表は `docs/status.md` に委譲する。開発者向けの詳細は `docs/development.md` に移す。

## 8. 対象ファイル

- `README.md`
- `docs/development.md`
- `work-units/complete/local_104/README_USER_DEVELOPMENT_DOC_SPLIT.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | README が利用者向けの現状、安全境界、管理コマンド、IPC、license を示している。 | new | docs | no |
| green | README から開発者向け build / test / hook の長い手順が分離されている。 | regression | docs | no |
| green | `docs/development.md` に Dev Container、build / test、hooks、BTstack dependency、architecture、repository layout がある。 | new | docs | no |
| green | README から release plan、license boundary、development docs へ辿れる。 | new | docs | no |

## 10. 検証

- README development-command split check: pass。README に長い開発者向け build / hook / Dev Container 手順が残っていないことを `rg` で確認した。
- Placeholder check: pass。本番文書へ残すべきでない仮テキストがないことを `rg` で確認した。
- `rg -n '[ \t]+$' README.md docs\development.md work-units\complete\local_104\README_USER_DEVELOPMENT_DOC_SPLIT.md`: pass。行末空白なし。`rg` は該当なしのため exit code `1`、stdout なし。
- `git diff --check -- README.md docs\development.md work-units\complete\local_104\README_USER_DEVELOPMENT_DOC_SPLIT.md`: pass。whitespace error なし。PowerShell checkout の CRLF 変換警告だけを出した。
- Link target check: pass。`docs/development.md`、`spec/operations/release-build-and-publish.md`、`spec/operations/release-license-boundary.md`、`spec/protocols/daemon-ipc-v1.md`、`spec/architecture/daemon-architecture-cutover.md` は存在する。

## 11. 実機実行条件

not applicable。

この work unit では Bluetooth adapter open、HID advertising、Switch pairing、report loop を実行しない。

## 12. 先送り事項

- M3: CMake / `justfile` に Release build 経路を追加し、README / development docs の build section と整合させる。
- M4: package artifact 実装後、README の入手手順を GitHub Release artifact 前提へ更新する。

## 13. チェックリスト

- [x] work unit の source と use case が明確である。
- [x] README を利用者向けに整理した。
- [x] 開発者向け記述を `docs/development.md` に移した。
- [x] release / license docs へのリンクを残した。
- [x] 根拠監査の状態を記録した。
- [x] 実機状態を記録した。
- [x] 検証結果または未実行理由を記録した。
