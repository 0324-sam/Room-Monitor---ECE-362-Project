#include "room.h"
#include "room_config.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define BME_I2C ROOM_BME_I2C
#define BME_SDA ROOM_BME_SDA
#define BME_SCL ROOM_BME_SCL

static uint8_t address = 0;
static bool ready = false;

static struct {
    uint16_t T1;
    int16_t T2, T3;
    uint8_t H1, H3;
    int16_t H2, H4, H5;
    int8_t H6;
} cal;

static uint16_t u16(const uint8_t *p) {
    return (uint16_t)p[0] |
           ((uint16_t)p[1] << 8);
}

static int16_t s12(uint16_t v) {
    if (v & 0x800) v |= 0xF000;
    return (int16_t)v;
}

static bool read_reg(uint8_t addr, uint8_t reg,
                     uint8_t *data, size_t len) {
    int w = i2c_write_timeout_us(
        BME_I2C, addr, &reg, 1, true, 20000);

    if (w != 1) return false;

    int r = i2c_read_timeout_us(
        BME_I2C, addr, data, len, false, 20000);

    return r == (int)len;
}

static bool write_reg(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    return i2c_write_timeout_us(
        BME_I2C, address, data, 2, false, 20000) == 2;
}

bool bme280_init(void) {
    ready = false;
    address = 0;

    i2c_init(BME_I2C, 100000);
    gpio_set_function(BME_SDA, GPIO_FUNC_I2C);
    gpio_set_function(BME_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(BME_SDA);
    gpio_pull_up(BME_SCL);

    sleep_ms(20);

    uint8_t id = 0;
    const uint8_t addresses[2] = {0x76, 0x77};

    for (int i = 0; i < 2; i++) {
        if (read_reg(addresses[i], 0xD0, &id, 1)
            && id == 0x60) {
            address = addresses[i];
            break;
        }
    }

    if (address == 0) return false;

    // Reset first so configuration is applied in sleep mode, including after
    // an MCU-only reset while the sensor remains powered.
    if (!write_reg(0xE0, 0xB6)) return false;
    sleep_ms(5);
    bool calibration_ready = false;
    for (int attempt = 0; attempt < 50; attempt++) {
        uint8_t status = 0;
        if (!read_reg(address, 0xF3, &status, 1)) return false;
        if (!(status & 0x01)) {
            calibration_ready = true;
            break;
        }
        sleep_ms(2);
    }
    if (!calibration_ready) return false;


    uint8_t a[26];
    uint8_t h[7];

    if (!read_reg(address, 0x88, a, 26))
        return false;

    if (!read_reg(address, 0xE1, h, 7))
        return false;

    cal.T1 = u16(&a[0]);
    cal.T2 = (int16_t)u16(&a[2]);
    cal.T3 = (int16_t)u16(&a[4]);

    cal.H1 = a[25];
    cal.H2 = (int16_t)u16(&h[0]);
    cal.H3 = h[2];

    uint16_t raw_h4 =
        ((uint16_t)h[3] << 4) | (h[4] & 0x0F);
    uint16_t raw_h5 =
        ((uint16_t)h[5] << 4) | (h[4] >> 4);

    cal.H4 = s12(raw_h4);
    cal.H5 = s12(raw_h5);
    cal.H6 = (int8_t)h[6];

    if (cal.T1 == 0) return false;

    if (!write_reg(0xF2, 0x01)) return false;
    if (!write_reg(0xF5, 0x00)) return false;
    if (!write_reg(0xF4, 0x27)) return false;

    sleep_ms(100);
    ready = true;
    return true;
}

bool bme280_read(float *temp, float *humidity) {
    if (!ready || !temp || !humidity)
        return false;

    uint8_t data[8];

    if (!read_reg(address, 0xF7, data, 8))
        return false;

    int32_t adc_t =
        ((int32_t)data[3] << 12) |
        ((int32_t)data[4] << 4) |
        (data[5] >> 4);

    int32_t adc_h =
        ((int32_t)data[6] << 8) | data[7];

    if (adc_t == 0x80000 || adc_h == 0x8000)
        return false;

    double var1 =
        ((double)adc_t / 16384.0 -
         (double)cal.T1 / 1024.0) * cal.T2;

    double x =
        (double)adc_t / 131072.0 -
        (double)cal.T1 / 8192.0;

    double var2 = x * x * cal.T3;
    double t_fine = var1 + var2;

    double t = t_fine / 5120.0;

    double v = t_fine - 76800.0;

    v = (adc_h -
        (cal.H4 * 64.0 + cal.H5 / 16384.0 * v)) *
        (cal.H2 / 65536.0 *
        (1.0 + cal.H6 / 67108864.0 * v *
        (1.0 + cal.H3 / 67108864.0 * v)));

    v = v * (1.0 - cal.H1 * v / 524288.0);

    if (v > 100.0) v = 100.0;
    if (v < 0.0) v = 0.0;

    *temp = (float)t;
    *humidity = (float)v;

    return true;
}
