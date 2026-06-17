#!/bin/sh
set -eu

repo_root=$(git rev-parse --show-toplevel)
cd "$repo_root"

git config core.hooksPath .githooks

hooks_path=$(git config --get core.hooksPath)
printf '%s\n' "Configured core.hooksPath=$hooks_path"
