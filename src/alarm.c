#include "room.h"
#include "room_config.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define SPEAKER_PIN ROOM_SPEAKER_PIN
#define PWM_DIVIDER 64

static uint slice;
static uint channel;

void alarm_init(void)
{
    gpio_init(SPEAKER_PIN);
    gpio_put(SPEAKER_PIN, 1);
    gpio_set_dir(SPEAKER_PIN, GPIO_OUT);

    slice = pwm_gpio_to_slice_num(SPEAKER_PIN);
    channel = pwm_gpio_to_channel(SPEAKER_PIN);

    pwm_config config = pwm_get_default_config();

    pwm_config_set_clkdiv(&config, (float)PWM_DIVIDER);
    pwm_config_set_wrap(&config, 5000);

    pwm_config_set_output_polarity(
        &config,
        channel == PWM_CHAN_A,
        channel == PWM_CHAN_B
    );

    pwm_init(slice, &config, false);
    pwm_set_chan_level(slice, channel, 0);

    gpio_set_function(SPEAKER_PIN, GPIO_FUNC_PWM);
    pwm_set_enabled(slice, true);
}

static void tone(uint32_t hz, uint32_t duration_ms) {
    if (hz == 0) {
        pwm_set_chan_level(slice, channel, 0);
        sleep_ms(duration_ms);
        return;
    }

    uint32_t sysclk = clock_get_hz(clk_sys);
    uint32_t top = sysclk / (PWM_DIVIDER * hz) - 1;

    if (top > 65535) top = 65535;
    if (top < 2) top = 2;

    pwm_set_chan_level(slice, channel, 0);
    pwm_set_counter(slice, 0);
    pwm_set_wrap(slice, top);
    pwm_set_chan_level(slice, channel, top / 2);

    sleep_ms(duration_ms);
    pwm_set_chan_level(slice, channel, 0);
}

void alarm_play(alert_t alert) {
    switch (alert) {
        case ALERT_TEMP:
            tone(1000, 180);
            tone(0, 60);
            tone(1400, 180);
            break;

        case ALERT_HUMIDITY:
            tone(700, 140);
            tone(900, 140);
            break;

        case ALERT_DARK:
            tone(450, 250);
            break;

        case ALERT_SENSOR:
            tone(1200, 90);
            tone(0, 60);
            tone(1200, 90);
            tone(0, 60);
            tone(1200, 90);
            break;

        case ALERT_NONE:
        default:
            pwm_set_chan_level(slice, channel, 0);
            break;
    }
}
