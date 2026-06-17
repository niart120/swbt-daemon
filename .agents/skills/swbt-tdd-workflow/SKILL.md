---
name: swbt-tdd-workflow
description: "Canon TDD workflow for swbt-daemon C11 code using CMake, Ninja, CTest, sanitizer presets, and focused work-unit test lists. Use when Codex is asked to do TDD, create a TDD test list, run red/green/refactor, add C unit tests, characterize Switch packet behavior, or implement protocol/IPC behavior one observable item at a time."
---

# swbt TDD Workflow

Use this skill to drive one small swbt behavior change at a time.

## Preconditions

- Work on a non-default branch unless the user explicitly requests otherwise.
- Inspect `git status --short` and preserve user changes.
- Use `swbt-spec-format` when the behavior belongs to a work-unit spec.
- Use `swbt-source-audit` before hard-coding protocol or BTstack facts.
- Use `swbt-hardware-harness` before any hardware-gated test.

## Test List

Before coding, choose one observable item from the spec's TDD Test List. If no list exists, create one with:

- input or state.
- expected observable result.
- test layer.
- whether hardware is required.

Do not put implementation details in the test item.

## Red

Add or update the smallest relevant C test. Run the narrowest command that demonstrates the expected failure.

Typical commands:

```console
cmake --build --preset linux-debug
ctest --preset linux-debug -R <test-name> --output-on-failure
```

If the failure is a build, collection, or environment problem unrelated to the expected behavior, do not count it as red.

## Green

Implement the minimum behavior needed for the selected item and related existing tests.

Run:

```console
cmake --build --preset linux-debug
ctest --preset linux-debug -R <test-name> --output-on-failure
```

When the change touches shared protocol code, also run the full debug preset:

```console
ctest --preset linux-debug --output-on-failure
```

## Refactor

Refactor only after green. Separate observable behavior changes from structure changes.

Run the same tests after refactor. Add sanitizer or cross build when the touched code justifies it:

```console
cmake --preset linux-asan
cmake --build --preset linux-asan
ctest --preset linux-asan --output-on-failure
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
```

## Good TDD Targets

- controller state validation.
- button and stick report packing.
- subcommand parser and response builder.
- SPI flash read response.
- rumble packet parser.
- JSON Lines IPC parser.
- owner acquire / release / disconnect neutral.

## Status Update

Update the spec or handoff with:

```text
TDD status:
- item:
- state: red | green | refactor-done | deferred
- commands:
- notes:
```
