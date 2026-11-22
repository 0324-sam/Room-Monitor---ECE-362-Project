#include "room.h"
#include "room_config.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define TEMP_THRESHOLD 30.0f
#define HUMIDITY_THRESHOLD 70.0f
// Starting point only: calibrate for the actual LDRs and 1 kOhm dividers.
#define LIGHT_THRESHOLD 1000

// Inspect these in the debugger if USB serial is not connected.
volatile uint16_t debug_light[4];
volatile bool debug_sensor_ok;
volatile float debug_temperature;
volatile float debug_humidity;

static void publish_debug(const uint16_t light[4], bool ok, float t, float h)
{
    for (int i = 0; i < 4; i++) debug_light[i] = light[i];
    debug_sensor_ok = ok;
    debug_temperature = t;
    debug_humidity = h;
}

static void print_readings(const uint16_t light[4], bool ok, float t, float h,
                           alert_t alert)
{
    printf("BME_OK=%d T10=%d H10=%d L1=%u L2=%u L3=%u L4=%u ALERT=%d\n",
           (int)ok, (int)(t * 10), (int)(h * 10),
           (unsigned)light[0], (unsigned)light[1],
           (unsigned)light[2], (unsigned)light[3], (int)alert);
}

int main(void)
{
    // PlatformIO disables UART stdio and enables native USB stdio.
    // GP0/GP1 remain exclusively I2C pins, even before BME initialization.
    stdio_init_all();
    sleep_ms(1500);

#if ROOM_TEST_MODE == 1
    light_init();
    while (true) {
        uint16_t light[4];
        light_read(light);
        publish_debug(light, false, 0, 0);
        printf("LDR1=%u LDR2=%u LDR3=%u LDR4=%u\n",
               (unsigned)light[0], (unsigned)light[1],
               (unsigned)light[2], (unsigned)light[3]);
        sleep_ms(500);
    }
#elif ROOM_TEST_MODE == 2
    bool ok = bme280_init();
    while (true) {
        float t = 0, h = 0;
        uint16_t light[4] = {0};
        if (!ok) ok = bme280_init();
        if (ok) ok = bme280_read(&t, &h);
        publish_debug(light, ok, t, h);
        print_readings(light, ok, t, h, ok ? ALERT_NONE : ALERT_SENSOR);
        sleep_ms(1000);
    }
#elif ROOM_TEST_MODE == 3
    display_init();
    uint32_t step = 0;
    while (true) {
        uint32_t phase = step++ % 4;
        float t = phase == 1 ? 34.0f : 24.0f;
        float h = phase == 2 ? 78.0f : 45.0f;
        uint16_t light[4] = {2000, 2100, 2200, 2300};
        alert_t alert = ALERT_NONE;
        if (phase == 1) alert = ALERT_TEMP;
        if (phase == 2) alert = ALERT_HUMIDITY;
        if (phase == 3) {
            alert = ALERT_DARK;
            for (int i = 0; i < 4; i++) light[i] = 300;
        }
        display_update(t, h, light, alert, true);
        publish_debug(light, true, t, h);
        print_readings(light, true, t, h, alert);
        sleep_ms(2000);
    }
#elif ROOM_TEST_MODE == 0
    display_init();
    light_init();
#if ROOM_ENABLE_ALARM
    alarm_init();

    alarm_play(ALERT_TEMP);
    sleep_ms(500);

    alarm_play(ALERT_HUMIDITY);
    sleep_ms(500);

    alarm_play(ALERT_DARK);
    sleep_ms(500);
    
    alert_t previous_alert = ALERT_NONE;
    uint64_t last_alarm_ms = 0;
#endif
    bool ok = bme280_init();
    while (true) {
        float t = 0, h = 0;
        uint16_t light[4];
        if (!ok) ok = bme280_init();
        if (ok) ok = bme280_read(&t, &h);
        light_read(light);
        uint32_t total = 0;
        for (int i = 0; i < 4; i++) total += light[i];
        alert_t alert = ALERT_NONE;
        if (!ok) alert = ALERT_SENSOR;
        else if (t >= TEMP_THRESHOLD) alert = ALERT_TEMP;
        else if (h >= HUMIDITY_THRESHOLD) alert = ALERT_HUMIDITY;
        else if (total / 4 < LIGHT_THRESHOLD) alert = ALERT_DARK;
        publish_debug(light, ok, t, h);
        display_update(t, h, light, alert, ok);
        print_readings(light, ok, t, h, alert);
#if ROOM_ENABLE_ALARM
        uint64_t now = to_ms_since_boot(get_absolute_time());
        if (alert != ALERT_NONE &&
            (alert != previous_alert || now - last_alarm_ms >= 10000)) {
            alarm_play(alert);
            last_alarm_ms = to_ms_since_boot(get_absolute_time());
        }
        previous_alert = alert;
#endif
        sleep_ms(1000);
    }
#else
#error "ROOM_TEST_MODE must be 0, 1, 2, or 3"
#endif
}
