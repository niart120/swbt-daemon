# Operations Specs

実機検証、Windows/WinUSB、開発運用、PR 前後の安定手順など、繰り返し使う運用 spec を置く。

単発の作業記録はここに置かず、work unit record または `spec/dev-journal.md` に書く。

## 現在の spec

- [Development Tooling](development-tooling.md): `just`、formatter、linter、Git hooks、CI quality job の選定理由と運用方針。
- [Just Task Runner Migration](just-task-runner-migration.md): Makefile から `just` への移行判断、実装結果、マルチプラットフォーム host + Dev Container 方針。
