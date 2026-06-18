# swbt-daemon repository initialization TODO

作成日: 2026-06-16
更新日: 2026-06-18

## 現在の方針

BTstack fork を root に置く方式は採用しない。

現在の構成は、clean な `swbt-daemon` repository を root とし、BTstack は Git submodule として `vendor/btstack` に固定する方式とする。

```text
swbt-daemon/
  .devcontainer/
  .github/
  api/
  apps/swbt-daemon/
  cmake/
  docs/
  swbt/
    core/
    switch/
    btstack_bridge/
  tests/
  tmp/
  vendor/
    btstack/        # Git submodule
```

## 現状メモ

```text
branch: main
BTstack submodule path: vendor/btstack
BTstack upstream: https://github.com/bluekitchen/btstack
BTstack pinned tag: v1.8.2
BTstack pinned commit: 075a0780f0fad7ff67d58ac19f46e8953656a752
```

`bindings/python/` は初期構成から外す。Python client は daemon IPC が固まった後の将来タスクにする。

## ライセンス方針

`swbt-daemon` 自前コードは MIT License とする。

BTstack は BlueKitchen の別ライセンスであり、MIT ではない。BTstack を含む、または BTstack にリンクしたビルド成果物は、MIT-only artifact として扱わない。

必要ファイル:

- `LICENSE`: swbt-daemon 自前コードにだけ適用される MIT License
- `THIRD_PARTY_NOTICES.md`: BTstack が別ライセンスであること、商用利用時は BlueKitchen 側の条件確認が必要であることを明記
- `vendor/btstack/LICENSE`: BTstack 側ライセンス
- `vendor/btstack/3rd-party/README.md`: BTstack が含む third-party dependency notice

release / binary 配布時の扱い:

- `LICENSE` と `THIRD_PARTY_NOTICES.md` を同梱する
- BTstack を含む配布物には `vendor/btstack/LICENSE` と `vendor/btstack/3rd-party/README.md` 相当の notice を同梱する
- release description で「original swbt-daemon code is MIT licensed」「builds that include BTstack are also subject to BTstack license」と明記する
- BTstack を含む binary を MIT-only と表現しない

## 初期構成で完了済み

- [x] `git init -b main`
- [x] `vendor/btstack` を submodule として追加
- [x] BTstack を `v1.8.2` / `075a0780f0fad7ff67d58ac19f46e8953656a752` に pin
- [x] root `README.md`
- [x] root `LICENSE`
- [x] `THIRD_PARTY_NOTICES.md`
- [x] `.devcontainer/`
- [x] root `CMakeLists.txt`
- [x] root `CMakePresets.json`
- [x] `cmake/toolchains/mingw-w64-x86_64.cmake`
- [x] `cmake/compiler_warnings.cmake`
- [x] `cmake/sanitizers.cmake`
- [x] `.github/workflows/ci.yml`
- [x] `docs/upstream-btstack.md`
- [x] `docs/hardware-test-log.md`
- [x] minimal `swbt_core`
- [x] minimal `swbt-daemon` stub
- [x] minimal C ABI stub
- [x] smoke test
- [x] `tmp/` の旧検討メモを現在方針へ更新

## 直近でやること

### Phase 0: 初期 commit

- [ ] `git status` で追加範囲を確認する
- [ ] CMake がある環境または Dev Container で configure/build/test を実行する
- [ ] 初期 skeleton を commit する

推奨 commit message:

```text
Initialize swbt-daemon skeleton
```

### Phase 1: BTstack source selection

- [x] `vendor/btstack/port/libusb`
- [x] `vendor/btstack/port/windows-winusb`
- [x] `vendor/btstack/src/classic`
- [x] `vendor/btstack/src/hci.c` など必要 source を調査する
- [x] CMake 側に `cmake/btstack_sources.cmake` を追加する
- [x] BTstack 本体を直接変更せず bridge 側で吸収できる範囲を確認する

### Phase 2: Switch protocol core

- [x] `swbt/switch/switch_report.*`
- [x] `swbt/switch/switch_subcommand.*`
- [x] `swbt/switch/switch_spi.*`
- [x] `swbt/switch/switch_rumble.*`
- [x] golden packet test

### Phase 3: daemon IPC

- [x] local TCP JSON Lines server
- [x] `hello`
- [x] `acquire` / `release`
- [x] `set_state`
- [x] `get_status`
- [x] owner disconnect neutral
- [x] heartbeat / timeout neutral

### Phase 4: BTstack bridge

- [x] HID Device registration
- [x] Output Report parser 接続
- [x] Subcommand `0x21` reply
- [x] Periodic `0x30` input report
- [x] libusb build
- [x] windows-winusb MinGW build

### Phase 5: Windows 実機確認

- [ ] 専用 USB Bluetooth dongle を用意する
- [ ] Zadig で WinUSB driver を割り当てる
- [ ] `swbt-daemon.exe` を Windows native で起動する
- [ ] Switch pairing
- [ ] report period `8000 / 8333 / 15000 / 16667 us` 比較
- [ ] `docs/hardware-test-log.md` に記録する

### Future: client libraries

- [ ] Python client
- [ ] CLI debug client
- [ ] C# client
- [ ] GUI

初期 repository には `bindings/python/` を置かない。daemon IPC と C protocol core が固まってから追加する。

## 初期段階でやらないこと

- BTstack 本体の広範囲改変
- `vendor/btstack/src/classic/hid_device.c` の validation 無効化
- daemon protocol の `tap`
- daemon protocol の `duration_ms`
- daemon protocol の `sequence`
- daemon protocol の `at_ms`
- 複数 controller 同時接続
- Python client 実装
- binary release

## 判断メモ

`vendor/btstack` submodule 方式を選んだ理由:

- root repository を `swbt-daemon` 固有の構成にできる
- BTstack 自身が持つ `3rd-party/` と swbt-daemon 側 dependency を混同しにくい
- BTstack の exact commit を親 repository から pin できる
- BTstack license と swbt-daemon license の境界を明示しやすい
- upstream BTstack 追従時の差分確認がしやすい

BTstack に patch が必要になった場合:

- まず `swbt/btstack_bridge/` で吸収する
- それでも必要なら BlueKitchen upstream に issue / patch を検討する
- fork が必要になった場合は submodule URL を自分の BTstack fork に切り替え、`docs/upstream-btstack.md` に理由と commit を記録する
