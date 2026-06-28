# Justfile Shell Entrypoint Inventory

## 1. 概要

`justfile` の公開 recipe と内部実行経路を棚卸しし、shell script を直接実行している format 系 recipe を Dev Container 内の `bash` 経由に寄せる。

完了後は、Windows filesystem checkout から Dev Container へ委譲する経路で、format script の実行が host 側の executable bit や shebang 解決に依存しない状態になる。公開 recipe の削除は、既存 spec / docs / CI で契約済みの入口を壊さない範囲に限る。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-28: `justfile` を棚卸しし、shell file を叩いているところを明示的に Dev Container の `bash` で起動する。不要な command や経路がないか確認して slim 化したい。
- 現状観測, 2026-06-28: `_format-in-container` は `scripts/format.sh`、`_format-check-in-container` は `scripts/check-format.sh` を直接実行している。
- 現状観測, 2026-06-28: 公開 recipe は `README.md` と `spec/operations/development-tooling.md` に標準入口として記録されている。`verify-ci` は `.github/workflows/ci.yml` から直接使われている。
- follow-up user request, 2026-06-28: `list-presets` の名前と実動作のずれだけを直す。

use case:

- actor: maintainer、CI、Codex agent。
- 入力または状態:
  - Windows native PowerShell または Unix shell から `just format`、`just format-check`、`just verify` を実行する。
  - Dev Container 内では format script が container 内 toolchain で実行される。
- 期待する観測結果:
  - format script は `bash scripts/format.sh` と `bash scripts/check-format.sh` として起動される。
  - `just list-presets` は CMake presets を表示し、pre-commit は quiet な `just check-presets` で CMake presets 読み取りだけを確認する。
  - 既存の公開 recipe 名と検証範囲は変わらない。
  - 不要と判断できない recipe は削除しない理由が record から追える。
- 制約:
  - `just verify` と `just verify-ci` の gate 意味を弱めない。
  - host build への退避経路を追加しない。
  - CI、Git hook、README、operations spec の既存入口を壊さない。

source から use case への変換:

今回の主目的は shell script の起動方法を堅くすることである。公開 recipe の削除は影響範囲が広く、現状では `spec/operations/development-tooling.md` が標準 recipe として列挙しているため、この work unit では削除ではなく棚卸結果の記録に留める。

## 3. 対象範囲

- `justfile` の公開 recipe と内部 recipe を棚卸しする。
- `scripts/format.sh` と `scripts/check-format.sh` の起動を Dev Container 内の `bash` 経由にする。
- `list-presets` を表示用 recipe とし、hook 用に `check-presets` を追加する。
- `spec/operations/development-tooling.md` に format script 起動の判断と今回の work unit を記録する。
- 削除しない recipe の判断を記録する。

## 4. 対象外

- 公開 recipe 名の変更。
- `just verify`、`just verify-ci`、`pre-push` の検証範囲変更。
- host build 経路の追加。
- CMake presets、CTest presets、GitHub Actions workflow の再設計。
- `scripts/format.sh` と `scripts/check-format.sh` の対象ファイル選定変更。
- Switch protocol bytes、BTstack source selection、report period、WinUSB/libusb behavior の変更。

## 5. 関連 spec / docs

- `justfile`
- `README.md`
- `.github/workflows/ci.yml`
- `.githooks/`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md`
- `work-units/complete/local_041/FORMAT_UNTRACKED_C_SOURCES.md`
- `work-units/complete/local_068/JUST_DEBUG_TIDY_CONTRACT.md`
- `work-units/complete/local_075/BUILD_CONFIGURATION_OPTIMIZATION.md`
- `work-units/complete/local_076/BUILD_TEST_TARGETS.md`

## 6. 根拠監査

not applicable。

Switch HID report bytes、BTstack source selection、report timing、subcommand、SPI、rumble、descriptor data、WinUSB/libusb behavior を変更しない。

## 7. 設計メモ

Tidy status:

- classification: structure change。
- decision: tidy first。
- reason: format script の起動方法だけを変更し、公開 recipe、検証範囲、format 対象、CMake / CTest 挙動を変えない。Windows filesystem checkout から Linux Dev Container へ bind mount する経路で、script の executable bit と shebang 解決に依存しない形へ寄せる。
- verification: `just --dry-run _format-in-container`、`just --dry-run _format-check-in-container`、`just --dry-run verify`、`just format-check`。

棚卸結果:

- `verify-ci` は CI workflow が `just verify-ci` を直接呼ぶため削除しない。
- `devcontainer-rebuild` は README と operations spec が Dev Container 定義変更後の再作成入口として扱っているため削除しない。
- `build-daemon-debug` と `build-debug-client` は operations spec が daemon / debug client の target build 入口として扱っているため削除しない。
- `build-tests-debug` は README と operations spec が unit test executable target の build 入口として扱っているため削除しない。
- `default` と `help` は同じ一覧表示だが、`default` は引数なしの入口、`help` は明示的な一覧入口として残す。
- `list-presets` は名前どおり `cmake --list-presets` の出力を表示する。pre-commit は表示を目的にしないため、CMake presets 読み取り確認だけを行う `check-presets` を使う。

## 8. 対象ファイル

- `justfile`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_098/JUSTFILE_SHELL_ENTRYPOINT_INVENTORY.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | format recipe dry-run shows `bash scripts/format.sh` and format-check recipe dry-run shows `bash scripts/check-format.sh` | regression | tooling | no |
| green | verify dry-run still routes through the same `verify` entrypoint and keeps the documented gate name | regression | tooling | no |
| green | public recipe inventory keeps documented command surface unless a recipe is proven unused | characterization | tooling/docs | no |
| green | `just format-check` runs through Dev Container and passes with the bash-based format script invocation | regression | tooling/integration | no |
| green | `just list-presets` displays CMake presets while `just check-presets` checks readability without preset output | regression | tooling | no |
| green | pre-commit uses `just check-presets` for quiet CMake preset readability check | regression | hook/tooling | no |

## 10. 検証

- `just --dry-run _format-in-container`: pass。`bash scripts/format.sh` を表示した。
- `just --dry-run _format-check-in-container`: pass。`bash scripts/check-format.sh` を表示した。
- `just --dry-run verify`: pass。top-level recipe は従来どおり `_run-or-delegate verify` を呼ぶ。
- `just format-check`: pass。Dev Container CLI は既存 container を使い、container 内で `bash scripts/check-format.sh` を実行した。
- `rg -n "^\\s+scripts/(format|check-format)\\.sh\\b|bash scripts/(format|check-format)\\.sh" justfile`: pass。直接実行は残らず、`bash` 経由の 2 行だけを検出した。
- `just --list`: pass。公開 recipe 一覧は維持されている。
- `git diff --check`: pass。whitespace error なし。Windows checkout の行末設定により `spec/operations/development-tooling.md` に CRLF 置換 warning が出たが、`*.md` は `.gitattributes` の LF 固定対象ではない。
- `just verify`: pass。Dev Container 内で `format-check`、`tidy`、fresh debug CTest、ASan CTest、Windows cross build が完了した。`format-check` は `bash scripts/check-format.sh` 経由で実行された。
- `just --dry-run _list-presets-in-container`: pass。`cmake --list-presets` を表示した。
- `just --dry-run _check-presets-in-container`: pass。`cmake --list-presets >/dev/null` を表示した。
- `just check-presets`: pass。Dev Container 内で CMake presets 読み取り確認を quiet に実行した。
- `just list-presets`: pass。Dev Container 内で `linux-debug`、`linux-asan`、`linux-clang-tidy`、`windows-mingw-debug` を表示した。
- `sh .githooks/pre-commit`: pass。staged diff の whitespace check 後、`just check-presets` を実行した。staged C source はなかったため `just format-check` は実行しなかった。
- `just --list`: pass。`check-presets` は CMake presets 読み取り確認、`list-presets` は CMake presets 表示として一覧に出る。
- `git diff --check`: pass。whitespace error なし。未属性指定の `.githooks/README.md`、`.githooks/pre-commit`、`spec/operations/development-tooling.md` には CRLF 置換 warning が出た。

## 11. 実機実行条件

実機不要。`justfile`、operations spec、work unit record の整理に閉じ、Bluetooth adapter open、Switch pairing、HID advertising、report loop は実行しない。

## 12. 先送り事項

none。

## 13. チェックリスト

- [x] source を user request と現状の `justfile` 観測から特定した。
- [x] use case を format script 起動と公開 recipe 棚卸に変換した。
- [x] 根拠監査と実機実行条件の要否を分けた。
- [x] `justfile` の direct shell script invocation を `bash` 経由に変更した。
- [x] `list-presets` を表示用、`check-presets` を hook 用の quiet check に分けた。
- [x] operations spec に安定判断を記録した。
- [x] 検証結果を記録した。
- [x] 完了 record を `work-units/complete/local_098/` へ移した。
