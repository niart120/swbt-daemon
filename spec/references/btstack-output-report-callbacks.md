# BTstack Output Report Callbacks Source Audit

## 1. 状態

recorded。

この reference は BTstack Classic HID Device の DATA / SET_REPORT callback を
`swbt_btstack_output_report_handler_handle` へ接続する根拠監査である。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| BTstack submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | `vendor/btstack` |
| BTstack HID Device header | same as submodule | `vendor/btstack/src/classic/hid_device.h` |
| BTstack HID Device implementation | same as submodule | `vendor/btstack/src/classic/hid_device.c` |
| BTstack HID shared definitions | same as submodule | `vendor/btstack/src/btstack_hid.h` |
| swbt parser bridge reference | current swbt implementation | `spec/references/btstack-output-report-parser-bridge.md` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| HID report type output | `HID_REPORT_TYPE_OUTPUT = 2` | source fact | `vendor/btstack/src/btstack_hid.h:100-104` | stable at pinned commit |
| report data callback registration | `hid_device_register_report_data_callback` | source fact | `vendor/btstack/src/classic/hid_device.h:112-116`; `vendor/btstack/src/classic/hid_device.c:900-904` | stable at pinned commit |
| report data callback shape | `callback(hid_cid, report_type, report_id, report_size, report)` | source fact | `vendor/btstack/src/classic/hid_device.h:112-116` | report ID separated |
| DATA call path | DATA packets extract report type, optional report ID, then pass payload after report ID | source fact | `vendor/btstack/src/classic/hid_device.c:620-639` | handler must rejoin report ID |
| SET_REPORT callback registration | `hid_device_register_set_report_callback` | source fact | `vendor/btstack/src/classic/hid_device.h:106-110`; `vendor/btstack/src/classic/hid_device.c:893-898` | stable at pinned commit |
| SET_REPORT call path | callback receives `report_size` and `report` without a separated report ID argument | source fact | `vendor/btstack/src/classic/hid_device.c:527-555` | handler receives full payload with `report_id = 0` |
| callback context | no user context in BTstack callback signatures | source fact | headers above | swbt uses a single active handler |

## 4. 実装判断

- DATA callback は BTstack が分離した `report_id` をそのまま handler に渡す。
- SET_REPORT callback は separated report ID を持たないため、handler に `report_id = 0` を渡し、payload を full report として扱う。
- BTstack callback signature には user context がないため、production registration は単一 active handler を保持する。
- `unregister` は active handler を解除し、BTstack 側へ `NULL` callback を登録する。BTstack は `NULL` を dummy callback に置き換える。

## 5. 未解決事項

- 実機 Switch が DATA と SET_REPORT のどちらを使うかは未検証である。
- output report を受け取った後の subcommand reply send queue はこの reference では扱わない。
- 実 BTstack run loop 上の callback thread timing は未検証である。
