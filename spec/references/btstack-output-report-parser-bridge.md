# BTstack Output Report Parser Bridge Source Audit

## 1. 状態

recorded。

この reference は Phase 4 の Output Report parser 接続で使う BTstack Classic HID report callback と report type の根拠監査である。

## 2. 参照元

| source | commit / version | path |
|---|---|---|
| BTstack submodule | `075a0780f0fad7ff67d58ac19f46e8953656a752` | `vendor/btstack` |
| Switch output report parser reference | current swbt implementation | `spec/references/switch-subcommand-core.md` |

## 3. 根拠監査

| 項目 | 値 | 根拠 | source | status |
|---|---:|---|---|---|
| BTstack HID output report type | `HID_REPORT_TYPE_OUTPUT = 2` | source fact | `vendor/btstack/src/btstack_hid.h:100-105` | stable BTstack API at pinned commit |
| HID report data callback signature | `callback(cid, report_type, report_id, report_size, report)` | source fact | `vendor/btstack/src/classic/hid_device.h:112-116` | stable BTstack API at pinned commit |
| report data callback registration | `hid_device_register_report_data_callback` stores callback and uses dummy for NULL | source fact | `vendor/btstack/src/classic/hid_device.c:900-904` | stable BTstack behavior at pinned commit |
| BTstack DATA report parsing | DATA packets extract report type, optionally extract declared report ID, then call report data callback with payload after report ID | source fact | `vendor/btstack/src/classic/hid_device.c:620-639` | bridge must rejoin report ID for swbt parser |
| swbt output report parser input shape | `swbt_switch_parse_output_report` expects output report ID at byte 0 | implementation fact | `swbt/switch/switch_subcommand.c`; `spec/references/switch-subcommand-core.md` | existing implementation contract |

## 4. 未解決事項

- 実機 Switch が DATA と SET_REPORT のどちらで output report を送るかは未検証である。
- `hid_device_register_set_report_callback` の接続はこの work unit では扱わない。
- Unsupported output report の実機 acceptability は未検証である。
