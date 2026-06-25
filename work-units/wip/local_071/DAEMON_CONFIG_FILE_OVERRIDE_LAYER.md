# Daemon Config File Override Layer

## 1. 概要

daemon 起動時パラメータを、環境変数だけに依存する形から、設定ファイルを基本値、環境変数を一時 override とする形へ移す work unit。

この work unit は、環境変数を即座に全廃することを目的にしない。目的は、`swbt_daemon_config_default()`、設定ファイル、環境変数 override の合成順序を固定し、値の解釈と検証を `swbt/daemon/config.*` の testable boundary に寄せることである。

完了後の起動設定は、未設定なら built-in default、設定ファイルがあればその値、環境変数があれば最後に override という順序で決まる。この work unit の設定ファイル schema は daemon の再利用可能な runtime 値に絞り、backend 起動モード、実機承認、診断出力 path は永続設定に含めない。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-24: 起動時パラメータの環境変数依存を減らし、設定ファイルへ順次置き換えたい。ただし設定ファイルに完全移行するのではなく、設定ファイルの値を環境変数で override できる形にしたい。
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`: 環境変数を optional runtime override、required hardware safety gate、optional diagnostic sink、development tooling gate に分類し、`swbt/daemon/config.*` へ runtime env parsing を寄せた。
- `docs/status.md`: production 起動条件と実機実行時の推奨環境変数を状態表として記録している。
- `work-units/complete/local_065/BONDED_RECONNECT_PERSISTENCE.md`: TLV-backed bond cache 経路は実機観測後に不採用として閉じた。設定ファイル移行では、この未採用 path を復活させない。
- `work-units/wip/local_072/ACTIVE_SWITCH_RECONNECT.md`: reconnect 用の永続状態または設定が必要になった場合は、active reconnect 側で境界を決めてからこの work unit へ取り込む。
- user discussion, 2026-06-25: active reconnect 用 Switch address は自動取得し、設定ファイル layer に永続化する。削除は設定ファイル上の値の除去で扱う。trace log では検証用に address を出してよいが、リポジトリ上の記録は scrub する。
- user discussion, 2026-06-26: 同一 TOML file 更新を前提にするなら、TOML parser / serializer のために C++ 導入または部分的な C++ 化も検討する。
- user discussion, 2026-06-26: dependency は `vendor/*` の Git submodule として pin し、BTstack と同じ依存管理方針に寄せる。
- user discussion, 2026-06-26: backend selection の既定を production に反転し、test / smoke 時だけ明示的に noop を選ぶ案は、CLI parser を含む別 work unit として扱う。診断出力 path も永続設定ではなく起動時引数に寄せる。

use case:

- actor: maintainer、hardware operator、production daemon。
- 入力または状態: 設定ファイルなしの clean checkout、設定ファイルありの daemon 起動、環境変数 override ありの daemon 起動、設定ファイルに unsupported key を含む起動。
- 期待する観測結果:
  - 設定ファイルがない場合は、現行 default と同じ値で起動設定が成立する。
  - 設定ファイルに書いた runtime 設定が daemon config に反映される。
  - 同じ key を環境変数で指定した場合は、環境変数が設定ファイルの値を override する。
  - 設定ファイルまたは環境変数に不正値がある場合、既存 config を部分更新せず startup config failure になる。
  - backend 起動モード、実機承認、診断出力 path はこの設定ファイル schema では受け付けない。
- 制約: CLI 起動モードと実機承認の再設計は別 work unit に分ける。Switch-facing bytes、BTstack source selection、report period の既定値そのものはこの work unit で変更しない。
- 対象外: GUI / system service integration、installation path policy の完成、設定ファイル format の外部互換保証、実機 reconnect 検証、CLI subcommand の本格実装。

source から use case への変換:

`local_045` は環境変数依存を分類し、env parsing を testable にした。この work unit では、その分類を使って設定ファイル layer を追加する。環境変数は廃止対象ではなく、設定ファイルに対する operator override として残す。

## 3. 対象範囲

- 起動設定の合成順序を `default -> config file -> environment override` として固定する。
- 設定ファイル format、key 名、型、unknown key の扱いを設計する。
- 設定ファイル path の探索または明示指定方法を設計する。
- `swbt/daemon/config.*` に設定ファイル入力と環境変数 override の合成 helper を追加する。
- `apps/swbt-daemon/main.c` から値の解釈をさらに減らし、process env / file path の収集だけに寄せる。
- runtime override の設定ファイル対応を実装する。
- 設定ファイルの初期 schema は `ipc`、`report`、`device.profile` に絞る。
- active reconnect 用の Switch address や reconnect policy は、この work unit では key を予約しない。`local_072` で必要な state / config boundary を決めてから扱う。
- `docs/status.md` または関連 operations spec に、設定ファイルと環境変数 override の優先順位を記録する。

## 4. 対象外

- `SWBT_RUN_HARDWARE` / `SWBT_HARDWARE_APPROVED` を削除または緩和すること。これは `local_073` で扱う。
- `SWBT_DAEMON_BACKEND` の削除、CLI 化、production 既定化、noop 明示化。これは `local_073` で扱う。
- 診断出力 path を設定ファイル key にすること。診断出力は `local_073` で CLI flag として扱う。
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
- hardware approval は設定ファイル key にしない。削除または緩和の是非は `local_073` で、CLI 起動モードと合わせて扱う。
- production backend selection は設定ファイル key にしない。daemon の実行モードであり、`local_073` で CLI flag として設計する。
- diagnostic path は設定ファイル key にしない。診断出力は永続設定ではなく、その起動だけの観測指定として CLI flag に寄せる。既存環境変数の扱いは `local_073` で互換性と削除方針を決める。
- `local_065` の bond cache path は不採用のため、設定ファイル key として追加しない。active reconnect に必要な Switch address や policy は、保存対象、削除手段、raw 値の扱いを `local_072` で決めてから取り込む。
- active reconnect address は将来の設定ファイル layer 対象にする。手書きの explicit address と daemon-managed learned address は同じ TOML file 内で別 key として分ける案を維持するが、この work unit の初期 schema には入れない。
- active reconnect の削除境界は、設定ファイル上の explicit address または daemon-managed learned address の除去とする。起動時環境変数による reset は採用しない。
- 設定ファイル format は実装前に確定する。現時点の第一候補は TOML である。理由は、table / scalar / array の範囲で十分で、YAML より暗黙型変換や alias / anchor の複雑さが少なく、daemon 設定としての可読性と可搬性を両立しやすいためである。
- 同一ファイル更新を採るため、TOML parser は read だけでなく serializer / round-trip update の性質を確認する。特に comment、key order、unknown key、明示設定 table を壊さず `[active_reconnect.learned]` だけを書けるかを採用条件に含める。
- TOML library は `toml11` を採用し、`vendor/toml11` の Git submodule として `v4.4.0` に pin する。理由は BTstack と依存管理方針を揃え、configure 時 network 依存の `FetchContent` を避けるためである。
- `toml11` は `swbt_toml11` interface target で system include として扱う。vendor header 由来の warning を自前警告設定の対象にしない。
- 初期 parse は TOML v1.0 として扱う。TOML v1.1 拡張や toml11 extension は、この work unit の明示設計なしには使わない。
- C++ 化は全面移行ではなく、設定ファイル parser / writer の小さな implementation island として検討する。daemon core、BTstack bridge、public C ABI、tests の大半は C11 のまま維持する。
- C++ island を採る場合は `project(... LANGUAGES C CXX)`、C++ standard、Windows MinGW cross build、clang-tidy 対象、sanitizer、third-party notice を local_071 の acceptance に含める。
- C++ dependency を選ぶ場合でも、`swbt/daemon/config.h` は C++ 型、例外、STL container を公開しない。C 側からは status code と POD input/output だけを見る。
- YAML は人間向け文書としては強いが、daemon 設定では parser dependency、暗黙型、複雑な構文 surface が重い。採用する場合は unknown key、型解釈、コメント保持、license impact を TOML より強く確認する。
- 新規 dependency を増やす場合は C build / cross build / license impact を確認する。依存を増やさないなら、限定 key-value format を採用し、escape や nested structure を後回しにする。
- `toml11` は MIT License であり、license impact は `THIRD_PARTY_NOTICES.md` に記録する。BTstack の非 MIT license 境界とは別に扱う。

## 8. 対象ファイル

- `apps/swbt-daemon/main.c`
- `.gitmodules`
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
- `THIRD_PARTY_NOTICES.md`
- `docs/status.md`
- `vendor/README.md`
- `vendor/toml11`
- `spec/operations/windows-native-preflight.md`
- `spec/operations/windows-hardware-bringup-sequence.md`
- `work-units/wip/local_071/DAEMON_CONFIG_FILE_OVERRIDE_LAYER.md`
- `work-units/wip/local_073/DAEMON_CLI_LAUNCH_MODE.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-skipped | missing config file uses built-in daemon defaults and preserves current no-op startup behavior | regression | unit | no |
| refactor-skipped | valid config file scalar values are applied before environment overrides | new | unit | no |
| refactor-skipped | config file `ipc.host` can be applied without dangling pointer ownership | new | unit | no |
| refactor-skipped | environment override wins over config file for the same runtime key | new | unit | no |
| refactor-skipped | invalid config file value fails without partially mutating existing daemon config | edge | unit | no |
| refactor-skipped | invalid environment override still fails after a valid config file without partially mutating existing daemon config | edge | unit | no |
| refactor-skipped | unknown config file key is rejected before applying any config file value | edge | unit | no |
| refactor-skipped | C++ config-file implementation island builds behind a C config boundary and passes debug, tidy, ASan, and Windows cross build | characterization | build/unit | no |
| refactor-skipped | TOML dependency is accepted only behind a C config boundary and passes debug, tidy, ASan, and Windows cross build | characterization | build/unit | no |
| deferred | backend launch mode, noop selection, hardware approval env removal, and diagnostic output paths move to CLI launch mode work | behavior | design/integration | no |
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

TDD status:
- source: 同一 TOML file 更新を前提にするなら、TOML parser / serializer を C++ implementation island に導入する必要がある。依存は BTstack と同じく `vendor/*` の Git submodule として pin する。
- use case: C caller は `swbt_daemon_config_apply_file()` を呼ぶだけで、実装側は `toml11` を使って TOML file を parse できる。空の TOML file は有効な設定ファイルとして扱い、built-in default を変更しない。
- item: TOML dependency is accepted only behind a C config boundary and passes debug, tidy, ASan, and Windows cross build。
- state: refactor-skipped
- commands:
  - red: `just build-debug`, `just test-debug`
  - green: `just build-debug`, `just test-debug`
  - verification: `just format-check`, `just tidy`, `just asan`, `just windows-cross`
- notes: red は空の TOML file を `SWBT_DAEMON_CONFIG_FILE_OK` として扱う test を追加し、既存実装が既存 file を常に parse error にしていたため `daemon_config_file_test` だけ失敗した。green では `toml11` を `vendor/toml11` submodule として `v4.4.0` / `be08ba2be2a964edcdb3d3e3ea8d100abc26f286` に pin し、`swbt_toml11` target で system include として `swbt/daemon/config_file.cpp` からだけ参照した。`toml::parse(..., toml::spec::v(1, 0, 0))` を使い、C++ exception は C boundary 外へ出さず parse error に変換する。TOML key の適用、unknown key policy、serializer / round-trip update は後続 item に残す。

Refactor status:
- decision: refactor-skipped
- change: vendor header warning を自前 warning と分離するため、toml11 upstream CMake target の直接 link ではなく `swbt_toml11` interface target を使う。
- unchanged behavior: optional missing file の no-op success、env override、hardware approval、diagnostic path、daemon main の起動順序は変更していない。
- verification: `just build-debug`, `just test-debug`, `just format-check`, `just tidy`, `just asan`, `just windows-cross`
- notes: TOML dependency は `swbt/daemon/config_file.cpp` の implementation detail に閉じる。`swbt/daemon/config.h` には TOML 型、STL 型、C++ exception を出さない。

TDD status:
- source: 設定ファイル schema の初期対象は `report`、`ipc`、`device.profile` であり、backend 起動モード、実機承認、診断出力 path は `local_073` に切り出した。
- use case: TOML file に指定した `report.period_us`、`ipc.port`、`ipc.backlog`、`ipc.heartbeat_timeout_ms`、`device.profile` が `swbt_daemon_config_t` に反映され、未指定 key は built-in default を維持する。
- item: valid config file scalar values are applied before environment overrides。
- state: refactor-skipped
- commands:
  - red: `just build-debug`; `$env:CTEST_ARGS='-R daemon_config_file_test'; just test-debug`; `just test-debug`
  - green: `just build-debug`; `just test-debug`
- notes: red は valid TOML test を追加し、既存実装が TOML を parse するだけで値を適用しないため `daemon_config_file_test` だけ失敗した。targeted `CTEST_ARGS` 実行は Dev Container 起動前の `docker ps` error になったため、full `just test-debug` で red / green を確認した。green では `report` / `ipc` / `device` table の既知 scalar key だけを型・範囲検証し、一時 `next` config に適用してから commit する。`ipc.host` は `const char *` の所有権を決める必要があるため、別 item に分けた。

Refactor status:
- decision: refactor-skipped
- change: 今回の cycle で追加した TOML helper は、known table lookup、integer range validation、device profile application に分かれており、次 item の `ipc.host` ownership を始める前に追加抽象化しない。
- unchanged behavior: optional missing file、empty TOML file、既存 env override、daemon main の起動順序は変更していない。
- verification: `just build-debug`, `just test-debug`
- notes: test helper は失敗時に label と actual / expected を stderr へ出すようにした。これは red 原因の確認と今後の regression 診断に効くが、daemon runtime behavior は変えない。

TDD status:
- source: `ipc.host` は TOML の string value であり、parse tree や temporary string への pointer を `swbt_daemon_config_t` に残すと、config copy や後続 apply で dangling / aliasing を起こし得る。
- use case: TOML file に指定した `ipc.host` が config に反映され、`swbt_daemon_config_t` を copy した後に元 config へ別の host を適用しても、copy 側の host 値は変わらない。
- item: config file `ipc.host` can be applied without dangling pointer ownership。
- state: refactor-skipped
- commands:
  - red: `just build-debug`; `just test-debug`
  - green: `just build-debug`; `just test-debug`
- notes: red は TOML `ipc.host` を適用しても default `127.0.0.1` のままで `daemon_config_file_test` だけ失敗した。green では `swbt_daemon_config_t` の `ipc_host` を owned char array にし、`swbt_daemon_config_set_ipc_host()` で env / TOML の両方を copy する。array copy により `swbt_daemon_config_t copied = config` 後も host 値は独立する。host 長は既存 `SWBT_DAEMON_IPC_ENDPOINT_HOST_SIZE` と同じ 16 byte に揃えた。

Refactor status:
- decision: refactor-skipped
- change: `ipc.host` ownership は behavior change 本体であり、green 後に追加の構造変更は行わない。
- unchanged behavior: default host、env override の空文字 no-op、IPC runner の host pointer surface、TOML scalar apply 済みの数値 key は維持する。
- verification: `just build-debug`, `just test-debug`
- notes: 既存 test の直接 pointer assignment は `swbt_daemon_config_set_ipc_host()` 経由に置き換えた。

TDD status:
- source: unknown key を黙って無視すると、設定 typo が実機設定として反映されたと operator が誤解し得る。
- use case: TOML file に未知 key が含まれる場合、既知 key が同じ file に含まれていても `SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE` で失敗し、既存 config を部分更新しない。
- item: unknown config file key is rejected before applying any config file value。
- state: refactor-skipped
- commands:
  - red: `just build-debug`; `just test-debug`
  - green: `just build-debug`; `just test-debug`
- notes: red は `[report] unexpected = true` を含む TOML file で、既存実装が unknown key を無視して `report.period_us` まで適用したため `daemon_config_file_test` だけ失敗した。green では root section を `report` / `ipc` / `device` に限定し、各 table の key も明示 allowlist で拒否する。

Refactor status:
- decision: refactor-skipped
- change: allowlist helper は root section と table key の 2 段だけに閉じ、schema が増えるまでは追加 abstraction を入れない。
- unchanged behavior: missing optional file、empty TOML、known scalar value apply、`ipc.host` ownership、env override は維持する。
- verification: `just build-debug`, `just test-debug`
- notes: unknown key policy は reject で固定した。将来 active reconnect key を追加する場合は、この allowlist と Test List を更新する。

TDD status:
- source: 設定ファイルは基本値であり、環境変数は一時 override として最後に適用する。
- use case: TOML file と環境変数が同じ runtime key を指定した場合、最終 daemon config は環境変数側の値を採用する。
- item: environment override wins over config file for the same runtime key。
- state: refactor-skipped
- commands:
  - characterization: `just build-debug`; `just test-debug`
- notes: 追加 test は現行実装で通った。`swbt_daemon_config_apply_file()` で設定ファイル値を入れた後、`swbt_daemon_config_apply_env()` を呼ぶ順序で `report.period_us`、`ipc.host`、`ipc.port`、`ipc.backlog`、`ipc.heartbeat_timeout_ms` が環境変数値に置き換わる。

Refactor status:
- decision: refactor-skipped
- change: test だけを追加した。合成順序は既存 public boundary の `apply_file -> apply_env` で表現できるため、新しい helper は追加しない。
- unchanged behavior: 設定ファイル単独の適用、unknown key reject、env validation の rollback は維持する。
- verification: `just build-debug`, `just test-debug`
- notes: CLI 起動時引数や config path discovery はこの cycle に混ぜない。

TDD status:
- source: 設定ファイルに不正値がある場合、daemon は startup config failure にし、途中まで読めた値を config に反映しない。
- use case: TOML file に有効な runtime key と不正な `device.profile` が混在する場合、`SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE` で失敗し、既存 config を維持する。
- item: invalid config file value fails without partially mutating existing daemon config。
- state: refactor-skipped
- commands:
  - characterization: `just build-debug`; `just test-debug`
- notes: 追加 test は現行実装で通った。`swbt_daemon_config_apply_file()` は `next = *config` に設定ファイル値を適用し、すべての validation が成功した場合だけ `*config = next` で commit する。

Refactor status:
- decision: refactor-skipped
- change: test だけを追加した。rollback 用の新しい abstraction は追加しない。
- unchanged behavior: valid TOML、unknown key reject、env override precedence は維持する。
- verification: `just build-debug`, `just test-debug`
- notes: 不正値として `device.profile = "unknown"` を使い、先行する `report` / `ipc` 値が部分適用されないことを確認した。

TDD status:
- source: 環境変数は設定ファイルより強い一時 override だが、不正な override が設定ファイル由来の有効 config を壊してはならない。
- use case: 有効な TOML file を適用した後、`report_period_us=0` を含む環境変数 override を適用すると `false` で失敗し、TOML file 適用後の config を維持する。
- item: invalid environment override still fails after a valid config file without partially mutating existing daemon config。
- state: refactor-skipped
- commands:
  - characterization: `just build-debug`; `just test-debug`
- notes: 追加 test は現行実装で通った。`swbt_daemon_config_apply_env()` は `next = *config` に override を適用し、最後の validation が成功した場合だけ `*config = next` で commit する。

Refactor status:
- decision: refactor-skipped
- change: test helper `expect_runtime_config_eq()` を追加した。production code は変更しない。
- unchanged behavior: 設定ファイル値の適用、env precedence、不正 TOML rollback は維持する。
- verification: `just build-debug`, `just test-debug`
- notes: 不正 env は `report_period_us=0` とし、他の env key が途中で parse されても commit されないことを確認した。

## 11. 実機実行条件

この work unit の設定合成、validation、override precedence の実装では実機は不要である。

理由: Bluetooth adapter open、Switch pairing、HID advertising、report loop を実行しない。backend 起動モード、実機承認、診断出力 path の変更は `local_073` で扱う。

実機が必要になるのは、設定ファイル移行と同時に production backend の既定化、report period 既定値変更、device info payload 変更、active reconnect 実装を行う場合である。その場合は別 work unit または `local_072` の実機実行条件に従う。

## 12. 先送り事項

- 観測: release-stable な設定ファイル path と discovery policy は OS ごとの運用方針を含む。
  先送り理由: 初期実装では明示 path から始める方が検証しやすく、installer / service manager がない段階で OS path を固定すると後で migration が必要になる。
  次の置き場: `spec/operations/` の設定ファイル運用 spec、または後続 work unit。
- 観測: CLI subcommand による `show-config` / `validate-config` は有用である。active reconnect の保存状態を持つ場合は、削除 command も候補になる。
  先送り理由: この work unit は daemon 起動設定の合成 layer を先に固定する。CLI 管理 surface は scope が広い。
  次の置き場: 後続 work unit。
- 観測: backend selection、noop 明示指定、hardware approval env の削除、診断出力 path は、設定ファイルではなく daemon 起動時引数として扱う方がよい。
  先送り理由: これらは永続設定ではなく「今回の起動でどの経路を使い、どこへ証跡を書くか」の指定であり、CLI parser と一緒に設計する必要がある。
  次の置き場: `work-units/wip/local_073/DAEMON_CLI_LAUNCH_MODE.md`。

## 13. チェックリスト

- [x] source を user request、`local_045`、`local_065`、`local_072`、`docs/status.md` から特定した。
- [x] use case を config file base + environment override として定義した。
- [ ] config file format と unknown key policy を決めた。
- [x] red test または characterization test を追加した。
- [ ] config file layer を実装した。
- [ ] environment override precedence を test で固定した。
- [ ] docs / status を更新した。
- [ ] 検証結果または未実行理由を記録した。
