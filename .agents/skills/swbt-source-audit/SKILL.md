---
name: swbt-source-audit
description: "BTstack, Nintendo Switch HID, Linux hid-nintendo, joycontrol, and hardware logs source-audit workflow for swbt-daemon. Use when Codex needs to introduce or change Switch protocol constants, HID reports, subcommands, SPI addresses, rumble packets, report timing, BTstack source selection, WinUSB/libusb backend behavior, or any other value that must be backed by upstream code, documentation, or measured hardware evidence."
---

# swbt Source Audit

Use this skill before treating any Switch HID, BTstack, or Bluetooth adapter behavior as a fact in `swbt-daemon`.

## Evidence Classes

Record evidence as one of:

| class | meaning |
|---|---|
| `source fact` | Directly observed in upstream source, documentation, or a pinned commit. |
| `implementation fact` | Observed in existing swbt code or tests. |
| `hardware observation` | Measured against Nintendo Switch hardware or a Bluetooth dongle. |
| `inference` | Reasoned from facts but not directly verified. |
| `unverified hypothesis` | Plausible but not safe to encode as a stable contract. |

Do not collapse these categories. A value from a reverse-engineering note and a value measured on local hardware are different evidence.

## Primary Sources

Prefer these sources, recording path, URL, commit, and line where possible:

- `vendor/btstack` source and documentation pinned by the parent repository.
- dekuNukem Nintendo Switch reverse engineering notes.
- Linux `hid-nintendo.c`.
- `joycontrol` implementation notes and behavior.
- `docs/hardware-test-log.md` entries created by swbt hardware runs.
- Local swbt tests that characterize packet layout or daemon behavior.

## What Requires Audit

Audit before adding or changing:

- HID descriptor bytes.
- input report IDs, output report IDs, and report packing.
- subcommand IDs and response payloads.
- SPI flash addresses and returned data.
- rumble packet layout.
- report period defaults and fallback values.
- BTstack source file lists and port selections.
- WinUSB/libusb backend assumptions.
- any magic number that crosses the daemon, BTstack, or Switch protocol boundary.

## Recording Rules

For each audited value, record:

- value and meaning.
- evidence class.
- source path or URL.
- source commit, version, or tag.
- line number when available.
- whether the value is stable, configurable, or hardware-observed only.
- follow-up work if the evidence is incomplete.

Use a work-unit spec in `spec/wip/local_{nnn}/FEATURE_NAME.md` for substantial decisions. Use `spec/dev-journal.md` for small observations and deferred questions. Use `docs/hardware-test-log.md` for hardware measurements.

## Safety Rules

- Do not modify `vendor/btstack` directly unless the work unit explicitly decides a submodule fork or upstream patch is required.
- Do not encode undocumented Switch protocol constants without source or characterization tests.
- Do not treat report rate as fixed hardware truth unless it is backed by measured evidence and documented fallback behavior.
- Do not mix hardware observations from different OS, driver, dongle, Switch firmware, or BTstack commits without recording those differences.

## Output

End the audit with:

```markdown
### Source Audit

| item | value | evidence | source | status |
|---|---:|---|---|---|

### Open Questions

- ...
```
