# swbt-daemon Agent / Skill 導入方針

- 作成日: 2026-06-17
- 調査対象期間: 2026-05-17 から 2026-06-17 までの直近1ヶ月
- 対象ユーザ: `niart120`
- 対象リポジトリ: `niart120/swbt-daemon`

## 目的

直近1ヶ月で `niart120` がアクティブに扱っていたリポジトリから、`AGENTS.md` と `.agents/skills/*/SKILL.md` の運用を調査し、`swbt-daemon` に導入する価値があるものを選別する。

この文書は導入方針であり、現時点で `AGENTS.md` や `.agents/skills` を本体へ追加するものではない。

## 調査方法

GitHub 上で次を確認した。

- 認証ユーザ: `niart120`
- repository search: `user:niart120 pushed:>=2026-05-17`
- commit search: `author:niart120 author-date:>=2026-05-17`
- PR search: `author:niart120 updated:>=2026-05-17`
- 各候補 repo の `AGENTS.md`、`.agents/skills/**/SKILL.md`、nested `AGENTS.md`

`pushed:>=2026-05-17` で該当した repo は次の4件だった。

| repo | default branch | 主な活動根拠 | Agent / skill 資産 | swbt への関連度 |
|---|---|---|---|---|
| `niart120/Project_NyX` | `master` | `Project_NyX#174`, `#171`, `#170` など。`ponkan-python` 連携、実機 capture、GUI 仕様が直近で更新されている。 | root `AGENTS.md`, `dev-journal`, `framework-spec-writing`, `macro-*`, `pr-merge-cleanup` | 中。Python/GUI/macro は不一致だが、仕様書・journal・実機依存境界は参考になる。 |
| `niart120/ponkan-python` | `master` | `ponkan-python#36`, `#35`, `#34` など。実機 capture library と release / Agentic SDD が直近で運用されている。 | root `AGENTS.md`, `agentic-sdd`, `agentic-self-review`, `spec-format`, `dev-journal`, `cc3dsfs-source-audit`, `n3dsxl-hardware-harness`, TDD 系, `pr-merge-cleanup`, `pypi-release` | 高。hardware safety、source audit、spec-driven workflow が `swbt-daemon` と近い。 |
| `niart120/5genSearch-web` | `main` | `5genSearch-web#150`, `#149`, `#146`, `#144` など。Codex migration と Rust/WASM / frontend spec 運用が直近で更新されている。 | root `AGENTS.md`, nested `wasm-pkg/AGENTS.md`, `spec/agent/AGENTS.md`, Vercel 系 skills, `gh-pages-release`, `pr-merge-cleanup` | 低から中。Rust/WASM の nested AGENTS と spec 運用は参考になるが、frontend / release skills は対象外。 |
| `niart120/swbt-daemon` | `main` | 現在の対象 repo。初期 skeleton と tmp 設計メモが存在する。 | 現時点で `.agents/` と `.codex/` は空。root / nested `AGENTS.md` なし。 | 導入先。 |

参照した代表ファイル:

- `ponkan-python`: https://github.com/niart120/ponkan-python/blob/master/AGENTS.md
- `ponkan-python` skills: https://github.com/niart120/ponkan-python/tree/master/.agents/skills
- `Project_NyX`: https://github.com/niart120/Project_NyX/blob/master/AGENTS.md
- `Project_NyX` skills: https://github.com/niart120/Project_NyX/tree/master/.agents/skills
- `5genSearch-web`: https://github.com/niart120/5genSearch-web/blob/main/AGENTS.md
- `5genSearch-web` Rust/WASM nested guide: https://github.com/niart120/5genSearch-web/blob/main/wasm-pkg/AGENTS.md

## swbt-daemon の現状

`swbt-daemon` は初期段階の C/CMake プロジェクトである。

- C11 / CMake / Ninja を主経路にする。
- 日常開発と CI 相当の再現環境は WSL2 + Dev Containers を基準にする。
- `.devcontainer/` は Ubuntu 24.04 上に CMake、Ninja、clang、clang-format、clang-tidy、mingw-w64、libusb、valgrind などを入れる。
- BTstack は `vendor/btstack` submodule として pin する。
- 自前コードは `api/`, `apps/`, `swbt/`, `tests/`, `docs/` に置く想定。
- Switch Pro Controller 相当の Bluetooth Classic HID Device daemon を実装する。
- daemon IPC は JSON Lines over local IPC を主経路にする。
- 実機検証は Windows native + 専用 USB Bluetooth dongle + WinUSB を優先する。
- Linux/libusb は unit test、protocol 検証、sanitizer、開発補助の経路として扱う。
- 現時点で Switch pairing や hardware behavior は未検証である。

この性質から、Python package / frontend / GUI / PyPI / GitHub Pages の skill は直接移植しない。移植候補は、仕様管理、source audit、実機安全境界、TDD、PR gate に限定する。

## 採用判断

### 採用候補 1: root `AGENTS.md`

採用する価値が高い。

`swbt-daemon` は初期段階だが、既に以下の判断が repo 固有の暗黙知になっている。

- BTstack submodule は原則 read-only に扱う。
- BTstack fork や patch は最後の手段にする。
- daemon IPC を主経路にし、C ABI は内部・テスト・将来の代替経路にする。
- daemon protocol に `tap` / `duration_ms` / `sequence` / `at_ms` を入れない。
- Windows native 実機検証と WSL2/Dev Container 日常開発を分ける。
- Dev Containers は C/CMake/Ninja の再現可能な主開発環境として扱い、個別の Windows host へ手作業で toolchain を入れる前提にしない。
- BTstack を含む binary を MIT-only と表現しない。

これらは agent が毎回誤って広げやすい領域なので、root `AGENTS.md` に固定するべきである。

推奨セクション:

- Communication: 日本語、事実ベース、簡潔。
- Project Overview: Switch Pro Controller daemon、BTstack submodule、daemon IPC。
- Current Scope / Non-goals: Joy-Con、NFC/IR、複数 controller、daemon-side macro、Python client は初期対象外。
- Repository Layout: `api/`, `apps/`, `swbt/`, `tests/`, `docs/`, `vendor/btstack/`。
- Development Environment: WSL2 + Dev Containers、Ubuntu 24.04、CMake/Ninja、clang、mingw-w64、libusb。
- BTstack Policy: submodule pin、upstream license、patch 方針、source selection 記録。
- Hardware Safety: 専用 Bluetooth dongle、WinUSB、Windows native、明示承認。
- C/CMake Rules: C11、CMake presets、ctest、sanitizer、warnings。
- Testing: unit / integration / hardware / manual log の分類。
- Git / Commit Rules: Conventional Commits、日本語 subject、scope は任意。

### 採用候補 2: `swbt-source-audit`

`ponkan-python` の `cc3dsfs-source-audit` を swbt 用に改名・縮小して採用する価値が高い。

swbt では原典が `cc3dsfs` ではなく、次のように分散している。

- BTstack source / documentation
- dekuNukem の Switch reverse engineering notes
- Linux `hid-nintendo.c`
- `joycontrol`
- Switch Pro Controller の HID descriptor / report / subcommand 実測
- Windows WinUSB / libusb の backend 挙動

導入時の名前は `.agents/skills/swbt-source-audit/SKILL.md` がよい。

書かせるべき内容:

- report ID、subcommand、SPI address、rumble packet、HID descriptor、report period の根拠。
- 事実、推定、未検証仮説の分離。
- 参照元 path / URL / commit / line の記録。
- 実機で確認した値の OS、Switch firmware、Bluetooth dongle、driver、BTstack commit、daemon commit。

禁止事項:

- 根拠不明の magic number を確定仕様として実装しない。
- BTstack 本体を直接改変する判断を source audit なしで進めない。
- 実測値と文献値を混同しない。

### 採用候補 3: `swbt-hardware-harness`

`ponkan-python` の `n3dsxl-hardware-harness` を swbt 用に大きく書き換えて採用する価値が高い。

swbt の実機リスクは、N3DSXL capture board ではなく Bluetooth adapter と Nintendo Switch pairing にある。従って安全条件は次のように置き換える。

- 普段使いの内蔵 Bluetooth / 常用 dongle を使わない。
- 専用 USB Bluetooth dongle を使う。
- Windows では Zadig で対象 dongle に WinUSB を割り当てたことを記録する。
- 実機 command、pairing、HID device advertising、report spam は人間の明示承認なしに実行しない。
- 実機検証時は `docs/hardware-test-log.md` に OS、dongle VID/PID、driver、BTstack commit、report period、Switch firmware、結果を記録する。
- owner disconnect neutral、heartbeat timeout、fail-safe neutral を実機試験項目に入れる。

導入時の名前は `.agents/skills/swbt-hardware-harness/SKILL.md` がよい。

推奨 marker / flag:

- CTest label: `hardware`
- 実行フラグ: `SWBT_RUN_HARDWARE=1`
- 明示承認フラグ: `SWBT_HARDWARE_APPROVED=1`

ただし、初期段階では自動 hardware test が存在しないため、skill はまず手順確認と log 記録用にする。

### 採用候補 4: `swbt-spec-format`

`ponkan-python` の `spec-format` と `Project_NyX` の `framework-spec-writing` は参考になるが、swbt にはそのまま入れない。

理由:

- 現在の swbt は `tmp/` に設計メモを置いている。
- まだ `spec/initial`, `spec/wip`, `spec/complete` の運用を始めていない。
- いきなり重い spec lifecycle を入れると、初期 skeleton の実装速度に対して管理負荷が大きい。

導入方針:

1. 初期 commit 前後は既存の `tmp/*.md` を維持する。
2. 実装 Work Unit が増え始めたら、`spec/initial/`, `spec/wip/local_{nnn}/`, `spec/complete/local_{nnn}/`, `spec/dev-journal.md` を導入する。
3. そのタイミングで `.agents/skills/swbt-spec-format/SKILL.md` を追加する。
4. `tmp/` の安定化した設計メモは `spec/initial` または `docs/` へ昇格し、古い検討メモとして残すものだけ `tmp/` に置く。

`swbt-spec-format` の必須セクション案:

- 概要
- 対象ファイル
- 参照元 / source audit
- 設計方針
- 実装仕様
- TDD Test List
- 検証コマンド
- Hardware gate
- 実装チェックリスト

### 採用候補 5: TDD 系 skills

`ponkan-python` の `tdd-workflow`, `tdd-test-list`, `tdd-one-cycle`, `tidy-first` は有用だが、初回導入では分割しすぎない方がよい。

swbt では C protocol core の小さい純粋関数が増える見込みなので、TDD との相性は良い。

導入方針:

- Phase 1 では root `AGENTS.md` に TDD 原則だけを書く。
- Phase 2 で `.agents/skills/swbt-tdd-workflow/SKILL.md` として、test-list / one-cycle / tidy-first を1つにまとめた軽量版を作る。
- 実行コマンドは Python / `uv` ではなく CMake / CTest に置き換える。

swbt 用コマンド例:

```console
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
cmake --preset linux-asan
cmake --build --preset linux-asan
ctest --preset linux-asan
```

TDD で特に対象にする領域:

- controller state validation
- button / stick report packing
- subcommand parser / response builder
- SPI flash read response
- rumble packet parser
- JSON Lines IPC parser
- owner acquire / release / disconnect neutral

### 採用候補 6: `agentic-self-review`

採用価値は中から高。

実装が進むと、swbt は次の gate を同時に扱う必要がある。

- Requirements / non-goals
- Source audit
- C unit tests
- sanitizer
- Windows cross build
- hardware not-run reason
- BTstack license / notice
- integration review

`agentic-self-review` はこの gate 結果を PR 前に圧縮する用途で有効である。

ただし、初期導入では root `AGENTS.md` の「完了報告ルール」に含め、専用 skill 化は Phase 2 でよい。

### 採用候補 7: `dev-journal`

採用価値は高い。

swbt では report rate、BTstack source selection、WinUSB/libusb 差、Switch 側の応答、daemon IPC 境界など、仕様書に昇格する前の観測が多くなる。これは `tmp/` の単発メモより、`spec/dev-journal.md` または `docs/dev-journal.md` に時系列で集約した方が後から拾いやすい。

導入方針:

- Phase 1 では `tmp/` を維持する。
- Phase 2 で `spec/dev-journal.md` を作成し、`.agents/skills/swbt-dev-journal/SKILL.md` を追加する。
- `docs/hardware-test-log.md` は実機観測、`spec/dev-journal.md` は設計判断・先送り事項、と役割を分ける。

### 採用を見送るもの

| 候補 | 見送り理由 |
|---|---|
| `pypi-release` | Python package / PyPI release 用。swbt の初期段階には不要。 |
| `gh-pages-release` | GitHub Pages release 用。swbt には該当しない。 |
| Vercel React skills | frontend / React / Vercel 固有。swbt には該当しない。 |
| `macro-development`, `macro-spec-writing` | Project NyX のマクロ API 固有。swbt の daemon IPC client は将来タスク。 |
| `framework-spec-writing` の丸ごと移植 | Python framework 用に重い。spec-format の一部だけ採用する。 |
| `tcr-workflow-exp` | TCR を明示的に使う段階ではない。必要になった時だけ再検討する。 |
| `pr-merge-cleanup` の即時導入 | GitHub Flow 自体は有用だが、現 repo は初期 skeleton 段階。PR template / branch 運用が固まってから入れる。 |

## 推奨導入順

### Phase 1: 最小導入

目的は、agent が swbt のスコープと安全境界を外さないようにすること。

追加候補:

```text
AGENTS.md
.agents/skills/swbt-source-audit/SKILL.md
.agents/skills/swbt-hardware-harness/SKILL.md
```

この段階で固定すること:

- BTstack submodule read-only 方針。
- daemon IPC 主経路。
- C ABI の位置づけ。
- WSL2 + Dev Containers を日常開発・Linux build・Windows cross build・静的解析の基準環境にする。
- 初期 non-goals。
- hardware command は明示承認なしに実行しない。
- source audit なしに protocol / report rate / HID descriptor 値を確定しない。

### Phase 2: 仕様駆動・TDD 導入

実装 Work Unit が複数並び始めたら導入する。

追加候補:

```text
spec/initial/
spec/wip/
spec/complete/
spec/dev-journal.md
.agents/skills/swbt-spec-format/SKILL.md
.agents/skills/swbt-tdd-workflow/SKILL.md
.agents/skills/swbt-dev-journal/SKILL.md
.agents/skills/swbt-agentic-self-review/SKILL.md
```

この段階で既存 `tmp/` のうち、恒久的な設計情報は `spec/initial` または `docs/` に移す。

### Phase 3: PR / release 運用

CI、PR template、branch 運用が安定した後に導入する。

追加候補:

```text
.github/PULL_REQUEST_TEMPLATE.md
.agents/skills/pr-merge-cleanup/SKILL.md
```

swbt 用に調整する点:

- 既定 merge method は repo 設定とユーザ方針に合わせる。
- Testing セクションに CMake / CTest / Windows cross build / hardware not-run reason を必ず含める。
- Hardware セクションを `docs/hardware-test-log.md` と対応させる。
- BTstack license / notices に触れる変更では `THIRD_PARTY_NOTICES.md` の確認を必須にする。

## root AGENTS.md の草案構成

実際に導入する場合は、次の構成を推奨する。

```markdown
# swbt-daemon Agent Guide

## Communication

## Project Overview

## Current Scope

## Non-goals

## Repository Layout

## Development Environment

## BTstack Policy

## Daemon IPC Policy

## Hardware Safety

## C / CMake Rules

## Testing And Verification

## Documentation Rules

## Commit Rules
```

`Development Environment` には次を明記する。

- 主開発環境は WSL2 + Dev Containers とする。
- `.devcontainer/Dockerfile` は Ubuntu 24.04、CMake、Ninja、clang、clang-format、clang-tidy、mingw-w64、libusb headers、valgrind を含む再現環境である。
- `.devcontainer/devcontainer.json` は VS Code C/C++ と CMake Tools extension を推奨し、container user は `ubuntu` とする。
- Linux native build、sanitizer、unit test、Windows MinGW cross build は Dev Container 内で再現できる前提にする。
- Windows native は WinUSB driver、Bluetooth dongle、Switch pairing、latency / report rate 実測のための実機検証環境として別扱いにする。
- host OS へ個別 toolchain を手作業で入れることを通常の前提にしない。

`ponkan-python` の `AGENTS.md` から流用するのは構成と運用思想だけにする。Python, `uv`, N3DSXL, cc3dsfs, PyPI などの固有語は残さない。

## swbt-source-audit の草案要点

```markdown
---
name: swbt-source-audit
description: "BTstack / Switch HID / Linux hid-nintendo / joycontrol などの参照元から、swbt-daemon に必要な protocol facts を抽出し、事実・推定・未検証仮説を分けて記録する skill。"
---

# swbt Source Audit

## 対象

- BTstack source / docs
- dekuNukem Switch reverse engineering notes
- Linux hid-nintendo.c
- joycontrol
- 実機 capture log / hardware-test-log

## 記録ルール

- 値には参照元 path / URL / commit を付ける。
- 文献値、upstream 実装値、実機観測値を分ける。
- report period は固定仕様ではなく実測候補として扱う。
- BTstack 本体 patch が必要な場合は理由と代替案を記録する。
```

## swbt-hardware-harness の草案要点

```markdown
---
name: swbt-hardware-harness
description: "Nintendo Switch と専用 USB Bluetooth dongle を使う swbt-daemon 実機検証の安全条件、承認境界、記録項目を確認する skill。"
---

# swbt Hardware Harness

## 実行前チェック

- 専用 USB Bluetooth dongle を使う。
- 普段使いの Bluetooth adapter を使わない。
- Windows では WinUSB 割り当て状態を記録する。
- 実機 command / pairing / report loop は人間承認後だけ実行する。

## 記録先

- `docs/hardware-test-log.md`

## 記録項目

- OS / driver / dongle VID:PID / BTstack commit / swbt commit
- Switch firmware / pairing 手順 / report period
- subcommand / input report / disconnect behavior
- cleanup 結果
```

## 導入時の完了条件

実際に Phase 1 を実装する場合、完了条件は次とする。

- root `AGENTS.md` が swbt 固有の scope、non-goals、BTstack policy、hardware safety を含む。
- `.agents/skills/swbt-source-audit/SKILL.md` が N3DSXL / cc3dsfs ではなく BTstack / Switch HID 向けに書き換わっている。
- `.agents/skills/swbt-hardware-harness/SKILL.md` が Bluetooth dongle / WinUSB / Switch pairing 向けに書き換わっている。
- skill 本文に Python / `uv` / PyPI / React / Vercel 固有の command が残っていない。
- root `AGENTS.md` が WSL2 + Dev Containers を標準開発環境として扱い、`.devcontainer/` の役割を明記している。
- CMake / CTest の検証コマンドが root `AGENTS.md` に明記されている。
- hardware 実行は `SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` のような明示条件なしに走らない方針になっている。
- `vendor/btstack` を直接編集しない方針が明記されている。
- `THIRD_PARTY_NOTICES.md` と BTstack license 境界に触れている。

## 実装状況

2026-06-18 に Phase 1 から Phase 3 までを実装した。

Phase 1:

- `AGENTS.md`
- `.agents/skills/swbt-source-audit/SKILL.md`
- `.agents/skills/swbt-hardware-harness/SKILL.md`

Phase 2:

- `spec/initial/README.md`
- `spec/initial/SWBT_DAEMON_INITIAL_DIRECTION.md`
- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DEVELOPMENT_PLAN.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`
- `spec/wip/README.md`
- `spec/complete/README.md`
- `spec/dev-journal.md`
- `.agents/skills/swbt-spec-format/SKILL.md`
- `.agents/skills/swbt-tdd-workflow/SKILL.md`
- `.agents/skills/swbt-dev-journal/SKILL.md`
- `.agents/skills/swbt-agentic-self-review/SKILL.md`

Phase 3:

- `.github/PULL_REQUEST_TEMPLATE.md`
- `.agents/skills/pr-merge-cleanup/SKILL.md`

検証:

- `skill-creator` の `quick_validate.py` で全7 skill が valid。
- `rg` で skill template の TODO と Python / PyPI / React / Vercel 固有 command の残存なしを確認。

## 結論

最も有用なのは `ponkan-python` の運用である。特に source audit、hardware harness、spec / TDD / gate reporting は、swbt の BTstack / Switch HID / 実機 bring-up と構造が近い。

ただし、`ponkan-python` の skill をそのままコピーしてはいけない。swbt は Python package ではなく C/CMake daemon であり、実機対象も N3DSXL capture board ではなく Nintendo Switch と Bluetooth adapter である。

推奨は次の順序である。

1. root `AGENTS.md` を追加し、swbt のスコープ・非対象・BTstack・hardware safety を固定する。
2. `swbt-source-audit` と `swbt-hardware-harness` だけを先に導入する。
3. 実装 Work Unit が増えた段階で `spec-format`, `dev-journal`, TDD, self-review を swbt 用に追加する。
4. PR / release 系 skill は、初期 skeleton と CI / PR template が安定してから検討する。
