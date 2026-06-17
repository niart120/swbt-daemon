# swbt-daemon 初期方針

## 概要

`swbt-daemon` は C11 / CMake / Ninja で実装するデーモンであり、Nintendo Switch からは Pro Controller 互換の Bluetooth Classic HID Device として見えることを目指す。

BTstack は `vendor/btstack` に固定した submodule として利用する。
デーモンは BTstack run loop、Bluetooth アダプターへのアクセス、Switch protocol state、report scheduling、local IPC ownership を所有する。

## アーキテクチャ判断

- 通常のクライアントインターフェースにはデーモン IPC を使う。
- C ABI はデーモン内部、テスト、将来の組み込み経路のために残す。
- fork または upstream patch の判断を文書化しない限り、`vendor/btstack` 配下の BTstack source は read-only として扱う。
- 日常の Linux ビルド、sanitizer ビルド、Windows MinGW cross build、静的解析には WSL2 + Dev Containers を使う。
- Switch pairing と report timing verification には、専用 WinUSB Bluetooth ドングルを使った Windows native 環境を使う。
- `tap`、`duration_ms`、`sequence`、`at_ms` のようなデーモン側の時間指定コマンドは実装しない。

## 現在の初期仕様

元の初期メモは `spec/initial/` に昇格済みである。

- `spec/initial/REPOSITORY_INITIALIZATION_TODO.md`
- `spec/initial/BTSTACK_SWITCH_DEVELOPMENT_PLAN.md`
- `spec/initial/BTSTACK_SWITCH_DAEMON_IPC_DESIGN.md`

導入実装記録は、この phase migration の元資料であるため、`tmp/swbt_agent_skill_adoption_policy.md` に残す。

メモが実装作業になった場合は、`spec/wip/local_{nnn}/` に範囲を絞った作業単位を作る。

## 初期検証コマンド

```console
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug

cmake --preset linux-asan
cmake --build --preset linux-asan
ctest --preset linux-asan

cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
```
