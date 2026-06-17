---
name: hardware-harness
description: "Nintendo Switch、Bluetooth Classic HID、専用 USB Bluetooth ドングル、WinUSB、libusb、report loop、pairing、disconnect behavior を扱う swbt-daemon 実機検証の安全境界と根拠記録のワークフロー。Codex が実機承認を必要とするコマンド、CTest label、manual pairing session、Switch-facing Bluetooth action を実行、設計、報告する前に使う。"
---

# 実機検証

実際の Nintendo Switch または Bluetooth アダプターと通信し得るコマンドや手順の前に、この skill を使う。

## 承認境界

ユーザが正確な範囲を明示承認していない限り、実機向けコマンドを実行しない。

実機向け作業には次を含める。

- Bluetooth アダプターを開く daemon build の起動。
- Switch pairing。
- HID Device advertising。
- periodic input report loop。
- 実機 console に対する output report / subcommand handling。
- `hardware` label のテスト。

コマンドやテストには、実機実行を明示する環境変数を使う。

```console
SWBT_RUN_HARDWARE=1
SWBT_HARDWARE_APPROVED=1
```

## 実行前の確認

実行前に次を確認して記録する。

- 専用 USB Bluetooth ドングルを使っていること。
- 内蔵または普段使いの Bluetooth アダプターを使っていないこと。
- OS と host environment。
- Windows driver state。Windows native の場合は WinUSB assignment を特に記録する。
- BTstack commit / tag。
- swbt commit / branch。
- daemon backend。`windows-winusb` または `libusb`。
- 設定した report period。
- Nintendo Switch firmware version。判明している場合だけでよい。

## 停止条件

次の場合は実機アクセスの前に停止する。

- 対象アダプターが曖昧である。
- アダプターがユーザの常用 Bluetooth device である。
- 承認が pairing、advertising、report loop、test scope のどれを対象にするか示していない。
- 実行する code path の cleanup behavior が不明である。
- neutral fail-safe なしで button pressed が残り得る。

## 実行ルール

- 自動実機テストができるまでは manual bring-up step を優先する。
- daemon log と実機メモは unit test output と分ける。
- cleanup の成功と失敗時 cleanup を記録する。
- owner disconnect、timeout、process exit では、可能な範囲で neutral state behavior を確認する。
- OS、ドライバー、ドングル、Switch firmware、BTstack commit、swbt commit を記録せずに、実機観測を一般的な真実として扱わない。

## 記録先

実機観測は `docs/hardware-test-log.md` に書く。

次の形式を使う。

```markdown
## YYYY-MM-DD: <短い題名>

- OS:
- environment:
- dongle:
- driver:
- backend:
- BTstack:
- swbt:
- Switch firmware:
- report period:
- command / test:
- result:
- cleanup:
- notes:
```

## 報告

実機検証を実行していない場合は、その理由を明示する。

実機検証を実行した場合は、次を報告する。

- 承認範囲。
- コマンドまたは manual procedure。
- adapter identity。
- daemon とバックエンドの設定。
- 結果。
- artifact または log path。
- cleanup result。
