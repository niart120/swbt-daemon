# Release License Boundary

## 1. 状態

current。

## 2. 目的

release 作業を始める前に、`swbt-daemon` の自前ファイルの license、BTstack を含む配布物の扱い、notice 確認の入口を明確にする。

この spec は法的助言ではない。repository 内で release operator と reviewer が確認する運用条件を定める。
この spec は配布方針を定めるが、artifact 作成、tag push、GitHub Release publish を単独では許可しない。

## 3. 適用範囲

- source repository と release artifact の license 表記。
- `vendor/btstack` を含む source 配布物。
- BTstack を link する binary artifact。
- 初回 Windows binary zip artifact の名前、内容、notice 同梱条件。
- release 前の checklist と review gate。

次は対象外である。

- version tag push の実行。
- GitHub Release publish の実行。
- installer と package manager 配布。
- BTstack 本体の license 変更。
- BlueKitchen GmbH との commercial license 判断。

## 4. 決定事項

初回 binary artifact の既定候補は Windows x86_64 zip とする。artifact 名は次の形式にする。

```text
swbt-daemon-v<version>-windows-x86_64.zip
```

`<version>` は release tag と同じ version 文字列を使う。tag は `v0.1.0` のように `v` prefix を持つため、artifact 名にも `v` prefix を残す。

zip の top-level directory は artifact 名から `.zip` を除いた名前にする。初回 Windows artifact は次の layout を候補とする。

```text
swbt-daemon-v<version>-windows-x86_64/
  README.md
  LICENSE
  THIRD_PARTY_NOTICES.md
  bin/
    swbt-daemon.exe
    swbt-debug-client.exe
  licenses/
    btstack/
      LICENSE
      3rd-party/README.md
    toml11/
      LICENSE
  manifest.json
```

`swbt-debug-client.exe` は daemon IPC の診断用 client として同梱候補に含める。ただし安定した end-user automation API として扱わない。利用者向け README と release notes では、主 artifact は `swbt-daemon.exe` であり、`swbt-debug-client.exe` は診断用であると明記する。

artifact と同じ GitHub Release には SHA256 checksum を置く。checksum は zip の外側に置く。

binary artifact、installer、package manager 配布、または BTstack を含む source archive を作る場合は、別 work unit として release scope を立てる。その work unit では、artifact 名、含める source / binary、BTstack の有無、notice 同梱方法、検証結果を記録する。

BTstack を含む source 配布物、binary artifact、release artifact を MIT-only artifact と表現しない。
自前ファイルの MIT License と、BTstack の license 条件は別に扱う。

root `LICENSE` は自前 project files の license text である。
`THIRD_PARTY_NOTICES.md` は bundled third-party notices の入口であり、root `LICENSE` の代替ではない。
BTstack を含む artifact では、source repository の license text と bundled third-party notices を別々に確認し、必要に応じて両方を同梱または参照できる状態にする。

初回 Windows binary artifact は `swbt-daemon.exe` が BTstack を link するため、BTstack を含む binary artifact として扱う。release page と README では次の事実を分けて書く。

- original `swbt-daemon` project files は MIT License である。
- binary builds that include or link BTstack are also subject to the BTstack license.
- BTstack license includes non-commercial / personal-use restrictions and commercial users need a separate BlueKitchen license path.

GitHub が tag から自動生成する source archive は、submodule contents を含む source distribution と同一視しない。submodule を含む source distribution を別途作る場合は、この spec の BTstack notice 同梱条件を適用する。

release 前 checklist は次の順に確認する。

1. binary artifact 作成前に `THIRD_PARTY_NOTICES.md` を確認する。
2. BTstack を含む、または link する artifact かを記録する。
3. BTstack を含む場合は、`vendor/btstack/LICENSE` と `vendor/btstack/3rd-party/README.md` を確認し、配布物に同梱または参照できる状態にする。
4. toml11 を含む場合は、`vendor/toml11/LICENSE` を確認し、配布物に同梱または参照できる状態にする。
5. `git submodule status vendor/btstack vendor/toml11` などで、artifact が参照する submodule commit を記録する。
6. package manifest に `swbt-daemon` commit、BTstack commit、toml11 commit、build preset、artifact name を記録する。zip checksum は artifact 外側の `.sha256` に記録する。
7. PR の BTstack / License 影響欄に、BTstack source selection の変更有無、`THIRD_PARTY_NOTICES.md` の確認結果、notice 同梱方法を書く。
8. artifact を作らない work unit では、artifact 未作成と license / notice 影響の未発生を record と PR に書く。

## 5. 根拠

- `LICENSE` は、自前の `swbt-daemon` project files だけに MIT License が適用され、third-party dependencies には適用されないと記録している。
- `THIRD_PARTY_NOTICES.md` は、BTstack が `vendor/btstack` の submodule であり、BTstack 固有の license 条件を持つと記録している。
- `THIRD_PARTY_NOTICES.md` は、BTstack を含む binary を MIT-only artifact と表現しないと記録している。
- `THIRD_PARTY_NOTICES.md` は、BTstack がある場合に `vendor/btstack/LICENSE` と `vendor/btstack/3rd-party/README.md` を source / binary 配布物と一緒に利用可能に保つと記録している。
- `THIRD_PARTY_NOTICES.md` は、toml11 が MIT License であり、`vendor/toml11/LICENSE` を source / binary 配布物と一緒に利用可能に保つと記録している。
- `README.md` は、BTstack を含む build / source distribution が BTstack license の対象にもなると記録している。
- `spec/architecture/daemon-architecture-cutover.md` は、release packaging と BTstack license を含む配布方針を architecture cutover の対象外としていた。
- `spec/operations/release-build-and-publish.md` は、M1 で初回 binary artifact の名前、内容、notice 同梱方法、BTstack 表記を決めると記録している。

この spec は release / license の運用方針を扱う。
Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は変更しない。
根拠監査は not applicable とする。

## 6. 関連 work units

- `work-units/complete/local_103/RELEASE_ARTIFACT_LICENSE_BOUNDARY.md`
- `work-units/complete/local_102/RELEASE_READINESS_PLAN.md`
- `work-units/complete/local_066/RELEASE_LICENSE_BOUNDARY_POLICY.md`
- `work-units/complete/local_050/DAEMON_APPLICATION_BOUNDARY_REARCHITECTURE.md`
- `work-units/complete/local_055/REARCHITECTURE_CUTOVER_ACCEPTANCE_AND_CLEANUP.md`

## 7. 未解決事項

- package manager 配布を行うかどうかは未定である。
- Linux binary artifact を初回 release に含めるかどうかは未定である。libusb 実機経路が未確認のため、初回 release の必須 artifact にはしない。
- installer を作るかどうかは未定である。
- GitHub Release を draft で作るか、tag workflow 完了後に publish するかは `spec/operations/release-build-and-publish.md` の M5 / M7 で扱う。
