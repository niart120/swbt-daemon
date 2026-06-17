---
name: swbt-spec-format
description: "Create, update, review, and complete swbt-daemon work-unit specifications under spec/initial, spec/wip, spec/complete, and spec/dev-journal. Use when Codex is asked to write a spec, design a feature, split work into local work units, move a spec to complete, record TDD lists, define hardware gates, or migrate stable tmp notes into permanent swbt documentation."
---

# swbt Spec Format

Use this skill when creating or updating swbt work-unit specifications.

## Locations

| purpose | path |
|---|---|
| initial project direction | `spec/initial/` |
| active work unit | `spec/wip/local_{nnn}/FEATURE_NAME.md` |
| completed work unit | `spec/complete/local_{nnn}/FEATURE_NAME.md` |
| small observations and deferred decisions | `spec/dev-journal.md` |
| hardware observations | `docs/hardware-test-log.md` |

Use uppercase snake case for `FEATURE_NAME.md`.

When creating a new work unit, inspect existing `spec/wip/local_*` and `spec/complete/local_*` directories and choose the next number.

## Required Sections

Work-unit specs should include:

```markdown
# <Feature Name>

## 1. Overview
## 2. Scope
## 3. Non-goals
## 4. Source Audit
## 5. Design
## 6. Target Files
## 7. TDD Test List
## 8. Verification
## 9. Hardware Gate
## 10. Checklist
```

Keep sections concise. Split large reference material into a separate spec or docs file.

## Source Audit

Use `swbt-source-audit` whenever the spec includes:

- Switch HID report bytes.
- BTstack source selection.
- report period.
- subcommand, SPI, rumble, or descriptor data.
- WinUSB/libusb behavior.

Mark source audit as `not applicable` only when the work does not touch protocol, hardware, BTstack, or backend facts.

## TDD Test List

Use behavior-focused test items. Include:

| status | item | type | layer | hardware |
|---|---|---|---|---|
| todo | observable behavior | new/regression/edge/characterization | unit/integration/hardware | no/yes |

Status values: `todo`, `red`, `green`, `refactor-done`, `deferred`.

## Hardware Gate

For hardware-gated work, state:

- required approval.
- adapter assumptions.
- environment variables.
- expected log target.
- cleanup requirements.

For non-hardware work, state the reason hardware is not required.

## Completing a Work Unit

Move `spec/wip/local_{nnn}` to `spec/complete/local_{nnn}` only after:

- checklist is updated.
- verification commands and results are recorded.
- source audit status is clear.
- hardware status is clear.
- implementation and non-goals match the spec.

Do not mark uncertain evidence as pass.
