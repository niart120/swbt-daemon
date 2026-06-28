# Release README Ready

## 1. 概要

この work unit は、tag 作成前の root README から release 整備の進行中メモと内部向け表現を外し、配布済み release の利用者が読む入口へ更新する。

完了後は、README が GitHub Releases、対応状況、実機利用時の注意、起動確認、入力送信の最小例、license 境界を示す。checksum や展開手順、release 作業手順、開発者向け release workflow 説明は README に置かない。

## 2. 起点 / ユースケース

source:

- ユーザ要求: publish 前に README に残った release 作業手順のメタ情報を外す。
- ユーザ要求: checksum や展開手順は README では過剰なので削る。
- ユーザ要求: `自前ファイル` の言い回しを避け、WinUSB driver 切り替え用に Zadig への誘導を README に入れる。
- ユーザ要求: `実機`、`片付け` など内部運用に寄った表現を避け、Switch 接続前の注意と起動引数の説明を利用者向けに整理する。
- `work-units/complete/local_104/README_USER_DEVELOPMENT_DOC_SPLIT.md` の先送り事項: package artifact 実装後、README の入手手順を GitHub Release artifact 前提へ更新する。
- `spec/operations/release-build-and-publish.md` M7: tag / GitHub Release の前に release candidate の利用者向け文書を確定する。

use case:

- release asset を取得する利用者は、README から GitHub Releases、Windows 版の同梱物、対応状況、実機利用時の注意を確認できる。
- ソースからビルドする開発者は、README から `docs/development.md` へ進める。
- Windows 実機利用者は、専用 USB Bluetooth ドングルだけを対象に WinUSB driver へ切り替える必要があることと、Zadig の入口を README で確認できる。
- Switch に接続する利用者は、`--adapter-location` が必須であることと、他の起動引数が任意であることを README で確認できる。
- reviewer は、README に release 整備中の作業計画や milestone 実行メモが残っていないことを確認できる。

## 3. 対象範囲

- root README の入手手順を GitHub Release assets 前提へ更新する。
- README から release readiness plan への進行中メタ情報を外す。
- README から checksum 確認と展開手順を外す。
- README に残す IPC は動作確認用 client の最小例に絞る。
- README に Zadig へのリンクと、専用 USB Bluetooth ドングルのドライバーを WinUSB に切り替える注意を追加する。
- README の license 表現から `自前ファイル` を外す。
- README の `利用時の注意事項` から `実機` や `片付け` などの内部運用寄り表現を外す。
- README の起動例に各引数の必須/任意と用途を追加する。
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
- `spec/operations/windows-native-preflight.md`
- `work-units/complete/local_104/README_USER_DEVELOPMENT_DOC_SPLIT.md`
- `work-units/complete/local_108/RELEASE_CANDIDATE_VALIDATION.md`

## 6. 根拠監査

not applicable。

この work unit は release README と開発者向け release 運用文の更新だけを扱う。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値を追加または変更しない。

## 7. 設計メモ

README は、配布後の利用者が読む情報だけに寄せる。release build / publish の内部手順は `docs/development.md` と operations spec に置く。

README は GitHub Releases の入口を示すが、checksum 確認や展開手順、tag push や publish の承認状態は書かない。release 実行の承認境界は operations spec と work unit record 側で扱う。

Zadig の説明は利用者が実機前に必要な driver 切り替えの入口だけにする。ドライバー切り替えの詳細手順、実機検証の記録項目、承認境界は `spec/operations/windows-native-preflight.md` に任せる。

README では `実機` を使わず、利用者が実際に行う操作として `Switch に接続する` と書く。終了時の確認は `接続状態の片付け` ではなく、`swbt-daemon` の停止と Switch 側に不要な接続が残っていないことの確認として書く。

## 8. 対象ファイル

- `README.md`
- `docs/development.md`
- `spec/operations/release-build-and-publish.md`
- `work-units/complete/local_109/RELEASE_README_READY.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | README が GitHub Releases、Windows 版の同梱物、ソースからのビルド手順の退避先を示している。 | new | docs | no |
| green | README に release readiness plan や milestone 実行中のメタ情報が残っていない。 | regression | docs | no |
| green | README に checksum 確認と展開手順が残っていない。 | regression | docs | no |
| green | README の IPC 記述が動作確認用 client の最小例と protocol link に絞られている。 | regression | docs | no |
| green | README が Zadig へのリンクと、専用 USB Bluetooth ドングルのドライバーを WinUSB に切り替える注意を示している。 | new | docs | no |
| green | README の license 表現に `自前ファイル` が残っていない。 | regression | docs | no |
| green | README の利用時注意に `実機` と `片付け` が残っていない。 | regression | docs | no |
| green | README の Switch 接続起動例が `--adapter-location` を必須、他の起動引数を任意として説明している。 | new | docs | no |
| green | `docs/development.md` の release 節が tag-driven workflow、draft release、publish 前確認の恒久的な運用文になっている。 | regression | docs | no |

## 10. 検証

- `rg -n "まだ提供|置く予定|初回 release の準備|M5 時点|Release 整備は" README.md docs\development.md`: pass。該当なし。
- `rg -n "Release Build And Publish Plan|release-build-and-publish|work unit|milestone|Operations specs|Architecture spec|checksum|sha256|Get-FileHash|展開|production backend|selector|report loop|cleanup confirmation|source checkout|third-party notice|commercial license|personal / non-commercial|local IPC" README.md`: pass。該当なし。
- `rg -n "GitHub Releases|swbt-daemon.exe|swbt-debug-client.exe|Development|Current State And Support Matrix|Daemon IPC v1|THIRD_PARTY_NOTICES.md" README.md`: pass。README の利用者向け入手、同梱物、ソースからのビルド手順の退避先、status、IPC、license 境界を確認した。
- `rg -n "Zadig|WinUSB|切り替えてください|変更しないでください|置き換えてよい|置き換えない|専用 USB Bluetooth ドングル|自前ファイル|コードと文書" README.md`: pass。Zadig / WinUSB 誘導、専用ドングルのドライバー切り替え指示、license 表現修正を確認した。
- `rg -n "実機|片付け|cleanup|必須|任意|--adapter-location|--config|--link-key-db|--trace-path|--hci-dump-path" README.md`: pass。`実機`、`片付け`、`cleanup` は該当なし。起動引数の必須/任意説明は該当あり。
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
