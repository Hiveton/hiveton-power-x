#include "app_tasks.h"

#include "app_protocol_arbiter.h"
#include "app_ui_navigation.h"
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
#define UI_TASK_REFRESH_TICKS 13U
#define PX1_KEY_DEBUG_SCREEN 0
#define PX1_MINIMAL_HEARTBEAT_DIAG 0
#define PX1_BUSINESS_HEARTBEAT_DIAG 0
#define PX1_BUSINESS_STAGE_DIAG 0
#define PX1_ENABLE_MEASURE_TASK 1
#define PX1_ENABLE_PROTOCOL_TASK 1
#define PX1_ENABLE_LEGACY_TASK 1

#define APP_TASK_STACK_MEASURE 512U
#define APP_TASK_STACK_PROTOCOL 512U
#define APP_TASK_STACK_LEGACY 384U
#define APP_TASK_STACK_UI 768U

static measure_snapshot_t g_measure_snapshot;
static app_protocol_arbiter_t g_protocol_arbiter;
static volatile uint8_t g_measure_snapshot_ready;

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
    if (snapshot == 0)
    {
        return;
    }

    taskENTER_CRITICAL();
    if (g_measure_snapshot_ready != 0U)
    {
        *snapshot = g_measure_snapshot;
    }
    taskEXIT_CRITICAL();
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

    for (;;)
    {
        if (bsp_adc_dma_fetch_window(&window) != 0)
        {
            measure_service_process_samples(window.voltage,
                                            window.current,
                                            BSP_ADC_SAMPLE_COUNT,
                                            &snapshot);

            snapshot.voltage_valid = voltage_valid;
            snapshot.current_valid = current_valid;
            snapshot.power_valid = (uint8_t)((voltage_valid != 0U) && (current_valid != 0U));
            if (current_valid == 0U)
            {
                snapshot.current_avg_ma = 0;
                snapshot.current_min_ma = 0;
                snapshot.current_max_ma = 0;
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
        else if (service_pd_prepare_emark_identity_request(tx_packet, &tx_length) != 0U)
        {
            (void)bsp_usbpd_port_transmit_sop_prime(tx_packet, tx_length);
        }

        measure_snapshot = (measure_snapshot_t){ 0 };
        app_copy_measure_snapshot(&measure_snapshot);
        service_pd_handle_vbus_measurement((measure_snapshot.voltage_valid != 0U) ? measure_snapshot.voltage_avg_mv : 0,
                                           20U);
        service_pd_handle_timeout_ms(20U);
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
    uint8_t refresh_ticks;

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
    bsp_backlight_set(ui_model_brightness_percent(&ui_state));
    bsp_lcd_set_rotation(ui_model_rotation_degrees(&ui_state));
    ui_renderer_init();
    refresh_ticks = UI_TASK_REFRESH_TICKS;
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
        (void)refresh_ticks;
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
        ui_model_advance_liveness(&ui_state);

        if (app_ui_navigation_apply(&ui_state, &protocol_snapshot, &keys) != 0U)
        {
            bsp_backlight_set(ui_model_brightness_percent(&ui_state));
            bsp_lcd_set_rotation(ui_model_rotation_degrees(&ui_state));
            redraw = 1U;
        }

        if (refresh_ticks >= UI_TASK_REFRESH_TICKS)
        {
            refresh_ticks = 0U;
            redraw = 1U;
        }
        else
        {
            ++refresh_ticks;
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
                         tskIDLE_PRIORITY + 4U,
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
                         tskIDLE_PRIORITY + 2U,
                         NULL);
    configASSERT(status == pdPASS);
#endif

#if PX1_ENABLE_MEASURE_TASK
    status = xTaskCreate(measure_task,
                         "measure",
                         APP_TASK_STACK_MEASURE,
                         NULL,
                         tskIDLE_PRIORITY + 2U,
                         NULL);
    configASSERT(status == pdPASS);
#endif

#if PX1_ENABLE_LEGACY_TASK
    status = xTaskCreate(legacy_charge_task,
                         "legacy",
                         APP_TASK_STACK_LEGACY,
                         NULL,
                         tskIDLE_PRIORITY + 1U,
                         NULL);
    configASSERT(status == pdPASS);
#endif

}
