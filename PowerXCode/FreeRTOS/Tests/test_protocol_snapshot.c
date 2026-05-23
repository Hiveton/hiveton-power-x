#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "service_pd.h"
#include "service_protocol_snapshot.h"

/*
 * TDD red step:
 * The first host compile of this test failed because
 * service_protocol_snapshot.h did not exist yet.
 */
static void test_protocol_snapshot_reset_and_pd_update(void)
{
    protocol_snapshot_t snapshot;

    protocol_snapshot_reset(&snapshot);
    assert(snapshot.kind == PROTOCOL_KIND_NONE);
    assert(snapshot.contract_mv == 0);
    assert(snapshot.contract_ma == 0);
    assert(snapshot.emark_present == 0);
    assert(snapshot.emark_current_a == 0U);
    assert(snapshot.emark_usb_speed_grade == 0U);
    assert(snapshot.emark_cable_type == 0U);
    assert(snapshot.legacy_step_offset == 0);
    assert(snapshot.request_state == PROTOCOL_REQUEST_IDLE);
    assert(snapshot.source_fixed_count == 0U);
    assert(snapshot.cc_orientation == 0U);
    assert(snapshot.pps_present == 0U);
    assert(snapshot.pps_min_mv == 0);
    assert(snapshot.pps_max_mv == 0);
    assert(snapshot.pps_max_ma == 0);

    protocol_snapshot_set_pd(&snapshot, 9000, 2000, 1);
    assert(snapshot.kind == PROTOCOL_KIND_PD);
    assert(snapshot.contract_mv == 9000);
    assert(snapshot.contract_ma == 2000);
    assert(snapshot.emark_present == 1);
    assert(snapshot.emark_current_a == 0U);
    assert(snapshot.emark_usb_speed_grade == 0U);
    assert(snapshot.emark_cable_type == 0U);
    assert(snapshot.legacy_step_offset == 0);
    assert(snapshot.request_state == PROTOCOL_REQUEST_READY);
    assert(snapshot.target_mv == 9000);
    assert(snapshot.cc_orientation == 0U);
    assert(snapshot.pps_present == 0U);
    assert(snapshot.pps_min_mv == 0);
    assert(snapshot.pps_max_mv == 0);
    assert(snapshot.pps_max_ma == 0);
}

static void test_protocol_snapshot_tracks_cc_orientation(void)
{
    protocol_snapshot_t snapshot;

    protocol_snapshot_reset(&snapshot);
    protocol_snapshot_set_cc_orientation(&snapshot, 2U);
    assert(snapshot.cc_orientation == 2U);

    protocol_snapshot_set_cc_orientation(&snapshot, 9U);
    assert(snapshot.cc_orientation == 0U);
}

static void test_protocol_snapshot_null_guards(void)
{
    protocol_snapshot_t snapshot = {
        .kind = PROTOCOL_KIND_QC,
        .contract_mv = 1234,
        .contract_ma = 5678,
        .emark_present = 9,
        .legacy_step_offset = 4,
    };

    protocol_snapshot_reset(NULL);
    protocol_snapshot_set_pd(NULL, 5000, 3000, 1);
    protocol_snapshot_set_legacy(NULL, PROTOCOL_KIND_QC, 9000, 2);

    assert(snapshot.kind == PROTOCOL_KIND_QC);
    assert(snapshot.contract_mv == 1234);
    assert(snapshot.contract_ma == 5678);
    assert(snapshot.emark_present == 9);
    assert(snapshot.legacy_step_offset == 4);
}

static void test_protocol_snapshot_legacy_update(void)
{
    protocol_snapshot_t snapshot;

    protocol_snapshot_reset(&snapshot);
    protocol_snapshot_set_legacy(&snapshot, PROTOCOL_KIND_QC, 5400, 2);

    assert(snapshot.kind == PROTOCOL_KIND_QC);
    assert(snapshot.contract_mv == 5400);
    assert(snapshot.contract_ma == 0);
    assert(snapshot.emark_present == 0U);
    assert(snapshot.legacy_step_offset == 2);
}

static void test_protocol_snapshot_other_update(void)
{
    protocol_snapshot_t snapshot;

    protocol_snapshot_reset(&snapshot);
    protocol_snapshot_set_other(&snapshot);

    assert(snapshot.kind == PROTOCOL_KIND_OTHER);
    assert(snapshot.contract_mv == 0);
    assert(snapshot.contract_ma == 0);
    assert(snapshot.emark_present == 0U);
    assert(snapshot.legacy_step_offset == 0);
}

static void test_service_pd_requests_default_pdo_after_source_capabilities(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0xC8U, 0xD0U, 0x02U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;

    service_pd_init();

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert(tx_length == 6U);
    assert(tx_packet[0] == 0x82U);
    assert((tx_packet[5] >> 4) == 1U);

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.kind == PROTOCOL_KIND_PD);
    assert(snapshot.contract_mv == 5000);
    assert(snapshot.contract_ma == 3000);
    assert(snapshot.emark_present == 0U);
    assert(snapshot.legacy_step_offset == 0);
    assert(snapshot.request_state == PROTOCOL_REQUEST_REQUESTING);
    assert(snapshot.target_mv == 5000);
    assert(snapshot.selected_pdo_index == 1U);
    assert(snapshot.source_fixed_count == 2U);
    assert(snapshot.source_fixed_mv[0] == 5000);
    assert(snapshot.source_fixed_ma[0] == 3000);
    assert(snapshot.source_fixed_mv[1] == 9000);
    assert(snapshot.source_fixed_ma[1] == 2000);
    assert(snapshot.pps_present == 0U);
}

static void test_service_pd_marks_pps_when_source_cap_contains_apdo(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0x00U, 0x00U, 0x00U, 0xC0U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;

    service_pd_init();

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    service_pd_copy_snapshot(&snapshot);

    assert(snapshot.kind == PROTOCOL_KIND_PD);
    assert(snapshot.source_fixed_count == 1U);
    assert(snapshot.source_fixed_mv[0] == 5000);
    assert(snapshot.pps_present == 1U);
    assert(snapshot.pps_min_mv == 0);
    assert(snapshot.pps_max_mv == 0);
    assert(snapshot.pps_max_ma == 0);
}

static uint32_t test_pd_get_u32_le(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static void test_service_pd_preserves_source_object_position_when_apdo_is_between_fixed_pdos(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x30U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0x3CU, 0x21U, 0xDCU, 0xC0U,
        0xC8U, 0xD0U, 0x02U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(9000);

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert(tx_length == 6U);
    assert((tx_packet[5] >> 4) == 3U);

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.request_state == PROTOCOL_REQUEST_REQUESTING);
    assert(snapshot.target_mv == 9000);
    assert(snapshot.selected_pdo_index == 3U);
    assert(snapshot.source_fixed_count == 2U);
    assert(snapshot.source_fixed_mv[1] == 9000);
    assert(snapshot.pps_present == 1U);
    assert(snapshot.pps_min_mv == 3300);
    assert(snapshot.pps_max_mv == 11000);
    assert(snapshot.pps_max_ma == 3000);
}

static void test_service_pd_requests_pps_apdo_when_target_matches_pps_range(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0x3CU, 0x21U, 0xDCU, 0xC0U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;
    uint32_t request_word;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(9000);

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert(tx_length == 6U);
    assert((tx_packet[5] >> 4) == 2U);

    request_word = test_pd_get_u32_le(&tx_packet[2]);
    assert(((request_word >> 9) & 0x0FFFU) == 450U);
    assert((request_word & 0x7FU) == 60U);

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.kind == PROTOCOL_KIND_PD);
    assert(snapshot.contract_mv == 9000);
    assert(snapshot.contract_ma == 3000);
    assert(snapshot.request_state == PROTOCOL_REQUEST_REQUESTING);
    assert(snapshot.target_mv == 9000);
    assert(snapshot.selected_pdo_index == 2U);
    assert(snapshot.pps_present == 1U);
    assert(snapshot.pps_min_mv == 3300);
    assert(snapshot.pps_max_mv == 11000);
    assert(snapshot.pps_max_ma == 3000);
}

static void test_service_pd_caps_fixed_request_current_to_3a_without_emark(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0xF4U, 0x41U, 0x06U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;
    uint32_t request_word;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(20000);

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert(tx_length == 6U);

    request_word = test_pd_get_u32_le(&tx_packet[2]);
    assert(((request_word >> 10) & 0x03FFU) == 300U);
    assert((request_word & 0x03FFU) == 300U);

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.contract_mv == 20000);
    assert(snapshot.contract_ma == 3000);
    assert(snapshot.emark_present == 0U);
}

static void test_service_pd_caps_pps_request_current_to_3a_without_emark(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0x64U, 0x21U, 0x90U, 0xC1U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;
    uint32_t request_word;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(20000);

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert(tx_length == 6U);

    request_word = test_pd_get_u32_le(&tx_packet[2]);
    assert(((request_word >> 9) & 0x0FFFU) == 1000U);
    assert((request_word & 0x7FU) == 60U);

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.contract_mv == 20000);
    assert(snapshot.contract_ma == 3000);
    assert(snapshot.pps_max_ma == 5000);
    assert(snapshot.emark_present == 0U);
}

static void test_service_pd_prepares_pending_request_after_caps(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0xC8U, 0xD0U, 0x02U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(9000);

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert(tx_length == 6U);
    assert(tx_packet[0] == 0x82U);
    assert((tx_packet[5] >> 4) == 2U);

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.request_state == PROTOCOL_REQUEST_REQUESTING);
    assert(snapshot.target_mv == 9000);
    assert(snapshot.selected_pdo_index == 2U);
}

static void test_service_pd_selects_fixed_pdo_and_updates_contract(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0xC8U, 0xD0U, 0x02U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(9000);

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert(tx_length == 6U);
    assert(tx_packet[0] == 0x82U);
    assert((tx_packet[5] >> 4) == 2U);

    tx_length = 0U;
    assert(service_pd_handle_rx_packet((const uint8_t[]){ 0x03U, 0x00U }, 2U, tx_packet, &tx_length) == 0U);
    assert(service_pd_handle_rx_packet((const uint8_t[]){ 0x06U, 0x00U }, 2U, tx_packet, &tx_length) == 0U);
    service_pd_handle_vbus_measurement(9000, 20U);

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.kind == PROTOCOL_KIND_PD);
    assert(snapshot.contract_mv == 9000);
    assert(snapshot.contract_ma == 2000);
    assert(snapshot.emark_present == 0U);
    assert(snapshot.legacy_step_offset == 0);

    service_pd_handle_detach();
    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.kind == PROTOCOL_KIND_NONE);
}

static void test_service_pd_waits_for_vbus_measurement_after_ps_rdy(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0xC8U, 0xD0U, 0x02U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(9000);

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert(service_pd_handle_rx_packet((const uint8_t[]){ 0x03U, 0x00U }, 2U, tx_packet, &tx_length) == 0U);
    assert(service_pd_handle_rx_packet((const uint8_t[]){ 0x06U, 0x00U }, 2U, tx_packet, &tx_length) == 0U);

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.kind == PROTOCOL_KIND_PD);
    assert(snapshot.contract_mv == 9000);
    assert(snapshot.request_state == PROTOCOL_REQUEST_ACCEPTED);

    service_pd_handle_vbus_measurement(5000, 20U);
    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.request_state == PROTOCOL_REQUEST_ACCEPTED);

    service_pd_handle_vbus_measurement(9000, 20U);
    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.request_state == PROTOCOL_REQUEST_READY);
}

static void test_service_pd_queues_new_target_while_verifying_vbus(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0xC8U, 0xD0U, 0x02U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(9000);

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert((tx_packet[5] >> 4) == 2U);
    assert(service_pd_handle_rx_packet((const uint8_t[]){ 0x03U, 0x00U }, 2U, tx_packet, &tx_length) == 0U);
    assert(service_pd_handle_rx_packet((const uint8_t[]){ 0x06U, 0x00U }, 2U, tx_packet, &tx_length) == 0U);

    service_pd_set_preferred_voltage_mv(5000);
    tx_length = 0U;
    assert(service_pd_prepare_pending_request(tx_packet, &tx_length) == 0U);
    assert(tx_length == 0U);

    tx_length = 0U;
    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 0U);
    assert(tx_length == 0U);

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.request_state == PROTOCOL_REQUEST_ACCEPTED);
    assert(snapshot.target_mv == 9000);

    service_pd_handle_vbus_measurement(9000, 20U);
    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.request_state == PROTOCOL_REQUEST_READY);
    assert(snapshot.target_mv == 9000);

    assert(service_pd_prepare_pending_request(tx_packet, &tx_length) == 1U);
    assert(tx_length == 6U);
    assert((tx_packet[5] >> 4) == 1U);
}

static void test_service_pd_failed_inflight_request_keeps_failed_target_when_new_target_is_queued(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0xC8U, 0xD0U, 0x02U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(9000);

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert((tx_packet[5] >> 4) == 2U);

    service_pd_set_preferred_voltage_mv(5000);
    assert(service_pd_handle_rx_packet((const uint8_t[]){ 0x04U, 0x00U }, 2U, tx_packet, &tx_length) == 0U);

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.request_state == PROTOCOL_REQUEST_FAILED);
    assert(snapshot.target_mv == 9000);
}

static void test_service_pd_fails_vbus_verification_and_returns_to_default_target(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0xC8U, 0xD0U, 0x02U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(9000);

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert(service_pd_handle_rx_packet((const uint8_t[]){ 0x03U, 0x00U }, 2U, tx_packet, &tx_length) == 0U);
    assert(service_pd_handle_rx_packet((const uint8_t[]){ 0x06U, 0x00U }, 2U, tx_packet, &tx_length) == 0U);

    service_pd_handle_vbus_measurement(5000, 500U);
    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.request_state == PROTOCOL_REQUEST_FAILED);
    assert(snapshot.target_mv == 9000);

    tx_length = 0U;
    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert(tx_length == 6U);
    assert((tx_packet[5] >> 4) == 1U);
    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.target_mv == 5000);
}

static void test_service_pd_detach_clears_high_voltage_preference_before_next_source(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0xF4U, 0x41U, 0x06U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(20000);

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert((tx_packet[5] >> 4) == 2U);

    service_pd_handle_detach();
    tx_length = 0U;
    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert(tx_length == 6U);
    assert((tx_packet[5] >> 4) == 1U);

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.target_mv == 5000);
    assert(snapshot.contract_mv == 5000);
}

static void test_service_pd_reject_clears_high_voltage_preference_before_next_source(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x20U,
        0x2CU, 0x91U, 0x01U, 0x00U,
        0xF4U, 0x41U, 0x06U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(20000);

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert((tx_packet[5] >> 4) == 2U);

    tx_length = 0U;
    assert(service_pd_handle_rx_packet((const uint8_t[]){ 0x04U, 0x00U }, 2U, tx_packet, &tx_length) == 0U);
    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.request_state == PROTOCOL_REQUEST_FAILED);
    assert(snapshot.target_mv == 20000);

    tx_length = 0U;
    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);
    assert(tx_length == 6U);
    assert((tx_packet[5] >> 4) == 1U);

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.target_mv == 5000);
    assert(snapshot.contract_mv == 5000);
}

static void test_service_pd_publishes_emark_identity_summary(void)
{
    uint8_t rx_packet[] = {
        0x0FU, 0x50U,
        0x01U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U,
        0x43U, 0x20U, 0x08U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot;

    service_pd_init();

    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 0U);
    service_pd_copy_snapshot(&snapshot);

    assert(snapshot.emark_present == 1U);
    assert(snapshot.emark_current_a == 5U);
    assert(snapshot.emark_usb_speed_grade == 3U);
    assert(snapshot.emark_cable_type == 2U);
}

static void test_service_pd_prepares_emark_discover_identity_after_contract_ready(void)
{
    uint8_t src_cap_packet[] = {
        0x01U, 0x10U,
        0x2CU, 0x91U, 0x01U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;

    service_pd_init();

    assert(service_pd_handle_rx_packet(src_cap_packet, sizeof(src_cap_packet), tx_packet, &tx_length) == 1U);
    assert(service_pd_handle_rx_packet((const uint8_t[]){ 0x03U, 0x00U }, 2U, tx_packet, &tx_length) == 0U);
    assert(service_pd_handle_rx_packet((const uint8_t[]){ 0x06U, 0x00U }, 2U, tx_packet, &tx_length) == 0U);
    service_pd_handle_vbus_measurement(5000, 20U);

    tx_length = 0U;
    assert(service_pd_prepare_emark_identity_request(tx_packet, &tx_length) == 1U);
    assert(tx_length == 6U);
    assert(tx_packet[0] == 0x8FU);
    assert((tx_packet[1] & 0x10U) == 0x10U);
    assert(tx_packet[2] == 0x01U);
    assert(tx_packet[3] == 0x80U);
    assert(tx_packet[4] == 0x00U);
    assert(tx_packet[5] == 0xFFU);

    tx_length = 0U;
    assert(service_pd_prepare_emark_identity_request(tx_packet, &tx_length) == 0U);
    assert(tx_length == 0U);
}

static void test_service_pd_timeout_accumulates_periodic_ticks(void)
{
    uint8_t rx_packet[] = {
        0x01U, 0x10U,
        0x2CU, 0x91U, 0x01U, 0x00U,
    };
    uint8_t tx_packet[6] = { 0 };
    uint8_t tx_length = 0U;
    protocol_snapshot_t snapshot = { 0 };
    uint8_t step;

    service_pd_init();
    service_pd_set_preferred_voltage_mv(5000);
    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);

    for (step = 0U; step < 24U; ++step)
    {
        service_pd_handle_timeout_ms(20U);
    }

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.kind == PROTOCOL_KIND_PD);
    assert(snapshot.contract_mv == 5000);
    assert(snapshot.contract_ma == 3000);
    assert(snapshot.legacy_step_offset == 0);

    service_pd_handle_timeout_ms(20U);
    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.kind == PROTOCOL_KIND_PD);
    assert(snapshot.request_state == PROTOCOL_REQUEST_FAILED);
    assert(snapshot.target_mv == 5000);
    assert(snapshot.selected_pdo_index == 1U);
}

int main(void)
{
    test_protocol_snapshot_reset_and_pd_update();
    test_protocol_snapshot_tracks_cc_orientation();
    test_protocol_snapshot_null_guards();
    test_protocol_snapshot_legacy_update();
    test_protocol_snapshot_other_update();
    test_service_pd_requests_default_pdo_after_source_capabilities();
    test_service_pd_marks_pps_when_source_cap_contains_apdo();
    test_service_pd_preserves_source_object_position_when_apdo_is_between_fixed_pdos();
    test_service_pd_requests_pps_apdo_when_target_matches_pps_range();
    test_service_pd_caps_fixed_request_current_to_3a_without_emark();
    test_service_pd_caps_pps_request_current_to_3a_without_emark();
    test_service_pd_prepares_pending_request_after_caps();
    test_service_pd_selects_fixed_pdo_and_updates_contract();
    test_service_pd_waits_for_vbus_measurement_after_ps_rdy();
    test_service_pd_queues_new_target_while_verifying_vbus();
    test_service_pd_failed_inflight_request_keeps_failed_target_when_new_target_is_queued();
    test_service_pd_fails_vbus_verification_and_returns_to_default_target();
    test_service_pd_detach_clears_high_voltage_preference_before_next_source();
    test_service_pd_reject_clears_high_voltage_preference_before_next_source();
    test_service_pd_publishes_emark_identity_summary();
    test_service_pd_prepares_emark_discover_identity_after_contract_ready();
    test_service_pd_timeout_accumulates_periodic_ticks();
    return 0;
}
