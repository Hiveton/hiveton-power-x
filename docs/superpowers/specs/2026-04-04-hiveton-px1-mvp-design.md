# Hiveton PX1 MVP Design

## Summary

This document defines the first production-oriented firmware stage for the Hiveton PX1 USB detector based on `CH32L103K8U6`, the finished PX1 hardware in `docs/SCH_HivetonPX1_2026-04-03.pdf`, the existing FreeRTOS framework in `PowerXCode/FreeRTOS`, the WCH reference examples in `docs/EVT`, and the ST7735S LCD references in `docs/0.96IPS京东方焊接13pin-ST7735S技术资料`.

The first stage is a `monitor-first MVP`, not a full engineering analyzer. It must boot quickly, show large readable live values, support PD and legacy fast-charge monitoring plus active negotiation, read basic E-Marker information, and fit within the memory limits of `CH32L103K8U6` without a full-screen framebuffer.

## Inputs And Confirmed Constraints

### Hardware facts confirmed from the provided materials

- MCU: `CH32L103K8U6` with built-in `USBPD` hardware.
- Firmware base: `PowerXCode/FreeRTOS`.
- Display: SPI color LCD using `ST7735S` reference code and command tables.
- Keys: `BTN_1`, `BTN_2`, `BTN_3`.
- LCD nets confirmed in schematic text extraction:
  - `LCD_SPI_CS`
  - `LCD_SPI_DC`
  - `LCD_SPI_RST`
  - `LCD_SPI_SCK`
  - `LCD_SPI_MOSI`
  - `LCD_BL_PWM`
- Type-C and protocol nets confirmed:
  - `USB_CC1`, `USB_CC2`
  - `USB_SBU1`, `USB_SBU2`
  - `USB_DP`, `USB_DM`
  - `VBUS_IN`, `VBUS_OUT`
- Additional control net confirmed:
  - `CC1_EXT_RD_CTL`
- MCU PD pins confirmed:
  - `PB6/CC1`
  - `PB7/CC2`

### Product goals confirmed during discussion

- Product style: `similar to Power-Z`, but first stage is not a clone-by-feature-count.
- User experience priority: `quickly read values`, not protocol-lab-first.
- Firmware platform: `FreeRTOS`, not bare metal.
- PC output: `not required` in stage 1.
- Ripple target for stage 1: `trend-level`, not calibrated oscilloscope-grade ripple.
- Protocol target for stage 1: cover `PD + QC + AFC + FCP` as far as practical, with PD as the most robust path.

## Scope

### In scope for stage 1

- Real-time voltage measurement.
- Real-time current measurement.
- Real-time power calculation.
- Min/avg/max style lightweight statistics suitable for UI pages.
- Trend-level ripple estimation.
- USB Type-C attach and orientation detection through `CC1/CC2`.
- Passive PD monitoring.
- Active PD negotiation for selectable fixed PDO targets.
- Basic E-Marker discovery and summary display.
- Legacy fast-charge detection and active negotiation for:
  - `QC2.0`
  - `QC3.0`
  - `AFC`
  - `FCP`
- 3-button UI framework for a small SPI LCD.
- Backlight control.
- Debug print hooks kept for development builds only.

### Explicitly out of scope for stage 1

- Oscilloscope-grade ripple accuracy claims.
- Full raw packet logger with long history retained in RAM.
- Host-side USB/UART data export protocol.
- Filesystem, SD card, or onboard logging storage.
- Full certification-grade compatibility matrix for every charger.
- Full cable database or complete VDO dump browser on the main UI.

## Product Behavior

### Default behavior

- Boot into a `main measurement page`.
- Show large-font `voltage` and `current` immediately.
- Show `power`, `protocol summary`, and `ripple trend` in secondary fields.
- Stay in passive monitoring until the user explicitly enters a negotiation action.

### Interaction goals

- One-handed use with 3 keys.
- Few menu levels.
- All common actions reachable in 1 or 2 operations.
- No screen design that depends on full-screen redraw.

## Recommended Architecture

The stage 1 firmware uses a `layered FreeRTOS architecture` with four persistent application tasks and a minimal shared-state model.

### Layer breakdown

#### 1. BSP layer

Responsible for chip-level and board-level drivers:

- GPIO
- ADC
- DMA
- TIM
- SPI
- PWM for backlight
- USBPD peripheral access
- D+/D- analog control path
- key GPIO scan

This layer should hide register details from upper modules.

#### 2. Service layer

Responsible for board-independent logic built on top of BSP:

- sampled measurement pipeline
- ripple estimation
- PD state machine
- E-Marker parser
- legacy fast-charge protocol engine
- UI view-model generation

#### 3. UI layer

Responsible for:

- widget drawing
- page layout
- page switching
- dirty-region rendering
- line-buffer-based LCD refresh

#### 4. App/controller layer

Responsible for:

- top-level operating mode
- user actions
- task coordination
- fault recovery

## Task Model

Stage 1 keeps the task count intentionally small to protect RAM.

### `measure_task`

Responsibilities:

- Trigger ADC sampling using DMA.
- Build raw sample windows for voltage and current channels.
- Apply calibration coefficients.
- Publish filtered display values and short-window statistics.
- Produce trend-level ripple indicators.

Design notes:

- Use short double-buffer DMA windows.
- Avoid floating-point-heavy math in fast paths.
- All outputs should be published as fixed-point integers such as `mV`, `mA`, `mW`.

### `protocol_task`

Responsibilities:

- Own the PD software state machine after USBPD interrupt events.
- Track attach, detach, orientation, and active CC.
- Parse `Source Capabilities`.
- Submit fixed-PDO requests.
- Track accept/reject/timeout/fallback.
- Execute E-Marker read sequences.

Design notes:

- USBPD interrupt should do the minimum possible work.
- Packet parsing and state transitions should happen in task context.
- The PD state machine is the highest-priority protocol path in the product.

### `legacy_charge_task`

Responsibilities:

- Detect and drive legacy protocols on `D+` / `D-`.
- Enter and leave QC/AFC/FCP negotiation modes.
- Report result summaries for UI and controller logic.

Design notes:

- This task should remain idle unless needed.
- The implementation must be modular per protocol.
- PD and legacy negotiation ownership must never fight for the same session.

### `ui_task`

Responsibilities:

- Scan and debounce keys.
- Translate short/long press actions.
- Maintain the page state machine.
- Render widgets into a small line buffer.
- Push dirty regions to LCD through SPI/DMA.

Design notes:

- UI must never parse PD packets or compute measurements directly.
- UI reads immutable snapshots or compact shared structs.

## RAM Strategy

`CH32L103K8U6` RAM is tight for a color UI product. The design must assume memory is the primary system constraint.

### Required rules

- No full-screen framebuffer.
- No full packet history ring kept in RAM.
- No large general-purpose UI scene graph.
- No protocol-specific oversized temporary buffers beyond what the peripheral already needs.

### Recommended RAM pattern

- LCD line buffer only:
  - around `1 to 8` lines depending on final SPI/DMA throughput tuning
- ADC short DMA buffers:
  - enough for quick statistics, not waveform capture mode
- Compact shared state blocks:
  - current measurement snapshot
  - current protocol snapshot
  - current UI state
- Font and icon assets in Flash as `const`

### Initial stack planning target

- `ui_task`: medium stack because of drawing helpers
- `protocol_task`: medium-high stack because of parsing and state logic
- `measure_task`: medium stack
- `legacy_charge_task`: medium stack, but mostly sleeping

The exact stack sizes must be tuned after the first integrated build and stack watermark inspection.

## UI Design

### Visual priority

The UI serves `fast reading first`.

Primary emphasis:

- voltage
- current

Secondary emphasis:

- power
- protocol type
- requested / contracted voltage
- ripple trend

### Pages

#### Main page

Purpose:

- default page at boot
- most-used page

Content:

- large `V`
- large `A`
- medium `W`
- protocol badge
- ripple trend indicator

#### Protocol page

Purpose:

- quick inspection of current negotiation state

Content:

- active protocol
- PD contract summary
- active CC channel
- cable/E-Marker summary if available

#### Trigger/negotiation page

Purpose:

- active user-controlled negotiation

Content:

- PD fixed PDO list or current target
- legacy protocol target options where supported
- success/fail status

#### Stats page

Purpose:

- quick engineering summary without turning the product into a packet logger

Content:

- average power
- max power
- min/max voltage
- min/max current
- ripple trend and estimated peak-to-peak

### Button mapping

#### `BTN_1`

- short press: previous page or decrement
- long press: continuous decrement or previous option sweep

#### `BTN_2`

- short press: confirm / enter
- long press: return to main page

#### `BTN_3`

- short press: next page or increment
- long press: quick entry into negotiation actions

### Rendering model

- Use static page layouts.
- Use a tiny widget set:
  - text label
  - big number
  - status badge
  - horizontal bar
  - list row
- Track dirty rectangles or dirty rows.
- Refresh only changed regions.

## Measurement Design

### Measurement outputs

The service layer must publish:

- `voltage_mv`
- `current_ma`
- `power_mw`
- `voltage_avg_mv`
- `current_avg_ma`
- `voltage_min_mv`
- `voltage_max_mv`
- `current_min_ma`
- `current_max_ma`
- `ripple_pp_est_mv`
- `ripple_level`

### Data pipeline

1. ADC samples are captured by DMA.
2. A short sample window is passed to the measurement service.
3. Raw counts are converted using calibration coefficients.
4. Display values are low-pass filtered for readability.
5. Statistics and ripple trend values are generated from a shorter dynamic window.

### Ripple definition for stage 1

Stage 1 ripple is defined as a `trend-grade estimate`, not an instrument-grade certification value.

The implementation should:

- remove the DC baseline or slow drift component
- estimate window peak-to-peak movement
- classify the result into a small number of visual levels

The UI may display:

- approximate ripple in `mV`
- plus a bar or level label such as `LOW`, `MID`, `HIGH`

### Why this limitation is intentional

True ripple characterization depends on analog front-end bandwidth, calibration, sampling strategy, and layout behavior. The stage 1 goal is to help compare chargers and cables consistently on the PX1 hardware, not to claim oscilloscope-equivalent bandwidth.

## PD Design

### Reference basis

The PD stack should start from the vendor examples in:

- `docs/EVT/EXAM/USBPD/USBPD_SNK`
- `docs/EVT/EXAM/USBPD/USBPD_SRC`

Stage 1 uses those examples as hardware bring-up references, but not as the final product architecture.

### Required PD capabilities

- attach detection
- orientation detection
- passive packet reception
- `Source Capabilities` decode
- user-initiated fixed PDO request
- contract result handling
- reset and recovery on detach or timeout

### Product behavior rules

- Passive monitor mode is default.
- Active PD requests occur only after explicit user action.
- Failure must be visible to the user with a short reason:
  - timeout
  - reject
  - unsupported
  - fallback

### PD prioritization

PD is the primary fast-charge path and must be the most reliable protocol path in stage 1. Legacy protocols may be feature-rich but must not destabilize the PD path.

## E-Marker Design

### Stage 1 behavior

Stage 1 reads and summarizes only the highest-value cable fields.

Recommended displayed summary fields:

- E-Marker present / absent
- cable current capability
- USB speed capability summary
- cable type / revision summary

### UI rule

Do not dump full VDO tables onto the main page. Full raw field browsing is explicitly deferred to stage 2 if it is still needed after MVP validation.

## Legacy Protocol Design

Stage 1 includes a separate `legacy protocol engine` for D+/D--based fast charging.

### Initial supported families

- `QC2.0`
- `QC3.0`
- `AFC`
- `FCP`

### Internal interface

Each protocol module should conform to a unified driver-style interface:

- `detect`
- `enter`
- `set_level`
- `read_status`
- `exit`

### Design rule

The controller owns protocol selection. The UI can request a target, but the UI must not manipulate D+/D- directly.

### Compatibility expectation

Stage 1 should be built for practical usefulness, not for claiming exhaustive edge-case compatibility. Real charger regression testing is expected to reveal follow-up tuning work.

## Controller State Machine

The top-level application should use a small explicit state machine.

### States

- `IDLE`
- `MONITOR`
- `NEGOTIATE_PD`
- `NEGOTIATE_LEGACY`
- `CABLE_INFO`
- `ERROR`

### State intent

#### `IDLE`

- no active source
- detached or invalid input

#### `MONITOR`

- default normal operation
- passive protocol observation
- live measurement display

#### `NEGOTIATE_PD`

- active PD target request in progress

#### `NEGOTIATE_LEGACY`

- active QC/AFC/FCP procedure in progress

#### `CABLE_INFO`

- cable interrogation and summary extraction

#### `ERROR`

- timeout
- invalid transition
- protocol failure requiring user-visible notice

### State ownership rule

The app/controller layer owns mode transitions. UI renders state. Service modules report status. BSP never decides product mode.

## Implementation References

The following provided materials should be treated as direct implementation references:

- Schematic:
  - `docs/SCH_HivetonPX1_2026-04-03.pdf`
- Existing FreeRTOS base:
  - `PowerXCode/FreeRTOS/User/main.c`
  - `PowerXCode/FreeRTOS/User/FreeRTOSConfig.h`
- WCH PD references:
  - `docs/EVT/EXAM/USBPD/USBPD_SNK`
  - `docs/EVT/EXAM/USBPD/USBPD_SRC`
- CH32L103 peripheral examples for reuse patterns:
  - `docs/EVT/EXAM/ADC/ADC_DMA`
  - `docs/EVT/EXAM/ADC/ADC_FastConvent`
  - `docs/EVT/EXAM/OPA/*`
  - `docs/EVT/EXAM/SPI/SPI_DMA`
- LCD references:
  - `docs/0.96IPS京东方焊接13pin-ST7735S技术资料/03-程序源码/11-0.96IPS显示屏STM32F103硬件SPI+DMA例程/HARDWARE/LCD`
  - `docs/0.96IPS京东方焊接13pin-ST7735S技术资料/03-程序源码/11-0.96IPS显示屏STM32F103硬件SPI+DMA例程/HARDWARE/SPI`
  - `docs/0.96IPS京东方焊接13pin-ST7735S技术资料/03-程序源码/11-0.96IPS显示屏STM32F103硬件SPI+DMA例程/HARDWARE/DMA`

## Risks And Mitigations

### Risk: RAM exhaustion

Mitigation:

- no framebuffer
- compact snapshots
- measured stack tuning
- optional compile-time disabling of protocol detail features

### Risk: PD and legacy protocol interactions become unstable

Mitigation:

- single controller-owned protocol session state
- strict mutual exclusion between PD negotiation and legacy negotiation

### Risk: Ripple expectation exceeds hardware truth

Mitigation:

- position stage 1 ripple as trend-level
- expose numeric estimate only as approximate
- reserve calibrated ripple work for stage 2

### Risk: UI becomes hard to maintain

Mitigation:

- tiny widget set
- static layouts
- view-model snapshots
- no direct protocol logic in drawing code

## Test Strategy Expectations

Stage 1 verification should include:

- boot and UI smoke test
- key interaction test
- attach/detach behavior on Type-C power sources
- fixed PDO negotiation success/failure cases
- representative charger testing for QC/AFC/FCP
- E-Marker read on known marked cable
- measurement sanity check against bench meter
- ripple trend comparison across at least two clearly different sources

## Deferred Stage 2 Items

- higher-confidence ripple measurement mode
- richer protocol event history
- more detailed E-Marker and raw packet inspection
- data export / upper-computer protocol
- deeper charger compatibility tuning
- optional waveform-like mini trends if memory budget allows

## Notes

- This workspace is currently not a Git repository, so the document cannot be committed here until the project is placed inside a repo or Git is initialized.
