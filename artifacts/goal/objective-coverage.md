# PX1 Objective Coverage Audit

- Time: 2026-05-19 13:10:24 +0800
- Mode: software-only
- Firmware: `/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/obj/FreeRTOS.bin`
- UI report: `/Users/hiveton/HivetonCode/HivetonPowerX/artifacts/ui-preview/current-ui-report.md`
- Flash report: `/Users/hiveton/HivetonCode/HivetonPowerX/artifacts/flash/latest-success.md`
- Hardware report: `artifacts/hardware-validation/latest.md`

| requirement | status | evidence |
| --- | --- | --- |
| 160x80 UI and 9 product pages | pass | all current-ui page PNGs are 160x80, listed in current-ui-report.md, and wired to dedicated renderer/page-route tests |
| design-reference visual regression | pass | tools/check_ui_reference_delta.sh passes against 160x80 reference pages |
| product task configuration | pass | measure/protocol/legacy tasks enabled and diagnostic screens disabled |
| product binary integration | pass | required BSP/Service/App/UI symbols are present; debug UI, mock, USB CDC, and USBFS symbols are forbidden |
| real voltage/current/power path | pass | INA226 0x40 with 5mR shunt is locked by board config and host test |
| USB PD detect and trigger | pass | PD Source_Cap default PDO, fixed PDO contract, PPS APDO request, PS_RDY/VBUS verification, UI PDO trigger, and SOP prime E-marker paths are behavior-tested |
| USB QC detect and trigger | pass | QC2/QC3 DP/DM mode, fixed-voltage requests, 200mV QC3 pulse stepping, VBUS confirmation, timeout release, and UI QC trigger paths are behavior-tested |
| protocol arbitration and VBUS fallback | pass | PD/QC arbitration priority, passive-QC hiding, CC detach recovery, and no-protocol VBUS fallback are behavior-tested |
| three-button browse/action interaction | pass | page order/action mode are tested; BTN2/BTN3 use EXTI and BTN1 uses the explicit scan fallback because PB15 shares EXTI15 with PA15 |
| CC and cable/E-marker pages | pass | CC/cable pages exist and SOP prime E-marker request, PD identity publish, and cable VDO summary extraction are behavior-tested |
| settings and readable text | pass | settings brightness/rotation/TRIG behavior, confirm handling, auto-apply mode, backlight driver, LCD rotation driver, and white dim text are covered |
| USB CDC removed / DPDM reserved | pass | product makefiles lack CDC/USBFS sources and board config reserves PA11/PA12 |
| WCH ISP flash | blocked | software-only mode; real success requires /Users/hiveton/HivetonCode/HivetonPowerX/artifacts/flash/latest-success.md with Status: flashed |
| hardware validation | blocked | software-only mode; real completion requires filled hardware report |

- Failed: 0
- Blocked: 2
