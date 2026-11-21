#include "room.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define LIGHT_COUNT 4
#define SAMPLES 16

static const uint pins[LIGHT_COUNT] = {
    40, 41, 42, 43
};

void light_init(void) {
    adc_init();

    for (int i = 0; i < LIGHT_COUNT; i++) {
        adc_gpio_init(pins[i]);
    }
}

void light_read(uint16_t light[4]) {
    for (int ch = 0; ch < LIGHT_COUNT; ch++) {
        adc_select_input(ch);

        sleep_us(10);
        (void)adc_read();

        uint32_t sum = 0;

        for (int n = 0; n < SAMPLES; n++) {
            sum += adc_read();
        }

        light[ch] = sum / SAMPLES;
    }
}
