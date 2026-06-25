# Third-Party Notices

This document records third-party dependency license boundaries for
`swbt-daemon`.

## BTstack

BTstack is expected to be checked out as a Git submodule at:

```text
vendor/btstack
```

BTstack is developed by BlueKitchen GmbH and is governed by its own license.
The BTstack license is not the MIT License used for original `swbt-daemon`
project files.

The BTstack license in the upstream repository includes non-commercial /
personal-use restrictions and points commercial users to BlueKitchen for
commercial licensing options.

When BTstack is present, keep these files available with source and binary
distributions:

```text
vendor/btstack/LICENSE
vendor/btstack/3rd-party/README.md
```

Binary builds that include or link BTstack are subject to the BTstack license
in addition to the license for original `swbt-daemon` files. Do not describe
such binaries as MIT-only artifacts.

Upstream:

```text
https://github.com/bluekitchen/btstack
```

## toml11

toml11 is expected to be checked out as a Git submodule at:

```text
vendor/toml11
```

This project pins toml11 at tag `v4.4.0` for daemon configuration file
parsing and serialization work. toml11 is distributed under the MIT License.

Keep this file available with source and binary distributions:

```text
vendor/toml11/LICENSE
```

Upstream:

```text
https://github.com/ToruNiina/toml11
```
