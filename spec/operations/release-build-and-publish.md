# Release Build And Publish Plan

## 1. 状態

current。

## 2. 目的

`swbt-daemon` の初回 binary release へ向けて、README、開発者向け docs、Release build、package artifact、GitHub Actions、tag / GitHub Release の作業順序を固定する。

この spec は作業計画であり、現時点では artifact publish を許可しない。tag push、GitHub Release publish、実機実行は、該当 milestone の work unit とユーザの明示承認を必要とする。

## 3. 適用範囲

- root README を利用者向けへ寄せる作業。
- 開発者向け build / test / hook / Dev Container 記述の退避先。
- Release build preset、`just` recipe、package layout。
- Release workflow と GitHub Release artifact upload。
- release 前の license / notice / smoke / checksum / 実機状態の gate。
- tag 作成と publish 前確認。

次はこの spec では実施しない。

- Switch HID protocol、IPC wire format、BTstack source selection の変更。
- package manager 配布。
- installer 作成。
- commercial BTstack license の判断。
- 実機操作の承認省略。

## 4. 決定事項

初回 release 整備は、次の milestone に分ける。

| milestone | 目的 | 完了条件 | 次の source |
|---|---|---|---|
| M0: release scope planning | release 作業を work unit / spec として閉じる。 | この spec、work unit record、operations index が更新されている。 | M1 |
| M1: artifact / license boundary | 初回 binary artifact の名前、内容、notice 同梱方法、BTstack 表記を決める。 | `spec/operations/release-license-boundary.md` が binary artifact を扱える形へ更新され、`THIRD_PARTY_NOTICES.md` との関係が明記されている。 | M2 |
| M2: user and developer docs split | README を利用者向けへ整理し、開発者向け手順を別 docs へ移す。 | root README が利用者向けの概要、対応範囲、入手、起動、安全境界、license に絞られ、開発手順は `docs/development.md` などへ移っている。 | M3 |
| M3: version and Release build path | version の正本と Release build preset / `just` recipe を追加する。 | CMake project version、runtime version、artifact version の関係が明記され、`windows-mingw-release` と release build recipe が通る。 | M4 |
| M4: package artifact and smoke | package layout、checksum、manifest、artifact smoke を実装する。 | release package に必要ファイルが入り、SHA256 と manifest が作られ、artifact 内 executable で non-hardware smoke が通る。 | M5 |
| M5: release workflow | tag-driven GitHub Actions を追加する。 | `refs/tags/v*` で verify、package、smoke、checksum、GitHub Release upload へ進む workflow がある。 | M6 |
| M6: release candidate validation | release candidate を main 上で検証する。 | `just verify`、release package smoke、license / notice check、実機状態が work unit に記録されている。 | M7 |
| M7: tag and GitHub Release | 明示承認後に tag と GitHub Release を作る。 | annotated tag、workflow run、GitHub Release、artifact checksum、post-release smoke / verification が記録されている。 | none |

release workflow は最終的に tag-driven にする。ただし tag push は production publish と同じ扱いにし、ユーザの明示承認なしに実行しない。

初回 binary artifact の既定候補は Windows x86_64 zip とする。理由は、現時点の実機検証主経路が Windows native + WinUSB + 専用 USB Bluetooth ドングルだからである。Linux binary artifact は、libusb 実機経路が未確認であるため初回 release の必須 artifact にしない。

Windows artifact は BTstack を link するため、MIT-only artifact と表現しない。少なくとも次を package または release page から利用可能にする。

- `LICENSE`
- `THIRD_PARTY_NOTICES.md`
- `vendor/btstack/LICENSE`
- `vendor/btstack/3rd-party/README.md`
- `vendor/toml11/LICENSE`

non-hardware smoke は、Bluetooth adapter open、HID advertising、Switch pairing、report loop を始めない範囲に限定する。候補は次のコマンドである。

```console
swbt-daemon help
swbt-daemon --backend noop
swbt-daemon config --backend noop
swbt-debug-client
```

`swbt-debug-client` は現行実装では `--help` を持たず、引数なしで usage を表示して exit code `2` を返す。M4 では、引数なし usage 表示を smoke として採用するか、`--help` を追加してから smoke 対象にする。

実機検証は release の品質表現に直結する。release notes で実機検証済みと書く場合は、release commit、artifact hash、OS、Bluetooth dongle VID/PID、driver、adapter location、Switch firmware、BTstack commit、swbt commit、report period、結果を `docs/hardware-test-log.md` に記録する。実機未実行で release する場合は、未実行と既知の確認範囲を release notes に書く。

## 5. 根拠

- `spec/operations/release-license-boundary.md` は、初回 Windows binary artifact の既定名、layout、notice 同梱条件、tag / GitHub Release の実行除外を記録している。
- `THIRD_PARTY_NOTICES.md` は、BTstack を含む binary build を MIT-only artifact と表現しないと記録している。
- `docs/status.md` は、既知の実機対応構成を Windows native、CSR8510 A10、WinUSB、Switch 2 firmware `22.1.0` と記録している。
- `CMakePresets.json` は Debug / ASan / clang-tidy / Windows Debug preset を持つが、Release preset を持たない。
- `swbt/support/swbt_version.c` は runtime version として `0.1.0-dev` を返す一方、`CMakeLists.txt` の project version は `0.1.0` である。
- `.github/workflows/ci.yml` は `just verify-ci` の CI workflow であり、release package workflow ではない。

この spec は release 作業順序と運用 gate を扱う。Switch HID bytes、BTstack source selection、report timing、WinUSB/libusb の実装値を変更しないため、根拠監査は not applicable とする。

## 6. 関連 work units

- `work-units/complete/local_105/RELEASE_VERSION_AND_BUILD_PATH.md`
- `work-units/complete/local_106/RELEASE_PACKAGE_AND_SMOKE.md`
- `work-units/complete/local_107/RELEASE_GITHUB_ACTIONS.md`
- `work-units/complete/local_104/README_USER_DEVELOPMENT_DOC_SPLIT.md`
- `work-units/complete/local_103/RELEASE_ARTIFACT_LICENSE_BOUNDARY.md`
- `work-units/complete/local_102/RELEASE_READINESS_PLAN.md`
- `work-units/complete/local_066/RELEASE_LICENSE_BOUNDARY_POLICY.md`

## 7. 未解決事項

- 初回 artifact 名は `spec/operations/release-license-boundary.md` の既定候補を使う。
- Windows artifact には `swbt-debug-client.exe` を診断用 client として含める。
- release manifest は M4 時点では package script が生成する JSON とし、workflow 側で追加項目が必要なら M5 で拡張する。
- M5 workflow は GitHub Release を draft で作る。publish は M7 の明示承認後に行う。
- 実機検証を M6 の必須 gate にするか、未実行を明記して release するか。
