# Upstream BTstack

`swbt-daemon` uses BTstack as a third-party dependency.

The intended location is:

```text
vendor/btstack
```

## Current Base

The project is currently pinned to BTstack `v1.8.2` as a Git submodule.

```text
upstream repository: https://github.com/bluekitchen/btstack
submodule path: vendor/btstack
base tag: v1.8.2
base commit: 075a0780f0fad7ff67d58ac19f46e8953656a752
```

When the submodule is updated, record the exact submodule commit here.

## Update Policy

- Keep Switch-specific behavior outside BTstack whenever practical.
- Prefer `swbt/btstack_bridge/` for integration code.
- Do not disable BTstack HID validation globally.
- Record any BTstack patch or fork requirement in this file.
