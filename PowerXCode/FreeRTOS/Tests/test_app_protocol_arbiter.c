#include <assert.h>

#include "app_protocol_arbiter.h"

static void test_pd_none_does_not_clear_legacy_qc(void)
{
    app_protocol_arbiter_t arbiter;
    protocol_snapshot_t snapshot;
    protocol_snapshot_t out;

    app_protocol_arbiter_init(&arbiter);

    protocol_snapshot_set_legacy(&snapshot, PROTOCOL_KIND_QC, 5000, 0);
    snapshot.request_state = PROTOCOL_REQUEST_AVAILABLE;
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_LEGACY, &snapshot);

    protocol_snapshot_reset(&snapshot);
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_PD, &snapshot);

    app_protocol_arbiter_copy(&arbiter, &out);
    assert(out.kind == PROTOCOL_KIND_QC);
    assert(out.request_state == PROTOCOL_REQUEST_AVAILABLE);
    assert(out.target_mv == 5000);
}

static void test_pd_snapshot_has_priority_over_legacy_qc(void)
{
    app_protocol_arbiter_t arbiter;
    protocol_snapshot_t snapshot;
    protocol_snapshot_t out;

    app_protocol_arbiter_init(&arbiter);

    protocol_snapshot_set_legacy(&snapshot, PROTOCOL_KIND_QC, 5000, 0);
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_LEGACY, &snapshot);

    protocol_snapshot_set_pd(&snapshot, 9000, 2000, 0);
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_PD, &snapshot);

    app_protocol_arbiter_copy(&arbiter, &out);
    assert(out.kind == PROTOCOL_KIND_PD);
    assert(out.contract_mv == 9000);
    assert(out.contract_ma == 2000);
}

static void test_legacy_none_clears_legacy_when_pd_is_absent(void)
{
    app_protocol_arbiter_t arbiter;
    protocol_snapshot_t snapshot;
    protocol_snapshot_t out;

    app_protocol_arbiter_init(&arbiter);

    protocol_snapshot_set_legacy(&snapshot, PROTOCOL_KIND_QC, 5000, 0);
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_LEGACY, &snapshot);

    protocol_snapshot_reset(&snapshot);
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_LEGACY, &snapshot);

    app_protocol_arbiter_copy(&arbiter, &out);
    assert(out.kind == PROTOCOL_KIND_NONE);
}

static void test_pd_detach_after_active_contract_does_not_restore_stale_legacy(void)
{
    app_protocol_arbiter_t arbiter;
    protocol_snapshot_t snapshot;
    protocol_snapshot_t out;

    app_protocol_arbiter_init(&arbiter);

    protocol_snapshot_set_legacy(&snapshot, PROTOCOL_KIND_QC, 9000, 0);
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_LEGACY, &snapshot);

    protocol_snapshot_set_pd(&snapshot, 9000, 3000, 0);
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_PD, &snapshot);

    protocol_snapshot_reset(&snapshot);
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_PD, &snapshot);

    app_protocol_arbiter_copy(&arbiter, &out);
    assert(out.kind == PROTOCOL_KIND_NONE);
}

static void test_cc_attached_pd_path_hides_passive_legacy_other(void)
{
    app_protocol_arbiter_t arbiter;
    protocol_snapshot_t snapshot;
    protocol_snapshot_t out;

    app_protocol_arbiter_init(&arbiter);

    protocol_snapshot_reset(&snapshot);
    protocol_snapshot_set_cc_orientation(&snapshot, 1U);
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_PD, &snapshot);

    protocol_snapshot_set_other(&snapshot);
    snapshot.request_state = PROTOCOL_REQUEST_AVAILABLE;
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_LEGACY, &snapshot);

    app_protocol_arbiter_copy(&arbiter, &out);
    assert(out.kind == PROTOCOL_KIND_NONE);
    assert(out.cc_attached == 1U);
    assert(out.cc_orientation == 1U);
}

static void test_cc_orientation_alone_hides_passive_legacy_other(void)
{
    app_protocol_arbiter_t arbiter;
    protocol_snapshot_t snapshot;
    protocol_snapshot_t out;

    app_protocol_arbiter_init(&arbiter);

    protocol_snapshot_reset(&snapshot);
    snapshot.cc_orientation = 2U;
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_PD, &snapshot);

    protocol_snapshot_set_other(&snapshot);
    snapshot.request_state = PROTOCOL_REQUEST_AVAILABLE;
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_LEGACY, &snapshot);

    app_protocol_arbiter_copy(&arbiter, &out);
    assert(out.kind == PROTOCOL_KIND_NONE);
    assert(out.cc_orientation == 2U);
}

static void test_cc_attached_pd_path_hides_passive_legacy_qc_available(void)
{
    app_protocol_arbiter_t arbiter;
    protocol_snapshot_t snapshot;
    protocol_snapshot_t out;

    app_protocol_arbiter_init(&arbiter);

    protocol_snapshot_reset(&snapshot);
    protocol_snapshot_set_cc_orientation(&snapshot, 1U);
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_PD, &snapshot);

    protocol_snapshot_set_legacy(&snapshot, PROTOCOL_KIND_QC, 9000, 0);
    snapshot.request_state = PROTOCOL_REQUEST_AVAILABLE;
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_LEGACY, &snapshot);

    app_protocol_arbiter_copy(&arbiter, &out);
    assert(out.kind == PROTOCOL_KIND_NONE);
    assert(out.cc_attached == 1U);
    assert(out.cc_orientation == 1U);
}

static void test_pd_cc_detach_clears_hidden_passive_legacy_qc_available(void)
{
    app_protocol_arbiter_t arbiter;
    protocol_snapshot_t snapshot;
    protocol_snapshot_t out;

    app_protocol_arbiter_init(&arbiter);

    protocol_snapshot_reset(&snapshot);
    protocol_snapshot_set_cc_orientation(&snapshot, 1U);
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_PD, &snapshot);

    protocol_snapshot_set_legacy(&snapshot, PROTOCOL_KIND_QC, 9000, 0);
    snapshot.request_state = PROTOCOL_REQUEST_AVAILABLE;
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_LEGACY, &snapshot);

    protocol_snapshot_reset(&snapshot);
    app_protocol_arbiter_publish(&arbiter, APP_PROTOCOL_SOURCE_PD, &snapshot);

    app_protocol_arbiter_copy(&arbiter, &out);
    assert(out.kind == PROTOCOL_KIND_NONE);
    assert(out.cc_attached == 0U);
    assert(out.cc_orientation == 0U);
}

int main(void)
{
    test_pd_none_does_not_clear_legacy_qc();
    test_pd_snapshot_has_priority_over_legacy_qc();
    test_legacy_none_clears_legacy_when_pd_is_absent();
    test_pd_detach_after_active_contract_does_not_restore_stale_legacy();
    test_cc_attached_pd_path_hides_passive_legacy_other();
    test_cc_orientation_alone_hides_passive_legacy_other();
    test_cc_attached_pd_path_hides_passive_legacy_qc_available();
    test_pd_cc_detach_clears_hidden_passive_legacy_qc_available();
    return 0;
}
