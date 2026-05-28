#include <assert.h>
#include <stdint.h>

#include "bsp_dpdm.h"
#include "service_charge_protocols.h"
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

    bsp_dpdm_mock_set_sample(0U, 0U);
    bsp_dpdm_set_mode(BSP_DPDM_MODE_BOTH);
    service_legacy_charge_init();
    protocol = service_legacy_charge_detect();

    assert(protocol == LEGACY_PROTOCOL_NONE);
    assert(bsp_dpdm_get_mode() == BSP_DPDM_MODE_HIZ);
}

static void test_legacy_charge_does_not_report_qc_from_passive_dpdm_bias(void)
{
    service_legacy_charge_init();
    bsp_dpdm_mock_set_sample(1U, 0U);

    assert(service_legacy_charge_detect() == LEGACY_PROTOCOL_NONE);
}

static void test_legacy_charge_request_drives_dpdm_mode(void)
{
    service_legacy_charge_init();
    bsp_dpdm_mock_set_sample(0U, 0U);

    service_legacy_charge_request_protocol(LEGACY_PROTOCOL_QC3);
    assert(bsp_dpdm_get_mode() == BSP_DPDM_MODE_BOTH);
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

static void test_charge_protocols_port_qc2_and_bc12_rules(void)
{
    bsp_dpdm_level_t dp_level;
    bsp_dpdm_level_t dm_level;
    int32_t actual_mv;

    assert(charge_protocol_qc2_levels_for_voltage_mv(9000, &dp_level, &dm_level, &actual_mv) == 1U);
    assert(actual_mv == 9000);
    assert(dp_level == BSP_DPDM_LEVEL_3300MV);
    assert(dm_level == BSP_DPDM_LEVEL_600MV);

    assert(charge_protocol_qc2_levels_for_voltage_mv(12000, &dp_level, &dm_level, &actual_mv) == 1U);
    assert(actual_mv == 12000);
    assert(dp_level == BSP_DPDM_LEVEL_600MV);
    assert(dm_level == BSP_DPDM_LEVEL_600MV);

    assert(charge_protocol_qc2_levels_for_voltage_mv(20000, &dp_level, &dm_level, &actual_mv) == 1U);
    assert(actual_mv == 20000);
    assert(dp_level == BSP_DPDM_LEVEL_3300MV);
    assert(dm_level == BSP_DPDM_LEVEL_3300MV);

    assert(charge_protocol_qc3_offset_for_voltage_mv(15000, 5000, -7, 75) == 50);
    assert(charge_protocol_bc12_type_from_probe(0U, 0U) == CHARGE_BC12_TYPE_SDP);
    assert(charge_protocol_bc12_type_from_probe(1U, 0U) == CHARGE_BC12_TYPE_CDP);
    assert(charge_protocol_bc12_type_from_probe(1U, 1U) == CHARGE_BC12_TYPE_DCP);
}

static void test_legacy_charge_records_requested_voltage_and_qc3_steps(void)
{
    legacy_charge_request_t request;
    bsp_dpdm_level_t dp_level;
    bsp_dpdm_level_t dm_level;

    service_legacy_charge_init();

    service_legacy_charge_request_voltage_mv(LEGACY_PROTOCOL_QC2, 11000);
    service_legacy_charge_copy_request(&request);
    assert(request.protocol == LEGACY_PROTOCOL_QC2);
    assert(request.target_mv == 9000);
    assert(request.qc3_step_offset == 0);
    assert(request.status == LEGACY_CHARGE_STATUS_REQUESTING);
    bsp_dpdm_get_levels(&dp_level, &dm_level);
    assert(dp_level == BSP_DPDM_LEVEL_3300MV);
    assert(dm_level == BSP_DPDM_LEVEL_600MV);

    service_legacy_charge_request_voltage_mv(LEGACY_PROTOCOL_QC2, 20000);
    service_legacy_charge_copy_request(&request);
    assert(request.target_mv == 20000);
    bsp_dpdm_get_levels(&dp_level, &dm_level);
    assert(dp_level == BSP_DPDM_LEVEL_3300MV);
    assert(dm_level == BSP_DPDM_LEVEL_3300MV);

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

static void test_legacy_charge_poll_reports_qc_state_and_dpdm_voltage(void)
{
    protocol_snapshot_t snapshot;
    legacy_charge_request_t request;

    service_legacy_charge_init();
    bsp_dpdm_mock_set_voltage_mv(600, 0);

    assert(service_legacy_charge_poll(5000, 50U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_OTHER);
    assert(snapshot.request_state == PROTOCOL_REQUEST_AVAILABLE);
    assert(snapshot.target_mv == 0);
    assert(snapshot.dp_mv == 600);
    assert(snapshot.dm_mv == 0);

    service_legacy_charge_request_voltage_mv(LEGACY_PROTOCOL_QC2, 9000);
    assert(service_legacy_charge_poll(9100, 50U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_QC);
    assert(snapshot.request_state == PROTOCOL_REQUEST_READY);
    assert(snapshot.target_mv == 9000);
    service_legacy_charge_copy_request(&request);
    assert(request.status == LEGACY_CHARGE_STATUS_READY);
}

static void test_legacy_charge_poll_ignores_dpdm_when_adc_unavailable(void)
{
    protocol_snapshot_t snapshot;

    service_legacy_charge_init();
    bsp_dpdm_mock_set_voltage_mv(600, 600);
    assert(service_legacy_charge_poll(5000, 50U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_OTHER);

    bsp_dpdm_mock_set_adc_unavailable();
    assert(service_legacy_charge_poll(5000, 50U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_NONE);
}

static void test_legacy_charge_poll_reports_zero_dpdm_when_adc_unavailable_during_request(void)
{
    protocol_snapshot_t snapshot;

    service_legacy_charge_init();
    bsp_dpdm_mock_set_adc_unavailable();
    service_legacy_charge_request_voltage_mv(LEGACY_PROTOCOL_QC2, 9000);

    assert(service_legacy_charge_poll(5000, 50U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_QC);
    assert(snapshot.dp_mv == 0);
    assert(snapshot.dm_mv == 0);
}

static void test_legacy_charge_poll_keeps_adc_dpdm_when_requesting(void)
{
    protocol_snapshot_t snapshot;

    service_legacy_charge_init();
    bsp_dpdm_mock_set_voltage_mv(610, 570);
    service_legacy_charge_request_voltage_mv(LEGACY_PROTOCOL_QC2, 9000);

    assert(service_legacy_charge_poll(5000, 50U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_QC);
    assert(snapshot.dp_mv == 610);
    assert(snapshot.dm_mv == 570);
}

static void test_legacy_charge_poll_clears_when_dpdm_bias_disappears(void)
{
    protocol_snapshot_t snapshot;

    service_legacy_charge_init();
    bsp_dpdm_mock_set_voltage_mv(600, 0);

    assert(service_legacy_charge_poll(5000, 50U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_OTHER);

    bsp_dpdm_mock_set_voltage_mv(0, 0);

    assert(service_legacy_charge_poll(5000, 50U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_NONE);
}

static void test_dpdm_sampling_preserves_bc_source_state(void)
{
    bsp_dpdm_sample_t sample;

    service_legacy_charge_init();
    bsp_dpdm_set_levels(BSP_DPDM_LEVEL_600MV, BSP_DPDM_LEVEL_600MV);
    assert(bsp_dpdm_mock_get_bc_source_mask() == 0x03U);

    assert(bsp_dpdm_sample_lines(&sample) == 1U);
    assert(bsp_dpdm_mock_get_bc_source_mask() == 0x03U);

    bsp_dpdm_set_levels(BSP_DPDM_LEVEL_600MV, BSP_DPDM_LEVEL_HIZ);
    assert(bsp_dpdm_mock_get_bc_source_mask() == 0x01U);

    assert(bsp_dpdm_sample_lines(&sample) == 1U);
    assert(bsp_dpdm_mock_get_bc_source_mask() == 0x01U);
}

static void test_legacy_charge_qc3_voltage_uses_200mv_steps(void)
{
    legacy_charge_request_t request;
    bsp_dpdm_level_t dp_level;
    bsp_dpdm_level_t dm_level;

    service_legacy_charge_init();
    service_legacy_charge_request_voltage_mv(LEGACY_PROTOCOL_QC3, 15000);
    service_legacy_charge_copy_request(&request);

    assert(request.protocol == LEGACY_PROTOCOL_QC3);
    assert(request.target_mv == 15000);
    assert(request.qc3_step_offset == 50);
    assert(request.status == LEGACY_CHARGE_STATUS_REQUESTING);
    assert(bsp_dpdm_mock_get_qc3_offset() == 50);
    assert(bsp_dpdm_mock_get_qc3_pulse_count() == 50);
    bsp_dpdm_get_levels(&dp_level, &dm_level);
    assert(dp_level == BSP_DPDM_LEVEL_600MV);
    assert(dm_level == BSP_DPDM_LEVEL_600MV);

    service_legacy_charge_request_voltage_mv(LEGACY_PROTOCOL_QC3, 9000);
    service_legacy_charge_copy_request(&request);
    assert(request.target_mv == 9000);
    assert(request.qc3_step_offset == 20);
    assert(bsp_dpdm_mock_get_qc3_offset() == 20);
    assert(bsp_dpdm_mock_get_qc3_pulse_count() == 80);
}

static void test_legacy_charge_timeout_releases_dpdm_request_levels(void)
{
    protocol_snapshot_t snapshot;
    legacy_charge_request_t request;
    bsp_dpdm_level_t dp_level;
    bsp_dpdm_level_t dm_level;

    service_legacy_charge_init();
    service_legacy_charge_request_voltage_mv(LEGACY_PROTOCOL_QC2, 20000);

    assert(service_legacy_charge_poll(5000, 2200U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_QC);
    assert(snapshot.request_state == PROTOCOL_REQUEST_FAILED);
    assert(snapshot.target_mv == 20000);

    service_legacy_charge_copy_request(&request);
    assert(request.protocol == LEGACY_PROTOCOL_QC2);
    assert(request.status == LEGACY_CHARGE_STATUS_FAILED);
    assert(service_legacy_charge_detect() == LEGACY_PROTOCOL_NONE);

    bsp_dpdm_get_levels(&dp_level, &dm_level);
    assert(dp_level == BSP_DPDM_LEVEL_HIZ);
    assert(dm_level == BSP_DPDM_LEVEL_HIZ);
}

static void test_legacy_charge_failed_state_remains_visible_after_release(void)
{
    protocol_snapshot_t snapshot;

    service_legacy_charge_init();
    service_legacy_charge_request_voltage_mv(LEGACY_PROTOCOL_QC2, 20000);

    assert(service_legacy_charge_poll(5000, 2200U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_QC);
    assert(snapshot.request_state == PROTOCOL_REQUEST_FAILED);
    assert(snapshot.target_mv == 20000);
    assert(service_legacy_charge_detect() == LEGACY_PROTOCOL_NONE);

    assert(service_legacy_charge_poll(5000, 50U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_QC);
    assert(snapshot.request_state == PROTOCOL_REQUEST_FAILED);
    assert(snapshot.target_mv == 20000);
    assert(service_legacy_charge_detect() == LEGACY_PROTOCOL_NONE);
}

static void test_legacy_charge_failed_state_clears_after_visible_hold(void)
{
    protocol_snapshot_t snapshot;

    service_legacy_charge_init();
    service_legacy_charge_request_voltage_mv(LEGACY_PROTOCOL_QC2, 20000);

    assert(service_legacy_charge_poll(5000, 2200U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_QC);
    assert(snapshot.request_state == PROTOCOL_REQUEST_FAILED);

    assert(service_legacy_charge_poll(5000, 1490U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_QC);
    assert(snapshot.request_state == PROTOCOL_REQUEST_FAILED);

    assert(service_legacy_charge_poll(5000, 20U, &snapshot) == 1U);
    assert(snapshot.kind == PROTOCOL_KIND_NONE);
    assert(snapshot.request_state == PROTOCOL_REQUEST_IDLE);
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
    assert(summary.max_voltage_v == 0U);
    assert(summary.cable_length_m == 0U);
}

static void test_emark_identity_summary_extracts_cable_vdo(void)
{
    const uint32_t identity_vdos[] = {
        0x00000000UL,
        0x00000000UL,
        0x00000000UL,
        (2UL << 18) | (1UL << 13) | (3UL << 9) | (2UL << 5) | 3UL,
    };
    emark_summary_t summary = { 0 };

    assert(service_emark_summarize_identity(identity_vdos,
                                            (uint8_t)(sizeof(identity_vdos) / sizeof(identity_vdos[0])),
                                            &summary) == 1U);
    assert(summary.present == 1U);
    assert(summary.current_capacity_a == 5U);
    assert(summary.usb_speed_grade == 3U);
    assert(summary.cable_type == 2U);
    assert(summary.max_voltage_v == 50U);
    assert(summary.cable_length_m == 1U);
}

static void test_emark_identity_summary_ignores_non_cable_vdo_current_bits(void)
{
    const uint32_t identity_vdos[] = {
        (2UL << 5) | 3UL,
        (2UL << 5) | 3UL,
        (2UL << 5) | 3UL,
    };
    emark_summary_t summary = { 0 };

    assert(service_emark_summarize_identity(identity_vdos,
                                            (uint8_t)(sizeof(identity_vdos) / sizeof(identity_vdos[0])),
                                            &summary) == 0U);
    assert(summary.present == 0U);
    assert(summary.current_capacity_a == 0U);
}

int main(void)
{
    test_emark_reset_clears_summary();
    test_legacy_charge_detect_defaults_to_none();
    test_legacy_charge_does_not_report_qc_from_passive_dpdm_bias();
    test_legacy_charge_request_drives_dpdm_mode();
    test_charge_protocols_port_qc2_and_bc12_rules();
    test_legacy_charge_records_requested_voltage_and_qc3_steps();
    test_legacy_charge_poll_reports_qc_state_and_dpdm_voltage();
    test_legacy_charge_poll_ignores_dpdm_when_adc_unavailable();
    test_legacy_charge_poll_reports_zero_dpdm_when_adc_unavailable_during_request();
    test_legacy_charge_poll_keeps_adc_dpdm_when_requesting();
    test_legacy_charge_poll_clears_when_dpdm_bias_disappears();
    test_dpdm_sampling_preserves_bc_source_state();
    test_legacy_charge_qc3_voltage_uses_200mv_steps();
    test_legacy_charge_timeout_releases_dpdm_request_levels();
    test_legacy_charge_failed_state_remains_visible_after_release();
    test_legacy_charge_failed_state_clears_after_visible_hold();
    test_emark_identity_summary_stays_conservative_without_cable_vdm();
    test_emark_identity_summary_extracts_cable_vdo();
    test_emark_identity_summary_ignores_non_cable_vdo_current_bits();
    return 0;
}
