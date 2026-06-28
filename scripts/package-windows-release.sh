#!/usr/bin/env sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/.." && pwd)
build_dir=${SWBT_RELEASE_BUILD_DIR:-"$repo_root/build/windows-mingw-release"}
dist_dir=${SWBT_RELEASE_DIST_DIR:-"$repo_root/dist"}
cache_file="$build_dir/CMakeCache.txt"

if [ ! -f "$cache_file" ]; then
    printf '%s\n' "CMake cache not found: $cache_file" >&2
    printf '%s\n' "Run: just build-windows-release" >&2
    exit 1
fi

version=${SWBT_RELEASE_VERSION:-}
if [ -z "$version" ]; then
    version=$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "$cache_file" | head -n 1)
fi
if [ -z "$version" ]; then
    printf '%s\n' "Could not determine release version from $cache_file" >&2
    exit 1
fi

case "$version" in
    v*) version_without_v=${version#v} ;;
    *) version_without_v=$version ;;
esac

artifact_name="swbt-daemon-v${version_without_v}-windows-x86_64"
stage_dir="$dist_dir/$artifact_name"
zip_path="$dist_dir/$artifact_name.zip"
sha_path="$dist_dir/$artifact_name.zip.sha256"

case "$dist_dir" in
    "$repo_root"/*) ;;
    *)
        printf '%s\n' "Refusing to package outside repository: $dist_dir" >&2
        exit 1
        ;;
esac

for path in \
    "$build_dir/swbt-daemon.exe" \
    "$build_dir/swbt-debug-client.exe" \
    "$repo_root/LICENSE" \
    "$repo_root/THIRD_PARTY_NOTICES.md" \
    "$repo_root/vendor/btstack/LICENSE" \
    "$repo_root/vendor/btstack/3rd-party/README.md" \
    "$repo_root/vendor/toml11/LICENSE"
do
    if [ ! -f "$path" ]; then
        printf '%s\n' "Required release input is missing: $path" >&2
        exit 1
    fi
done

cmake -E remove_directory "$stage_dir"
cmake -E rm -f "$zip_path" "$sha_path"
cmake -E make_directory \
    "$stage_dir/bin" \
    "$stage_dir/licenses/btstack/3rd-party" \
    "$stage_dir/licenses/toml11"

cmake -E copy_if_different "$repo_root/README.md" "$stage_dir/README.md"
cmake -E copy_if_different "$repo_root/LICENSE" "$stage_dir/LICENSE"
cmake -E copy_if_different "$repo_root/THIRD_PARTY_NOTICES.md" "$stage_dir/THIRD_PARTY_NOTICES.md"
cmake -E copy_if_different "$build_dir/swbt-daemon.exe" "$stage_dir/bin/swbt-daemon.exe"
cmake -E copy_if_different "$build_dir/swbt-debug-client.exe" "$stage_dir/bin/swbt-debug-client.exe"
cmake -E copy_if_different "$repo_root/vendor/btstack/LICENSE" "$stage_dir/licenses/btstack/LICENSE"
cmake -E copy_if_different "$repo_root/vendor/btstack/3rd-party/README.md" "$stage_dir/licenses/btstack/3rd-party/README.md"
cmake -E copy_if_different "$repo_root/vendor/toml11/LICENSE" "$stage_dir/licenses/toml11/LICENSE"

swbt_commit=$(git -C "$repo_root" rev-parse HEAD)
btstack_status=$(git -C "$repo_root" submodule status vendor/btstack | sed 's/^[[:space:]]*//')
toml11_status=$(git -C "$repo_root" submodule status vendor/toml11 | sed 's/^[[:space:]]*//')
if [ -n "$(git -C "$repo_root" status --short)" ]; then
    worktree_dirty=true
else
    worktree_dirty=false
fi

cat > "$stage_dir/manifest.json" <<EOF
{
  "artifact_name": "$artifact_name.zip",
  "version": "$version_without_v",
  "target": "windows-x86_64",
  "build_preset": "windows-mingw-release",
  "swbt_commit": "$swbt_commit",
  "btstack_submodule": "$btstack_status",
  "toml11_submodule": "$toml11_status",
  "worktree_dirty": $worktree_dirty,
  "includes_btstack": true,
  "license_notice_files": [
    "LICENSE",
    "THIRD_PARTY_NOTICES.md",
    "licenses/btstack/LICENSE",
    "licenses/btstack/3rd-party/README.md",
    "licenses/toml11/LICENSE"
  ]
}
EOF

(
    cd "$dist_dir"
    cmake -E tar cf "$artifact_name.zip" --format=zip "$artifact_name"
)

checksum=$(cmake -E sha256sum "$zip_path" | awk '{print $1}')
printf '%s  %s\n' "$checksum" "$artifact_name.zip" > "$sha_path"

printf '%s\n' "$zip_path"
printf '%s\n' "$sha_path"
