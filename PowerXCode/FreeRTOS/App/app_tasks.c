#include "app_tasks.h"

#include "app_trigger_control.h"
#include "FreeRTOS.h"
#include "task.h"

#include "bsp_usbpd_port.h"
#include "bsp_keys.h"
#include "bsp_adc_dma.h"
#include "service_pd.h"
#include "service_measure.h"
#include "service_legacy_charge.h"
#include "service_protocol_snapshot.h"
#include "ui_model.h"
#include "ui_pages.h"
#include "ui_renderer.h"

static measure_snapshot_t g_measure_snapshot;
static protocol_snapshot_t g_protocol_snapshot;
static volatile uint8_t g_measure_snapshot_ready;
static volatile uint8_t g_protocol_snapshot_ready;

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

static void app_publish_protocol_snapshot(const protocol_snapshot_t *snapshot)
{
    if (snapshot == 0)
    {
        return;
    }

    taskENTER_CRITICAL();
    g_protocol_snapshot = *snapshot;
    g_protocol_snapshot_ready = 1U;
    taskEXIT_CRITICAL();
}

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
    if (g_protocol_snapshot_ready != 0U)
    {
        *snapshot = g_protocol_snapshot;
    }
    taskEXIT_CRITICAL();
}

static void measure_task(void *pvParameters)
{
    bsp_adc_window_t window;
    measure_snapshot_t snapshot;
    uint8_t voltage_valid;
    uint8_t current_valid;

    (void)pvParameters;

    bsp_adc_dma_init();
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

static void protocol_task(void *pvParameters)
{
    protocol_snapshot_t snapshot;
    uint8_t rx_packet[BSP_USBPD_PORT_MAX_PACKET_SIZE];
    uint8_t tx_packet[BSP_USBPD_PORT_MAX_PACKET_SIZE];
    uint8_t rx_length;
    uint8_t tx_length;

    (void)pvParameters;

    service_pd_init();
    bsp_usbpd_port_init();
    protocol_snapshot_reset(&snapshot);
    app_publish_protocol_snapshot(&snapshot);

    for (;;)
    {
        if (bsp_usbpd_port_fetch_detach() != 0U)
        {
            service_pd_handle_detach();
        }

        rx_length = 0U;
        tx_length = 0U;
        if (bsp_usbpd_port_fetch_rx_packet(rx_packet, &rx_length) != 0U)
        {
            if (service_pd_handle_rx_packet(rx_packet, rx_length, tx_packet, &tx_length) != 0U)
            {
                (void)bsp_usbpd_port_transmit_sop(tx_packet, tx_length);
            }
        }

        service_pd_handle_timeout_ms(20U);
        service_pd_copy_snapshot(&snapshot);
        app_publish_protocol_snapshot(&snapshot);
        vTaskDelay(pdMS_TO_TICKS(20U));
    }
}

static void legacy_charge_task(void *pvParameters)
{
    legacy_protocol_t protocol;
    legacy_charge_request_t request;
    protocol_snapshot_t snapshot;
    protocol_kind_t kind;

    (void)pvParameters;

    service_legacy_charge_init();
    request = (legacy_charge_request_t){ LEGACY_PROTOCOL_NONE, 0, 0 };
    snapshot = (protocol_snapshot_t){ 0 };

    for (;;)
    {
        protocol = service_legacy_charge_detect();
        service_legacy_charge_copy_request(&request);

        if (protocol != LEGACY_PROTOCOL_NONE)
        {
            switch (protocol)
            {
                case LEGACY_PROTOCOL_QC2:
                case LEGACY_PROTOCOL_QC3:
                    kind = PROTOCOL_KIND_QC;
                    break;
                case LEGACY_PROTOCOL_AFC:
                    kind = PROTOCOL_KIND_AFC;
                    break;
                case LEGACY_PROTOCOL_FCP:
                    kind = PROTOCOL_KIND_FCP;
                    break;
                case LEGACY_PROTOCOL_NONE:
                default:
                    kind = PROTOCOL_KIND_NONE;
                    break;
            }

            protocol_snapshot_set_legacy(&snapshot,
                                         kind,
                                         request.target_mv,
                                         request.qc3_step_offset);
            app_publish_protocol_snapshot(&snapshot);
        }

        vTaskDelay(pdMS_TO_TICKS(50U));
    }
}

static void ui_task(void *pvParameters)
{
    bsp_keys_event_t keys;
    measure_snapshot_t measure_snapshot = { 0 };
    protocol_snapshot_t protocol_snapshot = { 0 };
    ui_model_state_t ui_state;

    (void)pvParameters;

    bsp_keys_init();
    ui_model_init(&ui_state);
    ui_renderer_init();

    for (;;)
    {
        bsp_keys_poll(&keys);
        app_copy_measure_snapshot(&measure_snapshot);
        app_copy_protocol_snapshot(&protocol_snapshot);

        if (ui_state.page == UI_PAGE_TRIGGER)
        {
            if (keys.btn1_short != 0U)
            {
                ui_model_trigger_prev(&ui_state);
            }

            if (keys.btn3_short != 0U)
            {
                ui_model_trigger_next(&ui_state);
            }

            if (keys.btn2_short != 0U)
            {
                app_trigger_control_apply(&ui_state, &protocol_snapshot);
            }
        }
        else
        {
            if (keys.btn1_short != 0U)
            {
                ui_model_prev_page(&ui_state);
            }

            if (keys.btn3_short != 0U)
            {
                ui_model_next_page(&ui_state);
            }
        }

        if (keys.btn2_long != 0U)
        {
            ui_state.page = UI_PAGE_MAIN;
        }

        ui_pages_draw(&ui_state, &measure_snapshot, &protocol_snapshot);

        vTaskDelay(pdMS_TO_TICKS(50U));
    }
}

void app_tasks_create(void)
{
    BaseType_t status;

    status = xTaskCreate(measure_task,
                         "measure",
                         configMINIMAL_STACK_SIZE,
                         NULL,
                         tskIDLE_PRIORITY + 2U,
                         NULL);
    configASSERT(status == pdPASS);

    status = xTaskCreate(protocol_task,
                         "protocol",
                         configMINIMAL_STACK_SIZE,
                         NULL,
                         tskIDLE_PRIORITY + 3U,
                         NULL);
    configASSERT(status == pdPASS);

    status = xTaskCreate(legacy_charge_task,
                         "legacy",
                         configMINIMAL_STACK_SIZE,
                         NULL,
                         tskIDLE_PRIORITY + 1U,
                         NULL);
    configASSERT(status == pdPASS);

    status = xTaskCreate(ui_task,
                         "ui",
                         configMINIMAL_STACK_SIZE,
                         NULL,
                         tskIDLE_PRIORITY + 1U,
                         NULL);
    configASSERT(status == pdPASS);
}
