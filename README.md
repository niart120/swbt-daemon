# swbt-daemon

`swbt-daemon` is an early-stage Nintendo Switch Bluetooth controller daemon.
The goal is to appear to the Switch as a Bluetooth Classic HID device compatible
with a Pro Controller, while exposing a local IPC interface for debug tools,
test runners, and future language bindings.

## Status

This repository is in initial development. The daemon does not yet pair with a
Switch and hardware behavior is not verified.

The current tree contains only the project skeleton, build entry points, license
policy, and design notes.

## Architecture

The intended runtime model is:

```text
client application
  CLI / tests / future language bindings
  |
  | JSON Lines over local IPC
  v
swbt-daemon
  - IPC server
  - single active owner
  - latest controller state
  - Switch Pro Controller protocol
  - BTstack HID Device bridge
  - WinUSB / libusb adapter backend
  |
  v
Nintendo Switch
```

The daemon owns the Bluetooth adapter, BTstack run loop, Switch protocol state,
and HID report scheduler. Clients send full controller-state snapshots. The
daemon protocol does not include timed commands such as `tap`, `duration_ms`,
`sequence`, or `at_ms`.

## Development Environment

The primary development environment is WSL2 with Dev Containers. The container
installs CMake, Ninja, compilers, MinGW, libusb headers, and analysis tools; the
host does not need those tools for the supported local path.

Hardware verification is expected to run on Windows native builds with a
dedicated USB Bluetooth dongle assigned to WinUSB with Zadig.

Local builds are supported inside the Dev Container.
Plain host builds are blocked by default so that local results do not depend on
untracked host toolchains.
CI is allowed, and an explicit unsupported host build can opt in with
`SWBT_ALLOW_HOST_BUILD=1` or `-DSWBT_ALLOW_HOST_BUILD=ON`.

Initial build commands:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

Sanitizer build:

```bash
cmake --preset linux-asan
cmake --build --preset linux-asan
ctest --preset linux-asan
```

Windows cross build:

```bash
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
```

Git hooks are tracked under `.githooks/`.
Enable them once per clone:

```bash
sh scripts/install-git-hooks.sh
```

`pre-commit` checks staged whitespace and CMake presets.
When staged C sources are present, it also runs `clang-format`.
`pre-push` runs the `linux-debug` fresh configure, build, and test path.
Set `SWBT_FULL_PRE_PUSH=1` to also run the formatter, clang-tidy, sanitizer, and
Windows cross-build paths.

Format and lint commands:

```bash
scripts/format.sh
scripts/check-format.sh
cmake --fresh --preset linux-clang-tidy
cmake --build --preset linux-clang-tidy
```

## BTstack Dependency

BTstack is intended to be consumed as a Git submodule at `vendor/btstack`.

```bash
git submodule update --init --recursive
```

The current CMake skeleton can configure without the submodule so the project
can bootstrap cleanly. Real Bluetooth functionality will require BTstack source
files from `vendor/btstack`.

## Repository Layout

```text
api/                    public C ABI surface
apps/swbt-daemon/       daemon executable
cmake/                  build helpers and toolchains
docs/                   project notes and hardware logs
swbt/core/              project core utilities
swbt/switch/            Switch controller protocol code
swbt/btstack_bridge/    BTstack integration boundary
tests/                  C unit tests
vendor/btstack/         BTstack submodule
```

## License

Original swbt-daemon project files are licensed under the MIT License. See
`LICENSE`.

BTstack is a third-party dependency with its own license terms. Builds and
source distributions that include or link BTstack are also subject to the
BTstack license. Such builds are intended for personal, non-commercial use
unless a separate commercial BTstack license is obtained from BlueKitchen.

See `THIRD_PARTY_NOTICES.md`.
