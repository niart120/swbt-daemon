# Vendor Dependencies

This directory is reserved for third-party dependencies.

`vendor/btstack` is expected to be a Git submodule pointing at BTstack:

```bash
git submodule add https://github.com/bluekitchen/btstack.git vendor/btstack
git submodule update --init --recursive
```

Do not place `swbt-daemon` project code under `vendor/`.
