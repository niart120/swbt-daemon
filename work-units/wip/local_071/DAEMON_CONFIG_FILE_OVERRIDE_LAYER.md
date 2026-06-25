# Daemon Config File Override Layer

## 1. 概要

daemon 起動時パラメータを、環境変数だけに依存する形から、設定ファイルを基本値、環境変数を一時 override とする形へ移す work unit。

この work unit は、環境変数を即座に全廃することを目的にしない。目的は、`swbt_daemon_config_default()`、設定ファイル、環境変数 override の合成順序を固定し、値の解釈と検証を `swbt/daemon/config.*` の testable boundary に寄せることである。

完了後の起動設定は、未設定なら built-in default、設定ファイルがあればその値、環境変数があれば最後に override という順序で決まる。実機安全 gate は通常設定と同列に扱わず、永続設定だけで adapter open が進まないようにする。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-24: 起動時パラメータの環境変数依存を減らし、設定ファイルへ順次置き換えたい。ただし設定ファイルに完全移行するのではなく、設定ファイルの値を環境変数で override できる形にしたい。
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`: 環境変数を optional runtime override、required hardware safety gate、optional diagnostic sink、development tooling gate に分類し、`swbt/daemon/config.*` へ runtime env parsing を寄せた。
- `docs/status.md`: production 起動条件と実機実行時の推奨環境変数を状態表として記録している。
- `work-units/complete/local_065/BONDED_RECONNECT_PERSISTENCE.md`: TLV-backed bond cache 経路は実機観測後に不採用として閉じた。設定ファイル移行では、この未採用 path を復活させない。
- `work-units/wip/local_072/ACTIVE_SWITCH_RECONNECT.md`: reconnect 用の永続状態または設定が必要になった場合は、active reconnect 側で境界を決めてからこの work unit へ取り込む。
- user discussion, 2026-06-25: active reconnect 用 Switch address は自動取得し、設定ファイル layer に永続化する。削除は設定ファイル上の値の除去で扱う。trace log では検証用に address を出してよいが、リポジトリ上の記録は scrub する。
- user discussion, 2026-06-26: 同一 TOML file 更新を前提にするなら、TOML parser / serializer のために C++ 導入または部分的な C++ 化も検討する。

use case:

- actor: maintainer、hardware operator、production daemon。
- 入力または状態: 設定ファイルなしの clean checkout、設定ファイルありの daemon 起動、環境変数 override ありの daemon 起動、実機安全 gate 未承認の production 起動。
- 期待する観測結果:
  - 設定ファイルがない場合は、現行 default と同じ値で起動設定が成立する。
  - 設定ファイルに書いた runtime / diagnostic 設定が daemon config に反映される。
  - 同じ key を環境変数で指定した場合は、環境変数が設定ファイルの値を override する。
  - 設定ファイルまたは環境変数に不正値がある場合、既存 config を部分更新せず startup config failure になる。
  - 永続設定だけでは実機承認が成立せず、adapter open 前に止まる。
- 制約: 実機安全 gate を緩めない。Switch-facing bytes、BTstack source selection、report period の既定値そのものはこの work unit で変更しない。
- 対象外: GUI / system service integration、installation path policy の完成、設定ファイル format の外部互換保証、実機 reconnect 検証、CLI subcommand の本格実装。

source から use case への変換:

`local_045` は環境変数依存を分類し、env parsing を testable にした。この work unit では、その分類を使って設定ファイル layer を追加する。環境変数は廃止対象ではなく、設定ファイルに対する operator override として残す。

## 3. 対象範囲

- 起動設定の合成順序を `default -> config file -> environment override` として固定する。
- 設定ファイル format、key 名、型、unknown key の扱いを設計する。
- 設定ファイル path の探索または明示指定方法を設計する。
- `swbt/daemon/config.*` に設定ファイル入力と環境変数 override の合成 helper を追加する。
- `apps/swbt-daemon/main.c` から値の解釈をさらに減らし、process env / file path の収集だけに寄せる。
- runtime override、diagnostic sink の設定ファイル対応を検討する。
- active reconnect 用の Switch address や reconnect policy は、この work unit では key を予約しない。`local_072` で必要な state / config boundary を決めてから扱う。
- hardware safety gate を永続設定で満たせないことを test で固定する。
- `docs/status.md` または関連 operations spec に、設定ファイルと環境変数 override の優先順位を記録する。

## 4. 対象外

- `SWBT_RUN_HARDWARE` / `SWBT_HARDWARE_APPROVED` を削除または緩和すること。
- 設定ファイルだけで実機承認済みにすること。
- CMake / Dev Container / hook / `just` の development tooling 環境変数を daemon config file へ移すこと。
- Switch protocol byte、report period の既定値、device info payload の変更。
- bond cache persistence の復活。`local_065` で削除した TLV-backed link key DB / bond cache path は設定ファイル key にしない。
- active reconnect の address 保存、reconnect policy、operator cleanup command。必要になった時点で `local_072` または後続 work unit から取り込む。
- service manager、Windows registry、installer、binary release。

## 5. 関連 spec / docs

- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`
- `work-units/complete/local_065/BONDED_RECONNECT_PERSISTENCE.md`
- `work-units/wip/local_072/ACTIVE_SWITCH_RECONNECT.md`
- `docs/status.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `spec/architecture/daemon-architecture-cutover.md`

## 6. 根拠監査

not applicable。

この work unit は daemon 起動設定の入力 layer と validation を扱う。Switch HID report bytes、subcommand、SPI、rumble、report period 採用値、BTstack source selection、WinUSB/libusb facts は追加または変更しない。

ただし active reconnect 用の address / policy を設定ファイル化する場合、BTstack outbound L2CAP 境界と実機 reconnect は `local_072` の根拠監査と実機実行条件に従う。

## 7. 設計メモ

- 優先順位は `built-in default < config file < environment override` とする。
- 設定ファイルがないことは通常起動の失敗にしない。
- 明示された設定ファイルが読めない、構文不正、不正値を含む場合は startup config failure とする。
- unknown key は初期実装では failure 候補とする。typo を黙って無視すると実機設定で危険な誤解を生むためである。
- hardware approval は通常 config と分ける。永続ファイルに `approved = true` のような値を置ける設計にはしない。
- production backend selection は設定ファイル化してよい候補だが、実機実行には別途 live approval が必要である。
- diagnostic path は設定ファイル化してよい。未設定なら no-op、明示 path の open failure をどう扱うかは既存挙動と整合させる。
- `local_065` の bond cache path は不採用のため、設定ファイル key として追加しない。active reconnect に必要な Switch address や policy は、保存対象、削除手段、raw 値の扱いを `local_072` で決めてから取り込む。
- active reconnect address は設定ファイル layer の対象にする。手書きの explicit address と daemon-managed learned address は同じ TOML file 内で別 key として分ける。daemon は learned address の table だけを更新する。
- active reconnect の削除境界は、設定ファイル上の explicit address または daemon-managed learned address の除去とする。起動時環境変数による reset は採用しない。
- 設定ファイル format は実装前に確定する。現時点の第一候補は TOML である。理由は、table / scalar / array の範囲で十分で、YAML より暗黙型変換や alias / anchor の複雑さが少なく、daemon 設定としての可読性と可搬性を両立しやすいためである。
- 同一ファイル更新を採るため、TOML parser は read だけでなく serializer / round-trip update の性質を確認する。特に comment、key order、unknown key、明示設定 table を壊さず `[active_reconnect.learned]` だけを書けるかを採用条件に含める。
- 現時点の調査候補は `toml11` と `toml++` である。C++ dependency を入れる場合は C runtime surface に C++ 型を出さず、`swbt/daemon/config.*` からは C ABI 風の境界に閉じる。
- C++ 化は全面移行ではなく、設定ファイル parser / writer の小さな implementation island として検討する。daemon core、BTstack bridge、public C ABI、tests の大半は C11 のまま維持する。
- C++ island を採る場合は `project(... LANGUAGES C CXX)`、C++ standard、Windows MinGW cross build、clang-tidy 対象、sanitizer、third-party notice を local_071 の acceptance に含める。
- C++ dependency を選ぶ場合でも、`swbt/daemon/config.h` は C++ 型、例外、STL container を公開しない。C 側からは status code と POD input/output だけを見る。
- YAML は人間向け文書としては強いが、daemon 設定では parser dependency、暗黙型、複雑な構文 surface が重い。採用する場合は unknown key、型解釈、コメント保持、license impact を TOML より強く確認する。
- 新規 dependency を増やす場合は C build / cross build / license impact を確認する。依存を増やさないなら、限定 key-value format を採用し、escape や nested structure を後回しにする。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `CMakeLists.txt`
- `CMakePresets.json`
- `cmake/compiler_warnings.cmake`
- `justfile`
- `scripts/format.sh`
- `scripts/check-format.sh`
- `swbt/daemon/config.h`
- `swbt/daemon/config.c`
- `swbt/daemon/config_file.cpp`
- `tests/daemon_runtime_test.c`
- `tests/daemon_production_backend_test.c`
- `docs/status.md`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `work-units/wip/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | missing config file uses built-in daemon defaults and preserves current no-op startup behavior | regression | unit | no |
| todo | valid config file values are applied before environment overrides | new | unit | no |
| todo | environment override wins over config file for the same runtime key | new | unit | no |
| todo | invalid config file value fails without partially mutating existing daemon config | edge | unit | no |
| todo | invalid environment override still fails after a valid config file without partially mutating existing daemon config | edge | unit | no |
| todo | unknown config file key is rejected or explicitly documented as ignored before implementation completes | edge | unit | no |
| todo | hardware approval cannot be granted by persistent config file alone | regression | integration | no |
| todo | diagnostic paths can be supplied by config file and remain overrideable by environment variables | new | unit/integration | no |
| refactor-skipped | C++ config-file implementation island builds behind a C config boundary and passes debug, tidy, ASan, and Windows cross build | characterization | build/unit | no |
| todo | TOML dependency is accepted only behind a C config boundary and passes debug, tidy, ASan, and Windows cross build | characterization | build/unit | no |
| deferred | active reconnect state / policy can be supplied through daemon config abstraction after `local_072` freezes the boundary | new | unit | no |
| deferred | choose and freeze release-stable config file path / discovery policy | behavior | design | no |

## 10. 検証

software test を実行した。実機検証は不要であり、実行していない。

起票時確認:

- `git branch --show-current`: `docs/config-file-work-units`。
- `git status --short`: `local_065` cleanup、`local_072` 起票、この record の更新を同じ branch 上で扱っている。
- 既存 work unit 番号は `complete/local_070` まで存在するため、この record は `wip/local_071` とした。

TDD status:
- source: 設定ファイルがない clean checkout では現行 default と同じ値で起動設定が成立する。
- use case: optional config file path が存在しない場合、daemon config は部分更新されず default を維持する。
- item: missing config file uses built-in daemon defaults and preserves current no-op startup behavior。
- state: refactor-skipped
- commands:
  - red: `just build-debug`
  - green: `just build-debug`
  - verification: `just test-debug`
- notes: `tests/daemon_config_file_test.c` を追加した。red は期待通り `swbt_daemon_config_file_source_t`、`swbt_daemon_config_apply_file`、`SWBT_DAEMON_CONFIG_FILE_OK` 未定義で compile failure になった。green では `swbt_daemon_config_apply_file` を追加し、optional path が未指定または `ENOENT` の場合だけ no-op success にした。required path の IO failure、既存 file の parse、TOML parser / serializer は後続 item に残す。targeted `CTEST_ARGS="-R daemon_config_file_test" just test-debug` は Dev Container 起動前の `docker ps` error になったため、同じ build tree で full `just test-debug` を実行した。

Refactor status:
- decision: refactor-skipped
- change: formatter 以外の構造変更は行わない。
- unchanged behavior: 今回の item は config file layer の入口だけを追加し、既存 env override、hardware approval、diagnostic path、daemon main の起動順序は変更していない。
- verification: `just build-debug`, `just test-debug`
- notes: parser / writer の導入、同ファイル更新、unknown key policy は次 item 以降の behavior であり、この cycle には混ぜない。

TDD status:
- source: 同一 TOML file 更新を採る場合、設定ファイル parser / writer のために C++ implementation island を使う可能性がある。
- use case: C caller は `swbt_daemon_config_apply_file()` を C boundary として呼び、実装側だけを C++ にできる。CMake / Just / tidy / sanitizer / Windows cross build は C++ 混在を標準 gate として扱う。
- item: C++ config-file implementation island builds behind a C config boundary and passes debug, tidy, ASan, and Windows cross build。
- state: refactor-skipped
- commands:
  - red: `just build-debug`
  - green: `just build-debug`
  - verification: `just test-debug`, `just format-check`, `just tidy`, `just asan`, `just windows-cross`
- notes: red は `swbt/daemon/config_file.cpp` を CMake source に追加したが `project(... LANGUAGES C)` のままだったため、C test から `swbt_daemon_config_apply_file` が未解決になった。green では `project(... LANGUAGES C CXX)`、C++17、C / C++ 別 warning、`CXX_CLANG_TIDY`、`clang++` preset、MinGW CXX compiler、C++ source の format 対象を追加した。`swbt_daemon_config_apply_file` 実装は `swbt/daemon/config_file.cpp` に移し、`swbt/daemon/config.h` は `extern "C"` guard を持つ。TOML dependency はまだ追加していない。

Refactor status:
- decision: refactor-skipped
- change: C++ build surface を固定するための構造変更であり、green 後の追加整理は行わない。
- unchanged behavior: optional config file missing の no-op success、既存 env override、hardware approval、diagnostic path、daemon main の起動順序は変更していない。
- verification: `just build-debug`, `just test-debug`, `just format-check`, `just tidy`, `just asan`, `just windows-cross`
- notes: C++ 導入範囲は `swbt/daemon/config_file.cpp` に閉じている。C++ 型、例外、STL container は `swbt/daemon/config.h` に公開していない。

## 11. 実機実行条件

この work unit の設定合成、validation、override precedence の実装では実機は不要である。

理由: Bluetooth adapter open、Switch pairing、HID advertising、report loop を実行しない。hardware safety gate の回帰は fake backend または software integration test で確認する。

実機が必要になるのは、設定ファイル移行と同時に production backend の既定化、report period 既定値変更、device info payload 変更、active reconnect 実装を行う場合である。その場合は別 work unit または `local_072` の実機実行条件に従う。

## 12. 先送り事項

- 観測: release-stable な設定ファイル path と discovery policy は OS ごとの運用方針を含む。
  先送り理由: 初期実装では明示 path から始める方が検証しやすく、installer / service manager がない段階で OS path を固定すると後で migration が必要になる。
  次の置き場: `spec/operations/` の設定ファイル運用 spec、または後続 work unit。
- 観測: CLI subcommand による `show-config` / `validate-config` は有用である。active reconnect の保存状態を持つ場合は、削除 command も候補になる。
  先送り理由: この work unit は daemon 起動設定の合成 layer を先に固定する。CLI 管理 surface は scope が広い。
  次の置き場: 後続 work unit。

## 13. チェックリスト

- [x] source を user request、`local_045`、`local_065`、`local_072`、`docs/status.md` から特定した。
- [x] use case を config file base + environment override として定義した。
- [ ] config file format と unknown key policy を決めた。
- [x] red test または characterization test を追加した。
- [ ] config file layer を実装した。
- [ ] environment override precedence を test で固定した。
- [ ] hardware safety gate が永続設定で緩まないことを確認した。
- [ ] docs / status を更新した。
- [ ] 検証結果または未実行理由を記録した。
