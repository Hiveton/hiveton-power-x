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
    assert(snapshot.legacy_step_offset == 0);

    protocol_snapshot_set_pd(&snapshot, 9000, 2000, 1);
    assert(snapshot.kind == PROTOCOL_KIND_PD);
    assert(snapshot.contract_mv == 9000);
    assert(snapshot.contract_ma == 2000);
    assert(snapshot.emark_present == 1);
    assert(snapshot.legacy_step_offset == 0);
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
    assert(service_pd_handle_rx_packet(rx_packet, sizeof(rx_packet), tx_packet, &tx_length) == 1U);

    for (step = 0U; step < 24U; ++step)
    {
        service_pd_handle_timeout_ms(20U);
    }

    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.kind == PROTOCOL_KIND_NONE);
    assert(snapshot.contract_mv == 0);
    assert(snapshot.contract_ma == 0);
    assert(snapshot.legacy_step_offset == 0);

    service_pd_handle_timeout_ms(20U);
    service_pd_copy_snapshot(&snapshot);
    assert(snapshot.kind == PROTOCOL_KIND_NONE);
}

int main(void)
{
    test_protocol_snapshot_reset_and_pd_update();
    test_protocol_snapshot_null_guards();
    test_protocol_snapshot_legacy_update();
    test_service_pd_selects_fixed_pdo_and_updates_contract();
    test_service_pd_timeout_accumulates_periodic_ticks();
    return 0;
}
