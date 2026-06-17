# swbt-daemon Initial Direction

## Summary

`swbt-daemon` is a C11 / CMake / Ninja daemon intended to appear to Nintendo Switch as a Bluetooth Classic HID Device compatible with a Pro Controller.

BTstack is consumed as a pinned submodule at `vendor/btstack`. The daemon owns BTstack run loop, Bluetooth adapter access, Switch protocol state, report scheduling, and local IPC ownership.

## Architecture Decisions

- Use daemon IPC as the normal client interface.
- Keep C ABI for daemon internals, tests, and future embedded alternatives.
- Keep BTstack source under `vendor/btstack` read-only unless a documented fork/upstream patch decision is made.
- Use WSL2 + Dev Containers for daily Linux builds, sanitizer builds, Windows MinGW cross builds, and static analysis.
- Use Windows native with a dedicated WinUSB Bluetooth dongle for Switch pairing and report timing verification.
- Do not implement daemon-side timed commands such as `tap`, `duration_ms`, `sequence`, or `at_ms`.

## Current Initial Specs

The original bootstrap notes have been promoted into `spec/initial/`:

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DEVELOPMENT_PLAN.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`

The adoption implementation record remains in `tmp/swbt_agent_skill_adoption_policy.md` because it is the source artifact for this phase migration.

When a note becomes implementation work, create a focused work unit under `spec/wip/local_{nnn}/`.

## Initial Verification Commands

```console
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug

cmake --preset linux-asan
cmake --build --preset linux-asan
ctest --preset linux-asan

cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
```
