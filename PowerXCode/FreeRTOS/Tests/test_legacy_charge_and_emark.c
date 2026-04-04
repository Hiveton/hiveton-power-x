#include <assert.h>
#include <stdint.h>

#include "bsp_dpdm.h"
#include "service_emark.h"
#include "service_legacy_charge.h"

static void test_emark_reset_clears_summary(void)
{
    emark_summary_t summary = {
        .current_capacity_a = 3U,
        .usb_speed_grade = 2U,
        .cable_type = 4U,
    };

    service_emark_reset(&summary);

    assert(summary.current_capacity_a == 0U);
    assert(summary.usb_speed_grade == 0U);
    assert(summary.cable_type == 0U);
}

static void test_legacy_charge_detect_defaults_to_none(void)
{
    legacy_protocol_t protocol;

    bsp_dpdm_set_mode(BSP_DPDM_MODE_BOTH);
    service_legacy_charge_init();
    protocol = service_legacy_charge_detect();

    assert(protocol == LEGACY_PROTOCOL_NONE);
    assert(bsp_dpdm_get_mode() == BSP_DPDM_MODE_HIZ);
}

static void test_legacy_charge_request_drives_dpdm_mode(void)
{
    service_legacy_charge_init();

    service_legacy_charge_request_protocol(LEGACY_PROTOCOL_QC3);
    assert(bsp_dpdm_get_mode() == BSP_DPDM_MODE_DP);
    assert(service_legacy_charge_detect() == LEGACY_PROTOCOL_QC3);

    service_legacy_charge_request_protocol(LEGACY_PROTOCOL_AFC);
    assert(bsp_dpdm_get_mode() == BSP_DPDM_MODE_DM);
    assert(service_legacy_charge_detect() == LEGACY_PROTOCOL_AFC);

    service_legacy_charge_request_protocol(LEGACY_PROTOCOL_FCP);
    assert(bsp_dpdm_get_mode() == BSP_DPDM_MODE_BOTH);
    assert(service_legacy_charge_detect() == LEGACY_PROTOCOL_FCP);

    service_legacy_charge_request_protocol(LEGACY_PROTOCOL_NONE);
    assert(bsp_dpdm_get_mode() == BSP_DPDM_MODE_HIZ);
    assert(service_legacy_charge_detect() == LEGACY_PROTOCOL_NONE);
}

static void test_legacy_charge_records_requested_voltage_and_qc3_steps(void)
{
    legacy_charge_request_t request;

    service_legacy_charge_init();

    service_legacy_charge_request_voltage_mv(LEGACY_PROTOCOL_QC2, 11000);
    service_legacy_charge_copy_request(&request);
    assert(request.protocol == LEGACY_PROTOCOL_QC2);
    assert(request.target_mv == 9000);
    assert(request.qc3_step_offset == 0);

    service_legacy_charge_request_qc3_step(1);
    service_legacy_charge_request_qc3_step(1);
    service_legacy_charge_copy_request(&request);
    assert(request.protocol == LEGACY_PROTOCOL_QC3);
    assert(request.target_mv == 5400);
    assert(request.qc3_step_offset == 2);

    service_legacy_charge_request_qc3_step(-1);
    service_legacy_charge_copy_request(&request);
    assert(request.target_mv == 5200);
    assert(request.qc3_step_offset == 1);
}

static void test_emark_identity_summary_stays_conservative_without_cable_vdm(void)
{
    const uint32_t identity_vdos[] = {
        0x12345678UL,
        0x9ABCDEF0UL,
    };
    emark_summary_t summary = {
        .present = 1U,
        .current_capacity_a = 5U,
        .usb_speed_grade = 3U,
        .cable_type = 7U,
    };

    assert(service_emark_summarize_identity(identity_vdos,
                                            (uint8_t)(sizeof(identity_vdos) / sizeof(identity_vdos[0])),
                                            &summary) == 0U);
    assert(summary.present == 0U);
    assert(summary.current_capacity_a == 0U);
    assert(summary.usb_speed_grade == 0U);
    assert(summary.cable_type == 0U);
}

int main(void)
{
    test_emark_reset_clears_summary();
    test_legacy_charge_detect_defaults_to_none();
    test_legacy_charge_request_drives_dpdm_mode();
    test_legacy_charge_records_requested_voltage_and_qc3_steps();
    test_emark_identity_summary_stays_conservative_without_cable_vdm();
    return 0;
}
