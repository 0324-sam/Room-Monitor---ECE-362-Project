#ifndef ROOM_H
#define ROOM_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ALERT_NONE = 0,
    ALERT_TEMP,
    ALERT_HUMIDITY,
    ALERT_DARK,
    ALERT_SENSOR
} alert_t;

void display_init(void);
void display_update(float temp, float humidity,
                    const uint16_t light[4],
                    alert_t alert, bool sensor_ok);

bool bme280_init(void);
bool bme280_read(float *temp, float *humidity);

void light_init(void);
void light_read(uint16_t light[4]);

void alarm_init(void);
void alarm_play(alert_t alert);

#endif
