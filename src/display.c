#include "room.h"
#include "room_config.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LCD_SPI ROOM_LCD_SPI
#define LCD_CS ROOM_LCD_CS
#define LCD_SCK ROOM_LCD_SCK
#define LCD_MOSI ROOM_LCD_MOSI
#define LCD_DC ROOM_LCD_DC
#define LCD_RST ROOM_LCD_RST

#define LCD_W 320
#define LCD_H 240
#define SCALE 2

#define BLACK 0x0000
#define WHITE 0xFFFF
#define CYAN  0x07FF
#define GREEN 0x07E0
#define RED   0xF800
#define YELLOW 0xFFE0

static const uint8_t digits[10][5] = {
    {0x3E,0x51,0x49,0x45,0x3E},
    {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},
    {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30},
    {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},
    {0x06,0x49,0x49,0x29,0x1E}
};

static const uint8_t letters[26][5] = {
    {0x7E,0x11,0x11,0x11,0x7E},
    {0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41},
    {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},
    {0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},
    {0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E},
    {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},
    {0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},
    {0x61,0x51,0x49,0x45,0x43}
};

static void lcd_cmd(uint8_t cmd) {
    gpio_put(LCD_DC, 0);
    gpio_put(LCD_CS, 0);
    spi_write_blocking(LCD_SPI, &cmd, 1);
    gpio_put(LCD_CS, 1);
}

static void lcd_data(const uint8_t *data, size_t len) {
    gpio_put(LCD_DC, 1);
    gpio_put(LCD_CS, 0);
    spi_write_blocking(LCD_SPI, data, len);
    gpio_put(LCD_CS, 1);
}

static void lcd_window(uint16_t x0, uint16_t y0,
                       uint16_t x1, uint16_t y1) {
    uint8_t b[4];

    lcd_cmd(0x2A);
    b[0] = x0 >> 8;
    b[1] = x0 & 255;
    b[2] = x1 >> 8;
    b[3] = x1 & 255;
    lcd_data(b, 4);

    lcd_cmd(0x2B);
    b[0] = y0 >> 8;
    b[1] = y0 & 255;
    b[2] = y1 >> 8;
    b[3] = y1 & 255;
    lcd_data(b, 4);
}

static void lcd_fill(int x, int y, int w, int h,
                     uint16_t color) {
    uint8_t buffer[64];

    if (w <= 0 || h <= 0) return;

    for (int i = 0; i < 32; i++) {
        buffer[i * 2] = color >> 8;
        buffer[i * 2 + 1] = color & 255;
    }

    lcd_window(x, y, x + w - 1, y + h - 1);
    lcd_cmd(0x2C);

    gpio_put(LCD_DC, 1);
    gpio_put(LCD_CS, 0);

    int pixels = w * h;
    while (pixels > 0) {
        int n = pixels > 32 ? 32 : pixels;
        spi_write_blocking(LCD_SPI, buffer, n * 2);
        pixels -= n;
    }

    gpio_put(LCD_CS, 1);
}

static void get_glyph(char c, uint8_t out[5]) {
    memset(out, 0, 5);

    if (c >= '0' && c <= '9') {
        memcpy(out, digits[c - '0'], 5);
    } else if (c >= 'A' && c <= 'Z') {
        memcpy(out, letters[c - 'A'], 5);
    } else if (c == ':') {
        out[1] = out[2] = 0x36;
    } else if (c == '.') {
        out[1] = out[2] = 0x60;
    } else if (c == '-') {
        out[0] = out[1] = out[2] =
        out[3] = out[4] = 0x08;
    } else if (c == '%') {
        out[0] = 0x63;
        out[1] = 0x13;
        out[2] = 0x08;
        out[3] = 0x64;
        out[4] = 0x63;
    }
}

static void lcd_char(int x, int y, char c,
                     uint16_t color) {
    uint8_t glyph[5];
    uint8_t pixels[6 * SCALE * 8 * SCALE * 2];
    int pos = 0;

    get_glyph(c, glyph);

    for (int py = 0; py < 8 * SCALE; py++) {
        for (int px = 0; px < 6 * SCALE; px++) {
            int row = py / SCALE;
            int col = px / SCALE;

            bool on = col < 5 && row < 7 &&
                      ((glyph[col] >> row) & 1);

            uint16_t pixel = on ? color : BLACK;

            pixels[pos++] = pixel >> 8;
            pixels[pos++] = pixel & 255;
        }
    }

    lcd_window(x, y,
               x + 6 * SCALE - 1,
               y + 8 * SCALE - 1);
    lcd_cmd(0x2C);
    lcd_data(pixels, sizeof(pixels));
}

static void lcd_print(int x, int y,
                      const char *str, uint16_t color) {
    while (*str && x + 6 * SCALE <= LCD_W) {
        lcd_char(x, y, *str++, color);
        x += 6 * SCALE;
    }
}

void display_init(void) {
    spi_init(LCD_SPI, 8000000);
    spi_set_format(LCD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(LCD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI, GPIO_FUNC_SPI);

    gpio_init(LCD_CS);
    gpio_init(LCD_DC);
    gpio_init(LCD_RST);

    gpio_set_dir(LCD_CS, GPIO_OUT);
    gpio_set_dir(LCD_DC, GPIO_OUT);
    gpio_set_dir(LCD_RST, GPIO_OUT);

    gpio_put(LCD_CS, 1);
    gpio_put(LCD_DC, 1);
    gpio_put(LCD_RST, 1);

    sleep_ms(10);
    gpio_put(LCD_RST, 0);
    sleep_ms(20);
    gpio_put(LCD_RST, 1);
    sleep_ms(120);

    lcd_cmd(0x01);
    sleep_ms(150);

    lcd_cmd(0x28);

    uint8_t format = 0x55;
    lcd_cmd(0x3A);
    lcd_data(&format, 1);

    uint8_t rotation = 0x28;
    lcd_cmd(0x36);
    lcd_data(&rotation, 1);

    lcd_cmd(0x11);
    sleep_ms(120);

    lcd_cmd(0x29);
    sleep_ms(50);

    lcd_fill(0, 0, LCD_W, LCD_H, BLACK);
    lcd_print(8, 8, "ROOM MONITOR", CYAN);
}

void display_update(float temp, float humidity,
                    const uint16_t light[4],
                    alert_t alert, bool sensor_ok) {
    char line[40];

    lcd_fill(0, 38, LCD_W, LCD_H - 38, BLACK);

    if (sensor_ok) {
        int t = (int)(temp * 10.0f +
                (temp >= 0 ? 0.5f : -0.5f));
        int hum = (int)(humidity * 10.0f + 0.5f);

        snprintf(line, sizeof(line),
                 "TEMP: %s%d.%d C",
                 t < 0 ? "-" : "",
                 abs(t) / 10, abs(t) % 10);
        lcd_print(8, 40, line, WHITE);

        snprintf(line, sizeof(line),
                 "HUM: %d.%d %%",
                 hum / 10, hum % 10);
        lcd_print(8, 72, line, WHITE);
    } else {
        lcd_print(8, 40, "SENSOR ERROR", RED);
    }

    uint32_t total =
        (uint32_t)light[0] + light[1] +
        light[2] + light[3];
    uint32_t average = total / 4;

    snprintf(line, sizeof(line),
             "LIGHT: %lu", (unsigned long)average);
    lcd_print(8, 104, line, WHITE);

    snprintf(line, sizeof(line),
             "L1:%u L2:%u",
             (unsigned)light[0], (unsigned)light[1]);
    lcd_print(8, 136, line, CYAN);

    snprintf(line, sizeof(line),
             "L3:%u L4:%u",
             (unsigned)light[2], (unsigned)light[3]);
    lcd_print(8, 168, line, CYAN);

    const char *message = "STATUS: OK";
    uint16_t color = GREEN;

    switch (alert) {
        case ALERT_TEMP:
            message = "WARN: HOT";
            color = RED;
            break;
        case ALERT_HUMIDITY:
            message = "WARN: HUMID";
            color = YELLOW;
            break;
        case ALERT_DARK:
            message = "WARN: DARK";
            color = YELLOW;
            break;
        case ALERT_SENSOR:
            message = "SENSOR ERROR";
            color = RED;
            break;
        default:
            break;
    }

    lcd_print(8, 200, message, color);
}
