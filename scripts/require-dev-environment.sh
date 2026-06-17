#!/bin/sh
set -eu

case "${CI:-}" in
    "" | "0" | "false" | "FALSE")
        ;;
    *)
        exit 0
        ;;
esac

case "${SWBT_DEVCONTAINER:-}" in
    "1" | "ON" | "on" | "true" | "TRUE")
        exit 0
        ;;
esac

case "${SWBT_ALLOW_HOST_BUILD:-}" in
    "1" | "ON" | "on" | "true" | "TRUE")
        exit 0
        ;;
esac

cat >&2 <<'EOF'
Local builds are supported only inside the Dev Container.
Open this repository in the Dev Container, or use Makefile targets so host commands are delegated through the Dev Container CLI.
Set SWBT_ALLOW_HOST_BUILD=1 only when explicitly opting into an unsupported host build.
EOF
exit 1
