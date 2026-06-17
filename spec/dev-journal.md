# 開発ジャーナル

swbt-daemon の設計観測、未解決事項、先送り判断の記録。

## 2026-06-18: エージェントと skill の Phase 3 導入

### 現状

`tmp/swbt_agent_skill_adoption_policy.md` は、プロジェクト固有の Codex guidance として Phase 1 から Phase 3 までの導入を定義した。

### 観測

リポジトリには root `AGENTS.md`、swbt 固有のプロジェクトスキル、仕様のひな形、PR テンプレートがある。
Dev Containers は再現可能な標準開発環境として明示されている。

### 判断

今後の実装作業では、安定した要件を `tmp/` から `spec/wip/local_{nnn}/` の範囲を絞った作業単位へ移す。
根拠監査、実機ゲート、TDD、自己レビュー、PR の後片付けにはプロジェクトスキルを使う。
