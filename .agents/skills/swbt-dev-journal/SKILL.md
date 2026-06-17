---
name: swbt-dev-journal
description: "Record swbt-daemon design observations, unresolved questions, deferred tasks, source-audit notes, report-rate findings, BTstack integration observations, and non-hardware bring-up decisions in spec/dev-journal.md. Use when Codex is asked to memo, journal, capture a follow-up, record a deferred decision, or preserve a small observation that is not yet a full work-unit spec."
---

# swbt Dev Journal

Use this skill to append concise design observations to `spec/dev-journal.md`.

## File

Write to `spec/dev-journal.md`. If the file does not exist, create:

```markdown
# Dev Journal

swbt-daemon の設計観測、未解決事項、先送り判断の記録。
```

## Entry Format

Append entries at the end:

```markdown
## YYYY-MM-DD: <title>

### Current State

<what is true now>

### Observation

<what was learned or what is unresolved>

### Decision

<decision, workaround, or condition for revisiting>
```

Keep entries factual. Prefer under 12 lines unless evidence requires more detail.

## What Belongs Here

- report period questions.
- BTstack source selection observations.
- WinUSB/libusb backend differences.
- daemon IPC boundary decisions.
- source audit questions too small for a work unit.
- deferred cleanup that should become a later spec.

## What Does Not Belong Here

- hardware run results; use `docs/hardware-test-log.md`.
- implementation task checklists; use work-unit specs.
- PR status; use the PR body or self-review report.
- speculation without an explicit uncertainty label.

## Promotion

If an entry becomes implementation work, create a `spec/wip/local_{nnn}/FEATURE_NAME.md` with `swbt-spec-format` and reference the journal entry.
