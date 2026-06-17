# BTstack Switch Controller Daemon 開発環境・開発方針

## 目的

Nintendo Switch に Bluetooth 経由で Pro Controller 相当の HID デバイスとして接続する daemon を開発する。開発環境は再現性を優先し、日常開発・単体テスト・クロスビルドを WSL2 + Dev Containers 上に集約する。実機確認は Windows ネイティブ環境で行う。

Bluetooth stack の低レイヤ処理は BTstack を利用する。BTstack は root repository に直接置かず、`vendor/btstack` の Git submodule として `v1.8.2` に pin する。Switch Pro Controller 固有のプロトコル、daemon IPC、公開 C ABI は自前実装として分離する。Python などの client library は将来タスクとし、初期 repository には置かない。

## 前提

対象は Windows を主実行環境とする。Windows 版では USB Bluetooth ドングルを WinUSB 経由で BTstack が直接制御する。対象ドングルには Zadig で WinUSB ドライバーを割り当てる。普段使いの内蔵 Bluetooth や常用ドングルを使わず、専用の USB Bluetooth ドングルを用意する。

Linux 版は libusb バックエンドとして扱う。Linux 版は単体テスト、protocol 実装検証、ASan / UBSan による検査に使う。Switch 実機との動作確認は Windows 版を優先する。

開発環境は Dev Containers 上の Ubuntu 24.04 系を基準にする。CMake + Ninja を主ビルド経路とし、`vendor/btstack` 内の upstream port Makefile は参照・比較・最小再現用として残す。

## 方針セルフレビュー

### 採用する方針

開発環境は WSL2 + Dev Containers を主経路にする。理由は、CMake、Ninja、clang、gcc、mingw-w64、libusb、sanitizer、静的解析を同一環境に固定できるためである。個別開発者の Windows 環境に MSYS2 や各種ツールを手作業で入れる方式より、再現性が高い。

Windows 実機確認は Windows ネイティブで行う。WinUSB ドライバー割り当て、Bluetooth ドングルの認識、Switch との pairing、再接続、report rate の実測は、WSL2 / container 内だけでは十分に検証できない。

CMake は自前追加部分を管理する。BTstack 全体を CMake 化するのではなく、daemon、Switch protocol core、C ABI、IPC、unit test を CMake の主対象にする。BTstack の必要ソースは明示的に取り込む。

C ABI は下層 API として残す。daemon は C ABI または内部 core を使って実装する。将来追加する Python / C# / CLI などのクライアントは daemon IPC を主経路にする。C ABI はテスト、組み込み、将来の直接利用向けの安定境界として扱う。

### 採用しない方針

WSL2 から USB passthrough で Bluetooth ドングルを直接扱う経路は主経路にしない。成立しても Windows ネイティブ WinUSB 経路とは別の検証になるため、開発変数が増える。

Python から BTstack や Bluetooth HCI を直接扱う実装は採用しない。Python はクライアント、マクロ、デバッグツール、テスト補助に限定する。

BTstack の共通処理を広範囲に改変しない。Switch 固有の HID report を通すために BTstack 本体の検証処理を無効化する設計は避ける。必要な例外処理は Switch 用 bridge に閉じ込める。

時間指定コマンド、シーケンス予約、daemon 側マクロ実行は初期範囲に入れない。daemon は最新 controller state を受け取り、固定周期で HID report を送信する責務に限定する。

### 主なリスク

CMake と BTstack upstream Makefile の二重管理が発生する。差分を抑えるため、CMake 側で取り込む BTstack source list は明示的に管理し、更新時に差分確認を行う。

クロスビルドに成功しても、Windows 実機で動作する保証はない。WinUSB、USB Bluetooth ドングル、Switch 側の pairing は Windows ネイティブでの手動検証が必要である。

Dev Containers は開発環境の再現性を上げるが、Windows 固有の DLL ロード、パス、実行権限、ドライバー割り当て問題を隠す可能性がある。Windows 実機確認手順を別に定義する。

report rate は実機・接続方式・Switch 側状態で揺れる可能性がある。daemon は 125Hz 相当を既定値にするが、60Hz、66.7Hz、120Hz、125Hz を設定で切り替えられるようにする。最終値は実測で決める。

## 開発環境の役割分担

| 環境 | 役割 |
|---|---|
| WSL2 + Dev Containers | 日常開発、CMake/Ninja ビルド、Linux/libusb 版ビルド、Windows/MinGW クロスビルド、単体テスト、静的解析、sanitizer |
| Windows ネイティブ | WinUSB ドライバー割り当て、Windows 版 daemon 実行、Switch との pairing、Bluetooth ドングル実機確認、latency / report rate 実測 |
| Linux ネイティブまたは Raspberry Pi | Linux/libusb 版の実機確認、udev ルール確認、Linux 固有の動作確認 |
| GitHub Actions | Linux build、Windows cross build、unit test、format check、静的解析の一部 |

## 現在のリポジトリ構成

```text
swbt-daemon/
  .devcontainer/
    devcontainer.json
    Dockerfile

  .github/
    workflows/
      ci.yml

  api/
    swbt.h
    swbt_c_api.c

  apps/
    swbt-daemon/
      main.c

  cmake/
    toolchains/
      mingw-w64-x86_64.cmake
    btstack_sources.cmake
    compiler_warnings.cmake
    sanitizers.cmake

  docs/
    hardware-test-log.md
    upstream-btstack.md

  swbt/
    core/
      swbt_version.c
      swbt_version.h

    switch/
      switch_controller_state.c
      switch_controller_state.h
      switch_report.c
      switch_report.h
      switch_subcommand.c
      switch_subcommand.h
      switch_spi.c
      switch_spi.h
      switch_rumble.c
      switch_rumble.h
      switch_constants.h

    btstack_bridge/
      README.md
      switch_btstack_bridge.c
      switch_btstack_bridge.h
      swbt_backend_libusb.c
      swbt_backend_winusb.c
      swbt_backend.h

  tests/
    swbt_smoke_test.c
    switch_report_test.c
    switch_subcommand_test.c
    switch_spi_test.c
    switch_rumble_test.c
    ipc_json_test.c
    daemon_owner_test.c

  tmp/
    btstack_switch_development_plan.md
    btstack_switch_daemon_ipc_design.md
    repository_initialization_todo.md

  vendor/
    README.md
    btstack/
      ...              # Git submodule, pinned to v1.8.2

  CMakeLists.txt
  CMakePresets.json
  LICENSE
  README.md
  THIRD_PARTY_NOTICES.md
```

`vendor/btstack` は Git submodule として扱う。現在の pin は `v1.8.2` / `075a0780f0fad7ff67d58ac19f46e8953656a752`。BTstack 本体へ加える変更は原則避ける。patch が必要な場合は、まず `swbt/btstack_bridge/` で吸収できないか確認し、submodule fork が必要になった時点で `docs/upstream-btstack.md` に理由と commit を記録する。

root `LICENSE` は swbt-daemon 自前コードだけに適用する MIT License とする。BTstack は別ライセンスであり、BTstack を含む binary / release は MIT-only artifact として扱わない。詳細は `THIRD_PARTY_NOTICES.md` に記録する。

## CMake 方針

### ビルド対象

| ターゲット | 種別 | 内容 |
|---|---|---|
| `swbt_core` | static library | Switch protocol、report builder、subcommand parser、SPI、rumble、controller state |
| `swbt_btstack_bridge` | static library | BTstack callback、HID device bridge、backend 抽象 |
| `swbt` | shared library | C ABI。必要に応じて `swbt.dll` / `libswbt.so` を生成 |
| `swbt-daemon` | executable | IPC server、owner 管理、BTstack run loop 起動 |
| `swbt_tests` | test executable | C unit tests |

### CMake options

```cmake
option(SWBT_BUILD_TESTS "Build unit tests" ON)
option(SWBT_BUILD_SHARED "Build shared C ABI library" ON)
option(SWBT_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(SWBT_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(SWBT_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
set(SWBT_BACKEND "libusb" CACHE STRING "Backend: libusb or windows-winusb")
set(SWBT_REPORT_RATE_HZ "125" CACHE STRING "Default HID report rate")
```

`SWBT_BACKEND` は `libusb` または `windows-winusb` を受け付ける。Windows cross build では `windows-winusb` を指定する。Linux native build では `libusb` を指定する。

### CMakePresets

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "linux-debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/linux-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "SWBT_BACKEND": "libusb",
        "SWBT_BUILD_TESTS": "ON"
      }
    },
    {
      "name": "linux-asan",
      "inherits": "linux-debug",
      "binaryDir": "${sourceDir}/build/linux-asan",
      "cacheVariables": {
        "SWBT_ENABLE_ASAN": "ON",
        "SWBT_ENABLE_UBSAN": "ON"
      }
    },
    {
      "name": "windows-mingw-debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/windows-mingw-debug",
      "toolchainFile": "${sourceDir}/cmake/toolchains/mingw-w64-x86_64.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "SWBT_BACKEND": "windows-winusb",
        "SWBT_BUILD_TESTS": "ON"
      }
    }
  ]
}
```

### MinGW toolchain file

```cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

## Dev Containers 方針

### Dockerfile

```Dockerfile
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    ca-certificates \
    clang \
    clang-format \
    clang-tidy \
    cmake \
    gdb \
    git \
    lcov \
    libusb-1.0-0-dev \
    lldb \
    make \
    mingw-w64 \
    ninja-build \
    pkg-config \
    valgrind \
    && rm -rf /var/lib/apt/lists/*
```

### devcontainer.json

```json
{
  "name": "swbt-daemon",
  "build": {
    "dockerfile": "Dockerfile"
  },
  "customizations": {
    "vscode": {
      "extensions": [
        "ms-vscode.cpptools",
        "ms-vscode.cmake-tools"
      ]
    }
  },
  "remoteUser": "ubuntu",
  "updateRemoteUserUID": true
}
```

`remoteUser` は Ubuntu 24.04 base image に存在する非 root の `ubuntu` にする。Dev Containers は `updateRemoteUserUID` により host user に UID/GID を合わせるため、workspace に root / nobody 所有の build artifacts が残りにくい。Linux 実機で libusb を使う場合は udev ルールを別途定義する。

Dev Container を使う場合、host 側に CMake / Ninja / MinGW を手動で入れる必要はない。host で直接 `cmake --preset ...` を実行したい場合だけ、host 側にも同等の toolchain を入れる。

## 日常開発コマンド

### Linux debug build

```bash
cmake --preset linux-debug
cmake --build build/linux-debug
ctest --test-dir build/linux-debug --output-on-failure
```

### Linux sanitizer build

```bash
cmake --preset linux-asan
cmake --build build/linux-asan
ctest --test-dir build/linux-asan --output-on-failure
```

### Windows cross build

```bash
cmake --preset windows-mingw-debug
cmake --build build/windows-mingw-debug
```

生成物の例:

```text
build/windows-mingw-debug/daemon/swbt-daemon.exe
build/windows-mingw-debug/api/swbt.dll
```

Windows ネイティブ環境で実行確認する場合は、生成物を Windows 側の作業ディレクトリへコピーする。

```powershell
.\swbt-daemon.exe --backend winusb --report-rate 125 --listen 127.0.0.1:59731
```

## Windows 実機確認手順

1. 専用 USB Bluetooth ドングルを用意する。
2. Zadig を管理者権限で起動する。
3. `Options -> List All Devices` を有効にする。
4. 対象の USB Bluetooth ドングルを選ぶ。
5. WinUSB を選んで driver を置き換える。
6. `swbt-daemon.exe` を起動する。
7. Nintendo Switch の「持ちかた/順番を変える」画面を開く。
8. daemon のログで pairing、接続、report 送信状態を確認する。
9. debug IPC client から controller state を送信する。
10. report rate、入力反映、再接続、終了時 neutral state を確認する。

注意点:

- WinUSB を割り当てた Bluetooth ドングルは、Windows 標準 Bluetooth 機能からは通常利用しない前提になる。
- 内蔵 Bluetooth を対象にしない。
- ドングルを戻す場合は、デバイスマネージャまたは Zadig で元の driver に戻す必要がある。

## daemon 開発方針

### thread モデル

```text
main thread
  - 起動引数処理
  - 設定読み込み
  - signal / shutdown 処理

ipc thread
  - TCP loopback で JSON Lines を受信
  - client owner を管理
  - set_state を state queue に投入

btstack thread
  - BTstack run loop を所有
  - HID report timer を所有
  - queue から最新 controller state を読む
  - Switch へ HID interrupt report を送信
```

BTstack API は btstack thread 以外から直接呼ばない。ipc thread は BTstack state を直接変更せず、thread-safe queue または double buffer を介する。

### controller state

client は常に full snapshot を送る。partial update は初期実装に入れない。

```json
{
  "type": "set_state",
  "seq": 120,
  "state": {
    "buttons": 1,
    "hat": "center",
    "lx": 2048,
    "ly": 2048,
    "rx": 2048,
    "ry": 2048,
    "accel": [0, 0, 0],
    "gyro": [0, 0, 0]
  }
}
```

`seq` は client 側の単調増加値とする。daemon は古い `seq` を受けても state を巻き戻さない。

### owner モデル

daemon は単一 active owner を持つ。複数 client が同時に controller state を送ると入力が競合するため、state 更新を許可する client は一つに限定する。

owner が切断した場合、daemon は neutral state へ戻す。接続中に一定時間 state 更新が途絶えた場合も neutral state へ戻す。

初期値:

```text
owner_timeout_ms = 500
neutral_on_disconnect = true
neutral_on_timeout = true
```

### report rate

daemon は BTstack timer で HID report を送る。既定値は 125Hz 相当とする。

```text
default_report_rate_hz = 125
report_period_us = 8000
```

設定可能値:

```text
60
66.7
120
125
```

起動引数例:

```bash
swbt-daemon --report-rate 125
swbt-daemon --report-rate 60
```

最終的な既定値は実機測定で決める。測定項目は次の通り。

```text
- daemon timer の予定時刻と実送信時刻の差
- BTstack の送信可能イベント間隔
- Switch 側で入力が落ちるか
- pairing 後の安定性
- reconnect 後の安定性
- Bluetooth ドングルごとの差
```

## C ABI 方針

C ABI は daemon IPC の代替ではなく、下層の安定境界として扱う。

```c
typedef struct swbt_handle swbt_handle_t;

int swbt_create(const swbt_config_t *config, swbt_handle_t **out_handle);
int swbt_start(swbt_handle_t *handle);
int swbt_stop(swbt_handle_t *handle);
void swbt_destroy(swbt_handle_t *handle);

int swbt_set_state(swbt_handle_t *handle, const swbt_controller_state_t *state);
int swbt_get_status(swbt_handle_t *handle, swbt_status_t *out_status);
int swbt_get_rumble(swbt_handle_t *handle, swbt_rumble_t *out_rumble);
const char *swbt_strerror(int code);
```

禁止事項:

- DLL / shared library 内から `exit()` を呼ばない。
- `start()` が呼び出しスレッドを永続的に占有しない。
- global singleton に依存しない。
- Python callback を BTstack thread から直接呼ばない。
- 可変長 buffer の所有権を呼び出し側と曖昧に共有しない。

## Client library 方針

Python client は将来追加する。初期 repository には `bindings/python/` を置かない。

将来追加する client は daemon IPC の thin client とする。Bluetooth、BTstack、USB、HID report 生成を Python / C# / CLI / GUI 側に持たせない。

```python
from switchbt import SwitchPadClient

pad = SwitchPadClient("127.0.0.1", 59731)
pad.acquire()
pad.set_state(buttons=0x0001, lx=2048, ly=2048, rx=2048, ry=2048)
pad.set_state(buttons=0x0000, lx=2048, ly=2048, rx=2048, ry=2048)
pad.release()
```

Python client に `tap()` のような便利関数を置く場合でも、daemon protocol には時間指定コマンドを追加しない。`tap()` は client 側で `set_state` を複数回送る薄い補助関数として扱う。

## テスト方針

### C unit tests

必須テスト:

```text
switch_subcommand_test:
  - 空 packet
  - 短い packet
  - 未知 report id
  - 未知 subcommand
  - SPI read address / size 境界
  - NFC/IR 系未対応 command

switch_report_test:
  - button bit 配置
  - stick 0x000 / 0x800 / 0xFFF
  - stick 範囲外 clamp
  - IMU little endian
  - report length 固定

switch_spi_test:
  - device info
  - controller color
  - factory calibration
  - address 範囲外

ipc_json_test:
  - valid set_state
  - unknown type
  - invalid json
  - missing field
  - wrong type
  - old seq

daemon_owner_test:
  - acquire
  - duplicate acquire
  - owner disconnect
  - timeout neutral
```

### sanitizer

Linux build で ASan / UBSan を有効にする。

```bash
cmake --preset linux-asan
cmake --build build/linux-asan
ctest --test-dir build/linux-asan --output-on-failure
```

CFLAGS の目安:

```text
-Wall
-Wextra
-Wconversion
-Wshadow
-Wswitch-enum
-Werror=implicit-function-declaration
```

### 実機テスト

Windows 実機で次を記録する。

```text
- Bluetooth ドングル名
- VID / PID
- WinUSB driver 割り当て状態
- Switch firmware version
- daemon commit hash
- report rate 設定
- pairing 成否
- reconnect 成否
- 10分連続入力の安定性
- neutral fail-safe 成否
- 終了時に Switch 側へ入力が残らないこと
```

latency 計測は初期段階では簡易でよい。daemon ログに state 受信時刻、report 反映時刻、HID 送信要求時刻を出す。映像キャプチャやハイスピード撮影による end-to-end 測定は安定後に行う。

## CI 方針

GitHub Actions ではハードウェア検証を行わない。CI はビルドと純粋なソフトウェアテストに限定する。

CI job:

```text
linux-debug:
  - cmake --preset linux-debug
  - build
  - ctest

linux-asan:
  - cmake --preset linux-asan
  - build
  - ctest

windows-mingw:
  - cmake --preset windows-mingw-debug
  - build

format:
  - clang-format check
```

ハードウェアを伴う検証は手元の Windows 実機で行い、結果を `docs/hardware-test-log.md` に記録する。

## 段階的な実装順序

### Phase 0: 開発環境

- Dev Containers を追加する。
- CMakePresets を追加する。
- Linux debug build を通す。
- Windows MinGW cross build を通す。
- 空の C unit test を CI で回す。

### Phase 1: Switch protocol core

- controller state を定義する。
- input report builder を実装する。
- subcommand parser を実装する。
- SPI read 応答を実装する。
- C unit test と golden packet test を追加する。

### Phase 2: daemon IPC

- TCP loopback JSON Lines server を実装する。
- owner 管理を実装する。
- full snapshot `set_state` を受け付ける。
- timeout neutral を実装する。
- IPC test を追加する。

### Phase 3: BTstack bridge

- BTstack HID device callback と protocol core を接続する。
- report timer を実装する。
- Linux/libusb build を通す。
- Windows/WinUSB build を通す。

### Phase 4: Windows 実機確認

- Zadig で専用 USB Bluetooth ドングルを WinUSB 化する。
- `swbt-daemon.exe` を起動する。
- Switch と pairing する。
- state 更新が入力に反映されることを確認する。
- report rate 60 / 66.7 / 120 / 125 を比較する。

### Phase 5: hardening

- malformed IPC test を追加する。
- malformed output report test を追加する。
- report scheduler jitter metrics を追加する。
- Windows WinUSB path を固める。
- release package に license / third-party notice を同梱する。

### Future: client libraries

- Python client を実装する。
- CLI debug client を実装する。
- 必要なら C# / GUI client を追加する。
- client-side helper として `tap()` を提供する場合でも daemon protocol に時間指定情報を送らない。

## Definition of Done

最小版の完了条件:

```text
- Dev Containers で Linux build が通る
- Dev Containers で Windows cross build が通る
- C unit test が CI で通る
- Windows 実機で daemon が起動する
- Switch が Pro Controller 相当として接続する
- debug IPC client から A/B/X/Y、方向入力、左右スティックを更新できる
- owner disconnect で neutral state に戻る
- daemon 終了時に入力が残らない
- report rate を起動引数で切り替えられる
- 実機テストログに使用ドングル、Switch firmware、commit hash、結果が記録される
```

## 未決事項

- C unit test framework を何にするか。
- JSON parser を自前最小実装にするか、軽量ライブラリを取り込むか。
- Windows 向け配布物を zip にするか installer にするか。
- libusb 版を正式サポートするか、開発補助扱いに留めるか。
- report rate の既定値を 125Hz のままにするか、実測後に変更するか。
- rumble を daemon status として IPC で配信するか、polling API にするか。

## 参考資料

- BTstack: Existing Ports — Windows WinUSB / libusb / Zadig の説明
  https://bluekitchen-gmbh.com/btstack/ports/existing_ports.html
- BTstack repository
  https://github.com/bluekitchen/btstack
- VS Code Dev Containers
  https://code.visualstudio.com/docs/devcontainers/containers
- CMake toolchains documentation
  https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html
- Cross Compiling With CMake
  https://cmake.org/cmake/help/book/mastering-cmake/chapter/Cross%20Compiling%20With%20CMake.html
