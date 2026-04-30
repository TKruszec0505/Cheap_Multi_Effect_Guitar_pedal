//
// Created by Tomasz on 28.01.2026.
//

#ifndef OLEDMENUGUI_H
#define OLEDMENUGUI_H

#include <avr/pgmspace.h>

#define OLED_ADDR  0x78

typedef enum {
    /*
    this enum is for keeping track of current state of menu,
    so we know what to display and how to react on button presses.
    Each state corresponds to a different menu or submenu in the OLED display. For example: 
    - STAN_MENU_GLOWNE: main menu with options like "VOL EQ", "MIDI", "BLINK", "PRESETS".
    - STAN_MENU_PWM: submenu for adjusting PWM settings.
    - EQ_MENU_STATE: submenu for adjusting EQ settings.
    - MIDI_MENU_STATE: submenu for MIDI settings.
    */
    //main menu
    MAIN_MANU_STATE,

    //presets menu
    PARAMS_MENU_STATE,
    PAR_ONE_EDIT_STATE,       //PAR1
    PAR_TWO_EDIT_STATE,      //PAR2
    PAR_THREE_EDIT_STATE,      //PAR3
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
    PWM4_EDIT_STATE,

} State;

/*
This array is stored in program memory (Flash) instead of RAM,
which is a common practice in AVR programming to save RAM space.
Each string in the array represents the name of a preset or effect that can 
be selected in the menu. The PROGMEM keyword indicates that the data is
stored in Flash memory, and you would typically use functions like
pgm_read_byte to read these strings when you need to display them on
the OLED or use them in your program logic.
*/
const char* const FX_names[] PROGMEM;


void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_write(uint8_t data);

void send_cmd(uint8_t cmd);
void oled_init(void);
void buffer_clear();

/*
This function is responsible for drawing a sequence of bytes (representing a character or graphic) into the display buffer.
- row: The row on the OLED display where the data should be drawn.
- col: The starting column on the OLED display where the data should be drawn.
- data: A pointer to the byte array that contains the pixel data to be drawn.   
- len: The number of bytes to draw from the data array.
- is_progmem: A flag indicating whether the data is stored in program memory (PROGMEM) or RAM. If this flag is set, the function will use pgm_read_byte to read the data; otherwise, it will read directly from RAM.
The function checks the boundaries to ensure that it does not attempt to draw outside the limits of the display. It then writes the pixel data into the appropriate location in the buffer, which will later be sent to the OLED display.
*/
void draw_to_buffer(uint8_t row, uint8_t col, const uint8_t* data, uint8_t len, uint8_t is_progmem);

//This function is basiclly a mix of clasic printf and set_cursor. It takes a position and string and draws it on the display. It handles the conversion of characters to their corresponding byte patterns using the OLED_ASCII_FONT array stored in PROGMEM. The function also ensures that it does not write beyond the display boundaries.
void print(uint8_t row, uint8_t col, const char* str);

//This function is basic for loop that takes 5bit starting with fixed value inside function(starts with zero bc this is for displaying ints) and iterate through them to display our given value.
void index_print(uint8_t row, uint8_t col, uint8_t i);

void update_display();

void draw_pwm_to_buffer(uint8_t row, uint8_t col, uint8_t val);

//menus

void draw_menu(int8_t state);
void draw_params_menu(int8_t ch, int8_t pwm_val,
                        int8_t pwm_val2, int8_t pwm_val3,
                        uint8_t cur_state, uint8_t sw,
                        uint8_t preset_id);
void draw_EQ_menu(uint8_t ch,uint8_t pwm_val, uint8_t pwm_val2,uint8_t pwm_val3, uint8_t cur_state);
void draw_MIDI_menu(uint8_t ch, uint8_t sw);

#endif
