# Specs

このディレクトリには安定した spec、参照資料、開発ジャーナルを置く。

spec は設計、protocol、挙動、方針を書く文書である。
1 つの work unit だけに閉じるとは限らず、複数の work unit record から参照されてもよい。
通常は work unit を先に立て、必要になったときだけ spec を作成または更新する。

## 配置

| path | 役割 |
|---|---|
| `architecture/` | daemon 構成、責務境界、module boundary、BTstack bridge などの構造。 |
| `protocols/` | Switch HID、daemon IPC、report format、subcommand、SPI、rumble などの protocol。 |
| `operations/` | 実機検証、Windows/WinUSB、開発運用、PR 前後の安定手順。 |
| `references/` | upstream 調査、外部資料、根拠の要約。規範ではない。 |
| `archive/` | 置き換え済みの spec。現在有効な判断として扱わない。 |
| `initial/` | 初期方針、初期計画、導入時の設計背景。現在有効とは限らない。 |
| `dev-journal.md` | まだ spec や work unit record にするほど固まっていない観測や先送り判断。 |

## ルール

- 実装が従う文書は `architecture/`、`protocols/`、`operations/` に置く。
- 根拠や外部資料の要約は `references/` に置き、規範として扱わない。
- 置き換えた spec は `archive/` に移し、可能なら移動先または後継 spec を記録する。
- work unit の完了判定、検証結果、実機状態は `work-units/` に記録する。
- work unit、spec、TDD Test List の作成順序は `operations/work-unit-spec-tdd-flow.md` に従う。

`spec/dev-journal.md` は例外的に、まだ spec や work unit record にするほど固まっていない観測や先送り判断を記録する。

## 現在の主要 spec

- `architecture/daemon-runtime-boundaries.md`: daemon runtime と BTstack bridge の責務境界。
- `protocols/daemon-ipc-v1.md`: daemon IPC v1 の通信仕様。
- `protocols/switch-hid-core.md`: Switch HID protocol core の current contract。
- `operations/development-tooling.md`: `just` を入口にする開発運用。
- `operations/windows-native-preflight.md`: Windows native 実機検証前の gate。
- `operations/work-unit-spec-tdd-flow.md`: source、use case、work unit、spec、TDD Test List の作成順序。
