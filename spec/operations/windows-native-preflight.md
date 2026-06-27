# Windows Native Preflight

## 1. 状態

current。

## 2. 目的

Windows native で `swbt-daemon.exe` を実機へ向けて起動する前に、承認、専用 USB Bluetooth ドングル、WinUSB driver assignment、起動引数、記録先、cleanup の確認項目を固定する。

この spec は preflight 手順であり、Windows native 実行、Switch pairing、HID advertising、periodic report loop、report period の実測成功を記録するものではない。

## 3. 適用範囲

- Windows filesystem 上の repository から、Dev Container ではなく Windows native build artifact を実機に向けて起動する前の確認。
- 専用 USB Bluetooth ドングルと WinUSB driver assignment の記録。
- Switch pairing、HID advertising、periodic input report loop、report period comparison の実行前 gate。
- `docs/hardware-test-log.md` に残す実機記録の必須項目。

次は対象外である。

- 実機コマンドの実行。
- pairing、advertising、report loop、disconnect behavior の成功判定。
- report period の default または fallback value の決定。
- BTstack source selection、WinUSB backend 実装、daemon runtime 実装の変更。

## 4. 決定事項

実機へ触れる操作は、ユーザが承認範囲を明示した場合だけ実行する。承認範囲は少なくとも次のどれを許可するかに分けて記録する。

- Bluetooth アダプターを開く daemon 起動。
- Switch pairing。
- HID advertising。
- periodic input report loop。
- report period comparison。
- cleanup 確認。

current implementation は `swbt-daemon.exe` の引数なし起動で production backend を選ぶ。ただし production backend は `--adapter-location winusb:<location-path>` 未指定では adapter open 前に失敗する。Bluetooth adapter を開かない確認では `--backend noop` を明示する。`SWBT_RUN_HARDWARE=1` と `SWBT_HARDWARE_APPROVED=1` は current implementation の実装上の分岐条件ではない。

エージェントまたは operator が実機向け手順を開始する条件は、ユーザが承認範囲を明示していることである。承認がない場合、Bluetooth adapter open、Switch pairing、HID advertising、report loop、IPC input、cleanup confirmation のいずれにも進まない。

対象アダプターは専用 USB Bluetooth ドングルに限定する。内蔵 Bluetooth と常用ドングルは使わない。対象アダプターが不明な場合は daemon 起動前に停止する。

Windows native では、実行前に次を `docs/hardware-test-log.md` へ記録する。

- OS と host environment。
- USB VID/PID。
- adapter identity。
- adapter location selector。形式は `winusb:<location-path>`。
- driver state。Zadig で WinUSB を割り当てたかを含む。
- backend。想定値は `windows-winusb` である。
- BTstack commit または tag。
- swbt commit または branch。
- Switch firmware。判明している場合だけでよい。
- command または manual procedure。`--adapter-location`、`--config`、`--link-key-db`、`--trace-path`、`--hci-dump-path` など、adapter open 前後の根拠に必要な引数を省略しない。
- cleanup plan。

report period comparison では、比較対象の period を 1 つの結果に混ぜない。`8000 us`、`8333 us`、`15000 us`、`16667 us` を試す場合は、それぞれ独立した記録として残す。これらの値は Phase 5 の比較候補であり、source-audited default または fallback value ではない。

`swbt-daemon.exe` native start checklist は、pairing、advertising、report loop の成功を開始前に主張しない。`--adapter-location winusb:<location-path>` は対象 adapter を絞るための条件であり、実機操作の承認を代替しない。成功、失敗、停止理由、cleanup 結果は実行後の hardware log にだけ記録する。

cleanup では daemon 停止、adapter state、neutral fail-safe の確認結果を記録する。neutral fail-safe を確認できなかった場合も、その状態を成功として扱わず記録する。

## 5. 根拠

`AGENTS.md` と `hardware-harness` skill は、実機コマンド、pairing、HID advertising、report loop を人間の明示承認なしに実行しない方針を定めている。

`work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md` は、production backend の実装上の環境変数分岐を削除し、起動 mode を `--backend production|noop` で扱う current implementation を記録している。実機承認は実装上の分岐ではなく運用 gate として残る。

`work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md` は、Windows WinUSB production run が `--adapter-location winusb:<location-path>` を必要とし、指定された USB location path に一致する device interface だけを open 対象にする software guard を記録している。

`work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md` は Windows native PowerShell から `just` が Dev Container へ委譲できることを記録している。この記録は Windows native daemon 実行や WinUSB runtime behavior を証明しない。

`spec/references/btstack-backend-build-matrix.md` は Windows MinGW cross build が `windows-winusb` backend で link できたことを記録している。この記録は Windows native execution、WinUSB driver assignment、Bluetooth dongle recognition、Switch pairing を証明しない。

`work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md` と `docs/hardware-test-log.md` は、CSR8510 A10、WinUSB、Switch2 firmware `22.1.0` の条件で Windows native 起動、Switch pairing、HID L2CAP open、report period `8000 / 8333 / 15000 / 16667 us` comparison、owner disconnect / heartbeat timeout / shutdown neutral fail-safe を記録している。`work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md` は、環境変数依存監査後の限定 smoke として `8000us` Button A + release を記録している。

この spec は実機手順の gate と記録項目を定める。Switch protocol bytes、BTstack source selection、WinUSB/libusb 実装値、report period default は追加しない。report period 候補は `local_037` で粗い受理確認済みだが、source-audited default または最適値として扱わない。

## 6. 関連 work units

- `work-units/complete/local_016/BTSTACK_BACKEND_BUILD_VERIFICATION.md`
- `work-units/complete/local_027/WINDOWS_NATIVE_PREFLIGHT.md`
- `work-units/complete/local_032/WINDOWS_NATIVE_JUST_DEVCONTAINER.md`
- `work-units/complete/local_037/WINDOWS_HARDWARE_BRINGUP.md`
- `work-units/complete/local_045/CODEBASE_ENV_DEPENDENCY_AUDIT.md`
- `work-units/complete/local_074/DAEMON_LAUNCH_MODE_FLAGS.md`
- `work-units/complete/local_077/ADAPTER_SELECTOR_GUARD.md`

## 7. 未解決事項

- preflight gate としての未解決事項はない。
- 別 adapter / firmware の互換性、長時間安定性、report jitter / latency / drop-rate の厳密測定は、この preflight spec では扱わない。後続 work unit では `work-units/complete/local_039/DAEMON_STATUS_OBSERVABILITY_PROTOCOL.md` の explicit unavailable contract を前提に measured value の扱いを決める。
