set shell := ["sh", "-eu", "-c"]
set windows-shell := ["powershell.exe", "-NoLogo", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command"]

# Show available recipes.
default:
    @just --list

# Show available recipes.
help:
    @just --list

# Ensure the Dev Container exists.
devcontainer-up:
    @just --justfile "{{justfile()}}" _devcontainer-up-{{os_family()}}

_devcontainer-up-unix:
    @cli="${DEVCONTAINER_CLI:-devcontainer}"; \
    workspace="${DEVCONTAINER_WORKSPACE:-{{justfile_directory()}}}"; \
    if ! command -v "$cli" >/dev/null 2>&1; then \
        printf '%s\n' 'devcontainer CLI was not found. Install the Dev Containers CLI or open this repository in the Dev Container.' >&2; \
        exit 127; \
    fi; \
    "$cli" up --workspace-folder "$workspace" ${DEVCONTAINER_UP_FLAGS:-}

_devcontainer-up-windows:
    @$cli = if ($env:DEVCONTAINER_CLI) { $env:DEVCONTAINER_CLI } else { 'devcontainer' }; $workspace = if ($env:DEVCONTAINER_WORKSPACE) { $env:DEVCONTAINER_WORKSPACE } else { '{{justfile_directory()}}' }; if (-not (Get-Command $cli -ErrorAction SilentlyContinue)) { [Console]::Error.WriteLine('devcontainer CLI was not found. Install the Dev Containers CLI or open this repository in the Dev Container.'); exit 127 }; $upFlags = @(); if ($env:DEVCONTAINER_UP_FLAGS) { $upFlags = $env:DEVCONTAINER_UP_FLAGS -split ' ' }; & $cli up --workspace-folder $workspace @upFlags; exit $LASTEXITCODE

# Recreate the Dev Container after environment changes.
devcontainer-rebuild:
    @just --justfile "{{justfile()}}" _devcontainer-rebuild-{{os_family()}}

_devcontainer-rebuild-unix:
    @DEVCONTAINER_UP_FLAGS="--remove-existing-container ${DEVCONTAINER_UP_FLAGS:-}" just --justfile "{{justfile()}}" devcontainer-up

_devcontainer-rebuild-windows:
    @$env:DEVCONTAINER_UP_FLAGS = "--remove-existing-container $env:DEVCONTAINER_UP_FLAGS"; & just --justfile "{{justfile()}}" _devcontainer-up-windows; exit $LASTEXITCODE

# List configured CMake presets.
list-presets:
    @just --justfile "{{justfile()}}" _run-or-delegate list-presets

# Configure linux-debug.
configure-debug:
    @just --justfile "{{justfile()}}" _run-or-delegate configure-debug

# Build linux-debug.
build-debug:
    @just --justfile "{{justfile()}}" _run-or-delegate build-debug

# Test linux-debug. Pass CTEST_ARGS through the environment when needed.
test-debug:
    @just --justfile "{{justfile()}}" _run-or-delegate test-debug

# Configure, build, and test linux-debug. Does not run format-check or clang-tidy.
debug:
    @just --justfile "{{justfile()}}" _run-or-delegate debug

# Format C sources.
format:
    @just --justfile "{{justfile()}}" _run-or-delegate format

# Check C source formatting.
format-check:
    @just --justfile "{{justfile()}}" _run-or-delegate format-check

# Configure and build linux-clang-tidy with clang-tidy enabled.
tidy:
    @just --justfile "{{justfile()}}" _run-or-delegate tidy

# Configure, build, and test linux-asan.
asan:
    @just --justfile "{{justfile()}}" _run-or-delegate asan

# Configure and build windows-mingw-debug.
windows-cross:
    @just --justfile "{{justfile()}}" _run-or-delegate windows-cross

# Run format-check, clang-tidy, debug, ASan, and Windows cross build.
verify:
    @just --justfile "{{justfile()}}" _run-or-delegate verify

# Run the CI non-hardware verification set.
verify-ci:
    @just --justfile "{{justfile()}}" _run-or-delegate verify-ci

_run-or-delegate recipe:
    @just --justfile "{{justfile()}}" _run-or-delegate-{{os_family()}} "{{recipe}}"

_run-or-delegate-unix recipe:
    @case "${SWBT_DEVCONTAINER:-}" in \
        1|ON|on|true|TRUE) \
            just --justfile "{{justfile()}}" "_{{recipe}}-in-container"; \
            ;; \
        *) \
            just --justfile "{{justfile()}}" _devcontainer-run "_{{recipe}}-in-container"; \
            ;; \
    esac

_run-or-delegate-windows recipe:
    @$swbtDevcontainer = $env:SWBT_DEVCONTAINER; if ($swbtDevcontainer -in @('1', 'ON', 'on', 'true', 'TRUE')) { & just --justfile "{{justfile()}}" "_{{recipe}}-in-container"; exit $LASTEXITCODE } else { & just --justfile "{{justfile()}}" _devcontainer-run-windows "_{{recipe}}-in-container"; exit $LASTEXITCODE }

_devcontainer-run target:
    @just --justfile "{{justfile()}}" _devcontainer-run-{{os_family()}} "{{target}}"

_devcontainer-run-unix target:
    @cli="${DEVCONTAINER_CLI:-devcontainer}"; \
    workspace="${DEVCONTAINER_WORKSPACE:-{{justfile_directory()}}}"; \
    if ! command -v "$cli" >/dev/null 2>&1; then \
        printf '%s\n' 'devcontainer CLI was not found. Install the Dev Containers CLI or open this repository in the Dev Container.' >&2; \
        exit 127; \
    fi; \
    "$cli" up --workspace-folder "$workspace" ${DEVCONTAINER_UP_FLAGS:-}; \
    "$cli" exec --workspace-folder "$workspace" env SWBT_DEVCONTAINER=1 CTEST_ARGS="${CTEST_ARGS:-}" just --justfile justfile "{{target}}"

_devcontainer-run-windows target:
    @$cli = if ($env:DEVCONTAINER_CLI) { $env:DEVCONTAINER_CLI } else { 'devcontainer' }; $workspace = if ($env:DEVCONTAINER_WORKSPACE) { $env:DEVCONTAINER_WORKSPACE } else { '{{justfile_directory()}}' }; if (-not (Get-Command $cli -ErrorAction SilentlyContinue)) { [Console]::Error.WriteLine('devcontainer CLI was not found. Install the Dev Containers CLI or open this repository in the Dev Container.'); exit 127 }; $upFlags = @(); if ($env:DEVCONTAINER_UP_FLAGS) { $upFlags = $env:DEVCONTAINER_UP_FLAGS -split ' ' }; & $cli up --workspace-folder $workspace @upFlags; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; & $cli exec --workspace-folder $workspace env SWBT_DEVCONTAINER=1 "CTEST_ARGS=$($env:CTEST_ARGS)" just --justfile justfile "{{target}}"; exit $LASTEXITCODE

_require-devcontainer:
    @case "${SWBT_DEVCONTAINER:-}" in \
        1|ON|on|true|TRUE) \
            ;; \
        *) \
            printf '%s\n' 'This recipe must run inside the Dev Container.' >&2; \
            exit 1; \
            ;; \
    esac

_prepare-workspace-in-container: _require-devcontainer
    @workspace="$(pwd)"; if ! git config --global --get-all safe.directory | grep -Fx -- "$workspace" >/dev/null 2>&1; then git config --global --add safe.directory "$workspace"; fi

_list-presets-in-container: _prepare-workspace-in-container
    cmake --list-presets >/dev/null

_configure-debug-in-container: _prepare-workspace-in-container
    cmake --fresh --preset linux-debug

_build-debug-in-container: _prepare-workspace-in-container
    cmake --build --preset linux-debug

_test-debug-in-container: _prepare-workspace-in-container
    ctest --preset linux-debug --output-on-failure ${CTEST_ARGS:-}

_debug-in-container: _prepare-workspace-in-container
    just --justfile "{{justfile()}}" _configure-debug-in-container
    just --justfile "{{justfile()}}" _build-debug-in-container
    just --justfile "{{justfile()}}" _test-debug-in-container

_format-in-container: _prepare-workspace-in-container
    scripts/format.sh

_format-check-in-container: _prepare-workspace-in-container
    scripts/check-format.sh

_tidy-in-container: _prepare-workspace-in-container
    cmake --fresh --preset linux-clang-tidy
    cmake --build --preset linux-clang-tidy

_asan-in-container: _prepare-workspace-in-container
    cmake --fresh --preset linux-asan -DCMAKE_C_COMPILER=clang
    cmake --build --preset linux-asan
    ctest --preset linux-asan --output-on-failure ${CTEST_ARGS:-}

_windows-cross-in-container: _prepare-workspace-in-container
    cmake --fresh --preset windows-mingw-debug
    cmake --build --preset windows-mingw-debug

_verify-in-container: _prepare-workspace-in-container
    just --justfile "{{justfile()}}" _format-check-in-container
    just --justfile "{{justfile()}}" _tidy-in-container
    just --justfile "{{justfile()}}" _debug-in-container
    just --justfile "{{justfile()}}" _asan-in-container
    just --justfile "{{justfile()}}" _windows-cross-in-container

_verify-ci-in-container: _verify-in-container
