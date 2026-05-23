# PX1 UI Interaction Completion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restore the generated Power-Z-style product UI as real 160x80 firmware screens, and connect navigation, PD/QC trigger flows, PDO display, CC, cable, settings, and verification.

**Architecture:** Keep the existing `BSP -> Service -> App -> UI` shape. Extend `ui_model` to a 9-page product model with browse/action modes, route pages through `ui_pages`, and keep hardware actions in `app_trigger_control` rather than the renderer.

**Tech Stack:** CH32L103K8U6 FreeRTOS C firmware, ST7735 line-buffer renderer, host-side C tests, MRS-generated makefile.

---

### Task 1: Lock the product page model with tests

**Files:**
- Modify: `PowerXCode/FreeRTOS/Tests/test_ui_model.c`
- Modify: `PowerXCode/FreeRTOS/Tests/test_app_ui_navigation.c`
- Modify: `PowerXCode/FreeRTOS/Tests/test_ui_pages.c`

- [x] Add tests requiring page order `MAIN -> SCOPE -> PROTOCOL -> TRIGGER -> PDO -> QC -> CC -> CABLE -> SETTINGS -> MAIN`.
- [x] Add tests requiring browse mode to switch pages with BTN1/BTN3.
- [x] Add tests requiring BTN2 to enter action mode on actionable pages and confirm trigger/QC/PDO actions.
- [x] Add tests requiring `ui_pages_draw()` to route SCOPE/PDO/QC/CC/CABLE to dedicated renderers.

### Task 2: Implement UI model and navigation

**Files:**
- Modify: `PowerXCode/FreeRTOS/UI/include/ui_model.h`
- Modify: `PowerXCode/FreeRTOS/UI/ui_model.c`
- Modify: `PowerXCode/FreeRTOS/App/app_ui_navigation.c`
- Modify: `PowerXCode/FreeRTOS/App/app_trigger_control.c`
- Modify: `PowerXCode/FreeRTOS/App/include/app_trigger_control.h`

- [x] Extend page enum and state fields.
- [x] Implement browse/action mode helpers.
- [x] Keep BTN2 long from forcing HOME; use it to enter/exit settings action mode.
- [x] Add PD-only and QC-only trigger apply helpers.
- [x] Add PDO page 20mV fine target stepping for PPS requests.

### Task 3: Implement the 8 firmware screens

**Files:**
- Modify: `PowerXCode/FreeRTOS/UI/include/ui_renderer.h`
- Modify: `PowerXCode/FreeRTOS/UI/ui_pages.c`
- Modify: `PowerXCode/FreeRTOS/UI/ui_renderer.c`

- [x] Redesign HOME with large meter values and status strip.
- [x] Add SCOPE using real snapshot min/max/ripple metrics.
- [x] Add PROTOCOL status matrix.
- [x] Add TRIGGER action-mode preset rail.
- [x] Add PDO list from `source_fixed_mv/ma`.
- [x] Add QC page from `dp_mv/dm_mv/request_state` and selected QC target.
- [x] Add distinct CC and CABLE pages with CC status plus E-MARK/current data.
- [x] Add SETTINGS page with brightness/rotate/TRIG manual-auto fields. USB CDC/debug setting is intentionally removed.
- [x] Guard Type-C/PD CC-attached snapshots from passive DP/DM `OTHER` fallback, so protocol UI can distinguish CC attached from real legacy/QC detection.

### Task 4: Verify and flash

**Files:**
- Use existing `PowerXCode/FreeRTOS/Tests/*.c`
- Use `PowerXCode/FreeRTOS/obj/makefile`

- [x] Run host tests.
- [x] Render 160x80 UI atlas to `artifacts/ui-preview/current-ui-atlas.png` and 4x preview.
- [x] Run cross build in `PowerXCode/FreeRTOS/obj`.
- [ ] Flash `PowerXCode/FreeRTOS/obj/FreeRTOS.bin` with `wchisp` after the board is in ISP mode. Current attempt is blocked because no `4348:55e0` / `1a86:55e0` WCH ISP USB device is visible.
- [ ] Confirm on hardware that pages switch, action mode edits target values, PD/QC state is visible, and UI does not freeze.
