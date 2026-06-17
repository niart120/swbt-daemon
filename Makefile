SHELL := /bin/sh

.DEFAULT_GOAL := help

DEVCONTAINER_CLI ?= devcontainer
DEVCONTAINER_WORKSPACE ?= $(CURDIR)
DEVCONTAINER_UP_FLAGS ?=
CTEST_ARGS ?=

PUBLIC_TARGETS := \
	list-presets \
	configure-debug \
	build-debug \
	test-debug \
	debug \
	format \
	format-check \
	tidy \
	asan \
	windows-cross \
	verify \
	verify-ci

.PHONY: help devcontainer-up devcontainer-rebuild devcontainer-run require-devcontainer
.PHONY: $(PUBLIC_TARGETS)
.PHONY: $(addsuffix -in-container,$(PUBLIC_TARGETS))

help:
	@printf '%s\n' \
		'Targets:' \
		'  make debug          Configure, build, and test linux-debug.' \
		'  make format-check   Run clang-format dry-run check.' \
		'  make tidy           Configure and build linux-clang-tidy.' \
		'  make asan           Configure, build, and test linux-asan.' \
		'  make windows-cross  Configure and build windows-mingw-debug.' \
		'  make verify         Run the full non-hardware verification set.' \
		'  make devcontainer-rebuild' \
		'                       Recreate the Dev Container after environment changes.' \
		'' \
		'Host execution delegates to the Dev Container CLI.' \
		'Inside the Dev Container, targets run directly.'

$(PUBLIC_TARGETS):
	@if [ "$${SWBT_DEVCONTAINER:-}" = "1" ] || \
	    [ "$${SWBT_DEVCONTAINER:-}" = "ON" ] || \
	    [ "$${SWBT_DEVCONTAINER:-}" = "on" ] || \
	    [ "$${SWBT_DEVCONTAINER:-}" = "true" ] || \
	    [ "$${SWBT_DEVCONTAINER:-}" = "TRUE" ]; then \
		$(MAKE) --no-print-directory $@-in-container; \
	else \
		$(MAKE) --no-print-directory devcontainer-run TARGET=$@-in-container; \
	fi

devcontainer-up:
	@if ! command -v "$(DEVCONTAINER_CLI)" >/dev/null 2>&1; then \
		printf '%s\n' 'devcontainer CLI was not found. Install the Dev Containers CLI or open this repository in the Dev Container.' >&2; \
		exit 127; \
	fi
	"$(DEVCONTAINER_CLI)" up --workspace-folder "$(DEVCONTAINER_WORKSPACE)" $(DEVCONTAINER_UP_FLAGS)

devcontainer-rebuild: DEVCONTAINER_UP_FLAGS := --remove-existing-container
devcontainer-rebuild: devcontainer-up

devcontainer-run:
	@if [ -z "$(TARGET)" ]; then \
		printf '%s\n' 'TARGET is required for devcontainer-run.' >&2; \
		exit 2; \
	fi
	@if ! command -v "$(DEVCONTAINER_CLI)" >/dev/null 2>&1; then \
		printf '%s\n' 'devcontainer CLI was not found. Install the Dev Containers CLI or open this repository in the Dev Container.' >&2; \
		exit 127; \
	fi
	"$(DEVCONTAINER_CLI)" up --workspace-folder "$(DEVCONTAINER_WORKSPACE)" $(DEVCONTAINER_UP_FLAGS)
	"$(DEVCONTAINER_CLI)" exec --workspace-folder "$(DEVCONTAINER_WORKSPACE)" env SWBT_DEVCONTAINER=1 make --no-print-directory "$(TARGET)" CTEST_ARGS="$(CTEST_ARGS)"

require-devcontainer:
	@if [ "$${SWBT_DEVCONTAINER:-}" != "1" ] && \
	    [ "$${SWBT_DEVCONTAINER:-}" != "ON" ] && \
	    [ "$${SWBT_DEVCONTAINER:-}" != "on" ] && \
	    [ "$${SWBT_DEVCONTAINER:-}" != "true" ] && \
	    [ "$${SWBT_DEVCONTAINER:-}" != "TRUE" ]; then \
		printf '%s\n' 'This target must run inside the Dev Container.' >&2; \
		exit 1; \
	fi

list-presets-in-container: require-devcontainer
	cmake --list-presets >/dev/null

configure-debug-in-container: require-devcontainer
	cmake --fresh --preset linux-debug

build-debug-in-container: require-devcontainer
	cmake --build --preset linux-debug

test-debug-in-container: require-devcontainer
	ctest --preset linux-debug --output-on-failure $(CTEST_ARGS)

debug-in-container: require-devcontainer
	$(MAKE) --no-print-directory configure-debug-in-container
	$(MAKE) --no-print-directory build-debug-in-container
	$(MAKE) --no-print-directory test-debug-in-container

format-in-container: require-devcontainer
	scripts/format.sh

format-check-in-container: require-devcontainer
	scripts/check-format.sh

tidy-in-container: require-devcontainer
	cmake --fresh --preset linux-clang-tidy
	cmake --build --preset linux-clang-tidy

asan-in-container: require-devcontainer
	cmake --fresh --preset linux-asan -DCMAKE_C_COMPILER=clang
	cmake --build --preset linux-asan
	ctest --preset linux-asan --output-on-failure $(CTEST_ARGS)

windows-cross-in-container: require-devcontainer
	cmake --fresh --preset windows-mingw-debug
	cmake --build --preset windows-mingw-debug

verify-in-container: require-devcontainer
	$(MAKE) --no-print-directory format-check-in-container
	$(MAKE) --no-print-directory tidy-in-container
	$(MAKE) --no-print-directory debug-in-container
	$(MAKE) --no-print-directory asan-in-container
	$(MAKE) --no-print-directory windows-cross-in-container

verify-ci-in-container: verify-in-container
