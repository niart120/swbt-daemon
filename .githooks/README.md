# Git Hooks

このディレクトリは Git hooks の正本です。

clone 後に一度だけ次のいずれかを実行する。

```console
sh scripts/install-git-hooks.sh
```

```console
scripts/install-git-hooks.ps1
```

設定される値:

```console
git config core.hooksPath .githooks
```

Git は tracked hooks を clone 時に自動有効化しない。
これは、リポジトリ内の任意コードが clone だけで実行可能になることを避けるための安全設計である。

## Hooks

- `pre-commit`: staged diff の whitespace、`just` 経由の CMake presets 読み取り、staged C source の format を確認する。
- `commit-msg`: Conventional Commits の形式と subject 末尾句点なしを確認する。
- `pre-push`: `just debug` を実行する。host からは `justfile` が Dev Container CLI へ委譲する。
- `SWBT_FULL_PRE_PUSH=1`: pre-push で `just verify` を実行する。
- `SWBT_SKIP_HOOKS=1`: hook を明示的にスキップする。
