# PX1 Real Device Closure

- Time: 2026-05-19 11:57:02 +0800
- Overall status: `failed`
- Message: PX1 final goal audit failed; inspect the hardware validation report and flash evidence.
- Firmware: `PowerXCode/FreeRTOS/obj/FreeRTOS.bin`
- Firmware SHA256: `beddc4fa8e86da0d21e041a2f90cd17fe1a82c09a3605d64d4eb6ffe5adab149`
- Hardware report: `artifacts/hardware-validation/latest.md`
- Latest flash attempt: `artifacts/flash/latest.md`
- Latest flash attempt SHA256: `beddc4fa8e86da0d21e041a2f90cd17fe1a82c09a3605d64d4eb6ffe5adab149`
- Latest flash attempt status: `flashed`
- ISP preflight report: `artifacts/flash/isp-preflight.md`
- ISP preflight status: `no-isp-device`
- ISP preflight diagnosis: A WCH USB serial device is visible, but no WCH ISP product ID 0x55e0 was found.
- Flash success report: `artifacts/flash/latest-success.md`
- Flash success SHA256: `beddc4fa8e86da0d21e041a2f90cd17fe1a82c09a3605d64d4eb6ffe5adab149`
- Flash success status: `flashed`

## Checklist

| item | status |
| --- | --- |
| no-flash software gate | pass |
| objective coverage audit | software-pass-blocked-hardware |
| WCH ISP flash | pass |
| hardware report preparation | preserved |
| final goal audit | fail |
