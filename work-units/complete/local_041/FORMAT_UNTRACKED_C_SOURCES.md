# Format Untracked C Sources

## 1. 概要

`just format` と `just format-check` が、Git に追跡済みの C source/header だけでなく、commit 前の未追跡 C source/header も対象にするようにした。

これにより、新規 `.c` / `.h` を stage または commit する前に formatter へ通せる。PR checkout 後の CI で初めて format violation が見つかる状態を防ぐ。

## 2. 起点 / ユースケース

source:

- ユーザ要求: PR #38 の調査結果を受け、formatter 対象列挙を修正し、`pr-merge-cleanup` まで実施する。
- bug: `scripts/format.sh` と `scripts/check-format.sh` が `git ls-files api apps swbt tests` だけを使い、未追跡 C source/header を対象に含めていなかった。

use case:

- actor: Windows native PowerShell または Dev Container 内で `just format` / `just format-check` を実行する開発者。
- 状態: `api/`、`apps/`、`swbt/`、`tests/` 配下に未追跡の `.c` / `.h` がある。
- 期待結果: 未追跡の非 ignored `.c` / `.h` も formatter 対象に入り、CI checkout 後に初めて format error が出る状態を避ける。
- 制約: `vendor/btstack/`、build output、`.gitignore` 対象の生成物は formatter 対象にしない。

source から use case へ変換した判断:

- 問題は CI と local の `just` 経路差ではなく、`git ls-files` が未追跡ファイルを返さないことによる対象ファイル集合の差である。
- `git ls-files --cached --others --exclude-standard -- api apps swbt tests` を使い、追跡済みと未追跡の非 ignored ファイルを同じ formatter path に渡す。

## 3. 対象範囲

- `scripts/format.sh` の file enumeration を更新する。
- `scripts/check-format.sh` の file enumeration を更新する。
- `spec/operations/development-tooling.md` に formatter 対象の方針を記録する。
- 未追跡 C source/header が `format-check` の対象になることを検証する。

## 4. 対象外

- formatter rule の変更。
- CMake presets、CTest presets、Dev Container image の変更。
- Git hooks の導入状態の修正。
- `vendor/btstack/` の formatter 対象化。

## 5. 関連 spec / docs

- `spec/operations/development-tooling.md`
- `scripts/format.sh`
- `scripts/check-format.sh`

## 6. 根拠監査

not applicable。

この work unit は tooling file enumeration だけを変更する。Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は追加または変更しない。

## 7. 設計メモ

- `--cached` で追跡済みファイルを含める。
- `--others` で未追跡ファイルを含める。
- `--exclude-standard` で `.gitignore` などの標準 ignore 対象を除外する。
- pathspec は従来通り `api apps swbt tests` に限定する。

## 8. 対象ファイル

- `scripts/format.sh`
- `scripts/check-format.sh`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_041/FORMAT_UNTRACKED_C_SOURCES.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | 未追跡の非 ignored `.c` が `just format-check` の対象になり、未整形なら失敗する | regression | tooling | no |
| green | 未追跡の非 ignored `.c` が `just format` で整形され、その後 `just format-check` が成功する | regression | tooling | no |
| green | ignored/generated file は formatter 対象にしない | regression | tooling | no |

## 10. 検証

```text
一時ファイル: tests/format_probe_untracked.c
内容: int main(void){return 0;}
just format-check -> expected fail
結果: tests/format_probe_untracked.c の clang-format violation を検出
```

```text
just format -> pass
just format-check -> pass
結果: 未追跡 tests/format_probe_untracked.c が整形され、再検査に成功
```

```text
一時ファイル: tests/.gitignore
内容: format_probe_ignored.c
一時ファイル: tests/format_probe_ignored.c
内容: int ignored(void){return 0;}
git check-ignore tests/format_probe_ignored.c -> tests/format_probe_ignored.c
git ls-files --cached --others --exclude-standard -- api apps swbt tests | Select-String -Pattern 'format_probe' -> tests/format_probe_untracked.c のみ
just format-check -> pass
結果: ignored C file は formatter 対象外
```

```text
検証用一時ファイル削除後:
just format-check -> pass
just verify-ci -> pass
```

`just verify-ci` の内訳:

- `_format-check-in-container`: pass
- `_tidy-in-container`: pass
- `_debug-in-container`: pass, 25/25 tests passed
- `_asan-in-container`: pass, 25/25 tests passed
- `_windows-cross-in-container`: pass

## 11. 実機実行条件

実機は不要。

この work unit は formatter 対象列挙の変更であり、Bluetooth adapter、Switch pairing、HID advertising、report loop を実行しない。

## 12. 先送り事項

none。

## 13. チェックリスト

- [x] source と use case が記録されている。
- [x] formatter 対象が tracked と untracked non-ignored C source/header を含む。
- [x] ignored/generated file が formatter 対象外である。
- [x] 検証結果が記録されている。
- [x] 根拠監査状態が明確である。
- [x] 実機状態が明確である。
