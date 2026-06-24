# Just Clean Recipe

## 1. 概要

この work unit は、`just` に CMake / Ninja 生成物を削除する `clean` recipe を追加する。

完了後は、`just clean` で Git の除外対象である `build/` と `cmake-build-*` だけを削除できる。`tmp/`、Dev Container、submodule は削除対象にしない。

## 2. 起点 / ユースケース

source:

- User request: `just` に clean 相当のコマンドがないため、`just clean` だけ追加する。
- Existing tooling contract: `justfile` は標準タスク入口であり、CMake preset の binary dir は `build/` 配下にある。
- Existing ignore contract: `.gitignore` は `/build/` と `/cmake-build-*/` を build output として除外している。

use case:

- actor: contributor / maintainer。
- input/state: CMake / Ninja build 後に `build/` 配下の生成物が残っている。
- expected observation: `just clean` が CMake build 出力だけを削除対象にし、実機証跡や一時 handoff を含む `tmp/` には触れない。
- constraints: Dev Container の再作成、submodule cleanup、実機 artifact cleanup は行わない。

source から use case への判断:

- `clean` は build 出力の整理に限定する。`tmp/` は実機証跡の置き場として参照されることがあるため、通常の clean には含めない。
- 削除対象は Git の除外設定に従う。tracked file を削除しないため、`git clean -fdX -- build/ 'cmake-build-*'` を使う。

## 3. 対象範囲

- `justfile` に `clean` recipe を追加する。
- Windows native PowerShell と POSIX shell の両方で同じ削除対象を使う。
- README、AGENTS、operations spec に `just clean` の対象範囲を記録する。
- dry-run で `clean` の recipe 展開と削除対象を確認する。

## 4. 対象外

- `tmp/` の削除。
- Dev Container の削除または再作成。
- submodule cleanup。
- `clean-tmp`、`distclean`、`clobber` などの追加 recipe。
- 実際の build artifact 削除の実行。

## 5. 関連 spec / docs

- `justfile`
- `README.md`
- `AGENTS.md`
- `spec/operations/development-tooling.md`
- `.gitignore`
- `CMakePresets.json`

## 6. 根拠監査

not applicable。

この work unit は task runner と build output cleanup だけを扱う。Switch HID report bytes、BTstack source selection、report period、subcommand、SPI、rumble、descriptor data、WinUSB/libusb の実装値は変更しない。

## 7. 設計メモ

`just clean` は Dev Container 経由にしない。削除対象は workspace 上の ignored build output であり、C toolchain の再現性とは関係しない。

実行内容は `git clean -fdX -- build/ 'cmake-build-*'` とする。`-X` により Git の除外対象だけを削除し、tracked file は削除しない。

## 8. 対象ファイル

- `justfile`
- `README.md`
- `AGENTS.md`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_070/JUST_CLEAN_RECIPE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | `just --list` exposes `clean` as a public recipe | regression | tooling | no |
| done | `clean` dispatches to OS-specific cleanup commands | characterization | tooling | no |
| done | cleanup target is limited to ignored `build/` and `cmake-build-*` paths | regression | tooling | no |
| done | docs and operations spec describe that `tmp/`, Dev Container, and submodule are not touched | regression | docs | no |

## 10. 検証

- `git branch --show-current`: pass。`main`。
- `git status --short`: pass。開始時は clean。
- `just --list`: pass。`clean # Remove ignored CMake build outputs.` が public recipe として表示された。
- `just --dry-run clean`: pass。Windows host では `_clean-windows` へ委譲することを確認した。
- `just --dry-run --justfile justfile _clean-windows`: pass。`git -C "<repo>" clean -fdX -- build/ 'cmake-build-*'` を実行する形に展開された。
- `just --dry-run --justfile justfile _clean-unix`: pass。POSIX shell でも同じ `git clean` pathspec を使う形に展開された。
- `git clean -ndX -- build/ 'cmake-build-*'`: pass。現在の workspace では `Would remove build/` だけを表示した。
- `git diff --check`: pass。PowerShell 上では Markdown file の LF/CRLF 変換 warning が出たが、whitespace error はなかった。

未実行:

- `just clean`: build artifact を実際に削除するため未実行。dry-run で削除対象を確認した。
- `just verify`: C source、CMake preset、test target は変更していないため未実行。

## 11. 実機実行条件

実機不要。

Bluetooth adapter、Switch pairing、HID advertising、report loop を実行しない task runner cleanup の変更である。`SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` は不要。

## 12. 先送り事項

none。

`clean-tmp` や `distclean` は今回の対象外だが、現時点では追加する必要がない。

## 13. チェックリスト

- [x] `just clean` を追加した。
- [x] 削除対象を `build/` と `cmake-build-*` に限定した。
- [x] `tmp/`、Dev Container、submodule を対象外として docs / spec に記録した。
- [x] dry-run で recipe 展開と削除対象を確認した。
- [x] 根拠監査の状態を記録した。
- [x] 実機未実行理由を記録した。
