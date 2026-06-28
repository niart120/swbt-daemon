# swbt-daemon

`swbt-daemon` は、Nintendo Switch に Bluetooth Classic HID Device として接続し、Pro Controller 相当の入力を送る daemon です。

daemon は local IPC でコントローラー状態の snapshot を受け取り、Switch Pro Controller protocol と BTstack HID Device backend へ渡します。daemon protocol は現在状態を受け取る設計であり、`tap`、`duration_ms`、`sequence`、`at_ms` のような時間指定 macro は扱いません。

## 現在の状態

現時点では GitHub Release の binary artifact はまだ提供していません。初回 release の準備は [Release Build And Publish Plan](spec/operations/release-build-and-publish.md) に沿って進めます。

確認済みの範囲:

- Windows native
- CSR8510 A10 USB Bluetooth ドングル
- WinUSB driver
- Switch 2 firmware `22.1.0`
- `windows-winusb` backend
- pairing、HID L2CAP open、subcommand reply、Switch UI への入力反映
- owner disconnect、heartbeat timeout、shutdown 時の neutral fail-safe
- pairing-free active reconnect の一部条件

未確認または未実装の範囲:

- 初代 Switch、Switch Lite、Switch OLED
- CSR8510 A10 以外の USB Bluetooth ドングル
- Linux + libusb 実機経路
- 厳密な latency、jitter、取りこぼし率
- 複数 controller 同時接続
- NFC / IR MCU / amiibo の意味処理
- rumble 周波数 / 振幅の意味変換

詳しい状態表は [docs/status.md](docs/status.md) を参照してください。実機ログは [docs/hardware-test-log.md](docs/hardware-test-log.md) に記録しています。

## 実機安全境界

実機へ触れる起動では、専用 USB Bluetooth ドングルを使ってください。内蔵 Bluetooth や普段使いのドングルを対象にしないでください。

production backend は `--adapter-location` がない場合、Bluetooth アダプターを開く前に失敗します。Windows では `swbt-daemon adapters` で候補を確認し、`winusb:<location-path>` selector を指定します。

実機実行では、次の範囲を人間が明示してから実行します。

- Bluetooth adapter open
- Switch pairing
- HID advertising
- report loop
- IPC input
- cleanup confirmation

## 入手と準備

binary artifact は初回 release 整備後に GitHub Release へ置く予定です。今は source から build してください。

```console
git submodule update --init --recursive
```

開発環境、build、test、Git hooks は [docs/development.md](docs/development.md) に分けています。

## 管理コマンド

Bluetooth アダプターを開かずに確認できるコマンド:

```console
swbt-daemon help
swbt-daemon adapters
swbt-daemon config --backend noop
swbt-daemon --backend noop
```

Windows 実機起動の形:

```console
swbt-daemon --adapter-location winusb:<location-path> --config swbt-daemon.toml --link-key-db swbt-link-key.tlv --trace-path trace.txt --hci-dump-path hci-dump.txt
```

`--trace-path` と `--hci-dump-path` は、startup、pairing、HID、cleanup の根拠を残すために使います。

## IPC と診断用 client

daemon は JSON Lines over local IPC を使います。クライアントは現在の controller state snapshot を送り、daemon は最後に受け取った状態を report loop へ反映します。

診断用 client:

```console
swbt-debug-client --port 37637 --button a --hold-ms 1000
```

`swbt-debug-client` は診断用です。安定した automation API としては daemon IPC v1 を参照してください。

- [Daemon IPC v1](spec/protocols/daemon-ipc-v1.md)

## ライセンス

Original `swbt-daemon` project files are licensed under the MIT License. See [LICENSE](LICENSE).

BTstack is a third-party dependency with its own license terms. Builds and source distributions that include or link BTstack are also subject to the BTstack license. Such builds are intended for personal, non-commercial use unless a separate commercial BTstack license is obtained from BlueKitchen.

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [Release License Boundary](spec/operations/release-license-boundary.md).

## 開発者向け情報

- [Development](docs/development.md)
- [Operations specs](spec/operations/README.md)
- [Architecture spec](spec/architecture/daemon-architecture-cutover.md)
