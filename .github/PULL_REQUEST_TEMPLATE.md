## Summary

<!-- 1-3 lines describing the purpose of this change. -->

## Related

- Spec:
- Dev journal:
- Issue:
- Source audit:

## Changes

-

## Source Audit

<!-- Required when changing Switch HID protocol, BTstack source selection, report timing, backend behavior, or hardware-facing constants. Use "not applicable" only with a reason. -->

| item | status | evidence |
|---|---|---|
| Switch HID / protocol facts |  |  |
| BTstack source / bridge impact |  |  |
| WinUSB / libusb backend assumptions |  |  |

## Testing

```text
# commands and results
```

## Hardware

- [ ] not applicable
- [ ] not run
- [ ] tested with Nintendo Switch hardware

Reason / evidence:

- Approval scope:
- OS / environment:
- Bluetooth dongle:
- driver:
- backend:
- Switch firmware:
- report period:
- log path:
- cleanup:

## BTstack / License Impact

- [ ] `vendor/btstack` untouched
- [ ] BTstack source selection changed
- [ ] `THIRD_PARTY_NOTICES.md` checked
- [ ] license / notice impact not applicable

Notes:

## Checklist

- [ ] Change scope matches the spec or request
- [ ] Non-goals remained out of scope
- [ ] CMake configure/build/test run or not-run reason documented
- [ ] Sanitizer/cross-build run or not-run reason documented
- [ ] Source audit status documented
- [ ] Hardware status documented
- [ ] Commit prefix matches the motivation
