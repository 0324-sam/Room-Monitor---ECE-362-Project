#ifndef ROOM_CONFIG_H
#define ROOM_CONFIG_H

// GPIO labels, NOT header positions. Matches the user's existing wiring.
#define ROOM_BME_I2C i2c0
#define ROOM_BME_SDA 0
#define ROOM_BME_SCL 1
#define ROOM_LCD_SPI spi0
#define ROOM_LCD_CS 17
#define ROOM_LCD_SCK 18
#define ROOM_LCD_MOSI 19
#define ROOM_LCD_DC 20
#define ROOM_LCD_RST 21
// TFT SDO is wired to GP16; this write-only display driver does not use it.
#define ROOM_SPEAKER_PIN 22

// 0 = integrated, 1 = LDR only, 2 = BME280 only, 3 = TFT simulated readings.
#ifndef ROOM_TEST_MODE
#define ROOM_TEST_MODE 0
#endif
// Leave disabled until a suitable speaker driver has been connected.
#ifndef ROOM_ENABLE_ALARM
#define ROOM_ENABLE_ALARM 0
#endif
#endif
