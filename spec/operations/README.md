# Operations Specs

実機検証、Windows/WinUSB、開発運用、PR 前後の安定手順など、繰り返し使う運用 spec を置く。

単発の作業記録はここに置かず、work unit record または `spec/dev-journal.md` に書く。

## 現在の spec

- [Development Tooling](development-tooling.md): `just`、formatter、linter、Git hooks、CI quality job の選定理由と運用方針。
- [Work Unit, Spec, And TDD Flow](work-unit-spec-tdd-flow.md): source、use case、work unit、spec、TDD Test List の関係と作成順序。
- [Windows Native Preflight](windows-native-preflight.md): Windows native 実機検証前の承認、専用ドングル、WinUSB、環境変数、hardware log 記録 gate。
