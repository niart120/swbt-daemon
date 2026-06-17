---
name: swbt-agentic-self-review
description: "Gate-oriented self-review for swbt-daemon changes before PR, merge, or handoff. Use when Codex needs to summarize requirements coverage, source-audit status, CMake/CTest results, sanitizer/cross-build results, hardware run or not-run reasons, BTstack/license impact, non-goals, remaining risks, and whether evidence proves a work unit is complete."
---

# swbt Agentic Self Review

Use this skill before declaring a swbt work unit done, opening a PR, or handing off substantial changes.

## Review Process

1. Identify the selected work unit, user intent, and non-goals.
2. Compare changed files with the spec and `AGENTS.md`.
3. Record source audit status for protocol, BTstack, backend, and hardware facts.
4. Record verification commands and exact results.
5. Record hardware status separately from automated tests.
6. Record BTstack license / notice impact.
7. Treat weak, indirect, or missing evidence as not proven.

## Gate Table

Use these gates:

| gate | result | evidence |
|---|---|---|
| Requirements | pass/fail/not proven | spec and user objective coverage |
| Non-goals | pass/fail | scope did not expand |
| Source Audit | pass/not applicable/not run | evidence for protocol/BTstack facts |
| Tests | pass/fail/not run | CTest or targeted command |
| Static / Build | pass/fail/not run | CMake configure/build, sanitizer, cross build |
| Hardware | pass/fail/not run/not applicable | approval, adapter, result, or reason |
| BTstack / License | pass/not applicable/not proven | submodule untouched, notices checked |
| Integration Review | pass/fail/not proven | diff reviewed against boundaries |

## Findings First

If there are issues, list findings before summaries:

```markdown
### Findings

| severity | finding | evidence | recommendation |
|---|---|---|---|
```

If no issues were found, still list residual risk and unrun gates.

## Report Template

```markdown
## swbt Self Review

### Work Unit
- selected:
- intent:
- non-goals:

### Findings
| severity | finding | evidence | recommendation |
|---|---|---|---|

### Gates
| gate | result | evidence |
|---|---|---|

### Verification
- commands:
- not run:

### Source / Hardware
- source audit:
- hardware:
- BTstack/license:

### Next
- follow-up:
- open risk:
```

## Rules

- Do not mark a work unit complete because tests merely pass; match evidence to requirements.
- Do not hide hardware not-run status behind unit test success.
- Do not treat `vendor/btstack` changes as safe without explicit source audit and license review.
