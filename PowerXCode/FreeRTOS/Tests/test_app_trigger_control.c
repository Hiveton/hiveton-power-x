#include <assert.h>

#include "app_trigger_control.h"

static int g_last_pd_mv;
static int g_last_legacy_mv;
static int g_last_legacy_protocol;
static int g_pd_calls;
static int g_legacy_calls;

void service_pd_set_preferred_voltage_mv(int32_t target_mv)
{
    g_last_pd_mv = (int)target_mv;
    g_pd_calls++;
}

void service_legacy_charge_request_voltage_mv(int protocol, int32_t target_mv)
{
    g_last_legacy_protocol = protocol;
    g_last_legacy_mv = (int)target_mv;
    g_legacy_calls++;
}

static void reset_counters(void)
{
    g_last_pd_mv = 0;
    g_last_legacy_mv = 0;
    g_last_legacy_protocol = 0;
    g_pd_calls = 0;
    g_legacy_calls = 0;
}

static void test_pd_trigger_uses_pd_service(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = { PROTOCOL_KIND_PD, 5000, 3000, 0U, 0 };

    reset_counters();
    ui_model_init(&state);
    ui_model_trigger_next(&state);

    app_trigger_control_apply(&state, &protocol);

    assert(g_pd_calls == 1);
    assert(g_last_pd_mv == 9000);
    assert(g_legacy_calls == 0);
}

static void test_qc_trigger_uses_legacy_service(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = { PROTOCOL_KIND_QC, 9000, 0, 0U, 0 };

    reset_counters();
    ui_model_init(&state);
    ui_model_trigger_next(&state);
    ui_model_trigger_next(&state);

    app_trigger_control_apply(&state, &protocol);

    assert(g_pd_calls == 0);
    assert(g_legacy_calls == 1);
    assert(g_last_legacy_protocol == 1);
    assert(g_last_legacy_mv == 12000);
}

static void test_no_protocol_defaults_to_pd_preference(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = { PROTOCOL_KIND_NONE, 0, 0, 0U, 0 };

    reset_counters();
    ui_model_init(&state);

    app_trigger_control_apply(&state, &protocol);

    assert(g_pd_calls == 1);
    assert(g_last_pd_mv == 5000);
    assert(g_legacy_calls == 0);
}

int main(void)
{
    test_pd_trigger_uses_pd_service();
    test_qc_trigger_uses_legacy_service();
    test_no_protocol_defaults_to_pd_preference();
    return 0;
}
