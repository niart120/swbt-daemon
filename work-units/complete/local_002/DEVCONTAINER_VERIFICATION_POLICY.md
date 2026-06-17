# Dev Container Verification Policy

## 1. 概要

Dev Container を標準検証環境とする方針が、AGENTS / README / project skills / CI /
Git hooks で一貫して伝わるようにする work unit。

特に、Codex が `SWBT_ALLOW_HOST_BUILD=1` を自発的に付けて host 側 toolchain で
configure / build / test を進めないこと、標準検証の入口を Makefile に固定して
host からは Dev Container CLI へ委譲することを明記する。

## 2. 対象範囲

- `SWBT_ALLOW_HOST_BUILD=1` の扱いを、ユーザが明示した unsupported host build に限定する。
- configure、build、test、format、clang-tidy、sanitizer、cross build を別ステップとして扱う。
- 標準検証コマンドは Makefile target に集約する。
- Dev Container 内では Makefile が CMake / CTest / format / static analysis を直接実行する。
- Dev Container 外の host では Makefile が host toolchain を使わず Dev Container CLI へ委譲する。
- CI は `devcontainers/ci` action で `.devcontainer/devcontainer.json` を使い、Dev Container 内で Makefile target を実行する。
- project skills の手順から host 実行へ誘導する曖昧さを減らす。

## 3. 対象外

- CMake preset の変更。
- 実機検証。

## 4. 関連 spec / docs

- `AGENTS.md`
- `README.md`
- `.agents/skills/pr-merge-cleanup/SKILL.md`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/agentic-self-review/SKILL.md`
- `.githooks/pre-commit`
- `.githooks/pre-push`
- `.github/workflows/ci.yml`
- `.devcontainer/Dockerfile`
- `Makefile`
- `spec/operations/development-tooling.md`
- `spec/dev-journal.md`

## 5. 根拠監査

not applicable。

Switch HID protocol、BTstack source selection、report timing、WinUSB/libusb の実装値は変更しない。

## 6. 設計メモ

- `SWBT_ALLOW_HOST_BUILD=1` は緊急回避やユーザ明示の host build opt-in であり、Codex の既定 fallback ではない。
- host 上の `make <target>` は `devcontainer up` と `devcontainer exec` へ委譲し、host toolchain を使わない。
- `.devcontainer/` 変更後は `make devcontainer-rebuild` で既存コンテナを作り直す。
- CI は runner native toolchain を apt で入れず、Dev Container を build して `make verify-ci` を実行する。
- Dev Container image には ASan/UBSan link に必要な clang runtime package を含める。
- `SWBT_BUILD_TESTS=ON` はテスト executable をビルド対象に含めるだけで、`ctest` 実行とは別である。
- PR 前の検証は、Dev Container または CI の configure / build / test / format / static analysis 結果を正本にする。

## 7. 対象ファイル

- `AGENTS.md`
- `README.md`
- `Makefile`
- `.devcontainer/Dockerfile`
- `.github/workflows/ci.yml`
- `.githooks/pre-commit`
- `.githooks/pre-push`
- `.agents/skills/pr-merge-cleanup/SKILL.md`
- `.agents/skills/tdd-workflow/SKILL.md`
- `.agents/skills/agentic-self-review/SKILL.md`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_002/DEVCONTAINER_VERIFICATION_POLICY.md`

## 8. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | docs が host opt-in をユーザ明示時のみと説明する | regression | docs | no |
| done | skills が Dev Container / CI gate を configure/build/test/format/static/cross build に適用する | regression | workflow | no |
| done | docs が build と test を別ステップとして説明する | regression | docs | no |
| done | host の Makefile target が Dev Container CLI へ委譲する | regression | workflow | no |
| done | CI が Dev Container 内で `make verify-ci` を実行する | regression | workflow | no |
| done | hooks が標準検証入口として Makefile target を使う | regression | workflow | no |
| done | `git diff --check` が通る | regression | docs | no |

## 9. 検証

- `make help`: pass。
- `make -n verify-ci-in-container SWBT_DEVCONTAINER=1`: pass。Dev Container 内 target の順序を dry-run で確認した。
- `make devcontainer-rebuild`: pass。`.devcontainer/Dockerfile` の `libclang-rt-18-dev` 追加を反映するため、既存コンテナを再作成した。
- `make list-presets`: pass。host から Dev Container CLI へ委譲し、container 内で `cmake --list-presets` を実行した。
- `make format-check`: pass。
- `make debug`: pass。`linux-debug` の configure、build、CTest を実行し、`swbt_smoke_test` が 1/1 passed。
- `make verify-ci`: pass。Dev Container 内で format check、`linux-clang-tidy`、`linux-debug` configure/build/test、`linux-asan` configure/build/test、`windows-mingw-debug` cross build を実行した。
- `git diff --check`: pass。

補足: 初回の `make verify-ci` は `linux-asan` link で `libclang_rt.asan` が見つからず失敗した。
`.devcontainer/Dockerfile` に `libclang-rt-18-dev` を追加し、`make devcontainer-rebuild` 後の再実行で解消した。

## 10. 実機実行条件

実機検証は不要。

この work unit は docs と skill guidance の変更だけを扱い、Switch pairing、HID advertising、
report loop、Bluetooth adapter 操作を実行しない。

## 11. チェックリスト

- [x] `SWBT_ALLOW_HOST_BUILD=1` の扱いをユーザ明示 opt-in に限定した。
- [x] configure / build / test の区別を明記した。
- [x] 標準検証入口を Makefile target に集約した。
- [x] host からの標準検証が Dev Container CLI へ委譲される。
- [x] CI が Dev Container を使って検証する。
- [x] skills から host fallback へ誘導する曖昧さを減らした。
- [x] 検証結果または未実行理由を記録した。
- [x] 実機状態を記録した。
