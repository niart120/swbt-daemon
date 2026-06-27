# Control Runtime Boundary Implementation

## 1. 概要

`swbt/control` と `swbt/runtime` を新設し、daemon IPC と将来の public C ABI が `swbt_app_t` と `daemon_host` を直接操作し続ける状態を解消する。

この work unit は、リアーキテクチャで得た単一の状態所有、一方向依存、旧経路を残さない方針を維持する。`swbt/runtime` は旧 `swbt_daemon_runtime_t` の復活ではなく、IPC を含まない runtime resource lifecycle と runtime-owned state を持つ層に限定する。`swbt/control` は operation semantics を集約するが、app-owned authoritative state を複製しない。

この work unit の完了条件は設計方針の記録ではない。`swbt/control` と `swbt/runtime` の最小実装を追加し、既存の daemon / IPC 経路がその境界を通り、build と unit tests で検証済みになることを完了条件にする。

初期実装は pairing / connect / disconnect の実機 link operation まで広げない。まず、controller state operation、neutral fail-safe、status 合成、IPC を含まない runtime host の責務境界を作る。

## 2. 起点 / ユースケース

source:

- user request, 2026-06-27: `swbt/control`、`swbt/runtime` の実装を新規 work unit として立ち上げ、設計と実装をそこで進めたい。
- `work-units/complete/local_081/APP_SNAPSHOT_VIEW_DECOMPOSITION.md`: app snapshot 分割後の先送り事項として、status 合成は `swbt/control` / `swbt/runtime` の境界整理と合わせて扱うと記録した。
- shared design note, 2026-06-27: https://chatgpt.com/share/6a3fc217-d5ec-83ee-b68c-1b730f0721c5
  - 要点: public C ABI と JSON Lines IPC は同じ `swbt/control` の意味論を使い、`swbt/runtime` は IPC を含まない実機 runtime として切り出す。`daemon_host` は runtime host と IPC runner の利用者へ下げる。
- `swbt/ipc/ipc_adapter.c`: JSON Lines command は現在 `swbt_app_acquire`、`swbt_app_revoke`、`swbt_app_set_state`、`swbt_app_read_status` を直接呼ぶ。
- `swbt/daemon/host.*`: daemon host は IPC start / stop、HID registration、output handler、report timer、neutral shutdown を 1 つの backend contract に束ねている。
- `api/swbt.h`: public C ABI は `swbt_version_string()` だけで、operation API がまだない。
- `spec/architecture/daemon-architecture-cutover.md`: adapter は `swbt_app_t` の field を直接参照しない方針を持つが、control / runtime の現行実装はまだない。

use case:

- actor: daemon IPC runner、future public C ABI caller、daemon process lifecycle。
- 入力または状態:
  - IPC client からの `acquire`、`release`、`set_state`、`get_status`。
  - future public C ABI からの `open`、`close`、`submit_state`、`submit_neutral`、`get_status`。
  - daemon startup / shutdown。
- 期待する観測結果:
  - IPC command path が `swbt/control` の operation API を通る。
  - public C ABI の operation API が `swbt/control` を通り、`swbt_ipc` に link しない。
  - daemon host が IPC を含まない `swbt/runtime` を起動し、IPC runner は daemon 側の利用者になる。
  - report scheduler は app controller state を runtime 経由または runtime-owned callback 経由で読むが、IPC status 合成とは結合しない。
  - `swbt/control` status は app-owned status と runtime status を合成する。
- 制約:
  - IPC JSON wire format は変更しない。
  - Switch-facing report bytes、report period、BTstack source selection は変更しない。
  - 実機 link operation は新規公開しても、実機接続の成功をこの work unit の完了条件にしない。

source から use case への変換:

`local_081` で app snapshot を用途別 read API に分けたが、IPC と future public C ABI がそれぞれ app を直接操作すると owner、sequence、neutral、status 合成の意味論が二重化する。次は app 直接操作を `swbt/control` に集約し、daemon process 固有の IPC lifecycle と、IPC を含まない runtime lifecycle を分ける必要がある。

## 3. 対象範囲

- `swbt/control/` を追加する。
  - `swbt_control_t` または同等の opaque/control object を定義する。
  - `swbt_control_acquire_client` / `release_client` / `submit_client_state` のような IPC client 用 operation を定義する。
  - `swbt_control_submit_state` / `submit_neutral` のような direct API 用 operation を定義する。
  - direct API 用の synthetic client id と sequence allocator は control 側へ閉じ込める。ただし、controller ownership と latest accepted sequence の authoritative state は app に残す。
  - `swbt_control_get_status` で app-owned status と runtime status を合成する。
- `swbt/runtime/` を追加する。
  - IPC start / stop を含まない runtime host contract を定義する。
  - runtime host は app lifetime を所有しない。composition root が所有する app への参照を受け、HID registration、output handler、report timer、neutral shutdown の runtime resource lifecycle を扱う。
  - runtime host は initialized/running、HID registration、output handler registration、report timer running、active HID cid/link summary、pending neutral send、last runtime error のような runtime-owned state を持つ。
  - runtime status は daemon lifecycle ではなく runtime resource / link / hardware の要約として扱う。
  - noop runtime backend を用意し、unit test で start/stop/order を検証する。
- daemon host を薄くする。
  - daemon host は runtime host と IPC runner の構成を持つ。
  - IPC start / stop は daemon process 側の責務として残す。
  - production backend の既存 port を runtime backend へ渡す adapter を用意する。
- IPC adapter / runner を `swbt/control` 経由に移す。
  - IPC wire format を変えずに `acquire`、`release`、`set_state`、`get_status` を control operation へ委譲する。
- public C ABI の最小 operation を追加する。
  - `swbt_open` / `swbt_close`。
  - `swbt_submit_state` / `swbt_submit_neutral`。
  - `swbt_get_status`。
  - 初期実装は noop/runtime-unavailable status を返してよいが、`swbt_ipc` へ link しない。
- CMake targets を追加または更新する。
  - `swbt_control`。
  - `swbt_runtime`。
  - shared target `swbt` は `swbt_control` を通る。
- tests を追加または更新する。
  - control operation tests。
  - runtime host tests。
  - IPC adapter / runner tests。
  - public C ABI smoke test。
- `spec/architecture/daemon-architecture-cutover.md` または新規 architecture spec を更新する。
- work unit record を実装完了ベースで更新し、実行した検証を記録する。

## 4. 対象外

- Switch-facing report bytes の変更。
- report period の変更。
- BTstack source selection の変更。
- 実機 pairing 成功、Switch 接続成功、HID advertising 成功の検証。
- 複数 controller 同時接続。
- `tap`、`duration_ms`、`sequence`、`at_ms` を daemon protocol または public C ABI に追加すること。
- public C ABI の binary release。
- `swbt_state_t` の field 分割。

## 5. 関連 spec / docs

- `work-units/complete/local_081/APP_SNAPSHOT_VIEW_DECOMPOSITION.md`
- `work-units/wip/local_080/DEVICE_API_PRODUCTION_PATH.md`
- `spec/architecture/daemon-architecture-cutover.md`
- `spec/protocols/daemon-ipc-v1.md`
- `spec/operations/work-unit-spec-tdd-flow.md`
- `api/swbt.h`
- external input: https://chatgpt.com/share/6a3fc217-d5ec-83ee-b68c-1b730f0721c5

## 6. 根拠監査

not applicable for the initial control/runtime boundary.

この work unit は、初期実装では Switch HID report bytes、subcommand bytes、SPI address、rumble packet、BTstack source selection、report period、WinUSB/libusb facts を追加または変更しない。production backend の port wiring を移動する場合も、既存の port と既存 behavior の所有境界変更として扱う。

BTstack source selection、report period、HID descriptor、pairing sequence、実機 link state 名を追加または変更する場合は、その時点で `source-audit` と `hardware-harness` の要否を再判断する。

## 7. 設計メモ

### 7.1 初期モジュール境界

```text
api/swbt.h
  public C ABI
  swbt/control への薄い wrapper

swbt/control
  operation semantics
  owner / sequence / neutral / status synthesis
  app と runtime を直接呼ぶ唯一の high-level layer
  app-owned authoritative state は持たない
  direct API 用 synthetic client id / sequence allocator は持ってよい

swbt/runtime
  IPC を含まない runtime host
  HID registration / output handler / report timer / neutral shutdown
  initialized / running / HID registration / report timer / link summary / pending neutral send
  app lifetime は所有しない

swbt/daemon
  process lifecycle
  IPC runner
  runtime host の利用者

swbt/application
  owner lease / latest controller state / rumble / app metrics
```

### 7.2 初期 API 候補

内部 control API:

```c
typedef struct swbt_control swbt_control_t;

typedef struct {
    swbt_app_t *app;
    swbt_runtime_host_t *runtime;
} swbt_control_config_t;

swbt_control_result_t swbt_control_init(swbt_control_t *control,
                                        const swbt_control_config_t *config);
swbt_control_result_t swbt_control_acquire_client(swbt_control_t *control, uint32_t client_id);
swbt_control_result_t swbt_control_release_client(swbt_control_t *control, uint32_t client_id);
swbt_control_result_t swbt_control_submit_client_state(swbt_control_t *control,
                                                       uint32_t client_id,
                                                       const swbt_state_t *state,
                                                       uint64_t sequence);
swbt_control_result_t swbt_control_submit_state(swbt_control_t *control,
                                                const swbt_state_t *state);
swbt_control_result_t swbt_control_submit_neutral(swbt_control_t *control);
swbt_control_result_t swbt_control_get_status(const swbt_control_t *control,
                                              swbt_control_status_t *out_status);
```

runtime API:

```c
typedef struct swbt_runtime_host swbt_runtime_host_t;

swbt_runtime_host_result_t
swbt_runtime_host_init(swbt_runtime_host_t *runtime,
                       const swbt_runtime_host_config_t *config,
                       const swbt_runtime_host_backend_t *backend,
                       void *backend_context);
swbt_runtime_host_result_t swbt_runtime_host_start(swbt_runtime_host_t *runtime);
swbt_runtime_host_result_t swbt_runtime_host_send_neutral_now(swbt_runtime_host_t *runtime);
void swbt_runtime_host_stop(swbt_runtime_host_t *runtime);
```

public C ABI:

```c
typedef struct swbt swbt_t;

swbt_result_t swbt_open(const swbt_open_options_t *options, swbt_t **out_swbt);
void swbt_close(swbt_t *swbt);
swbt_result_t swbt_submit_state(swbt_t *swbt, const swbt_state_t *state);
swbt_result_t swbt_submit_neutral(swbt_t *swbt);
swbt_result_t swbt_get_status(swbt_t *swbt, swbt_status_t *out_status);
```

### 7.3 実装順序

1. `swbt/runtime` に IPC を含まない host を切り出し、noop backend tests を通す。
2. `swbt/daemon/host` を runtime host + IPC runner の利用者へ薄くする。
3. `swbt/control` を追加し、app と runtime を束ねた operation tests を通す。
4. IPC adapter / runner を control 経由へ移す。wire format は変えない。
5. public C ABI の minimal operation wrapper を追加する。shared target `swbt` は `swbt_ipc` に link しない。
6. architecture spec と work unit record を更新する。

### 7.4 事前妥当性評価

Tidy status:

- classification: behavior change。
- decision: do not tidy。
- reason: `swbt/control`、`swbt/runtime`、public C ABI の追加は内部構造だけでなく public C ABI、target topology、daemon composition に触れる設計変更である。単なる整理として実装せず、TDD item と architecture spec 更新を伴う work unit として扱う。
- verification: この事前評価では code 実装と build / test は未実行。実装後に targeted CTests、`just debug`、必要に応じて `just windows-cross` で確認する。

妥当と判断する点:

- `swbt_app_t` の owner、latest accepted sequence、controller state、rumble、metrics を唯一の app-owned state として残す限り、`local_050` 以降の単一状態所有を損なわない。
- `swbt/runtime` が runtime resource state を持つことは妥当である。問題にするべきなのは、runtime が app lifetime、controller ownership、latest accepted sequence、controller state の authoritative owner になることである。
- IPC と public C ABI が同じ `swbt/control` operation semantics を通るなら、owner、sequence、neutral、status 合成の二重実装を避けられる。
- `swbt/runtime` から IPC start / stop を外すなら、daemon process lifecycle と runtime resource lifecycle を分けられる。
- shared target `swbt` が `swbt_ipc` や `swbt_daemon_host` に link しないことを build test で固定すれば、public C ABI が daemon IPC 実装へ癒着しない。

実装前に守る制約:

- `swbt/runtime` は runtime resource state を持つ。app を生成、破棄、公開せず、composition root が app lifetime を所有し、runtime は state provider / output event target として参照する。
- `swbt/control` は controller owner、latest accepted sequence、neutral policy の authoritative state を複製しない。必要な direct API 用 synthetic client id / sequence allocator は operation 入力の補助に閉じ、最終判断は app API へ委譲する。
- `swbt/control` は IPC JSON codec、socket、BTstack vendor header、daemon CLI を include しない。
- `swbt/runtime` は IPC runner、IPC adapter、JSON codec を include しない。
- public C ABI はこの work unit では thin wrapper / smoke surface に留める。Bluetooth adapter open、HCI power on、pairing、advertising、connect success を public contract として固定しない。
- 旧経路を compatibility layer として残さない。daemon / IPC caller を新経路へ移したら、旧 app-direct call path は同じ work unit 内で削除する。

再評価条件:

- runtime status schema を IPC JSON wire format に露出したくなった場合。
- public C ABI の `swbt_open` が実際の Bluetooth adapter open / listen / connect を意味し始めた場合。
- runtime が app lifetime を持たない形では production backend wiring が過度に複雑になる場合。
- runtime-owned state と app-owned state の境界が、unit test で観測できないほど曖昧になった場合。
- `local_080` の device send path と同時に進めないと production 経路が二重化する場合。

## 8. 対象ファイル

- `api/swbt.h`
- `api/swbt_c_api.c`
- `swbt/control/*`
- `swbt/runtime/*`
- `swbt/application/app.*`
- `swbt/daemon/host.*`
- `swbt/daemon/ipc_runner.*`
- `swbt/daemon/production_backend.*`
- `swbt/ipc/ipc_adapter.*`
- `CMakeLists.txt`
- `tests/*control*`
- `tests/*runtime*`
- `tests/daemon_host_test.c`
- `tests/daemon_ipc_runner_test.c`
- `tests/architecture_journey_test.c`
- `spec/architecture/daemon-architecture-cutover.md`
- `work-units/wip/local_082/CONTROL_RUNTIME_BOUNDARY_IMPLEMENTATION.md`

## 9. TDD Test List（TDD テスト一覧）

| status | item | type | layer | hardware |
|---|---|---|---|---|
| refactor-done | runtime host starts HID/output/report runtime without IPC start callback | new | unit | no |
| refactor-skipped | runtime host owns runtime resource state while leaving app lifetime and app-owned state outside runtime | new | unit/review | no |
| refactor-skipped | runtime host shutdown neutralizes state and stops runtime resources once | regression | unit | no |
| todo | daemon host starts IPC runner as daemon responsibility and delegates HID/report runtime to runtime host | regression | integration | no |
| todo | control submit client state preserves IPC owner and sequence semantics | regression | unit | no |
| todo | control submit state for direct API hides owner id and sequence from caller | new | unit | no |
| todo | control get status combines app-owned status and runtime status without changing IPC JSON status | regression | integration | no |
| todo | IPC adapter delegates acquire/release/set_state/get_status to control instead of app direct calls | regression | integration | no |
| todo | public C ABI exposes open/close/submit_state/submit_neutral/get_status and does not link swbt_ipc | new | unit/build | no |
| todo | CMake include boundary tests recognize swbt_control and swbt_runtime targets | regression | build | no |
| todo | old daemon_host backend no longer requires ipc_start/ipc_stop in runtime backend contract | characterization | review | no |

TDD status:

- source: user request, 2026-06-27。
- use case: `swbt/control` と `swbt/runtime` の実装を新規 work unit として進める。
- last item: runtime host shutdown neutralizes state and stops runtime resources once。
- state: refactor-skipped。
- commands:
  - red: `just build-tests-debug`
    - result: pass.
  - red: `CTEST_ARGS="-R runtime_host_test" just test-debug`
    - result: expected failure. `runtime_host_stop` stopped resources but did not neutralize app controller state.
  - green: `just format`; `just build-tests-debug`; `CTEST_ARGS="-R runtime_host_test" just test-debug`
    - result: pass.
  - refactor: skipped.
    - reason: the shutdown behavior is a single call through app revoke plus existing idempotent resource flags. No separate structure change was useful.
- notes: third cycle added shutdown neutralization through app revoke. Runtime still does not own app state; it requests the app-owned revoke policy.
- next red candidate: daemon host starts IPC runner as daemon responsibility and delegates HID/report runtime to runtime host。

## 10. 検証

一部実行。

実行:

- `just format`
  - result: pass。
- `just format-check`
  - result: pass。
- `just build-tests-debug`
  - result: pass。`swbt_runtime` と `runtime_host_test` を含む debug unit test target build が成功した。
- `CTEST_ARGS="-R runtime_host_test" just test-debug`
  - result: pass。1/1 tests passed。

予定:

- targeted CTests for new runtime/control tests。
- `just debug`。
- 変更範囲が CMake include boundary に触れる場合は relevant CMake tests。
- 実装が shared target `swbt` に触れた後は `just windows-cross` も候補にする。

## 11. 実機実行条件

この work unit の通常範囲では実機検証を実行しない。理由は、初期実装では module boundary、noop runtime、IPC / public API operation semantics を扱い、Bluetooth adapter open、HCI power on、Switch pairing、HID advertising、実機 report loop の成功を完了条件にしないためである。

実機 link operation、pairing、HID advertising、report loop の behavior を追加または変更する場合は、人間の明示承認、`SWBT_RUN_HARDWARE=1`、`SWBT_HARDWARE_APPROVED=1`、専用 USB Bluetooth dongle、`hardware-harness`、`docs/hardware-test-log.md` の更新を必要条件として再定義する。

## 12. 先送り事項

- 観測: pairing / connect / disconnect の user-facing API 名は `connect_start`、`accept_connection_start`、`advertise_start` のどれが適切か未確定である。
  先送り理由: 初期 control/runtime boundary は submit_state / neutral / status と daemon runtime 分離を先に通す。
  次の置き場: link operation API work unit または architecture spec。

- 観測: runtime status の詳細 schema はまだ不安定である。
  先送り理由: IPC JSON wire format はこの work unit で変えない。public C ABI status も minimal shape から始める。
  次の置き場: status schema work unit または protocol / architecture spec。

## 13. チェックリスト

- [x] source と use case を記録した。
- [x] implementation completion を work unit の完了条件にした。
- [x] TDD Test List を作成した。
- [x] 事前妥当性評価を記録した。
- [ ] `swbt/runtime` を実装した。初期 start path は実装済み。shutdown neutral、runtime-owned state status、daemon host delegation は未完了。
- [ ] `swbt/runtime` が runtime-owned state を持ち、app lifetime と app-owned state を所有または公開しないことを検証した。
- [ ] `swbt/control` を実装した。
- [ ] daemon host を runtime host + IPC runner の利用者へ薄くした。
- [ ] IPC adapter / runner を control 経由に移した。
- [ ] public C ABI minimal operation を追加した。
- [ ] CMake target と include boundary を更新した。
- [ ] relevant tests を追加または更新した。
- [ ] 検証コマンドと結果を記録した。
- [ ] 実機未実行理由を維持または実機実行条件を更新した。
- [ ] work unit record を complete へ移した。
