# Vendor Dependencies

This directory is reserved for third-party dependencies.

`vendor/btstack` is expected to be a Git submodule pointing at BTstack:

```bash
git submodule add https://github.com/bluekitchen/btstack.git vendor/btstack
git submodule update --init --recursive
```

`vendor/toml11` is expected to be a Git submodule pointing at toml11:

```bash
git submodule add https://github.com/ToruNiina/toml11.git vendor/toml11
git -C vendor/toml11 checkout v4.4.0
git submodule update --init --recursive
```

Do not place `swbt-daemon` project code under `vendor/`.
