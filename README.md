# swbt-daemon

`swbt-daemon` は、Nintendo Switch に Bluetooth Classic HID Device として接続し、Pro Controller 相当の入力を送るデーモンです。外部クライアントから受け取ったコントローラー状態を Switch へ送信します。

## ダウンロード

リリース版は [GitHub Releases](https://github.com/niart120/swbt-daemon/releases) から入手します。

Windows 版には、デーモン本体の `swbt-daemon.exe`、動作確認用の `swbt-debug-client.exe`、README、ライセンス文書、第三者ライセンス表記が入っています。

ソースからビルドする場合は [Development](docs/development.md) を参照してください。

## 対応状況

確認済み:

- Windows + WinUSB
- CSR8510 A10 USB Bluetooth ドングル
- Switch 2 ファームウェア `22.1.0`
- Switch とのペアリング
- Switch への入力反映
- 切断時や終了時のニュートラル入力復帰

未確認または未実装:

- 初代 Switch、Switch Lite、Switch OLED
- CSR8510 A10 以外の USB Bluetooth ドングル
- Linux + libusb 実機経路
- 複数コントローラー同時接続
- NFC / IR MCU / amiibo の意味処理

詳しい状態表は [Current State And Support Matrix](docs/status.md) を参照してください。

## 利用時の注意事項

実機に接続する場合は、専用 USB Bluetooth ドングルを使ってください。内蔵 Bluetooth や普段使いのドングルを対象にしないでください。

Windows では、専用 USB Bluetooth ドングルのドライバーを WinUSB に切り替える必要があります。切り替えには [Zadig](https://zadig.akeo.ie/) を利用できます。Zadig では必ず専用ドングルだけを選び、内蔵 Bluetooth や普段使いのドングルを選ばないでください。

対象ドングルを指定しない起動では、`swbt-daemon` は Bluetooth アダプターを開く前に終了します。Windows では `swbt-daemon adapters` で候補を確認し、`winusb:<location-path>` を指定します。

実機へ接続する前に、次の範囲を確認してください。

- 使用する Bluetooth ドングル
- Switch とのペアリング
- 入力送信の開始
- 終了後の接続状態の片付け

## 起動と確認

Windows 実機起動の例:

```console
swbt-daemon --adapter-location winusb:<location-path> --config swbt-daemon.toml --link-key-db swbt-link-key.tlv --trace-path trace.txt --hci-dump-path hci-dump.txt
```

`--trace-path` と `--hci-dump-path` は、問題調査用のログを残すために使います。

Bluetooth アダプターを開かない確認用コマンド:

```console
swbt-daemon help
swbt-daemon adapters
swbt-daemon config --backend noop
swbt-daemon --backend noop
```

## 入力の送信

`swbt-daemon` はローカル IPC で現在のコントローラー状態を受け取り、Switch へ送信します。

動作確認用クライアントの例:

```console
swbt-debug-client --port 37637 --button a --hold-ms 1000
```

IPC の形式は [Daemon IPC v1](spec/protocols/daemon-ipc-v1.md) を参照してください。

## ライセンス

`swbt-daemon` のコードと文書は MIT License です。詳しくは [LICENSE](LICENSE) を参照してください。

Windows 配布物は BTstack を含むため、BTstack のライセンス条件も適用されます。BTstack を含む配布物は、BlueKitchen から別の商用ライセンスを得ていない限り、個人・非商用利用を前提にしてください。

第三者ライセンス表記は [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) を参照してください。

## 開発者向け情報

- [Development](docs/development.md)
- [Current State And Support Matrix](docs/status.md)
- [Daemon IPC v1](spec/protocols/daemon-ipc-v1.md)
