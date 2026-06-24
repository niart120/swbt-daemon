# Release License Boundary

## 1. 状態

current。

## 2. 目的

release 作業を始める前に、`swbt-daemon` の自前ファイルの license、BTstack を含む配布物の扱い、notice 確認の入口を明確にする。

この spec は法的助言ではない。repository 内で release operator と reviewer が確認する運用条件を定める。

## 3. 適用範囲

- source repository と release artifact の license 表記。
- `vendor/btstack` を含む source 配布物。
- BTstack を link する binary artifact。
- release 前の checklist と review gate。

次は対象外である。

- version tag push。
- GitHub Release publish。
- installer、package manager、binary archive の作成。
- BTstack 本体の license 変更。
- BlueKitchen GmbH との commercial license 判断。

## 4. 決定事項

binary release は現時点の対象外である。
現在の repository の対象範囲では、release artifact を作成せず、tag push と publish も行わない。

binary artifact、installer、package manager 配布、または BTstack を含む source archive を作る場合は、別 work unit として release scope を立てる。
その work unit では、artifact 名、含める source / binary、BTstack の有無、notice 同梱方法、検証結果を記録する。

## 5. 根拠

- `LICENSE` は、自前の `swbt-daemon` project files だけに MIT License が適用され、third-party dependencies には適用されないと記録している。
- `THIRD_PARTY_NOTICES.md` は、BTstack が `vendor/btstack` の submodule であり、BTstack 固有の license 条件を持つと記録している。
- `README.md` は、BTstack を含む build / source distribution が BTstack license の対象にもなると記録している。
- `spec/architecture/daemon-architecture-cutover.md` は、release packaging と BTstack license を含む配布方針を architecture cutover の対象外としていた。

この spec は release / license の運用方針を扱う。
Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は変更しない。
根拠監査は not applicable とする。

## 6. 関連 work units

- `work-units/wip/local_066/RELEASE_LICENSE_BOUNDARY_POLICY.md`
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`

## 7. 未解決事項

- 初回 binary release の artifact 名、同梱ファイル、publish 手順は未定である。
- package manager 配布を行うかどうかは未定である。
