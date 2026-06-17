---
name: swbt-hardware-harness
description: "Safety and evidence workflow for swbt-daemon hardware tests with Nintendo Switch, Bluetooth Classic HID, dedicated USB Bluetooth dongles, WinUSB, libusb, report loops, pairing, and disconnect behavior. Use before Codex runs, designs, or reports any hardware-gated command, CTest label, manual pairing session, or Switch-facing Bluetooth action."
---

# swbt Hardware Harness

Use this skill before any command or procedure that can interact with a real Nintendo Switch or Bluetooth adapter.

## Approval Boundary

Do not run hardware-facing commands unless the user explicitly approves the exact scope.

Hardware-facing work includes:

- starting a daemon build that opens a Bluetooth adapter.
- Switch pairing.
- HID Device advertising.
- periodic input report loops.
- output report / subcommand handling against a real console.
- tests labeled `hardware`.

Use explicit environment gates in commands or tests:

```console
SWBT_RUN_HARDWARE=1
SWBT_HARDWARE_APPROVED=1
```

## Required Setup

Before a run, confirm and record:

- dedicated USB Bluetooth dongle is used.
- built-in or daily-use Bluetooth adapter is not used.
- OS and host environment.
- Windows driver state, especially WinUSB assignment when using Windows native.
- BTstack commit / tag.
- swbt commit / branch.
- daemon backend: `windows-winusb` or `libusb`.
- configured report period.
- Nintendo Switch firmware version when known.

## Stop Conditions

Stop before hardware access if:

- the target adapter is ambiguous.
- the adapter is the user's regular Bluetooth device.
- approval does not name pairing, advertising, report loop, or test scope.
- cleanup behavior is unknown for the code path being exercised.
- the test could keep buttons pressed without a neutral fail-safe.

## Execution Rules

- Prefer manual bring-up steps until automated hardware tests exist.
- Keep daemon logs and hardware notes separate from unit test output.
- Record successful cleanup and failure cleanup.
- On owner disconnect, timeout, or process exit, verify neutral state behavior where possible.
- Do not treat a hardware observation as general truth without recording OS, driver, dongle, Switch firmware, BTstack commit, and swbt commit.

## Recording Target

Write hardware observations to `docs/hardware-test-log.md`.

Use this shape:

```markdown
## YYYY-MM-DD: <short title>

- OS:
- environment:
- dongle:
- driver:
- backend:
- BTstack:
- swbt:
- Switch firmware:
- report period:
- command / test:
- result:
- cleanup:
- notes:
```

## Reporting

If hardware was not run, explicitly say why.

If hardware was run, report:

- approval scope.
- command or manual procedure.
- adapter identity.
- daemon/backend configuration.
- result.
- artifact or log path.
- cleanup result.
