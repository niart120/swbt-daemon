# 開発ジャーナル

swbt-daemon の設計観測、未解決事項、先送り判断の記録。

## 2026-06-18: エージェントと skill の Phase 3 導入

### 現状

`tmp/swbt_agent_skill_adoption_policy.md` は、project-local な Codex guidance として Phase 1 から Phase 3 までの導入を定義した。

### 観測

repository には root `AGENTS.md`、swbt-specific な project skills、spec scaffolding、PR template がある。
Dev Containers は再現可能な標準開発環境として明示されている。

### 判断

今後の implementation work では、安定した requirements を `tmp/` から `spec/wip/local_{nnn}/` の focused work unit へ移す。
source audit、hardware gate、TDD、self-review、PR cleanup には project skills を使う。
