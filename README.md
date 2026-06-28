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
- Linux + libusb での Switch 接続
- 複数コントローラー同時接続
- NFC / IR MCU / amiibo の意味処理

詳しい状態表は [Current State And Support Matrix](docs/status.md) を参照してください。

## 利用時の注意事項

Windows では、Switch 接続に使う専用の USB Bluetooth ドングルを用意し、ドライバーを WinUSB に切り替える必要があります。内蔵 Bluetooth や普段使いのドングルはこの用途に使わないでください。

ドライバーの切り替えには [Zadig](https://zadig.akeo.ie/) を利用できます。Zadig は外部ツールです。利用によって生じた問題について、本プロジェクトは責任を負いません。Zadig で WinUSB に切り替えるのは、上記の専用 USB Bluetooth ドングルのドライバーだけにしてください。

ドングルは、確認済みの CSR8510 A10 チップ搭載品を推奨します。CSR8510 A10 以外の USB Bluetooth ドングルは未確認です。

ドングルを接続して WinUSB に切り替えた後、`swbt-daemon adapters` で `winusb:<location-path>` を確認します。Switch に接続する起動では、この値を `--adapter-location` に指定します。指定しない場合、`swbt-daemon` は Bluetooth アダプターを開く前に終了します。

Switch に接続する前に、次の項目を確認してください。

- 使用する専用 USB Bluetooth ドングル
- ドライバーが WinUSB に切り替わっていること
- `winusb:<location-path>` の値
- Switch とペアリングを開始してよいこと
- 入力送信を開始してよいこと
- 終了時に `swbt-daemon` を停止し、Switch 側に不要な接続が残っていないこと

## 起動と確認

確認用コマンド:

```console
# ヘルプを表示する
swbt-daemon help

# Windows で指定できる winusb:<location-path> の候補を表示する
swbt-daemon adapters

# Bluetooth アダプターを開かずに設定を確認する
swbt-daemon config --backend noop

# Bluetooth アダプターを開かずに起動できることを確認する
swbt-daemon --backend noop
```

ここで確認した `winusb:<location-path>` は、Switch に接続する起動で `--adapter-location` に指定します。

Windows で Switch に接続する起動例:

```console
swbt-daemon --adapter-location winusb:<location-path> --config swbt-daemon.toml --link-key-db swbt-link-key.tlv --trace-path trace.txt --hci-dump-path hci-dump.txt
```

引数:

- `--adapter-location winusb:<location-path>`: 必須。`swbt-daemon adapters` で確認した専用ドングルを指定します。
- `--config swbt-daemon.toml`: 任意。設定ファイルを使う場合に指定します。
- `--link-key-db swbt-link-key.tlv`: 任意。Switch との接続情報を保存し、再接続に使う場合に指定します。
- `--trace-path trace.txt`: 任意。`swbt-daemon` の診断ログを保存します。
- `--hci-dump-path hci-dump.txt`: 任意。Bluetooth 通信の調査用ログを保存します。

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
