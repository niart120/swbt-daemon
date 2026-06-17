# Development Tooling

この spec は、formatter と linter の選定理由、実行経路、初期ルールを定める。

対象は `api/`、`apps/`、`swbt/`、`tests/` にある自前の C source と header である。

`vendor/btstack/` は upstream source なので、formatter と linter の対象にしない。

## 基本方針

- ローカルの標準実行環境は Dev Container とする。
- 標準検証コマンドの入口は Makefile target とする。
- host から Makefile target を実行した場合は、Dev Container CLI へ委譲する。
- Dev Container 定義を変更した後の再作成は `make devcontainer-rebuild` で行う。
- CI は GitHub Actions から Dev Container を起動して実行する。
- host OS に formatter や linter を入れることは必須にしない。
- host OS へ入れる場合は、エディタ補助や手元の事前確認として扱う。
- `SWBT_ALLOW_HOST_BUILD=1` はユーザが明示的に unsupported host build を許可した場合だけ使う。

host OS を標準経路にしない理由は、toolchain、system include、tool version の差で結果が揺れることを避けるためである。

## formatter

formatter は `clang-format` を使う。

選定理由は次のとおりである。

- C/C++ ecosystem で広く使われている。
- `.clang-format` だけで機械的に結果を揃えられる。
- Dev Container と CI に入れやすい。
- CMake や Ninja の build 経路に依存せず、format check を単独で実行できる。

初期ルールは `.clang-format` に置く。

現在の主な方針は次のとおりである。

- `BasedOnStyle: LLVM` を土台にする。
- indent は 4 spaces とする。
- column limit は 100 とする。
- tab は使わない。
- include の並び替えはしない。
- pointer alignment は `char *value` の形に寄せる。
- 短い `if` と loop を一行に畳まない。

format check は `make format-check` で実行する。

自動整形は `make format` で実行する。

## linter

linter と static analysis は `clang-tidy` を使う。

選定理由は次のとおりである。

- CMake の `C_CLANG_TIDY` に自然に接続できる。
- project には既に `SWBT_ENABLE_CLANG_TIDY` の入口がある。
- compile commands を前提に解析できるため、C source の include path や build option と整合しやすい。
- `clang-analyzer-*`、`bugprone-*`、`portability-*` で初期段階から検出価値の高い問題を拾える。

初期ルールは `.clang-tidy` に置く。

現在は対象範囲を `api/`、`apps/`、`swbt/`、`tests/` に絞り、system header は対象にしない。

`WarningsAsErrors: '*'` は初期段階の小さい codebase を前提にした強い設定である。

BTstack integration が進んで false positive やノイズが増えた場合は、check set または warnings-as-errors の範囲を見直す。

`clang-tidy` は Makefile 経由で実行する。

```console
make tidy
```

## 採用しなかった候補

- `cpplint` は Google C++ style への寄りが強く、この C11/CMake project の標準にはしない。
- `ruff` や `prettier` は主対象言語が異なるため採用しない。
- `uncrustify` や `astyle` は使えるが、Dev Container、CMake、CI との接続を増やす理由が薄い。
- `include-what-you-use` は有用だが、初期導入ではノイズと運用負荷が大きいため採用しない。

## Git hooks と CI

`pre-commit` は staged C source がある場合に `make format-check` を実行する。

`pre-push` は通常 `make debug` で configure、build、test を実行する。

`SWBT_FULL_PRE_PUSH=1` を指定した場合は `make verify` で format check、`linux-clang-tidy`、sanitizer、Windows cross build も実行する。

CI では Dev Container 内で `make verify-ci` を実行する。
