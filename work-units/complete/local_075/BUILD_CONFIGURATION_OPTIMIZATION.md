# Build Configuration Optimization

## 1. 概要

CMake / CMake preset / justfile のビルド経路を棚卸しし、不要な再 configure、不要な target build、直列実行、テスト実行設定の欠落を整理して、通常開発と検証のビルド時間を短縮する work unit。

この work unit は、最初に現在のビルドオプションと entrypoint の事実を明らかにする。そのうえで、`just debug`、targeted test、`just verify`、CI 相当の検証がどの target と preset を使うべきかを決め、CMakeLists / CMakePresets / justfile の変更を小さく分けて進める。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-26: `CMakeLists.txt` の最適化、ビルド対象の整理、justfile 内のコマンド修正によりビルド時間を短縮したい。並列化が施せていない可能性があり、ビルドオプション周りも明らかにしたい。
- 現状観測, 2026-06-26: `CMakePresets.json` は Ninja generator を使うが、build preset に jobs / native tool option は設定していない。
- 現状観測, 2026-06-26: `justfile` の `_debug-in-container`、`_tidy-in-container`、`_asan-in-container`、`_windows-cross-in-container` は `cmake --fresh` を使い、既存 build tree があっても再 configure する。
- 現状観測, 2026-06-26: `justfile` の `cmake --build --preset ...` は target を指定せず、tests、debug client、daemon、shared library を含む default build を実行する。
- 現状観測, 2026-06-26: `ctest --preset ...` は `--output-on-failure` と `CTEST_ARGS` を使うが、標準経路に `--parallel` はない。
- 現状観測, 2026-06-26: `just verify` は format-check、tidy、debug、ASan、Windows cross build を直列に実行する。
- 採用判断, 2026-06-26: pre-push の full `just verify` は維持し、検証範囲を削らずに CTest 並列化、ローカル増分 debug、target 指定 build、Dev Container image cache、ccache で短縮する。

use case:

- actor: maintainer、CI、Codex agent。
- 入力または状態:
  - 実装中に `just build-debug` / `just test-debug` / `just debug` を繰り返す。
  - PR 前に `just verify` を実行する。
  - 失敗箇所だけを絞って再実行する。
  - Windows host から Dev Container 経由で同じ entrypoint を使う。
- 期待する観測結果:
  - 現在の CMake option、preset、target、test 実行設定が record と必要なら operations docs から追える。
  - 開発中の build / test は不要な full configure や不要 target build を避ける。
  - `just verify` は検証の意味を変えず、必要な gate を過不足なく実行する。
  - 並列化を入れる場合は Ninja / CTest / Dev Container / CI のどこで効くかを明示する。
  - Windows native PowerShell からの委譲経路と Dev Container 内実行の挙動が一致する。
- 制約:
  - `just verify` の gate 意味を弱めない。
  - host build opt-in gate、Dev Container 入口、Windows cross build、clang-tidy、ASan の標準経路を壊さない。
  - BTstack source selection の意味を変える場合は根拠監査を必要とする。

source から use case への変換:

ビルド時間短縮は単に jobs を増やすだけでは閉じない。現状は Ninja を使っているため build phase 自体は Ninja の既定並列に委ねられている可能性がある。一方で、`--fresh` による再 configure、default target の広さ、CTest の直列実行、verify gate の直列構成は、明示的に見直す価値がある。この work unit では、測定なしの推測で削るのではなく、現状設定と所要時間を観測してから変更する。

## 3. 対象範囲

- 現在の CMake option、cache variable、preset、build preset、test preset、just entrypoint を棚卸しする。
- `cmake --build` の target 指定方針を決める。
- `SWBT_BUILD_TESTS`、`SWBT_BUILD_SHARED`、daemon / debug client / unit test target の既定 build 対象を整理する。
- `cmake --fresh` を常時使う recipe と、増分 configure / build を許す recipe を分ける。
- Ninja build jobs と CTest parallel jobs の指定方法を決める。
- Dev Container 委譲時に jobs / `CTEST_ARGS` / 追加環境変数が壊れずに伝わるか確認する。
- `just debug`、`just build-debug`、`just test-debug`、`just tidy`、`just asan`、`just windows-cross`、`just verify`、`just verify-ci` の役割を整理する。
- 必要なら `spec/operations/development-tooling.md` または関連 operations docs を更新する。
- 変更前後で build / test 所要時間を記録し、短縮が事実か、単に gate を削っただけかを分ける。

## 4. 対象外

- Switch protocol byte、HID descriptor、report period、subcommand、SPI、rumble の変更。
- BTstack 本体の編集。
- `vendor/btstack` の source selection 変更。ただし不要 target build を避けるための CMake target 依存整理は対象に含める。
- CI provider の runner size、sccache 導入、self-hosted runner。
- binary release、installer、service manager。
- 実機 pairing、HID advertising、report loop の実行。

## 5. 関連 spec / docs

- `CMakeLists.txt`
- `CMakePresets.json`
- `justfile`
- `cmake/compiler_warnings.cmake`
- `cmake/sanitizers.cmake`
- `cmake/btstack_sources.cmake`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md`
- `work-units/complete/local_041/FORMAT_UNTRACKED_C_SOURCES.md`
- `work-units/complete/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/complete/local_073/DAEMON_CONFIG_LINK_KEY_RECONNECT.md`

## 6. 根拠監査

not applicable for the initial record。Switch protocol bytes、BTstack source selection、report timing、WinUSB/libusb の採用値はこの work unit の主対象ではない。

ただし、BTstack source family の選択、backend source の追加削除、WinUSB/libusb backend の意味を変える場合は `source-audit` を使う。その場合は文献値、upstream 実装値、swbt 実装値、推定を分けて記録する。

## 7. 設計メモ

- Ninja generator は CMake preset で指定済みである。並列化がまったく無いとは断定しない。未確認なのは、`cmake --build` に jobs を明示する必要があるか、CTest が直列実行になっているか、Dev Container 委譲で jobs 指定を渡せるかである。
- `cmake --fresh` は build tree の stale cache を避けるには有効だが、開発中の反復では configure 時間を毎回払う。`verify` や `tidy` では fresh を維持し、`build-debug` / targeted test では増分を許す分離が候補になる。
- default target を広くしたまま build time を語ると、daemon 本体、debug client、shared library、すべての test executable が混ざる。開発用 target と gate 用 target を分けて測る。
- `SWBT_BUILD_TESTS=ON` は test gate には必要だが、daemon だけの build には過剰な可能性がある。`SWBT_BUILD_SHARED=ON` も、C ABI library を使わない通常 daemon 開発では切り替え候補になる。
- `just verify` の直列実行はログと失敗切り分けには有利である。並列化する場合でも、どの gate が同じ build tree を共有するか、build directory を分けているか、CPU / disk contention がかえって遅くしないかを測る。
- CTest の `--parallel` は unit test 実行時間を短縮できる可能性があるが、test が同じ port、file path、environment variable、diagnostic output を共有していないか確認する。
- `ctest --parallel 8` は `linux-debug` で 41/41 pass し、同じ build tree の直列実行 `4.38 sec` に対して `1.73 sec` だった。既定値は `8` とし、`SWBT_CTEST_PARALLEL_LEVEL` で上書き可能にする。
- `ccache` は見つかった場合だけ CMake compiler launcher に設定する。これにより、古い local Dev Container では launcher なしで通り、新しい Dev Container / CI image では cache が効く。
- Dev Container image cache は `devcontainers/ci` の `imageName` / `cacheFrom` / `push: filter` を使う。`main` push だけ GHCR に push し、pull request は cache image を参照する。

## 8. 対象ファイル

- `CMakeLists.txt`
- `CMakePresets.json`
- `justfile`
- `cmake/*.cmake`
- `tests/cmake/*.cmake`
- `scripts/format.sh`
- `scripts/check-format.sh`
- `.github/workflows/*`
- `spec/operations/development-tooling.md`
- `work-units/complete/local_075/BUILD_CONFIGURATION_OPTIMIZATION.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| green | build option inventory records current CMake options, presets, default targets, and just entrypoints before behavior changes | characterization | docs/build | no |
| green | incremental debug build path avoids forced fresh configure while verify still starts from a clean or explicitly refreshed configure boundary | behavior | build/integration | no |
| green | daemon-only or targeted build path can build `swbt-daemon` without building unrelated test executables when tests are not requested | behavior | build/integration | no |
| green | test execution can use explicit CTest parallelism without sharing mutable ports or diagnostic paths between tests | behavior | test/integration | no |
| green | Dev Container delegation preserves build/test job settings from Windows PowerShell and in-container execution | regression | integration | no |
| green | `just verify` still covers format-check, clang-tidy, debug build/test, ASan, and Windows cross build after command restructuring | regression | integration | no |
| green | build-time measurements compare before and after commands with the same source tree and same build cache assumptions | characterization | docs/build | no |
| green | CI uses a Dev Container image cache without requiring pull requests to push images | behavior | ci | no |
| green | CI compiler cache is enabled through ccache when available and remains optional for older local containers | behavior | ci/build | no |

## 10. 検証

initial record creation では未実行。その後、CMake / justfile / CI 設定を変更し、以下を確認した。

開始時の静的確認:

- `CMakePresets.json`: `linux-debug`、`linux-asan`、`linux-clang-tidy`、`windows-mingw-debug` は Ninja generator を使う。
- `CMakePresets.json`: build preset に jobs 指定はない。
- `CMakePresets.json`: test preset に `outputOnFailure` はあるが、parallel 指定はない。
- `justfile`: `_debug-in-container` は `cmake --fresh --preset linux-debug`、`cmake --build --preset linux-debug`、`ctest --preset linux-debug --output-on-failure` を直列実行する。
- `justfile`: `_asan-in-container` と `_windows-cross-in-container` も `cmake --fresh` から始まる。
- `justfile`: `_verify-in-container` は format-check、tidy、debug、ASan、Windows cross build を直列実行する。
- `ctest --preset linux-debug --parallel 8 --output-on-failure`: pass、41/41、`1.73 sec`。
- `ctest --preset linux-debug --output-on-failure`: pass、41/41、`4.38 sec`。
- `just build-daemon-debug`: pass。現行 container では `ccache launcher: not found` と表示され、launcher なしで継続する。
- `just test-debug`: pass、41/41、`1.74 sec`。
- `just debug`: pass、41/41、`1.75 sec`。configure は `cmake --preset linux-debug` で実行された。
- `just asan`: pass、41/41、`1.86 sec`。CTest は 8 並列で実行された。
- `just verify`: pass。`format-check`、`tidy`、fresh debug build/test、ASan build/test、Windows cross build を確認した。debug / ASan の CTest は 8 並列で実行された。
- `just devcontainer-rebuild`: pass。Dev Container image を再作成し、`ccache` package が install された。
- rebuild 後の `just build-daemon-debug`: pass。CMake configure で `ccache launcher: /usr/bin/ccache` を確認した。

今後の測定候補:

- fresh `just debug`
- repeated `just build-debug`
- repeated `just test-debug`
- targeted `CTEST_ARGS='-R <test>' just test-debug`
- `just verify`
- target 指定あり `cmake --build --preset linux-debug --target swbt-daemon`
- CTest parallel 指定あり `ctest --preset linux-debug --parallel <n> --output-on-failure`

## 11. 実機実行条件

実機不要。CMake、CMake preset、justfile、Dev Container、unit / build gate の整理に閉じるため、Bluetooth adapter open、Switch pairing、HID advertising、report loop は実行しない。

実機に進む必要が生じるのは、ビルド対象整理が production backend の runtime behavior、BTstack source selection、WinUSB/libusb backend、diagnostic output の実機運用を変える場合だけである。その場合は別 work unit またはこの record の scope 更新として `hardware-harness` を使い、人間の明示承認、専用 USB Bluetooth dongle、WinUSB driver assignment、`docs/hardware-test-log.md` への記録を条件にする。

## 12. 先送り事項

- 観測: `just verify` の gate 間並列実行は、build directories が分かれているため可能に見える。
  先送り理由: ログの読みやすさ、CPU / disk contention、Dev Container runner の負荷、失敗時の停止条件を先に決める必要がある。
  次の置き場: この work unit の測定後に判断する。必要なら後続 work unit に分離する。

## 13. チェックリスト

- [x] source を user request と現状の CMake / justfile 観測から特定した。
- [x] use case を開発中の反復 build/test、PR 前 verify、失敗箇所の再実行、Windows host 委譲として定義した。
- [x] 根拠監査と実機実行条件の要否を分けた。
- [x] 現在の CMake option / preset / target / just entrypoint を測定付きで棚卸しした。
- [x] build target と test target の整理方針を決めた。
- [x] `cmake --fresh` を使う recipe と増分を許す recipe を分けた。
- [x] Ninja / CTest jobs 指定の扱いを実装または明示的に不要と判断した。
- [x] Windows PowerShell から Dev Container への引き渡しを確認した。
- [x] CI の Dev Container image cache と ccache 方針を実装した。
- [x] `just verify` の coverage が弱くなっていないことを確認した。
- [x] 変更前後の build time と検証結果を記録した。
