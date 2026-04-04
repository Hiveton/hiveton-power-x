# Hiveton PX1 MVP Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the first integrated PX1 firmware on top of `PowerXCode/FreeRTOS` with live measurement, trend-level ripple, PD monitoring/triggering, legacy fast-charge handling, and a low-memory SPI LCD UI.

**Architecture:** Extend the existing WCH FreeRTOS demo into a layered firmware with `bsp`, `service`, `ui`, and `app` boundaries. Keep `USBPD` as the primary protocol path, add a separate legacy D+/D- engine, and use line-buffer LCD rendering instead of a framebuffer.

**Tech Stack:** `CH32L103K8U6`, WCH peripheral library, FreeRTOS, vendor USBPD examples, ADC/DMA/SPI/OPA examples, ST7735S LCD reference code

---

## File Structure

### Existing files to modify

- `PowerXCode/FreeRTOS/User/main.c`
  - Replace the demo LED tasks with system bootstrap and application task startup.
- `PowerXCode/FreeRTOS/User/ch32l103_it.c`
  - Add project interrupt handlers that delegate to drivers and services.
- `PowerXCode/FreeRTOS/User/ch32l103_it.h`
  - Add handler declarations used by the new modules.
- `PowerXCode/FreeRTOS/User/ch32l103_conf.h`
  - Include `ch32l103_usbpd.h` if not already pulled indirectly and keep peripheral coverage aligned with new drivers.
- `PowerXCode/FreeRTOS/User/FreeRTOSConfig.h`
  - Tune heap, task counts, timer settings, and assert/debug behavior for the product firmware.
- `PowerXCode/FreeRTOS/obj/makefile`
  - Add newly created source files to the build if the existing generated project requires manual inclusion.
- `PowerXCode/FreeRTOS/obj/sources.mk`
  - Keep source listings aligned if the build system depends on static source enumeration.
- `PowerXCode/FreeRTOS/obj/User/subdir.mk`
  - Register new user-layer sources if required by the current build flow.

### New directories to create

- `PowerXCode/FreeRTOS/App`
- `PowerXCode/FreeRTOS/App/include`
- `PowerXCode/FreeRTOS/Bsp`
- `PowerXCode/FreeRTOS/Bsp/include`
- `PowerXCode/FreeRTOS/Service`
- `PowerXCode/FreeRTOS/Service/include`
- `PowerXCode/FreeRTOS/UI`
- `PowerXCode/FreeRTOS/UI/include`
- `PowerXCode/FreeRTOS/Assets`
- `PowerXCode/FreeRTOS/Tests`

### New files to create

- `PowerXCode/FreeRTOS/App/include/app_controller.h`
- `PowerXCode/FreeRTOS/App/app_controller.c`
- `PowerXCode/FreeRTOS/App/include/app_tasks.h`
- `PowerXCode/FreeRTOS/App/app_tasks.c`
- `PowerXCode/FreeRTOS/Bsp/include/bsp_board.h`
- `PowerXCode/FreeRTOS/Bsp/bsp_board.c`
- `PowerXCode/FreeRTOS/Bsp/include/bsp_keys.h`
- `PowerXCode/FreeRTOS/Bsp/bsp_keys.c`
- `PowerXCode/FreeRTOS/Bsp/include/bsp_adc_dma.h`
- `PowerXCode/FreeRTOS/Bsp/bsp_adc_dma.c`
- `PowerXCode/FreeRTOS/Bsp/include/bsp_lcd_st7735.h`
- `PowerXCode/FreeRTOS/Bsp/bsp_lcd_st7735.c`
- `PowerXCode/FreeRTOS/Bsp/include/bsp_backlight.h`
- `PowerXCode/FreeRTOS/Bsp/bsp_backlight.c`
- `PowerXCode/FreeRTOS/Bsp/include/bsp_usbpd_port.h`
- `PowerXCode/FreeRTOS/Bsp/bsp_usbpd_port.c`
- `PowerXCode/FreeRTOS/Bsp/include/bsp_dpdm.h`
- `PowerXCode/FreeRTOS/Bsp/bsp_dpdm.c`
- `PowerXCode/FreeRTOS/Service/include/service_measure.h`
- `PowerXCode/FreeRTOS/Service/service_measure.c`
- `PowerXCode/FreeRTOS/Service/include/service_pd.h`
- `PowerXCode/FreeRTOS/Service/service_pd.c`
- `PowerXCode/FreeRTOS/Service/include/service_emark.h`
- `PowerXCode/FreeRTOS/Service/service_emark.c`
- `PowerXCode/FreeRTOS/Service/include/service_legacy_charge.h`
- `PowerXCode/FreeRTOS/Service/service_legacy_charge.c`
- `PowerXCode/FreeRTOS/Service/include/service_protocol_snapshot.h`
- `PowerXCode/FreeRTOS/UI/include/ui_model.h`
- `PowerXCode/FreeRTOS/UI/ui_model.c`
- `PowerXCode/FreeRTOS/UI/include/ui_renderer.h`
- `PowerXCode/FreeRTOS/UI/ui_renderer.c`
- `PowerXCode/FreeRTOS/UI/include/ui_pages.h`
- `PowerXCode/FreeRTOS/UI/ui_pages.c`
- `PowerXCode/FreeRTOS/UI/include/ui_widgets.h`
- `PowerXCode/FreeRTOS/UI/ui_widgets.c`
- `PowerXCode/FreeRTOS/Assets/font_digits_24.h`
- `PowerXCode/FreeRTOS/Assets/font_ui_12.h`
- `PowerXCode/FreeRTOS/Tests/test_measure_service.c`
- `PowerXCode/FreeRTOS/Tests/test_ui_model.c`
- `PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c`

### File responsibilities

- `App/*`
  - System mode, task startup, task coordination, event routing.
- `Bsp/*`
  - Hardware-facing drivers with no product policy.
- `Service/*`
  - Measurement math, protocol state machines, cable parsing, legacy charge abstraction.
- `UI/*`
  - View-model creation, page definitions, rendering, widget drawing.
- `Assets/*`
  - Compile-time font and icon resources.
- `Tests/*`
  - Host-buildable logic tests for service/UI-model behavior that does not require chip peripherals.

### Notes on tests

- The current repo does not include an existing embedded test harness.
- We will therefore add small host-side C tests for pure logic modules first, then use on-device integration checks for hardware modules.
- TDD should be applied to the pure service/UI-model logic where feasible before writing the production implementation.

## Task 1: Create Product Skeleton And Replace Demo Startup

**Files:**
- Create: `PowerXCode/FreeRTOS/App/include/app_controller.h`
- Create: `PowerXCode/FreeRTOS/App/app_controller.c`
- Create: `PowerXCode/FreeRTOS/App/include/app_tasks.h`
- Create: `PowerXCode/FreeRTOS/App/app_tasks.c`
- Create: `PowerXCode/FreeRTOS/Bsp/include/bsp_board.h`
- Create: `PowerXCode/FreeRTOS/Bsp/bsp_board.c`
- Modify: `PowerXCode/FreeRTOS/User/main.c`
- Modify: `PowerXCode/FreeRTOS/User/FreeRTOSConfig.h`
- Test: `PowerXCode/FreeRTOS/obj/FreeRTOS.elf`

- [ ] **Step 1: Write the failing startup integration test checklist**

Create `PowerXCode/FreeRTOS/Tests/test_startup_checklist.md` with this content:

```md
# Startup Integration Checklist

- Boot reaches product banner instead of demo task prints.
- Scheduler starts application tasks only:
  - measure_task
  - protocol_task
  - legacy_charge_task
  - ui_task
- No GPIOA demo LED toggling remains.
- FreeRTOS heap and task stack sizes are product-oriented, not demo-oriented.
```

- [ ] **Step 2: Verify the current firmware fails the checklist**

Run:

```bash
sed -n '1,220p' PowerXCode/FreeRTOS/User/main.c
```

Expected:

- The file still contains `task1_task`, `task2_task`, and `GPIO_Toggle_INIT`, proving the product startup has not been implemented yet.

- [ ] **Step 3: Add the minimal application bootstrap interfaces**

Create `PowerXCode/FreeRTOS/App/include/app_controller.h`:

```c
#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "debug.h"

typedef enum
{
    APP_MODE_IDLE = 0,
    APP_MODE_MONITOR,
    APP_MODE_NEGOTIATE_PD,
    APP_MODE_NEGOTIATE_LEGACY,
    APP_MODE_CABLE_INFO,
    APP_MODE_ERROR,
} app_mode_t;

void app_controller_init(void);
app_mode_t app_controller_get_mode(void);
void app_controller_set_mode(app_mode_t mode);

#endif
```

Create `PowerXCode/FreeRTOS/App/app_controller.c`:

```c
#include "app_controller.h"

static app_mode_t g_app_mode = APP_MODE_IDLE;

void app_controller_init(void)
{
    g_app_mode = APP_MODE_MONITOR;
}

app_mode_t app_controller_get_mode(void)
{
    return g_app_mode;
}

void app_controller_set_mode(app_mode_t mode)
{
    g_app_mode = mode;
}
```

Create `PowerXCode/FreeRTOS/App/include/app_tasks.h`:

```c
#ifndef APP_TASKS_H
#define APP_TASKS_H

void app_tasks_create(void);

#endif
```

Create `PowerXCode/FreeRTOS/App/app_tasks.c`:

```c
#include "FreeRTOS.h"
#include "task.h"
#include "app_tasks.h"

static void measure_task(void *argument)
{
    (void)argument;
    for(;;)
    {
        vTaskDelay(10);
    }
}

static void protocol_task(void *argument)
{
    (void)argument;
    for(;;)
    {
        vTaskDelay(10);
    }
}

static void legacy_charge_task(void *argument)
{
    (void)argument;
    for(;;)
    {
        vTaskDelay(20);
    }
}

static void ui_task(void *argument)
{
    (void)argument;
    for(;;)
    {
        vTaskDelay(16);
    }
}

void app_tasks_create(void)
{
    xTaskCreate(measure_task, "measure", 384, 0, 6, 0);
    xTaskCreate(protocol_task, "protocol", 512, 0, 7, 0);
    xTaskCreate(legacy_charge_task, "legacy", 384, 0, 5, 0);
    xTaskCreate(ui_task, "ui", 512, 0, 4, 0);
}
```

Create `PowerXCode/FreeRTOS/Bsp/include/bsp_board.h`:

```c
#ifndef BSP_BOARD_H
#define BSP_BOARD_H

void bsp_board_init(void);

#endif
```

Create `PowerXCode/FreeRTOS/Bsp/bsp_board.c`:

```c
#include "bsp_board.h"

void bsp_board_init(void)
{
}
```

- [ ] **Step 4: Replace the demo `main.c` with product bootstrap**

Update `PowerXCode/FreeRTOS/User/main.c` to:

```c
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_board.h"
#include "app_controller.h"
#include "app_tasks.h"

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);

    printf("PX1 boot\r\n");
    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());
    printf("FreeRTOS Kernel Version:%s\r\n", tskKERNEL_VERSION_NUMBER);

    bsp_board_init();
    app_controller_init();
    app_tasks_create();
    vTaskStartScheduler();

    while(1)
    {
    }
}
```

Update `PowerXCode/FreeRTOS/User/FreeRTOSConfig.h` values to:

```c
#define configTICK_RATE_HZ              ( ( TickType_t ) 1000 )
#define configTOTAL_HEAP_SIZE           ( ( size_t ) ( 10 * 1024 ) )
#define configTIMER_QUEUE_LENGTH        6
```

- [ ] **Step 5: Run build to verify the product skeleton compiles**

Run:

```bash
make -C PowerXCode/FreeRTOS/obj
```

Expected:

- Build succeeds and updates `PowerXCode/FreeRTOS/obj/FreeRTOS.elf`.

- [ ] **Step 6: Commit**

```bash
git add PowerXCode/FreeRTOS/User/main.c PowerXCode/FreeRTOS/User/FreeRTOSConfig.h PowerXCode/FreeRTOS/App PowerXCode/FreeRTOS/Bsp
git commit -m "feat: add px1 firmware skeleton"
```

## Task 2: Add Shared Snapshots And Testable Measurement Logic

**Files:**
- Create: `PowerXCode/FreeRTOS/Service/include/service_measure.h`
- Create: `PowerXCode/FreeRTOS/Service/service_measure.c`
- Create: `PowerXCode/FreeRTOS/Tests/test_measure_service.c`
- Test: `PowerXCode/FreeRTOS/Tests/test_measure_service.c`

- [ ] **Step 1: Write the failing test for measurement math**

Create `PowerXCode/FreeRTOS/Tests/test_measure_service.c`:

```c
#include <assert.h>
#include "service_measure.h"

int main(void)
{
    const uint16_t voltage_samples[4] = {1000, 1010, 990, 1005};
    const uint16_t current_samples[4] = {200, 201, 199, 200};
    measure_snapshot_t snapshot;

    measure_service_reset();
    measure_service_process_samples(voltage_samples, current_samples, 4, &snapshot);

    assert(snapshot.voltage_avg_mv > 0);
    assert(snapshot.current_avg_ma > 0);
    assert(snapshot.power_mw > 0);
    assert(snapshot.voltage_max_mv >= snapshot.voltage_min_mv);
    assert(snapshot.ripple_pp_est_mv >= 0);
    return 0;
}
```

- [ ] **Step 2: Verify the test fails because the module does not exist**

Run:

```bash
cc -IPowerXCode/FreeRTOS/Service/include PowerXCode/FreeRTOS/Tests/test_measure_service.c -o /tmp/test_measure_service
```

Expected:

- Compile fails with missing `service_measure.h` or missing symbols.

- [ ] **Step 3: Add the minimal measurement interface and implementation**

Create `PowerXCode/FreeRTOS/Service/include/service_measure.h`:

```c
#ifndef SERVICE_MEASURE_H
#define SERVICE_MEASURE_H

#include <stdint.h>
#include <stddef.h>

typedef struct
{
    int32_t voltage_avg_mv;
    int32_t current_avg_ma;
    int32_t power_mw;
    int32_t voltage_min_mv;
    int32_t voltage_max_mv;
    int32_t current_min_ma;
    int32_t current_max_ma;
    int32_t ripple_pp_est_mv;
    uint8_t ripple_level;
} measure_snapshot_t;

void measure_service_reset(void);
void measure_service_process_samples(const uint16_t *voltage_samples,
                                     const uint16_t *current_samples,
                                     size_t sample_count,
                                     measure_snapshot_t *snapshot);

#endif
```

Create `PowerXCode/FreeRTOS/Service/service_measure.c`:

```c
#include "service_measure.h"

static int32_t average_u16(const uint16_t *samples, size_t count)
{
    size_t i;
    uint32_t sum = 0;
    for(i = 0; i < count; ++i)
    {
        sum += samples[i];
    }
    return (int32_t)(sum / count);
}

void measure_service_reset(void)
{
}

void measure_service_process_samples(const uint16_t *voltage_samples,
                                     const uint16_t *current_samples,
                                     size_t sample_count,
                                     measure_snapshot_t *snapshot)
{
    size_t i;
    int32_t v_avg = average_u16(voltage_samples, sample_count);
    int32_t i_avg = average_u16(current_samples, sample_count);
    int32_t v_min = voltage_samples[0];
    int32_t v_max = voltage_samples[0];
    int32_t i_min = current_samples[0];
    int32_t i_max = current_samples[0];

    for(i = 1; i < sample_count; ++i)
    {
        if(voltage_samples[i] < v_min) v_min = voltage_samples[i];
        if(voltage_samples[i] > v_max) v_max = voltage_samples[i];
        if(current_samples[i] < i_min) i_min = current_samples[i];
        if(current_samples[i] > i_max) i_max = current_samples[i];
    }

    snapshot->voltage_avg_mv = v_avg;
    snapshot->current_avg_ma = i_avg;
    snapshot->power_mw = (v_avg * i_avg) / 1000;
    snapshot->voltage_min_mv = v_min;
    snapshot->voltage_max_mv = v_max;
    snapshot->current_min_ma = i_min;
    snapshot->current_max_ma = i_max;
    snapshot->ripple_pp_est_mv = v_max - v_min;
    snapshot->ripple_level = (snapshot->ripple_pp_est_mv > 20) ? 2 : 1;
}
```

- [ ] **Step 4: Run the measurement test and verify it passes**

Run:

```bash
cc -IPowerXCode/FreeRTOS/Service/include PowerXCode/FreeRTOS/Tests/test_measure_service.c PowerXCode/FreeRTOS/Service/service_measure.c -o /tmp/test_measure_service && /tmp/test_measure_service
```

Expected:

- Command exits with code `0`.

- [ ] **Step 5: Refactor snapshot names to match the spec exactly**

Check the struct still exposes:

```c
voltage_avg_mv
current_avg_ma
power_mw
voltage_min_mv
voltage_max_mv
current_min_ma
current_max_ma
ripple_pp_est_mv
ripple_level
```

- [ ] **Step 6: Commit**

```bash
git add PowerXCode/FreeRTOS/Service/include/service_measure.h PowerXCode/FreeRTOS/Service/service_measure.c PowerXCode/FreeRTOS/Tests/test_measure_service.c
git commit -m "feat: add measurement snapshot service"
```

## Task 3: Bring Up ADC DMA Driver And Connect It To `measure_task`

**Files:**
- Create: `PowerXCode/FreeRTOS/Bsp/include/bsp_adc_dma.h`
- Create: `PowerXCode/FreeRTOS/Bsp/bsp_adc_dma.c`
- Modify: `PowerXCode/FreeRTOS/App/app_tasks.c`
- Modify: `PowerXCode/FreeRTOS/User/ch32l103_it.c`
- Test: `PowerXCode/FreeRTOS/obj/FreeRTOS.elf`

- [ ] **Step 1: Write the failing integration expectation**

Create `PowerXCode/FreeRTOS/Tests/test_adc_dma_checklist.md`:

```md
# ADC DMA Checklist

- ADC DMA driver initializes without demo code dependencies.
- DMA completion interrupt sets a ready flag or notifies `measure_task`.
- `measure_task` consumes sample windows and updates a shared snapshot.
```

- [ ] **Step 2: Verify the current project has no ADC DMA product driver**

Run:

```bash
rg -n "bsp_adc_dma|measure_service_process_samples" PowerXCode/FreeRTOS
```

Expected:

- Only the new service exists or no BSP ADC driver exists yet.

- [ ] **Step 3: Add minimal ADC DMA driver interfaces**

Create `PowerXCode/FreeRTOS/Bsp/include/bsp_adc_dma.h`:

```c
#ifndef BSP_ADC_DMA_H
#define BSP_ADC_DMA_H

#include <stdint.h>

#define BSP_ADC_SAMPLE_COUNT 64

typedef struct
{
    uint16_t voltage[BSP_ADC_SAMPLE_COUNT];
    uint16_t current[BSP_ADC_SAMPLE_COUNT];
} bsp_adc_window_t;

void bsp_adc_dma_init(void);
uint8_t bsp_adc_dma_fetch_window(bsp_adc_window_t *window);
void bsp_adc_dma_irq_handler(void);

#endif
```

Create `PowerXCode/FreeRTOS/Bsp/bsp_adc_dma.c` with a minimal polled stub first:

```c
#include <string.h>
#include "bsp_adc_dma.h"

static bsp_adc_window_t g_window;
static volatile uint8_t g_ready;

void bsp_adc_dma_init(void)
{
    memset(&g_window, 0, sizeof(g_window));
    g_ready = 0;
}

uint8_t bsp_adc_dma_fetch_window(bsp_adc_window_t *window)
{
    if(!g_ready)
    {
        return 0;
    }

    memcpy(window, &g_window, sizeof(g_window));
    g_ready = 0;
    return 1;
}

void bsp_adc_dma_irq_handler(void)
{
    g_ready = 1;
}
```

- [ ] **Step 4: Wire the driver into `measure_task` and interrupts**

Update the `measure_task` body in `PowerXCode/FreeRTOS/App/app_tasks.c` to:

```c
    bsp_adc_window_t window;
    measure_snapshot_t snapshot;

    bsp_adc_dma_init();
    measure_service_reset();
    for(;;)
    {
        if(bsp_adc_dma_fetch_window(&window))
        {
            measure_service_process_samples(window.voltage,
                                            window.current,
                                            BSP_ADC_SAMPLE_COUNT,
                                            &snapshot);
        }
        vTaskDelay(5);
    }
```

Add interrupt delegation in `PowerXCode/FreeRTOS/User/ch32l103_it.c`:

```c
void DMA1_Channel1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void DMA1_Channel1_IRQHandler(void)
{
    bsp_adc_dma_irq_handler();
}
```

- [ ] **Step 5: Build and verify the project still links**

Run:

```bash
make -C PowerXCode/FreeRTOS/obj
```

Expected:

- Build succeeds with the new ADC DMA module included.

- [ ] **Step 6: Commit**

```bash
git add PowerXCode/FreeRTOS/Bsp/include/bsp_adc_dma.h PowerXCode/FreeRTOS/Bsp/bsp_adc_dma.c PowerXCode/FreeRTOS/App/app_tasks.c PowerXCode/FreeRTOS/User/ch32l103_it.c
git commit -m "feat: wire adc dma into measure task"
```

## Task 4: Add LCD BSP And Minimal Low-Memory Renderer

**Files:**
- Create: `PowerXCode/FreeRTOS/Bsp/include/bsp_lcd_st7735.h`
- Create: `PowerXCode/FreeRTOS/Bsp/bsp_lcd_st7735.c`
- Create: `PowerXCode/FreeRTOS/Bsp/include/bsp_backlight.h`
- Create: `PowerXCode/FreeRTOS/Bsp/bsp_backlight.c`
- Create: `PowerXCode/FreeRTOS/UI/include/ui_renderer.h`
- Create: `PowerXCode/FreeRTOS/UI/ui_renderer.c`
- Create: `PowerXCode/FreeRTOS/Assets/font_digits_24.h`
- Create: `PowerXCode/FreeRTOS/Assets/font_ui_12.h`
- Test: `PowerXCode/FreeRTOS/obj/FreeRTOS.elf`

- [ ] **Step 1: Write the failing rendering checklist**

Create `PowerXCode/FreeRTOS/Tests/test_ui_render_checklist.md`:

```md
# UI Render Checklist

- LCD driver exposes init, set-window, and push-pixels APIs.
- UI renderer uses a line buffer, not a framebuffer.
- Backlight PWM driver can set brightness.
```

- [ ] **Step 2: Verify the current tree has no PX1 LCD BSP**

Run:

```bash
rg -n "bsp_lcd_st7735|ui_renderer|line_buffer" PowerXCode/FreeRTOS
```

Expected:

- No product LCD/UI renderer implementation exists yet.

- [ ] **Step 3: Add the BSP and renderer interfaces**

Create `PowerXCode/FreeRTOS/Bsp/include/bsp_lcd_st7735.h`:

```c
#ifndef BSP_LCD_ST7735_H
#define BSP_LCD_ST7735_H

#include <stdint.h>

#define LCD_WIDTH 160
#define LCD_HEIGHT 80

void bsp_lcd_init(void);
void bsp_lcd_set_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void bsp_lcd_push_pixels(const uint16_t *pixels, uint16_t count);

#endif
```

Create `PowerXCode/FreeRTOS/Bsp/include/bsp_backlight.h`:

```c
#ifndef BSP_BACKLIGHT_H
#define BSP_BACKLIGHT_H

#include <stdint.h>

void bsp_backlight_init(void);
void bsp_backlight_set(uint8_t percent);

#endif
```

Create `PowerXCode/FreeRTOS/UI/include/ui_renderer.h`:

```c
#ifndef UI_RENDERER_H
#define UI_RENDERER_H

void ui_renderer_init(void);
void ui_renderer_draw_boot_screen(void);

#endif
```

- [ ] **Step 4: Add the minimal implementation using a line buffer**

Create `PowerXCode/FreeRTOS/UI/ui_renderer.c`:

```c
#include "ui_renderer.h"
#include "bsp_lcd_st7735.h"
#include "bsp_backlight.h"

static uint16_t g_line_buffer[LCD_WIDTH];

void ui_renderer_init(void)
{
    bsp_lcd_init();
    bsp_backlight_init();
    bsp_backlight_set(80);
}

void ui_renderer_draw_boot_screen(void)
{
    uint16_t x;
    for(x = 0; x < LCD_WIDTH; ++x)
    {
        g_line_buffer[x] = 0x0000;
    }

    bsp_lcd_set_window(0, 0, LCD_WIDTH, 1);
    bsp_lcd_push_pixels(g_line_buffer, LCD_WIDTH);
}
```

Create the BSP `.c` files with minimal stubs that compile, with Task 9 explicitly responsible for replacing those stubs using the ST7735S/SPI/DMA reference code.

- [ ] **Step 5: Build and verify the renderer links**

Run:

```bash
make -C PowerXCode/FreeRTOS/obj
```

Expected:

- Build succeeds with the UI renderer linked in.

- [ ] **Step 6: Commit**

```bash
git add PowerXCode/FreeRTOS/Bsp/include/bsp_lcd_st7735.h PowerXCode/FreeRTOS/Bsp/bsp_lcd_st7735.c PowerXCode/FreeRTOS/Bsp/include/bsp_backlight.h PowerXCode/FreeRTOS/Bsp/bsp_backlight.c PowerXCode/FreeRTOS/UI/include/ui_renderer.h PowerXCode/FreeRTOS/UI/ui_renderer.c PowerXCode/FreeRTOS/Assets
git commit -m "feat: add lcd renderer skeleton"
```

## Task 5: Add UI Model, Pages, And Key Handling

**Files:**
- Create: `PowerXCode/FreeRTOS/Bsp/include/bsp_keys.h`
- Create: `PowerXCode/FreeRTOS/Bsp/bsp_keys.c`
- Create: `PowerXCode/FreeRTOS/UI/include/ui_model.h`
- Create: `PowerXCode/FreeRTOS/UI/ui_model.c`
- Create: `PowerXCode/FreeRTOS/UI/include/ui_pages.h`
- Create: `PowerXCode/FreeRTOS/UI/ui_pages.c`
- Create: `PowerXCode/FreeRTOS/UI/include/ui_widgets.h`
- Create: `PowerXCode/FreeRTOS/UI/ui_widgets.c`
- Create: `PowerXCode/FreeRTOS/Tests/test_ui_model.c`
- Modify: `PowerXCode/FreeRTOS/App/app_tasks.c`
- Test: `PowerXCode/FreeRTOS/Tests/test_ui_model.c`

- [ ] **Step 1: Write the failing UI-model test**

Create `PowerXCode/FreeRTOS/Tests/test_ui_model.c`:

```c
#include <assert.h>
#include "ui_model.h"

int main(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(state.page == UI_PAGE_MAIN);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_PROTOCOL);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_MAIN);
    return 0;
}
```

- [ ] **Step 2: Verify the test fails because `ui_model` does not exist**

Run:

```bash
cc -IPowerXCode/FreeRTOS/UI/include PowerXCode/FreeRTOS/Tests/test_ui_model.c -o /tmp/test_ui_model
```

Expected:

- Compile fails with missing header or symbols.

- [ ] **Step 3: Add the minimal UI model**

Create `PowerXCode/FreeRTOS/UI/include/ui_model.h`:

```c
#ifndef UI_MODEL_H
#define UI_MODEL_H

typedef enum
{
    UI_PAGE_MAIN = 0,
    UI_PAGE_PROTOCOL,
    UI_PAGE_TRIGGER,
    UI_PAGE_STATS,
} ui_page_t;

typedef struct
{
    ui_page_t page;
} ui_model_state_t;

void ui_model_init(ui_model_state_t *state);
void ui_model_next_page(ui_model_state_t *state);
void ui_model_prev_page(ui_model_state_t *state);

#endif
```

Create `PowerXCode/FreeRTOS/UI/ui_model.c`:

```c
#include "ui_model.h"

void ui_model_init(ui_model_state_t *state)
{
    state->page = UI_PAGE_MAIN;
}

void ui_model_next_page(ui_model_state_t *state)
{
    state->page = (ui_page_t)((state->page + 1) % 4);
}

void ui_model_prev_page(ui_model_state_t *state)
{
    state->page = (ui_page_t)((state->page + 3) % 4);
}
```

- [ ] **Step 4: Run the UI-model test and verify it passes**

Run:

```bash
cc -IPowerXCode/FreeRTOS/UI/include PowerXCode/FreeRTOS/Tests/test_ui_model.c PowerXCode/FreeRTOS/UI/ui_model.c -o /tmp/test_ui_model && /tmp/test_ui_model
```

Expected:

- Command exits with code `0`.

- [ ] **Step 5: Hook key scan and page switching into `ui_task`**

Update `PowerXCode/FreeRTOS/App/app_tasks.c` so `ui_task`:

```c
    ui_model_state_t ui_state;

    bsp_keys_init();
    ui_model_init(&ui_state);
    ui_renderer_init();
    ui_renderer_draw_boot_screen();

    for(;;)
    {
        const key_event_t event = bsp_keys_poll();
        if(event == KEY_EVENT_BTN1_SHORT)
        {
            ui_model_prev_page(&ui_state);
        }
        else if(event == KEY_EVENT_BTN3_SHORT)
        {
            ui_model_next_page(&ui_state);
        }
        vTaskDelay(10);
    }
```

- [ ] **Step 6: Commit**

```bash
git add PowerXCode/FreeRTOS/Bsp/include/bsp_keys.h PowerXCode/FreeRTOS/Bsp/bsp_keys.c PowerXCode/FreeRTOS/UI/include/ui_model.h PowerXCode/FreeRTOS/UI/ui_model.c PowerXCode/FreeRTOS/UI/include/ui_pages.h PowerXCode/FreeRTOS/UI/ui_pages.c PowerXCode/FreeRTOS/UI/include/ui_widgets.h PowerXCode/FreeRTOS/UI/ui_widgets.c PowerXCode/FreeRTOS/Tests/test_ui_model.c PowerXCode/FreeRTOS/App/app_tasks.c
git commit -m "feat: add ui model and key navigation"
```

## Task 6: Integrate USBPD Driver Port And Protocol Snapshot Tests

**Files:**
- Create: `PowerXCode/FreeRTOS/Bsp/include/bsp_usbpd_port.h`
- Create: `PowerXCode/FreeRTOS/Bsp/bsp_usbpd_port.c`
- Create: `PowerXCode/FreeRTOS/Service/include/service_protocol_snapshot.h`
- Create: `PowerXCode/FreeRTOS/Service/include/service_pd.h`
- Create: `PowerXCode/FreeRTOS/Service/service_pd.c`
- Create: `PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c`
- Modify: `PowerXCode/FreeRTOS/User/ch32l103_it.c`
- Test: `PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c`

- [ ] **Step 1: Write the failing protocol snapshot test**

Create `PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c`:

```c
#include <assert.h>
#include "service_protocol_snapshot.h"

int main(void)
{
    protocol_snapshot_t snapshot;

    protocol_snapshot_reset(&snapshot);
    assert(snapshot.kind == PROTOCOL_KIND_NONE);

    protocol_snapshot_set_pd(&snapshot, 9000, 3000, 1);
    assert(snapshot.kind == PROTOCOL_KIND_PD);
    assert(snapshot.contract_mv == 9000);
    assert(snapshot.contract_ma == 3000);
    assert(snapshot.emark_present == 1);
    return 0;
}
```

- [ ] **Step 2: Verify the test fails because the snapshot module does not exist**

Run:

```bash
cc -IPowerXCode/FreeRTOS/Service/include PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c -o /tmp/test_protocol_snapshot
```

Expected:

- Compile fails with missing protocol snapshot definitions.

- [ ] **Step 3: Add the snapshot and PD service skeleton**

Create `PowerXCode/FreeRTOS/Service/include/service_protocol_snapshot.h`:

```c
#ifndef SERVICE_PROTOCOL_SNAPSHOT_H
#define SERVICE_PROTOCOL_SNAPSHOT_H

#include <stdint.h>

typedef enum
{
    PROTOCOL_KIND_NONE = 0,
    PROTOCOL_KIND_PD,
    PROTOCOL_KIND_QC,
    PROTOCOL_KIND_AFC,
    PROTOCOL_KIND_FCP,
} protocol_kind_t;

typedef struct
{
    protocol_kind_t kind;
    int32_t contract_mv;
    int32_t contract_ma;
    uint8_t emark_present;
} protocol_snapshot_t;

void protocol_snapshot_reset(protocol_snapshot_t *snapshot);
void protocol_snapshot_set_pd(protocol_snapshot_t *snapshot, int32_t mv, int32_t ma, uint8_t emark_present);

#endif
```

Create the matching `.c` content inside `PowerXCode/FreeRTOS/Service/service_pd.c` or a dedicated snapshot file:

```c
#include "service_protocol_snapshot.h"

void protocol_snapshot_reset(protocol_snapshot_t *snapshot)
{
    snapshot->kind = PROTOCOL_KIND_NONE;
    snapshot->contract_mv = 0;
    snapshot->contract_ma = 0;
    snapshot->emark_present = 0;
}

void protocol_snapshot_set_pd(protocol_snapshot_t *snapshot, int32_t mv, int32_t ma, uint8_t emark_present)
{
    snapshot->kind = PROTOCOL_KIND_PD;
    snapshot->contract_mv = mv;
    snapshot->contract_ma = ma;
    snapshot->emark_present = emark_present;
}
```

Create `PowerXCode/FreeRTOS/Bsp/include/bsp_usbpd_port.h` with:

```c
#ifndef BSP_USBPD_PORT_H
#define BSP_USBPD_PORT_H

void bsp_usbpd_port_init(void);
void bsp_usbpd_irq_handler(void);

#endif
```

- [ ] **Step 4: Run the snapshot test and verify it passes**

Run:

```bash
cc -IPowerXCode/FreeRTOS/Service/include PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c PowerXCode/FreeRTOS/Service/service_pd.c -o /tmp/test_protocol_snapshot && /tmp/test_protocol_snapshot
```

Expected:

- Command exits with code `0`.

- [ ] **Step 5: Delegate USBPD IRQs into the port layer**

Update `PowerXCode/FreeRTOS/User/ch32l103_it.c`:

```c
void USBPD_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void USBPD_IRQHandler(void)
{
    bsp_usbpd_irq_handler();
}
```

Base `bsp_usbpd_port.c` on the vendor examples from:

```c
docs/EVT/EXAM/USBPD/USBPD_SNK/User/PD_Process.c
docs/EVT/EXAM/USBPD/USBPD_SRC/User/PD_Process.c
```

but move hardware register code into the BSP file and product policy into `service_pd.c`.

- [ ] **Step 6: Commit**

```bash
git add PowerXCode/FreeRTOS/Bsp/include/bsp_usbpd_port.h PowerXCode/FreeRTOS/Bsp/bsp_usbpd_port.c PowerXCode/FreeRTOS/Service/include/service_protocol_snapshot.h PowerXCode/FreeRTOS/Service/include/service_pd.h PowerXCode/FreeRTOS/Service/service_pd.c PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c PowerXCode/FreeRTOS/User/ch32l103_it.c
git commit -m "feat: add pd snapshot and usbpd port"
```

## Task 7: Add E-Marker Summary And Legacy Charge Engine

**Files:**
- Create: `PowerXCode/FreeRTOS/Service/include/service_emark.h`
- Create: `PowerXCode/FreeRTOS/Service/service_emark.c`
- Create: `PowerXCode/FreeRTOS/Bsp/include/bsp_dpdm.h`
- Create: `PowerXCode/FreeRTOS/Bsp/bsp_dpdm.c`
- Create: `PowerXCode/FreeRTOS/Service/include/service_legacy_charge.h`
- Create: `PowerXCode/FreeRTOS/Service/service_legacy_charge.c`
- Modify: `PowerXCode/FreeRTOS/App/app_tasks.c`
- Test: `PowerXCode/FreeRTOS/obj/FreeRTOS.elf`

- [ ] **Step 1: Write the failing engine checklist**

Create `PowerXCode/FreeRTOS/Tests/test_legacy_engine_checklist.md`:

```md
# Legacy Engine Checklist

- D+/D- control is isolated inside a BSP module.
- Legacy protocols share one engine API:
  - detect
  - enter
  - set_level
  - read_status
  - exit
- E-Marker summary is a compact struct, not a raw VDO dump in UI memory.
```

- [ ] **Step 2: Verify the engine modules do not exist yet**

Run:

```bash
rg -n "service_legacy_charge|service_emark|bsp_dpdm" PowerXCode/FreeRTOS
```

Expected:

- No legacy or E-Marker product modules exist yet.

- [ ] **Step 3: Add compact E-Marker and legacy engine headers**

Create `PowerXCode/FreeRTOS/Service/include/service_emark.h`:

```c
#ifndef SERVICE_EMARK_H
#define SERVICE_EMARK_H

#include <stdint.h>

typedef struct
{
    uint8_t present;
    uint8_t current_capacity_a;
    uint8_t usb_speed_grade;
    uint8_t cable_type;
} emark_summary_t;

void service_emark_reset(emark_summary_t *summary);

#endif
```

Create `PowerXCode/FreeRTOS/Service/include/service_legacy_charge.h`:

```c
#ifndef SERVICE_LEGACY_CHARGE_H
#define SERVICE_LEGACY_CHARGE_H

typedef enum
{
    LEGACY_PROTOCOL_NONE = 0,
    LEGACY_PROTOCOL_QC2,
    LEGACY_PROTOCOL_QC3,
    LEGACY_PROTOCOL_AFC,
    LEGACY_PROTOCOL_FCP,
} legacy_protocol_t;

void service_legacy_charge_init(void);
legacy_protocol_t service_legacy_charge_detect(void);

#endif
```

- [ ] **Step 4: Add minimal implementations and start the sleeping task**

Create the `.c` files with compile-only minimal behavior:

```c
void service_legacy_charge_init(void)
{
}

legacy_protocol_t service_legacy_charge_detect(void)
{
    return LEGACY_PROTOCOL_NONE;
}
```

Then update `legacy_charge_task` in `PowerXCode/FreeRTOS/App/app_tasks.c`:

```c
    service_legacy_charge_init();
    for(;;)
    {
        (void)service_legacy_charge_detect();
        vTaskDelay(100);
    }
```

- [ ] **Step 5: Build and verify the integration still passes**

Run:

```bash
make -C PowerXCode/FreeRTOS/obj
```

Expected:

- Build succeeds with the E-Marker and legacy engine placeholders integrated.

- [ ] **Step 6: Commit**

```bash
git add PowerXCode/FreeRTOS/Service/include/service_emark.h PowerXCode/FreeRTOS/Service/service_emark.c PowerXCode/FreeRTOS/Bsp/include/bsp_dpdm.h PowerXCode/FreeRTOS/Bsp/bsp_dpdm.c PowerXCode/FreeRTOS/Service/include/service_legacy_charge.h PowerXCode/FreeRTOS/Service/service_legacy_charge.c PowerXCode/FreeRTOS/App/app_tasks.c
git commit -m "feat: add legacy charge and emark skeletons"
```

## Task 8: Integrate UI Snapshots And Render Main Measurement Page

**Files:**
- Modify: `PowerXCode/FreeRTOS/UI/ui_renderer.c`
- Modify: `PowerXCode/FreeRTOS/UI/ui_pages.c`
- Modify: `PowerXCode/FreeRTOS/UI/ui_widgets.c`
- Modify: `PowerXCode/FreeRTOS/App/app_tasks.c`
- Test: `PowerXCode/FreeRTOS/obj/FreeRTOS.elf`

- [ ] **Step 1: Write the failing UI integration checklist**

Create `PowerXCode/FreeRTOS/Tests/test_main_ui_checklist.md`:

```md
# Main UI Checklist

- Main page renders voltage and current as primary values.
- Secondary values include power and protocol summary.
- UI refreshes from shared snapshots instead of reading peripherals directly.
```

- [ ] **Step 2: Verify the current renderer only draws a boot stub**

Run:

```bash
sed -n '1,220p' PowerXCode/FreeRTOS/UI/ui_renderer.c
```

Expected:

- The renderer only clears one line or draws a stub, so the main UI page is not implemented yet.

- [ ] **Step 3: Add the minimal main-page render path**

Update `PowerXCode/FreeRTOS/UI/ui_renderer.c` with a function like:

```c
void ui_renderer_draw_main_page(const measure_snapshot_t *measure,
                                const protocol_snapshot_t *protocol)
{
    ui_widgets_draw_big_value(0, 0, measure->voltage_avg_mv, "mV");
    ui_widgets_draw_big_value(0, 28, measure->current_avg_ma, "mA");
    ui_widgets_draw_small_value(0, 56, measure->power_mw, "mW");
    ui_widgets_draw_protocol_badge(92, 56, protocol->kind);
}
```

Keep all drawing backed by line-buffer pushes or tiny region buffers only.

- [ ] **Step 4: Feed snapshots into the renderer from `ui_task`**

Update `PowerXCode/FreeRTOS/App/app_tasks.c` to:

```c
    measure_snapshot_t measure_snapshot;
    protocol_snapshot_t protocol_snapshot;

    for(;;)
    {
        ui_renderer_draw_main_page(&measure_snapshot, &protocol_snapshot);
        vTaskDelay(50);
    }
```

Replace the local snapshot variables in the next integration step with shared snapshot fetch functions published by the measurement and protocol services.

- [ ] **Step 5: Build and verify the firmware still links**

Run:

```bash
make -C PowerXCode/FreeRTOS/obj
```

Expected:

- Build succeeds and the main-page render path is available.

- [ ] **Step 6: Commit**

```bash
git add PowerXCode/FreeRTOS/UI/ui_renderer.c PowerXCode/FreeRTOS/UI/ui_pages.c PowerXCode/FreeRTOS/UI/ui_widgets.c PowerXCode/FreeRTOS/App/app_tasks.c
git commit -m "feat: render main measurement page"
```

## Task 9: Replace Skeleton Drivers With Real Hardware Logic And Validate On Board

**Files:**
- Modify: `PowerXCode/FreeRTOS/Bsp/bsp_adc_dma.c`
- Modify: `PowerXCode/FreeRTOS/Bsp/bsp_lcd_st7735.c`
- Modify: `PowerXCode/FreeRTOS/Bsp/bsp_backlight.c`
- Modify: `PowerXCode/FreeRTOS/Bsp/bsp_keys.c`
- Modify: `PowerXCode/FreeRTOS/Bsp/bsp_usbpd_port.c`
- Modify: `PowerXCode/FreeRTOS/Bsp/bsp_dpdm.c`
- Modify: `PowerXCode/FreeRTOS/Service/service_pd.c`
- Modify: `PowerXCode/FreeRTOS/Service/service_legacy_charge.c`
- Modify: `PowerXCode/FreeRTOS/Service/service_emark.c`
- Test: `PowerXCode/FreeRTOS/obj/FreeRTOS.hex`

- [ ] **Step 1: Implement the ADC driver from the WCH examples**

Use these references:

```bash
docs/EVT/EXAM/ADC/ADC_DMA
docs/EVT/EXAM/ADC/ADC_FastConvent
docs/EVT/EXAM/OPA/*
```

Expected implementation:

- real ADC channel configuration
- DMA circular capture
- ISR completion/half-completion notification
- channel mapping matching the PX1 schematic

- [ ] **Step 2: Implement the ST7735S driver and backlight**

Use these references:

```bash
docs/0.96IPS京东方焊接13pin-ST7735S技术资料/03-程序源码/11-0.96IPS显示屏STM32F103硬件SPI+DMA例程/HARDWARE/LCD/lcd.c
docs/0.96IPS京东方焊接13pin-ST7735S技术资料/03-程序源码/11-0.96IPS显示屏STM32F103硬件SPI+DMA例程/HARDWARE/LCD/lcd_init.c
docs/0.96IPS京东方焊接13pin-ST7735S技术资料/03-程序源码/11-0.96IPS显示屏STM32F103硬件SPI+DMA例程/HARDWARE/SPI/spi.c
docs/0.96IPS京东方焊接13pin-ST7735S技术资料/03-程序源码/11-0.96IPS显示屏STM32F103硬件SPI+DMA例程/HARDWARE/DMA/dma.c
```

Expected implementation:

- pin mapping updated to PX1 schematic nets
- SPI transmit path
- optional DMA burst for pixel pushes
- PWM backlight brightness control

- [ ] **Step 3: Implement key GPIOs and debounce logic**

Expected implementation:

- GPIO input init for `BTN_1`, `BTN_2`, `BTN_3`
- short-press and long-press classification
- no blocking delays in `ui_task`

- [ ] **Step 4: Port real USBPD handling into the product architecture**

Use these references:

```bash
docs/EVT/EXAM/USBPD/USBPD_SNK/User/PD_Process.c
docs/EVT/EXAM/USBPD/USBPD_SRC/User/PD_Process.c
```

Expected implementation:

- hardware register control in `bsp_usbpd_port.c`
- product state updates in `service_pd.c`
- PDO parsing
- fixed PDO request support
- timeout and detach recovery

- [ ] **Step 5: Implement legacy protocol and E-Marker behavior**

Expected implementation:

- D+/D- hardware control in `bsp_dpdm.c`
- protocol module behavior in `service_legacy_charge.c`
- compact E-Marker parsing and summary production in `service_emark.c`

- [ ] **Step 6: Build final firmware image**

Run:

```bash
make -C PowerXCode/FreeRTOS/obj
```

Expected:

- Build succeeds and updates:
  - `PowerXCode/FreeRTOS/obj/FreeRTOS.elf`
  - `PowerXCode/FreeRTOS/obj/FreeRTOS.hex`

- [ ] **Step 7: Flash and perform hardware validation**

Run the project’s normal flash/download flow for CH32L103 and verify:

```md
- LCD displays main page.
- Voltage/current update live.
- Button navigation works.
- PD monitor recognizes a known PD source.
- PD trigger can request at least one fixed PDO successfully.
- At least one legacy protocol path is detected on compatible hardware.
- E-Marker summary appears with a known marked cable.
```

- [ ] **Step 8: Commit**

```bash
git add PowerXCode/FreeRTOS/Bsp PowerXCode/FreeRTOS/Service PowerXCode/FreeRTOS/UI PowerXCode/FreeRTOS/App
git commit -m "feat: integrate px1 hardware firmware"
```

## Self-Review

### Spec coverage

- Architecture: covered by Tasks 1, 4, 5, 6, 7.
- Measurement and trend-level ripple: covered by Tasks 2, 3, 8, 9.
- PD monitor and trigger: covered by Tasks 6 and 9.
- E-Marker summary: covered by Tasks 6, 7, and 9.
- Legacy protocols: covered by Tasks 7 and 9.
- Quick-read UI with 3-button navigation: covered by Tasks 4, 5, and 8.
- FreeRTOS-based implementation: covered by Tasks 1 through 9.

### Placeholder scan

- No `TODO` or `TBD` placeholders are left in the task steps.
- Board flashing is intentionally described through the project’s actual download flow because the exact command is not established in the current workspace.

### Type consistency

- App modes use the `APP_MODE_*` naming family.
- UI pages use the `UI_PAGE_*` naming family.
- Measurement snapshot fields match the design doc names.
- Protocol snapshot fields use one compact struct across UI and services.

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-04-04-hiveton-px1-mvp.md`. Two execution options:

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

**Which approach?**
