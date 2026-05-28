#include "app_tasks.h"

#include "app_protocol_arbiter.h"
#include "app_ui_navigation.h"
#include <limits.h>
#include "FreeRTOS.h"
#include "task.h"

#include "bsp_backlight.h"
#include "bsp_usbpd_port.h"
#include "bsp_keys.h"
#include "bsp_adc_dma.h"
#include "bsp_lcd_st7735.h"
#include "service_pd.h"
#include "service_measure.h"
#include "service_legacy_charge.h"
#include "service_protocol_snapshot.h"
#include "ui_model.h"
#include "ui_pages.h"
#include "ui_renderer.h"

#define UI_TASK_POLL_PERIOD_MS 20U
#define UI_TASK_LIVE_REFRESH_MS 20U
#define UI_TASK_PROTOCOL_REFRESH_MS 100U
#define UI_TASK_SLOW_REFRESH_MS 250U
#define APP_TASK_PRIORITY_MEASURE (tskIDLE_PRIORITY + 3U)
#define APP_TASK_PRIORITY_PROTOCOL (tskIDLE_PRIORITY + 3U)
#define APP_TASK_PRIORITY_UI (tskIDLE_PRIORITY + 2U)
#define APP_TASK_PRIORITY_LEGACY (tskIDLE_PRIORITY + 1U)
#define PX1_KEY_DEBUG_SCREEN 0
#define PX1_MINIMAL_HEARTBEAT_DIAG 0
#define PX1_BUSINESS_HEARTBEAT_DIAG 0
#define PX1_BUSINESS_STAGE_DIAG 0
#define PX1_ENABLE_MEASURE_TASK 1
#define PX1_ENABLE_PROTOCOL_TASK 1
#define PX1_ENABLE_LEGACY_TASK 1

#define APP_TASK_STACK_MEASURE 640U
#define APP_TASK_STACK_PROTOCOL 640U
#define APP_TASK_STACK_LEGACY 384U
#define APP_TASK_STACK_UI 1024U

static measure_snapshot_t g_measure_snapshot;
static app_protocol_arbiter_t g_protocol_arbiter;
static TickType_t g_measure_snapshot_tick;
static volatile uint8_t g_measure_snapshot_ready;
static volatile uint8_t g_trigger_boot_requested;

static uint16_t app_ui_refresh_period_ms(ui_page_t page)
{
    switch (page)
    {
        case UI_PAGE_MAIN:
        case UI_PAGE_DPDM:
        case UI_PAGE_POWER_STATS:
            return UI_TASK_LIVE_REFRESH_MS;
        case UI_PAGE_SCOPE:
        case UI_PAGE_RIPPLE:
        case UI_PAGE_TRIGGER_SELECT:
        case UI_PAGE_TRIGGER_ADJUST:
        case UI_PAGE_PROTOCOL:
        case UI_PAGE_PDO:
        case UI_PAGE_EMARK:
        case UI_PAGE_CAPACITY:
            return UI_TASK_PROTOCOL_REFRESH_MS;
        case UI_PAGE_MENU:
        case UI_PAGE_SETTINGS:
            return UI_TASK_SLOW_REFRESH_MS;
        default:
            return UI_TASK_LIVE_REFRESH_MS;
    }
}

void app_tasks_set_trigger_boot(uint8_t enabled)
{
    g_trigger_boot_requested = (enabled != 0U) ? 1U : 0U;
}

#if PX1_KEY_DEBUG_SCREEN || PX1_MINIMAL_HEARTBEAT_DIAG
static uint8_t app_keys_event_mask(const bsp_keys_event_t *keys)
{
    uint8_t mask;

    if (keys == 0)
    {
        return 0U;
    }

    mask = 0U;
    if ((keys->btn1_short != 0U) || (keys->btn1_long != 0U))
    {
        mask |= 0x01U;
    }
    if ((keys->btn2_short != 0U) || (keys->btn2_long != 0U))
    {
        mask |= 0x02U;
    }
    if ((keys->btn3_short != 0U) || (keys->btn3_long != 0U))
    {
        mask |= 0x04U;
    }

    return mask;
}
#endif

#if PX1_MINIMAL_HEARTBEAT_DIAG
static void app_draw_diag_bit_row(uint16_t y, uint8_t mask, uint16_t active_color)
{
    uint8_t index;

    bsp_lcd_fill_rect(0U, y, LCD_WIDTH, 10U, 0x0000U);
    for (index = 0U; index < 3U; ++index)
    {
        uint16_t color;
        uint16_t x;

        color = (((mask >> index) & 0x01U) != 0U) ? active_color : 0x2104U;
        x = (uint16_t)(8U + index * 42U);
        bsp_lcd_fill_rect(x, y, 30U, 10U, color);
    }
}

static void app_draw_minimal_key_diag(uint8_t raw_high_mask,
                                      uint8_t active_mask,
                                      uint8_t seen_irq_mask,
                                      uint8_t event_mask,
                                      uint16_t heartbeat)
{
    uint16_t marker_x;

    app_draw_diag_bit_row(6U, raw_high_mask, 0xFFFFU);
    app_draw_diag_bit_row(22U, active_mask, 0x07E0U);
    app_draw_diag_bit_row(38U, seen_irq_mask, 0xFFE0U);
    app_draw_diag_bit_row(54U, event_mask, 0xF81FU);

    marker_x = (uint16_t)(heartbeat % (LCD_WIDTH - 12U));
    bsp_lcd_fill_rect(0U, 70U, LCD_WIDTH, 10U, 0x0000U);
    bsp_lcd_fill_rect(marker_x, 70U, 12U, 10U, (heartbeat & 0x10U) ? 0xF800U : 0x001FU);
}
#endif

#if PX1_BUSINESS_HEARTBEAT_DIAG
static void app_draw_business_heartbeat(uint16_t heartbeat)
{
    uint16_t x;
    uint16_t color;

    x = (uint16_t)(heartbeat % (LCD_WIDTH - 12U));
    color = (heartbeat & 0x08U) ? 0xF800U : 0x07E0U;
    bsp_lcd_fill_rect(0U, 76U, LCD_WIDTH, 4U, 0x0000U);
    bsp_lcd_fill_rect(x, 76U, 12U, 4U, color);
}
#endif

#if PX1_BUSINESS_STAGE_DIAG
static void app_draw_stage_marker(uint8_t index, uint16_t color)
{
    uint16_t x;

    x = (uint16_t)(index * 20U);
    bsp_lcd_fill_rect(x, 0U, 18U, 4U, color);
}
#endif

static void app_publish_measure_snapshot(const measure_snapshot_t *snapshot)
{
    if (snapshot == 0)
    {
        return;
    }

    taskENTER_CRITICAL();
    g_measure_snapshot = *snapshot;
    g_measure_snapshot_tick = xTaskGetTickCount();
    g_measure_snapshot_ready = 1U;
    taskEXIT_CRITICAL();
}

#if PX1_ENABLE_PROTOCOL_TASK || PX1_ENABLE_LEGACY_TASK
static void app_publish_protocol_snapshot(app_protocol_source_t source,
                                          const protocol_snapshot_t *snapshot)
{
    if (snapshot == 0)
    {
        return;
    }

    taskENTER_CRITICAL();
    app_protocol_arbiter_publish(&g_protocol_arbiter, source, snapshot);
    taskEXIT_CRITICAL();
}
#endif

static void app_copy_measure_snapshot(measure_snapshot_t *snapshot)
{
    TickType_t publish_tick;
    TickType_t now_tick;
    uint32_t elapsed_ms;
    int32_t live_current_ma;
    int32_t live_power_mw;
    uint8_t ready;

    if (snapshot == 0)
    {
        return;
    }

    taskENTER_CRITICAL();
    ready = g_measure_snapshot_ready;
    if (ready != 0U)
    {
        *snapshot = g_measure_snapshot;
        publish_tick = g_measure_snapshot_tick;
    }
    else
    {
        publish_tick = xTaskGetTickCount();
    }
    taskEXIT_CRITICAL();

    if (ready == 0U)
    {
        return;
    }

    now_tick = xTaskGetTickCount();
    elapsed_ms = (uint32_t)((now_tick - publish_tick) * portTICK_PERIOD_MS);
    if (elapsed_ms == 0U)
    {
        return;
    }

    if ((UINT32_MAX - snapshot->stat_elapsed_ms) < elapsed_ms)
    {
        snapshot->stat_elapsed_ms = UINT32_MAX;
    }
    else
    {
        snapshot->stat_elapsed_ms += elapsed_ms;
    }
    snapshot->stat_elapsed_s = snapshot->stat_elapsed_ms / 1000U;

    live_current_ma = snapshot->current_avg_ma;
    if (live_current_ma == INT32_MIN)
    {
        live_current_ma = INT32_MAX;
    }
    else if (live_current_ma < 0)
    {
        live_current_ma = -live_current_ma;
    }

    live_power_mw = snapshot->power_mw;
    if (live_power_mw == INT32_MIN)
    {
        live_power_mw = INT32_MAX;
    }
    else if (live_power_mw < 0)
    {
        live_power_mw = -live_power_mw;
    }

    snapshot->stat_capacity_mah = (uint32_t)((snapshot->stat_charge_ma_ms +
                                              (uint64_t)live_current_ma * elapsed_ms) /
                                             3600000ULL);
    snapshot->stat_energy_mwh = (uint32_t)((snapshot->stat_energy_mw_ms +
                                            (uint64_t)live_power_mw * elapsed_ms) /
                                           3600000ULL);
}

static void app_copy_protocol_snapshot(protocol_snapshot_t *snapshot)
{
    if (snapshot == 0)
    {
        return;
    }

    taskENTER_CRITICAL();
    app_protocol_arbiter_copy(&g_protocol_arbiter, snapshot);
    taskEXIT_CRITICAL();
}

static void measure_task(void *pvParameters)
{
    bsp_adc_window_t window;
    measure_snapshot_t snapshot;
    uint8_t voltage_valid;
    uint8_t current_valid;
    TickType_t last_sample_tick;

    (void)pvParameters;

#if PX1_BUSINESS_STAGE_DIAG
    app_draw_stage_marker(1U, 0xF800U);
#endif
    bsp_adc_dma_init();
#if PX1_BUSINESS_STAGE_DIAG
    app_draw_stage_marker(1U, 0x07E0U);
#endif
    measure_service_reset();
    voltage_valid = bsp_adc_dma_voltage_is_calibrated();
    current_valid = bsp_adc_dma_current_is_calibrated();
    snapshot = (measure_snapshot_t){ 0 };
    snapshot.voltage_valid = voltage_valid;
    snapshot.current_valid = current_valid;
    snapshot.power_valid = (uint8_t)((voltage_valid != 0U) && (current_valid != 0U));
    app_publish_measure_snapshot(&snapshot);
    last_sample_tick = xTaskGetTickCount();

    for (;;)
    {
        if (bsp_adc_dma_fetch_window(&window) != 0)
        {
            TickType_t now_tick;
            uint32_t elapsed_ms;

            now_tick = xTaskGetTickCount();
            elapsed_ms = (uint32_t)((now_tick - last_sample_tick) * portTICK_PERIOD_MS);
            last_sample_tick = now_tick;
            measure_service_process_samples_precise_timed(window.voltage,
                                                          window.current,
                                                          window.current_deci_ma,
                                                          BSP_ADC_SAMPLE_COUNT,
                                                          elapsed_ms,
                                                          &snapshot);

            snapshot.voltage_valid = voltage_valid;
            snapshot.current_valid = current_valid;
            snapshot.power_valid = (uint8_t)((voltage_valid != 0U) && (current_valid != 0U));
            snapshot.mcu_temp_valid = (uint8_t)bsp_adc_dma_read_mcu_temp_deci_c(&snapshot.mcu_temp_deci_c);
            if (current_valid == 0U)
            {
                snapshot.current_avg_ma = 0;
                snapshot.current_avg_deci_ma = 0;
                snapshot.current_min_ma = 0;
                snapshot.current_max_ma = 0;
                snapshot.current_min_deci_ma = 0;
                snapshot.current_max_deci_ma = 0;
                snapshot.stat_current_avg_deci_ma = 0;
                snapshot.stat_current_max_deci_ma = 0;
                snapshot.power_mw = 0;
                snapshot.power_valid = 0U;
            }

            if (voltage_valid == 0U)
            {
                snapshot.voltage_avg_mv = 0;
                snapshot.voltage_min_mv = 0;
                snapshot.voltage_max_mv = 0;
                snapshot.ripple_pp_est_mv = 0U;
                snapshot.ripple_level = 0U;
                snapshot.ripple_sample_count = 0U;
                snapshot.power_mw = 0;
                snapshot.power_valid = 0U;
            }

            app_publish_measure_snapshot(&snapshot);
        }

        vTaskDelay(pdMS_TO_TICKS(10U));
    }
}

#if PX1_ENABLE_PROTOCOL_TASK
static void protocol_task(void *pvParameters)
{
    measure_snapshot_t measure_snapshot;
    protocol_snapshot_t snapshot;
    uint8_t rx_packet[BSP_USBPD_PORT_MAX_PACKET_SIZE];
    uint8_t tx_packet[BSP_USBPD_PORT_MAX_PACKET_SIZE];
    uint8_t rx_length;
    uint8_t tx_length;

    (void)pvParameters;

#if PX1_BUSINESS_STAGE_DIAG
    app_draw_stage_marker(0U, 0xF800U);
#endif
    service_pd_init();
    bsp_usbpd_port_init();
#if PX1_BUSINESS_STAGE_DIAG
    app_draw_stage_marker(0U, 0x07E0U);
#endif
    protocol_snapshot_reset(&snapshot);
    app_publish_protocol_snapshot(APP_PROTOCOL_SOURCE_PD, &snapshot);

    for (;;)
    {
        if (bsp_usbpd_port_fetch_detach() != 0U)
        {
            service_pd_handle_detach();
            bsp_usbpd_port_resume_rx();
        }

        rx_length = 0U;
        tx_length = 0U;
        if (bsp_usbpd_port_fetch_rx_packet(rx_packet, &rx_length) != 0U)
        {
            if (service_pd_handle_rx_packet(rx_packet, rx_length, tx_packet, &tx_length) != 0U)
            {
                (void)bsp_usbpd_port_transmit_sop(tx_packet, tx_length);
            }
            else
            {
                bsp_usbpd_port_resume_rx();
            }
        }
        else if (service_pd_prepare_pending_request(tx_packet, &tx_length) != 0U)
        {
            (void)bsp_usbpd_port_transmit_sop(tx_packet, tx_length);
        }
        else if (service_pd_prepare_source_cap_request(tx_packet, &tx_length) != 0U)
        {
            (void)bsp_usbpd_port_transmit_sop(tx_packet, tx_length);
        }
        else if (service_pd_prepare_emark_identity_request(tx_packet, &tx_length) != 0U)
        {
            (void)bsp_usbpd_port_transmit_sop_prime(tx_packet, tx_length);
        }

        measure_snapshot = (measure_snapshot_t){ 0 };
        app_copy_measure_snapshot(&measure_snapshot);
        service_pd_handle_vbus_measurement((measure_snapshot.voltage_valid != 0U) ? measure_snapshot.voltage_avg_mv : 0,
                                           20U);
        service_pd_handle_timeout_ms(20U);
        bsp_usbpd_port_monitor_tick_ms(20U);
        service_pd_copy_snapshot(&snapshot);
        protocol_snapshot_set_cc_orientation(&snapshot, bsp_usbpd_port_current_cc());
        app_publish_protocol_snapshot(APP_PROTOCOL_SOURCE_PD, &snapshot);
        vTaskDelay(pdMS_TO_TICKS(20U));
    }
}
#endif

#if PX1_ENABLE_LEGACY_TASK
static void legacy_charge_task(void *pvParameters)
{
    measure_snapshot_t measure_snapshot;
    protocol_snapshot_t snapshot;

    (void)pvParameters;

#if PX1_BUSINESS_STAGE_DIAG
    app_draw_stage_marker(2U, 0xF800U);
#endif
    service_legacy_charge_init();
#if PX1_BUSINESS_STAGE_DIAG
    app_draw_stage_marker(2U, 0x07E0U);
#endif
    snapshot = (protocol_snapshot_t){ 0 };

    for (;;)
    {
        measure_snapshot = (measure_snapshot_t){ 0 };
        app_copy_measure_snapshot(&measure_snapshot);
        if (service_legacy_charge_poll((measure_snapshot.voltage_valid != 0U) ? measure_snapshot.voltage_avg_mv : 0,
                                       50U,
                                       &snapshot) != 0U)
        {
            app_publish_protocol_snapshot(APP_PROTOCOL_SOURCE_LEGACY, &snapshot);
        }

        vTaskDelay(pdMS_TO_TICKS(50U));
    }
}
#endif

static void ui_task(void *pvParameters)
{
    bsp_keys_event_t keys;
#if PX1_KEY_DEBUG_SCREEN || PX1_MINIMAL_HEARTBEAT_DIAG
    bsp_keys_event_t last_keys = { 0 };
    bsp_keys_debug_t key_debug;
#endif
#if PX1_KEY_DEBUG_SCREEN
    uint16_t event_count;
#endif
#if PX1_KEY_DEBUG_SCREEN || PX1_MINIMAL_HEARTBEAT_DIAG || PX1_BUSINESS_HEARTBEAT_DIAG
    uint16_t heartbeat;
#endif
    measure_snapshot_t measure_snapshot = { 0 };
    protocol_snapshot_t protocol_snapshot = { 0 };
    ui_model_state_t ui_state;
    TickType_t last_redraw_tick;

    (void)pvParameters;

#if PX1_MINIMAL_HEARTBEAT_DIAG
    bsp_lcd_fill_color(0x0000U);
    bsp_lcd_fill_rect(0U, 0U, LCD_WIDTH, 4U, 0x001FU);
#endif
    bsp_keys_init();
#if PX1_MINIMAL_HEARTBEAT_DIAG
    bsp_lcd_fill_rect(0U, 0U, LCD_WIDTH, 4U, 0x07E0U);
#endif
    ui_model_init(&ui_state);
    if (g_trigger_boot_requested != 0U)
    {
        ui_model_open_trigger(&ui_state);
        service_pd_set_sink_hold(1U);
        service_pd_request_source_capabilities();
    }
    bsp_lcd_set_rotation(ui_model_rotation_degrees(&ui_state));
    ui_renderer_init();
    ui_pages_draw(&ui_state, &measure_snapshot, &protocol_snapshot);
    bsp_backlight_set(ui_model_brightness_percent(&ui_state));
    last_redraw_tick = xTaskGetTickCount();
#if PX1_KEY_DEBUG_SCREEN
    event_count = 0U;
#endif
#if PX1_KEY_DEBUG_SCREEN || PX1_MINIMAL_HEARTBEAT_DIAG || PX1_BUSINESS_HEARTBEAT_DIAG
    heartbeat = 0U;
#endif

    for (;;)
    {
        uint8_t redraw;
#if PX1_KEY_DEBUG_SCREEN || PX1_MINIMAL_HEARTBEAT_DIAG
        uint8_t event_mask;
#endif

#if PX1_KEY_DEBUG_SCREEN || PX1_MINIMAL_HEARTBEAT_DIAG || PX1_BUSINESS_HEARTBEAT_DIAG
        ++heartbeat;
#endif
        redraw = 0U;
        bsp_keys_poll(&keys);
#if PX1_KEY_DEBUG_SCREEN || PX1_MINIMAL_HEARTBEAT_DIAG
        event_mask = app_keys_event_mask(&keys);
        if (event_mask != 0U)
        {
            last_keys = keys;
#if PX1_KEY_DEBUG_SCREEN
            ++event_count;
#endif
        }
#endif

#if PX1_MINIMAL_HEARTBEAT_DIAG
        (void)measure_snapshot;
        (void)protocol_snapshot;
        (void)last_redraw_tick;
        (void)event_count;
        (void)redraw;
        (void)ui_state;
        bsp_keys_get_debug_state(&key_debug);
        app_draw_minimal_key_diag(key_debug.raw_high_mask,
                                  key_debug.active_mask,
                                  key_debug.seen_irq_mask,
                                  app_keys_event_mask(&last_keys),
                                  heartbeat);
        vTaskDelay(pdMS_TO_TICKS(UI_TASK_POLL_PERIOD_MS));
        continue;
#endif
        app_copy_measure_snapshot(&measure_snapshot);
        app_copy_protocol_snapshot(&protocol_snapshot);
        ui_renderer_update_scope_history(&measure_snapshot);
        ui_renderer_update_ripple_history(&ui_state, &measure_snapshot);
        ui_model_advance_liveness(&ui_state);

        if (app_ui_navigation_apply(&ui_state, &protocol_snapshot, &keys) != 0U)
        {
            bsp_backlight_set(ui_model_brightness_percent(&ui_state));
            bsp_lcd_set_rotation(ui_model_rotation_degrees(&ui_state));
            redraw = 1U;
        }

        if (redraw == 0U)
        {
            uint16_t period_ms;

            period_ms = app_ui_refresh_period_ms(ui_state.page);
            if (period_ms != 0U)
            {
                TickType_t now_tick;
                TickType_t period_ticks;

                now_tick = xTaskGetTickCount();
                period_ticks = pdMS_TO_TICKS(period_ms);
                if (period_ticks == 0U)
                {
                    period_ticks = 1U;
                }
                if ((now_tick - last_redraw_tick) >= period_ticks)
                {
                    redraw = 1U;
                }
            }
        }

#if PX1_KEY_DEBUG_SCREEN
        (void)redraw;
        (void)measure_snapshot;
        (void)protocol_snapshot;
        bsp_keys_get_debug_state(&key_debug);
        ui_renderer_draw_key_debug_page(key_debug.raw_high_mask,
                                        key_debug.active_mask,
                                        key_debug.pending_irq_mask,
                                        key_debug.seen_irq_mask,
                                        app_keys_event_mask(&last_keys),
                                        ui_state.page,
                                        event_count,
                                        heartbeat);
#else
        if (redraw != 0U)
        {
            ui_pages_draw(&ui_state, &measure_snapshot, &protocol_snapshot);
            last_redraw_tick = xTaskGetTickCount();
        }
#endif
#if PX1_BUSINESS_HEARTBEAT_DIAG
        app_draw_business_heartbeat(heartbeat);
#endif

        vTaskDelay(pdMS_TO_TICKS(UI_TASK_POLL_PERIOD_MS));
    }
}

void app_tasks_create(void)
{
    BaseType_t status;

    app_protocol_arbiter_init(&g_protocol_arbiter);

    status = xTaskCreate(ui_task,
                         "ui",
                         APP_TASK_STACK_UI,
                         NULL,
                         APP_TASK_PRIORITY_UI,
                         NULL);
    configASSERT(status == pdPASS);

#if PX1_MINIMAL_HEARTBEAT_DIAG
    return;
#endif

#if PX1_ENABLE_PROTOCOL_TASK
    status = xTaskCreate(protocol_task,
                             "protocol",
                             APP_TASK_STACK_PROTOCOL,
                             NULL,
                             APP_TASK_PRIORITY_PROTOCOL,
                             NULL);
    configASSERT(status == pdPASS);
#endif

#if PX1_ENABLE_MEASURE_TASK
    status = xTaskCreate(measure_task,
                         "measure",
                         APP_TASK_STACK_MEASURE,
                         NULL,
                         APP_TASK_PRIORITY_MEASURE,
                         NULL);
    configASSERT(status == pdPASS);
#endif

#if PX1_ENABLE_LEGACY_TASK
    status = xTaskCreate(legacy_charge_task,
                              "legacy",
                              APP_TASK_STACK_LEGACY,
                              NULL,
                              APP_TASK_PRIORITY_LEGACY,
                              NULL);
    configASSERT(status == pdPASS);
#endif

}
