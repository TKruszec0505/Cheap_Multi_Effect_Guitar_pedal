//
// Created by Tomasz on 28.01.2026.
//

#ifndef OLEDMENUGUI_H
#define OLEDMENUGUI_H
#ifndef OLEDMENUGUI_H_
#define OLEDMENUGUI_H_

#include <avr/pgmspace.h>

#define OLED_ADDR  0x78

typedef enum {
    STAN_MENU_GLOWNE,
    STAN_MENU_PWM,
    STAN_EDYCJA_WARTOSCI_PWM
} TypStanu;

extern const unsigned char epd_bitmap_logo1[] PROGMEM;
extern const uint8_t char_ARROW[] PROGMEM;
extern const uint8_t font_ON[] PROGMEM;
extern const uint8_t font_OFF[] PROGMEM;
extern const uint8_t font_WSTECZ[] PROGMEM;
extern const uint8_t cyfry[10][5] PROGMEM;

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_write(uint8_t data);

void send_cmd(uint8_t cmd);
void oled_init(void);
void oled_clear(void);
void oled_set_cursor(uint8_t row, uint8_t col);

void draw(const uint8_t* d, uint8_t len);
void draw_from_progmem(const uint8_t* d, uint8_t len);
void oled_draw_bitmap(const uint8_t* bitmap);
void rysuj_wartosc_pwm(uint8_t val);
void draw_menu(int8_t state);
void draw_params_menu(int8_t wybrana, int8_t pwm_val, uint8_t edycja, uint8_t sw);
#endif
#endif //OLEDMENUGUI_H
