# 開発ジャーナル

swbt-daemon の設計観測、未解決事項、先送り判断の記録。

## 2026-06-18: エージェントと skill の Phase 3 導入

### 現状

`tmp/swbt_agent_skill_adoption_policy.md` は、プロジェクト固有の Codex guidance として Phase 1 から Phase 3 までの導入を定義した。

### 観測

リポジトリには root `AGENTS.md`、swbt 固有の project skills、work unit record のひな形、PR テンプレートがある。
Dev Containers は再現可能な標準開発環境として明示されている。

### 判断

今後の実装作業では、安定した要件を `tmp/` から範囲を絞った work unit record へ移す。
根拠監査、実機実行条件の確認、TDD、セルフRv、PR の後片付けには project skills を使う。

## 2026-06-18: Dev Container 前提のローカル検証導線

### 現状

PR #1 のローカル確認は Dev Container 外で実行され、`cmake` / `ninja` はあったが `clang-format`、`clang`、`clang-tidy`、MinGW は無かった。

### 観測

`SWBT_ALLOW_HOST_BUILD=1` 付きの `linux-debug` configure/build/test は通った一方、format、clang-tidy、ASan、Windows cross build は CI 依存になった。

### 判断

次の work unit で、Dev Container 起動確認、必須 toolchain の presence check、`scripts/check-format.sh` までを一括確認する smoke command を追加する。
host build opt-in は残すが、PR 前の完全検証は Dev Container または CI を正本にする。

## 2026-06-27: BTstack source selection のビルド負荷観測

### 現状

`cmake/btstack_sources.cmake` はバックエンドごとに `vendor/btstack` の source を広く選ぶ。`local_076` は debug gate の build target を test executable に絞ったが、BTstack source selection は変更していない。

### 観測

既存ビルド出力の `compile_commands.json` では、linux-debug / linux-asan が 254 コンパイル項目中 170 項目、windows-mingw-debug が 253 項目中 169 項目を `vendor/btstack` に使っていた。`.ninja_log` の直近コンパイル処理時間の合計では、`vendor/btstack` が linux-debug 90.72s / 136.18s、linux-asan 91.10s / 138.35s、windows-mingw-debug 93.38s / 142.95s を占めた。これは並列ビルドの実経過時間ではない。

### 判断

BTstack source selection 削減は `local_076` の後続 work unit にしない。次に扱う条件は、clean build の実経過時間で `swbt_btstack` が主要因だと確認でき、Classic HID / libusb / windows-winusb に必要な source を根拠監査できる場合とする。
