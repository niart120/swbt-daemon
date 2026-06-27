# Daemon CLI Management Surface

## 1. 概要

daemon の production run loop に入る前に、実機操作者や CI が現在の起動条件、設定、adapter selector 候補を確認できる CLI management surface を整理する work unit。

現行 `swbt-daemon` は subcommand を持たない。受け付けるのは `--backend`、`--config`、`--link-key-db`、診断 path、`--adapter-location` などの起動オプションだけである。

この work unit では、追加する subcommand を `help`、`adapters`、`config` の 3 種に絞る。`help` は command 一覧と現行起動オプションを表示する。`adapters` は前情報なしでは指定しにくい `--adapter-location` の候補を表示する。`config` は effective config と起動時だけ効く状態を表示し、invalid config では nonzero で終了する。独立した `validate-config` はこの段階では採用しない。

## 2. 起点 / ユースケース

source:

- user discussion, 2026-06-27: 「何の subcommand があると嬉しそうか」から考える。`--adapter-location` は前情報なしに指定できないため何とかしたい。現在の config を見せる意味では `show-config` は有用そうだが、`validate-config` が必要かは疑問。subcommand 管理の責務も決める必要がある。
- user discussion, 2026-06-27: subcommand は `help` / `adapters` / `config` の 3 種に絞る。
- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`: `show-config` / `validate-config` / reconnect state cleanup は有用だが、daemon 起動引数の work unit から分離した先送り事項。
- `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md`: production backend は `--adapter-location` 未指定では adapter open 前に失敗する。selector は安全境界として重要だが、操作者が候補を知る手段が不足している。
- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`: daemon config は `default -> TOML config file -> environment override` の順で合成される。backend 起動 mode、実機承認、診断 path は永続設定に入れない。
- `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`: `--config` で learned Switch address target を指定し、`--link-key-db` で TLV-backed Classic link key DB を接続する。pairing-free active reconnect は実機で観測済み。
- `docs/status.md`: current implementation の production 起動条件、config path、link key DB、adapter selector、active reconnect の状態表。

現行 CLI surface:

- subcommand はない。
- `swbt-daemon` は引数なしで production backend を選ぶ。
- `--backend production|noop` / `--backend=production|noop` を受け付ける。
- `--config <path>` / `--config=<path>` を受け付ける。
- `--link-key-db <path>` / `--link-key-db=<path>` を受け付ける。
- `--trace-path <path>`、`--hci-dump-path <path>`、`--crash-dump-path <path>` と `--key=value` 形式を受け付ける。
- `--adapter-location <selector>` / `--adapter-location=<selector>` を受け付ける。
- unknown option は拒否される。

use case:

- maintainer は、`help` で command 一覧、現行起動オプション、command 別の使い方を確認したい。
- 実機操作者は、実機起動前にどの adapter selector を指定すべきかを、Switch pairing、HID advertising、report loop へ進まずに確認したい。
- 実機操作者は、起動前に effective config、backend、adapter selector、diagnostic path、active reconnect address source、link key DB configured state を確認したい。
- maintainer / CI は、config file と environment override の合成が壊れていないことを確認したい。独立した `validate-config` ではなく、`config` command の nonzero exit と出力で確認する。
- maintainer は、daemon startup の parser と management command の parser / dispatcher が混ざりすぎない責務境界を確認したい。

期待する観測結果:

- command surface は `help` / `adapters` / `config` の 3 種に絞られている。
- `help` は command 一覧、現行起動オプション、command 別の使い方を表示する。
- `adapters` は `--adapter-location` に渡せる selector 候補を出す目的に閉じている。
- `config` は現在の effective config を表示し、invalid config では nonzero で終了する。
- `validate-config` は独立 command として採用しない。
- subcommand 管理の責務が、起動オプション parser、main、production backend、adapter enumeration port の間で分けられている。

source から use case への変換:

`local_074` の先送り事項は「read-only 管理コマンドを増やす」ではなく、「実機起動前に判断材料を得る CLI surface が不足している」という source として扱う。`help` で入口を明示し、`adapters` で selector 候補を得て、`config` で起動前の設定状態を確認する 3 command に分ける。

## 3. 対象範囲

- 現行 `swbt-daemon` が持つ起動オプションを棚卸しし、subcommand がない事実を record に残す。
- 採用する subcommand を `help` / `adapters` / `config` の 3 種に固定する。
  - `help`: command 一覧、現行起動オプション、command 別の使い方を表示する。
  - `adapters`: `--adapter-location` に渡せる selector 候補を表示する。
  - `config`: effective config と起動時にだけ効く path / backend / selector 状態を表示する。
- 独立した `validate-config` を採用しない理由を残す。`config` が invalid config で nonzero を返すなら、まずはそれで足りる。
- reconnect state cleanup は破壊的操作なので、読み取り専用 command の後続候補に残す。
- subcommand 管理の責務境界を決める。
  - `launch_options` が command 名まで parse するのか。
  - 新しい daemon CLI / management module を作るのか。
  - `main.c` は dispatch だけにするのか。
  - adapter enumeration を production backend から分離し、BTstack / WinUSB / libusb port 境界へ置くのか。
- 実装順序は `help` で surface を固定し、次に `adapters`、最後に `config` を基本候補にする。`adapters` を先に進める場合は、WinUSB / libusb enumeration の根拠確認を先に行う。

## 4. 対象外

- 破壊的な state cleanup command の実装。
- daemon IPC protocol への macro executor 追加。
- `tap`、`duration_ms`、`sequence`、`at_ms` の daemon protocol 化。
- GUI client、Python client、C# client。
- 複数 controller 同時接続。
- binary release、installer、service manager。
- `--config`、`--link-key-db`、`--backend`、diagnostic path flag の再設計。
- learned Switch address や link key DB の破壊的削除実装。必要性が固まった後、別 work unit で扱う。
- 独立した `validate-config` command。必要性が出た場合は `config --check` または後続 command として再検討する。

## 5. 関連 spec / docs

- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`
- `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md`
- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`
- `docs/status.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/work-unit-spec-tdd-flow.md`

## 6. 根拠監査

`help` と `config` は Switch HID report bytes、BTstack source selection、report timing、subcommand、SPI、rumble、descriptor data を追加または変更しない。

`adapters` は WinUSB / libusb enumeration の source fact を使うため、既存 local_077 の根拠監査と current pinned BTstack / generated WinUSB patch を確認して実装した。

| 項目 | 値 | 根拠 | source | status |
|---|---|---|---|---|
| libusb selector 表示 | `adapters` は Bluetooth USB class `0xE0/0x01/0x01` または BTstack の既知 VID/PID whitelist に該当する device から `libusb:<bus>:<port-path>` を表示する | source fact / implementation fact | `vendor/btstack/platform/libusb/hci_transport_h2_libusb.c`, `swbt/btstack_bridge/production_btstack.c`, `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md` | implemented; software verified |
| libusb port path | selector の port path は `libusb_get_bus_number()` と `libusb_get_port_numbers()` から作る。BTstack の `hci_transport_usb_set_bus_and_path(...)` が同じ bus / port numbers を照合する | source fact | `vendor/btstack/platform/libusb/hci_transport_h2_libusb.c`, `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md` | current pinned BTstack |
| WinUSB selector 表示 | `adapters` は SetupAPI で USB device interface を列挙し、Service が `WinUSB` の device から `SPDRP_LOCATION_PATHS` を読んで `winusb:<location-path>` を表示する | source fact / implementation fact | `swbt/btstack_bridge/production_btstack.c`, `cmake/btstack_winusb_location_patch.cmake`, `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md` | implemented; cross build verified |
| WinUSB selector 適用境界 | local_077 の generated WinUSB transport は同じ `SPDRP_LOCATION_PATHS` を `hci_transport_usb_set_location_path(...)` の selector と完全一致させ、一致した interface だけを `usb_try_open_device(...)` の対象にする | source fact | `cmake/btstack_winusb_location_patch.cmake`, `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md` | current implementation |
| adapter inventory safety boundary | `adapters` は `libusb_get_device_list()` / SetupAPI property read までで、`hci_init()`、`hci_power_control(HCI_POWER_ON)`、`usb_try_open_device(...)`、HID advertising、Switch pairing、report loop を呼ばない | implementation fact | `apps/swbt-daemon/main.c`, `swbt/daemon/cli.c`, `swbt/btstack_bridge/production_btstack.c` | software verified by build/test review; hardware not opened |

未検証:

- Windows WinUSB の `adapters` 出力を実機 host 上で実行して CSR8510 A10 の selector が表示されることは未確認。local_077 では同じ location path selector による adapter open は確認済みだが、この work unit では実機列挙を実行していない。
- Linux libusb の `adapters` 出力を実機 host 上で実行して専用 USB Bluetooth dongle の selector が表示されることは未確認。

## 7. 設計メモ

- 現行 `launch_options` は起動オプション parser であり、subcommand dispatcher ではない。ここに management command の実行責務を混ぜると、起動設定の合成と管理操作が密結合になる。
- `main.c` は薄い dispatch に留めるのがよい。production run と management command の分岐は `main.c` に残せるが、各 command の詳細は別 module に逃がす。
- 採用: `swbt/daemon/cli.*` が command 名、`help`、`adapters` / `config` の dispatch、表示契約を持つ。`launch_options.*` は startup 用 option parser として維持する。
- `main.c` は `swbt_daemon_cli_dispatch_with_ports(...)` を production / noop startup parser より前に呼ぶ。command が処理済みなら backend 起動へ進まない。
- CLI module は production adapter を直接 include しない。`adapters` は `swbt_daemon_cli_ports_t` の `list_adapters` callback 経由で呼ぶ。
- `config` は `swbt_daemon_launch_options_parse()` と `swbt_daemon_launch_config_prepare()` を再利用する。`swbt-daemon config [options]` の `[options]` は通常起動の option と同じ意味だが、backend 起動は行わない。
- `adapters` は production backend ではなく adapter inventory / BTstack bridge の port 境界で扱う。目的は selector 候補の表示であり、HCI power on、HID advertising、Switch pairing、report loop を始めない。
- `config` は config validation と output を兼ねる。invalid config で nonzero を返し、valid config では effective config を表示するため、独立した `validate-config` は採用しない。
- reconnect cleanup は `forget-switch` や `clear-reconnect-state` のような名前が考えられるが、TOML update と link key DB deletion の失敗 policy が固まるまで実装しない。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `swbt/daemon/cli.*`
- `swbt/daemon/launch_options.*`
- `swbt/daemon/config.*`
- `swbt/daemon/config_file.*`
- `swbt/btstack_bridge/usb_adapter_location.*`
- `swbt/btstack_bridge/production_btstack.*`
- `tests/daemon_launch_options_test.c`
- `tests/btstack_usb_adapter_location_test.c`
- `docs/status.md`
- `work-units/complete/local_078/DAEMON_CLI_MANAGEMENT_SURFACE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| done | current CLI characterization records that `swbt-daemon` has no subcommands and rejects unknown positional command names | characterization | unit/docs | no |
| done | `help` prints command list and current startup options without opening production backend | new | unit/integration | no |
| done | `adapters` lists selector strings without HCI power on, HID advertising, Switch pairing, or report loop | new | unit/integration | no |
| done | `config` prints effective config and startup-only option state without opening production backend | new | unit/integration | no |
| deferred | standalone validation is added only if `config` nonzero exit is not enough for CI or operator workflow | behavior | unit/integration | no |
| deferred | reconnect state cleanup requires dry-run output, explicit confirmation, and separate failure policy for TOML update vs link key DB deletion | edge | unit/docs | no |

TDD status:
- source: user discussion, 2026-06-27。subcommand は `help` / `adapters` / `config` の 3 種に絞る。
- use case: maintainer は `help` で CLI surface を確認し、実機操作者は `adapters` と `config` で実機起動前の判断材料を得る。
- item: `help` / `adapters` / `config` CLI management surface。
- state: green
- commands:
  - red: `just build-debug`。`swbt/daemon/cli.c` と `swbt_daemon_cli_ports_t` が未実装のため compile error。
  - green: `just build-debug` pass。
  - targeted: `$env:CTEST_ARGS='-R "swbt_daemon_(help|config)_.*|daemon_cli_test" --output-on-failure'; just test-debug` pass。3/3 tests passed。
  - cross build: `just windows-cross` pass。
- notes: `adapters` の real hardware inventory 実行は未実行。software test は fake inventory port と Windows cross build で、dispatch / selector output / invalid argument / config output を固定した。

## 10. 検証

- 要件整理:
  - `rg -n -- "--config|--link-key-db|--trace-path|--hci-dump-path|--crash-dump-path|--backend|--adapter-location|unknown_option_is_rejected|no_options_defaults_to_production_backend" swbt\daemon\launch_options.c tests\daemon_launch_options_test.c docs\status.md` pass。現行 `swbt-daemon` は起動オプションだけを持ち、subcommand は持たないことを確認した。
  - user discussion, 2026-06-27: subcommand は `help` / `adapters` / `config` の 3 種に絞る。
- TDD red:
  - `just build-debug` expected fail。`swbt/daemon/cli.c` 未追加の時点で CMake configure が source missing で失敗。
  - `just build-debug` expected fail。`swbt_daemon_cli_ports_t` / `swbt_daemon_cli_dispatch_with_ports()` 未実装の時点で `daemon_cli_test` compile error。
- TDD green / targeted verification:
  - `just build-debug` pass。
  - `$env:CTEST_ARGS='-R daemon_cli_test --output-on-failure'; just test-debug` pass。1/1 tests passed。
  - `$env:CTEST_ARGS='-R "swbt_daemon_(help|config)_.*|daemon_cli_test" --output-on-failure'; just test-debug` pass。3/3 tests passed。
  - `just windows-cross` pass。Windows WinUSB backend の SetupAPI inventory path と `swbt-daemon.exe` link を確認。
  - `scripts/format.sh` pass。
  - `just tidy` pass。
  - `just verify` pass。format-check、clang-tidy、fresh debug build/test、ASan、Windows cross build を確認。
- 未実行:
  - `swbt-daemon adapters` の実機 host 出力確認は未実行。理由: USB inventory の real output は host / driver state に依存し、HCI power on 以降を含む確認ではないが、専用 dongle の識別結果は実機環境で別途記録する必要がある。

## 11. 実機実行条件

この work unit の software verification では実機を使っていない。`help` / `config` は実機不要である。

`adapters` は USB inventory を読むだけで、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、report loop に触れない設計にした。ただし実機 host 上の出力を証跡として採る場合は、専用 USB Bluetooth dongle、driver assignment、location path、VID/PID または hardware ID を記録する。

HCI power on、pairing、advertising、report loop を含む確認は、この work unit の範囲外であり、`hardware-harness` の承認境界に従う。

## 12. 先送り事項

- 観測: standalone validation は、`config` が config validation と nonzero exit を持つなら重複する。
  先送り理由: CI が出力不要の validation command を必要とするか未確認であり、現時点の採用 command は `help` / `adapters` / `config` の 3 種で足りる。
  次の置き場: `config` の output contract を決めるときに、`config --check` が必要か再判断する。
- 観測: reconnect state cleanup は learned Switch address と link key DB の破壊的削除を含み得る。
  先送り理由: 読み取り専用の管理コマンドと同じ TDD cycle に混ぜると、TOML key removal、ファイル削除、部分失敗、操作者の確認の設計が膨らむ。
  次の置き場: local_078 の読み取り専用 command 実装後、必要なら reconnect state cleanup 用の後続 work unit record を起票する。

## 13. チェックリスト

- [x] 現行 `swbt-daemon` が subcommand を持たないことを確認した。
- [x] 現行起動オプションを棚卸しした。
- [x] user discussion から adapter discovery、`show-config`、`validate-config`、責務境界の論点を source に追加した。
- [x] 採用 subcommand を `help` / `adapters` / `config` の 3 種に絞った。
- [x] `validate-config` を独立 command としては採用しない判断を記録した。
- [x] TDD Test List を 3 command surface から始める形へ更新した。
- [x] `help` command を実装した。
- [x] `adapters` command を実装した。
- [x] `config` command を実装した。
- [x] `launch_options` と management command の責務境界を記録した。
- [x] `adapters` の WinUSB / libusb enumeration 境界を根拠監査へ記録した。
- [x] targeted software verification と未実行の実機 output 確認理由を記録した。
- [x] 最終 `just verify` を実行し、結果を記録した。
- [x] 完了後に work unit record を `work-units/complete/local_078/` へ移した。
