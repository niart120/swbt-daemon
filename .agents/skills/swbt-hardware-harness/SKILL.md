---
name: swbt-hardware-harness
description: "Nintendo Switch、Bluetooth Classic HID、専用 USB Bluetooth dongle、WinUSB、libusb、report loop、pairing、disconnect behavior を扱う swbt-daemon 実機検証の安全境界と根拠記録のワークフロー。Codex が hardware-gated command、CTest label、manual pairing session、Switch-facing Bluetooth action を実行、設計、報告する前に使う。"
---

# swbt hardware harness（実機安全境界）

実際の Nintendo Switch または Bluetooth adapter と通信し得る command や手順の前に、この skill を使う。

## 承認境界

ユーザが正確な範囲を明示承認していない限り、hardware-facing command を実行しない。

hardware-facing work には次を含める。

- Bluetooth adapter を開く daemon build の起動。
- Switch pairing。
- HID Device advertising。
- periodic input report loop。
- 実機 console に対する output report / subcommand handling。
- `hardware` label の test。

command や test には明示的な environment gate を使う。

```console
SWBT_RUN_HARDWARE=1
SWBT_HARDWARE_APPROVED=1
```

## 実行前の確認

実行前に次を確認して記録する。

- 専用 USB Bluetooth dongle を使っていること。
- 内蔵または普段使いの Bluetooth adapter を使っていないこと。
- OS と host environment。
- Windows driver state。Windows native の場合は WinUSB assignment を特に記録する。
- BTstack commit / tag。
- swbt commit / branch。
- daemon backend。`windows-winusb` または `libusb`。
- 設定した report period。
- Nintendo Switch firmware version。判明している場合だけでよい。

## 停止条件

次の場合は hardware access の前に停止する。

- 対象 adapter が曖昧である。
- adapter がユーザの常用 Bluetooth device である。
- 承認が pairing、advertising、report loop、test scope のどれを対象にするか示していない。
- 実行する code path の cleanup behavior が不明である。
- neutral fail-safe なしで button pressed が残り得る。

## 実行ルール

- 自動 hardware test ができるまでは manual bring-up step を優先する。
- daemon log と hardware note は unit test output と分ける。
- cleanup の成功と失敗時 cleanup を記録する。
- owner disconnect、timeout、process exit では、可能な範囲で neutral state behavior を確認する。
- OS、driver、dongle、Switch firmware、BTstack commit、swbt commit を記録せずに、実機観測を一般的な真実として扱わない。

## 記録先

hardware observation は `docs/hardware-test-log.md` に書く。

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

hardware を実行していない場合は、その理由を明示する。

hardware を実行した場合は、次を報告する。

- 承認範囲。
- command または manual procedure。
- adapter identity。
- daemon/backend configuration。
- 結果。
- artifact または log path。
- cleanup result。
