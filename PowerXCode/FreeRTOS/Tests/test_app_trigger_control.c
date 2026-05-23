#include <assert.h>

#include "app_trigger_control.h"
#include "service_legacy_charge.h"

static int g_last_pd_mv;
static int g_last_legacy_mv;
static legacy_protocol_t g_last_legacy_protocol;
static int g_pd_calls;
static int g_legacy_calls;

void service_pd_set_preferred_voltage_mv(int32_t target_mv)
{
    g_last_pd_mv = (int)target_mv;
    g_pd_calls++;
}

void service_legacy_charge_request_voltage_mv(legacy_protocol_t protocol, int32_t target_mv)
{
    g_last_legacy_protocol = protocol;
    g_last_legacy_mv = (int)target_mv;
    g_legacy_calls++;
}

static void reset_counters(void)
{
    g_last_pd_mv = 0;
    g_last_legacy_mv = 0;
    g_last_legacy_protocol = LEGACY_PROTOCOL_NONE;
    g_pd_calls = 0;
    g_legacy_calls = 0;
}

static void test_pd_trigger_uses_pd_service(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
        .contract_mv = 5000,
        .contract_ma = 3000,
    };

    reset_counters();
    ui_model_init(&state);
    ui_model_trigger_next(&state);

    app_trigger_control_apply_auto(&state, &protocol);

    assert(g_pd_calls == 1);
    assert(g_last_pd_mv == 9000);
    assert(g_legacy_calls == 0);
}

static void test_qc_trigger_uses_legacy_service(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_QC,
        .contract_mv = 9000,
    };

    reset_counters();
    ui_model_init(&state);
    ui_model_trigger_next(&state);
    ui_model_trigger_next(&state);

    app_trigger_control_apply_auto(&state, &protocol);

    assert(g_pd_calls == 0);
    assert(g_legacy_calls == 1);
    assert(g_last_legacy_protocol == LEGACY_PROTOCOL_QC2);
    assert(g_last_legacy_mv == 12000);
}

static void test_qc_non_fixed_target_uses_qc3(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_QC,
        .contract_mv = 5000,
    };

    reset_counters();
    ui_model_init(&state);
    ui_model_trigger_next(&state);
    ui_model_trigger_next(&state);
    ui_model_trigger_next(&state);

    app_trigger_control_apply_auto(&state, &protocol);

    assert(g_pd_calls == 0);
    assert(g_legacy_calls == 1);
    assert(g_last_legacy_protocol == LEGACY_PROTOCOL_QC3);
    assert(g_last_legacy_mv == 15000);
}

static void test_no_protocol_defaults_to_pd_preference(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };

    reset_counters();
    ui_model_init(&state);

    app_trigger_control_apply_auto(&state, &protocol);

    assert(g_pd_calls == 1);
    assert(g_last_pd_mv == 5000);
    assert(g_legacy_calls == 0);
}

static void test_other_protocol_does_not_assume_qc_in_auto_trigger(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_OTHER,
        .request_state = PROTOCOL_REQUEST_AVAILABLE,
    };

    reset_counters();
    ui_model_init(&state);
    ui_model_trigger_next(&state);

    app_trigger_control_apply_auto(&state, &protocol);

    assert(g_pd_calls == 1);
    assert(g_last_pd_mv == 9000);
    assert(g_legacy_calls == 0);
}

static void test_qc_page_forces_legacy_qc_request(void)
{
    ui_model_state_t state;

    reset_counters();
    ui_model_init(&state);
    state.page = UI_PAGE_QC;
    ui_model_qc_next(&state);

    app_trigger_control_apply_qc(&state, 0);

    assert(g_pd_calls == 0);
    assert(g_legacy_calls == 1);
    assert(g_last_legacy_protocol == LEGACY_PROTOCOL_QC2);
    assert(g_last_legacy_mv == 9000);
}

static void test_qc_page_uses_qc3_for_15v_target(void)
{
    ui_model_state_t state;

    reset_counters();
    ui_model_init(&state);
    state.page = UI_PAGE_QC;
    ui_model_qc_next(&state);
    ui_model_qc_next(&state);
    ui_model_qc_next(&state);

    app_trigger_control_apply_qc(&state, 0);

    assert(g_pd_calls == 0);
    assert(g_legacy_calls == 1);
    assert(g_last_legacy_protocol == LEGACY_PROTOCOL_QC3);
    assert(g_last_legacy_mv == 15000);
}

static void test_qc_page_uses_pd_service_when_pd_contract_is_active(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
        .contract_mv = 5000,
        .contract_ma = 3000,
        .request_state = PROTOCOL_REQUEST_READY,
    };

    reset_counters();
    ui_model_init(&state);
    state.page = UI_PAGE_QC;
    ui_model_qc_next(&state);

    app_trigger_control_apply_qc(&state, &protocol);

    assert(g_pd_calls == 1);
    assert(g_last_pd_mv == 9000);
    assert(g_legacy_calls == 0);
}

static void test_pdo_page_pd_request_uses_fine_target(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
        .pps_present = 1U,
        .pps_min_mv = 3300,
        .pps_max_mv = 21000,
    };

    reset_counters();
    ui_model_init(&state);
    state.page = UI_PAGE_PDO;
    ui_model_pdo_target_next(&state);

    app_trigger_control_apply_pd(&state, &protocol);

    assert(g_pd_calls == 1);
    assert(g_last_pd_mv == 5020);
    assert(g_legacy_calls == 0);
}

int main(void)
{
    test_pd_trigger_uses_pd_service();
    test_qc_trigger_uses_legacy_service();
    test_qc_non_fixed_target_uses_qc3();
    test_no_protocol_defaults_to_pd_preference();
    test_other_protocol_does_not_assume_qc_in_auto_trigger();
    test_qc_page_forces_legacy_qc_request();
    test_qc_page_uses_qc3_for_15v_target();
    test_qc_page_uses_pd_service_when_pd_contract_is_active();
    test_pdo_page_pd_request_uses_fine_target();
    return 0;
}
