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

    //presets menu
    STAN_MENU_PWM,
    STAN_EDYCJA_WARTOSCI_PWM,       //PAR1
    STAN_EDYCJA_WARTOSCI_PWM2,      //PAR2
    STAN_EDYCJA_WARTOSCI_PWM3,      //PAR3
    PRESET_EDIT_STATE,

    //modulations menu(lfo,auto pan,...)
    MOD_MENU_STATE,

    //EQ menu
    EQ_MENU_STATE,
    PWM5_EDIT_STATE,                //VOL L
    PWM6_EDIT_STATE,                //VOL R
    PWM7_EDIT_STATE,                //eq pwm

    //MIDI MODE MENU
    MIDI_MENU_STATE,

    DISTROTION_MENU_STATE,
    PWM4_EDIT_STATE,        //JEDNO PWM DLA OBU KANAŁÓW

} TypStanu;

typedef struct {
    const uint8_t* data;
    uint8_t len;
} FontResource;

extern const FontResource FX_presets[] PROGMEM;
const char* const FX_names[] PROGMEM;

extern const uint8_t char_ARROW[] ;
extern const uint8_t font_ON[] ;
extern const uint8_t font_OFF[] ;
extern const uint8_t font_WSTECZ[] PROGMEM;

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_write(uint8_t data);

void send_cmd(uint8_t cmd);
void oled_init(void);
void buffer_clear();


void draw_to_buffer(uint8_t row, uint8_t col, const uint8_t* data, uint8_t len, uint8_t is_progmem);
void print(uint8_t row, uint8_t col, const char* str);
void update_display();

void draw_pwm_to_buffer(uint8_t row, uint8_t col, uint8_t val);
void draw_menu(int8_t state);

void draw_params_menu(int8_t wybrana, int8_t pwm_val,
                        int8_t pwm_val2, int8_t pwm_val3,
                        uint8_t aktualny_stan, uint8_t sw,
                        uint8_t preset_id);
void draw_EQ_menu(uint8_t wybrana,uint8_t pwm_val, uint8_t pwm_val2,uint8_t pwm_val3, uint8_t aktualny_stan);


extern const uint8_t font_PAR1[] PROGMEM;
extern const uint8_t font_PAR2[] PROGMEM;
extern const uint8_t font_PAR3[] PROGMEM;
#endif
