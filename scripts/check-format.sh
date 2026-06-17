#!/bin/sh
set -eu

repo_root=$(git rev-parse --show-toplevel)
cd "$repo_root"

if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format was not found. Run this inside the Dev Container." >&2
    exit 127
fi

set --
for path in $(git ls-files api apps swbt tests); do
    case "$path" in
        *.c | *.h)
            set -- "$@" "$path"
            ;;
    esac
done

if [ "$#" -eq 0 ]; then
    exit 0
fi

clang-format --dry-run --Werror "$@"
