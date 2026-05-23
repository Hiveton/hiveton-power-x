#include <assert.h>
#include <stdint.h>

#include "bsp_keys.h"

#define TEST_KEY_BTN1 0x01U
#define TEST_KEY_BTN2 0x02U
#define TEST_KEY_BTN3 0x04U
#define TEST_RAW_IDLE (TEST_KEY_BTN1 | TEST_KEY_BTN3)

static void test_idle_levels_are_not_pressed(void)
{
    assert(bsp_keys_active_mask_from_levels(0x05U) == 0x00U);
}

static void test_btn1_and_btn3_are_active_low(void)
{
    assert(bsp_keys_active_mask_from_levels(0x04U) == 0x01U);
    assert(bsp_keys_active_mask_from_levels(0x01U) == 0x04U);
}

static void test_btn2_is_active_high(void)
{
    assert(bsp_keys_active_mask_from_levels(0x07U) == 0x02U);
}

static void test_irq_events_are_latched_until_consumed(void)
{
    bsp_keys_init();
    bsp_keys_record_irq_mask(TEST_KEY_BTN1);
    bsp_keys_record_irq_mask(TEST_KEY_BTN3);
    assert(bsp_keys_consume_irq_mask() == (TEST_KEY_BTN1 | TEST_KEY_BTN3));
    assert(bsp_keys_consume_irq_mask() == 0U);
}

static void test_btn1_poll_fallback_emits_short_event_on_release(void)
{
    bsp_keys_event_t event;

    bsp_keys_init();
    bsp_keys_poll(&event);
    assert(event.btn1_short == 0U);
    assert(event.btn2_short == 0U);
    assert(event.btn3_short == 0U);

    bsp_keys_mock_set_raw_high_mask(TEST_KEY_BTN3);
    bsp_keys_poll(&event);
    assert(event.btn1_short == 0U);

    bsp_keys_mock_set_raw_high_mask(TEST_RAW_IDLE);
    bsp_keys_poll(&event);
    assert(event.btn1_short == 1U);
    assert(event.btn2_short == 0U);
    assert(event.btn3_short == 0U);
}

static void test_btn2_irq_emits_short_event(void)
{
    bsp_keys_event_t event;

    bsp_keys_init();
    bsp_keys_record_irq_mask(TEST_KEY_BTN2);
    bsp_keys_poll(&event);
    assert(event.btn1_short == 0U);
    assert(event.btn2_short == 1U);
    assert(event.btn3_short == 0U);
}

static void test_btn2_irq_press_release_emits_short_event(void)
{
    bsp_keys_event_t event;

    bsp_keys_init();
    bsp_keys_mock_set_raw_high_mask(TEST_RAW_IDLE | TEST_KEY_BTN2);
    bsp_keys_record_irq_mask(TEST_KEY_BTN2);
    bsp_keys_poll(&event);
    assert(event.btn1_short == 0U);
    assert(event.btn2_short == 0U);
    assert(event.btn3_short == 0U);

    bsp_keys_mock_set_raw_high_mask(TEST_RAW_IDLE);
    bsp_keys_poll(&event);
    assert(event.btn1_short == 0U);
    assert(event.btn2_short == 1U);
    assert(event.btn3_short == 0U);
}

static void test_btn3_irq_emits_short_event(void)
{
    bsp_keys_event_t event;

    bsp_keys_init();
    bsp_keys_record_irq_mask(TEST_KEY_BTN3);
    bsp_keys_poll(&event);
    assert(event.btn1_short == 0U);
    assert(event.btn2_short == 0U);
    assert(event.btn3_short == 1U);
}

static void test_btn3_irq_press_release_emits_short_event(void)
{
    bsp_keys_event_t event;

    bsp_keys_init();
    bsp_keys_mock_set_raw_high_mask(TEST_KEY_BTN1);
    bsp_keys_record_irq_mask(TEST_KEY_BTN3);
    bsp_keys_poll(&event);
    assert(event.btn1_short == 0U);
    assert(event.btn2_short == 0U);
    assert(event.btn3_short == 0U);

    bsp_keys_mock_set_raw_high_mask(TEST_RAW_IDLE);
    bsp_keys_poll(&event);
    assert(event.btn1_short == 0U);
    assert(event.btn2_short == 0U);
    assert(event.btn3_short == 1U);
}

static void test_btn2_long_press_does_not_emit_short_event(void)
{
    bsp_keys_event_t event;
    uint8_t index;
    uint8_t saw_long;

    bsp_keys_init();
    bsp_keys_mock_set_raw_high_mask(TEST_RAW_IDLE | TEST_KEY_BTN2);
    bsp_keys_record_irq_mask(TEST_KEY_BTN2);
    saw_long = 0U;
    for (index = 0U; index < 20U; ++index)
    {
        bsp_keys_poll(&event);
        assert(event.btn2_short == 0U);
        if (event.btn2_long != 0U)
        {
            saw_long = 1U;
        }
    }

    assert(saw_long == 1U);

    bsp_keys_mock_set_raw_high_mask(TEST_RAW_IDLE);
    bsp_keys_poll(&event);
    assert(event.btn2_short == 0U);
    assert(event.btn2_long == 0U);
}

int main(void)
{
    test_idle_levels_are_not_pressed();
    test_btn1_and_btn3_are_active_low();
    test_btn2_is_active_high();
    test_irq_events_are_latched_until_consumed();
    test_btn1_poll_fallback_emits_short_event_on_release();
    test_btn2_irq_emits_short_event();
    test_btn2_irq_press_release_emits_short_event();
    test_btn3_irq_emits_short_event();
    test_btn3_irq_press_release_emits_short_event();
    test_btn2_long_press_does_not_emit_short_event();
    return 0;
}
