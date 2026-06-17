# Dev Journal

swbt-daemon の設計観測、未解決事項、先送り判断の記録。

## 2026-06-18: Agent and Skill Adoption Completed Through Phase 3

### Current State

`tmp/swbt_agent_skill_adoption_policy.md` defined Phase 1 through Phase 3 adoption for project-local Codex guidance.

### Observation

The repository now has root `AGENTS.md`, swbt-specific project skills, spec scaffolding, and a PR template. Dev Containers are explicitly treated as the standard reproducible development environment.

### Decision

Future implementation work should move stable requirements from `tmp/` into focused work units under `spec/wip/local_{nnn}/` and use the project skills for source audit, hardware gates, TDD, self-review, and PR cleanup.
