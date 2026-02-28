//
// Created by Tomasz on 28.01.2026.
//

#ifndef OLEDMENUGUI_H
#define OLEDMENUGUI_H

#include <avr/pgmspace.h>

#define OLED_ADDR  0x78

typedef enum {
    //main menu
    STAN_MENU_GLOWNE,
    PWM5_STATE,                     //VOL
    PWM5_EDIT_STATE,

    //fx menu
    STAN_MENU_PWM,
    STAN_EDYCJA_WARTOSCI_PWM,       //PAR1
    STAN_EDYCJA_WARTOSCI_PWM2,      //PAR2
    STAN_EDYCJA_WARTOSCI_PWM3,      //PAR3
    PWM4_STATE,                     //DISTORTION GAIN
    PWM4_EDIT_STATE,                //OUT1: 0/1 OUT2:0/1 OUT3: 0/1
    PRESET_EDIT_STATE ,

    //modulations menu(lfo,auto pan,...)
    MOD_MENU_STATE,

    //EQ menu
    EQ_MENU_STATE

} TypStanu;

typedef struct {
    const uint8_t* data;
    uint8_t len;
} FontResource;

extern const FontResource FX_presets[] PROGMEM;

extern const unsigned char epd_bitmap_logo1[] PROGMEM;
extern const uint8_t char_ARROW[] ;
extern const uint8_t font_ON[] ;
extern const uint8_t font_OFF[] ;
extern const uint8_t font_WSTECZ[] PROGMEM;
extern const uint8_t cyfry[10][5] PROGMEM;

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_write(uint8_t data);

void send_cmd(uint8_t cmd);
void oled_init(void);
void buffer_clear();


void draw_to_buffer(uint8_t row, uint8_t col, const uint8_t* data, uint8_t len, uint8_t is_progmem);


void rysuj_wartosc_pwm_to_buffer(uint8_t row, uint8_t col, uint8_t val);
void draw_menu(int8_t state);

void draw_params_menu_1(int8_t wybrana, int8_t pwm_val,
                        int8_t pwm_val2, int8_t pwm_val3,
                        uint8_t aktualny_stan, uint8_t sw,
                        uint8_t preset_id);

extern const uint8_t font_PAR1[] PROGMEM;
extern const uint8_t font_PAR2[] PROGMEM;
extern const uint8_t font_PAR3[] PROGMEM;
#endif
