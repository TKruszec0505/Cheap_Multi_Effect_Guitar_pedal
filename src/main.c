#include <avr/io.h>
#include <util/delay.h>
#include "OledMenuGui.h"
#include <avr/interrupt.h>
#include <stdbool.h>

#define LED_PIN PB1

//for mosfet output config
#define PRESET_OUT_1 PC0
#define PRESET_OUT_2 PC1
#define PRESET_OUT_3 PC2
#define PRESET_OUT_4 PC3

#define BAUD 31250
#define MYUBRR (F_CPU/16/BAUD-1)
#define BUFFER_SIZE 32

volatile uint8_t uart_buffer[BUFFER_SIZE];
volatile uint8_t head = 0;
volatile uint8_t tail = 0;

State current_state = MAIN_MANU_STATE;

int8_t main_menu = 0;
int8_t pwm_menu = 0;
int8_t mod_menu = 0;
int8_t op_midi_menu = 0;
int8_t op_eq_menu = 0;
int8_t op_dst_menu = 0;

int8_t par_one_val = 80;
int8_t par_two_val = 80;
int8_t par_three_val = 80;

int8_t vol_r_lvl = 80;
int8_t vol_l_lvl = 80;
int8_t eq_lvl = 80;

bool par_one_pwm_active = 1;
bool par_two_pwm_active = 1;
bool par_three_pwm_active = 1;

bool vol_r_active = 0;
bool vol_l_active = 0;
bool eq_active = 1;

bool midi_active = 0;

int8_t preset_id = 5; //5 bc its bypass(test fv-1 program)

uint8_t update = 1;

void uart_init(unsigned int baud) {
    uint16_t ubrr_value = (F_CPU / 16 / baud) - 1;
    UBRR0H = (uint8_t)(ubrr_value >> 8);
    UBRR0L = (uint8_t)ubrr_value;

    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);

    UCSR0C = (3 << UCSZ00);
}

ISR(USART_RX_vect) {
    uint8_t data = UDR0;
    uint8_t next_head = (head + 1) % BUFFER_SIZE;
    if (next_head != tail) {
        uart_buffer[head] = data;
        head = next_head;
    }
}

char cmd_buffer[BUFFER_SIZE];
uint8_t cmd_index = 0;

int16_t uart_get_char(void) {
    if (head == tail) return -1;    // empty
    uint8_t data = uart_buffer[tail];
    tail = (tail + 1) % BUFFER_SIZE;
    return data;
}

void blink() {
    TCCR1A &= ~(1 << COM1A1);
    for (uint8_t i = 0; i < 3; i++) {
        PORTB |= (1 << LED_PIN);
        _delay_ms(500);
        PORTB &= ~(1 << LED_PIN);
        _delay_ms(500);
    }
}

void set_preset(uint8_t id) {
    uint8_t val = id % 8;

    PORTC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2));

    if (!(val & (1 << 0))) PORTC |= (1 << PC0);
    if (!(val & (1 << 1))) PORTC |= (1 << PC1);
    if (!(val & (1 << 2))) PORTC |= (1 << PC2);
}
void setup_all_timers() {
    // Timer 1 (PB1, PB2) - Fast PWM 8-bit
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS10);

    // Timer 2 (PB3, PD3) - Fast PWM
    // COM2A1 (dla PB3) i COM2B1 (dla PD3) w jednym rejestrze
    TCCR2A = (1 << COM2A1) | (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS20);

    // Timer 0 (PD5, PD6) - Fast PWM (dla Vol L/R)
    TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);
    TCCR0B = (1 << CS01);
}

void upadte_par_one(uint8_t v) {
    OCR1A = (uint16_t) (((uint32_t) v * 255) / 100);
}

void upadte_par_two(uint8_t v) {
    OCR1B = (uint16_t) (((uint32_t) v * 255) / 100);
}

void update_par_three(uint8_t v) {
    //8bit
    OCR2A = (uint16_t) (((uint32_t) v * 255) / 100);
}

void update_vol_l(uint8_t v_l) {
    OCR0A = (uint8_t) (((uint32_t) v_l * 255) / 100);
}

void update_vol_r(uint8_t v_r) {
    OCR0B = (uint8_t) (((uint32_t) v_r * 255) / 100);
}

void update_eq(uint8_t v) {
    OCR2B = (uint8_t) (((uint32_t) v * 255) / 100);
}

int main(void) {
    //PWM: PB1 (OC1A), PB2 (OC1B), PB3 (OC2A)
    DDRB |= (1 << PB1) | (1 << PB2) | (1 << PB3);

    //enkodera i przycisku (PD2, PD5, PD6) z podciągnięciem
    DDRD &= ~((1 << PD2) | (1 << PD4) | (1 << PD7));
    PORTD |= (1 << PD2) | (1 << PD4) | (1 << PD7);

    // PPORT C init
    DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3);

    i2c_init();
    oled_init();

    uart_init(BAUD);
    sei();

    setup_all_timers();

    PORTC |= (1 << PRESET_OUT_4);
    set_preset(5);

    par_one_pwm_active = 1;

    draw_menu(main_menu);


    uint8_t stan_A_poprzedni = (PIND & (1 << PD7));

    while (1) {
        //obrot enkodera
        uint8_t stan_A = (PIND & (1 << PD7));
        if (stan_A != stan_A_poprzedni && stan_A == 0) {
            uint8_t w_prawo = (PIND & (1 << PD4));

            //main menu
            if (current_state == MAIN_MANU_STATE) {
                if (w_prawo) main_menu++;
                else main_menu--;
                if (main_menu > 3) main_menu = 0;
                if (main_menu < 0) main_menu = 3;
            }
            //presets menu
            else if (current_state == PARAMS_MENU_STATE) {
                if (w_prawo) pwm_menu++;
                else pwm_menu--;
                if (pwm_menu > 5) pwm_menu = 0;
                if (pwm_menu < 0) pwm_menu = 5;
            } else if (current_state == PAR_ONE_EDIT_STATE) {
                if (w_prawo && par_one_val < 100) par_one_val += 5;
                else if (!w_prawo && par_one_val > 0) par_one_val -= 5;
                if (par_one_pwm_active) upadte_par_one(par_one_val);
            } else if (current_state == PAR_TWO_EDIT_STATE) {
                if (w_prawo && par_two_val < 100) par_two_val += 5;
                else if (!w_prawo && par_two_val > 0) par_two_val -= 5;
                if (par_one_pwm_active) upadte_par_two(par_two_val);
            } else if (current_state == PAR_THREE_EDIT_STATE) {
                if (w_prawo && par_three_val < 100) par_three_val += 5;
                else if (!w_prawo && par_three_val > 0) par_three_val -= 5;
                if (par_one_pwm_active) update_par_three(par_three_val);
            } else if (current_state == PRESET_EDIT_STATE) {
                if (w_prawo) preset_id++;
                else preset_id--;
                if (preset_id > 15) preset_id = 0; //0-15(16 presets)
                if (preset_id < 0) preset_id = 15;
                if (preset_id > 7) (PORTC &= ~(1 << PRESET_OUT_4)); //0-7 from ROM 8-15 EEPROM(we are switching T1)
                else (PORTC |= (1 << PRESET_OUT_4));
                set_preset(preset_id);
            }
            //EQ-VOL menu
            else if (current_state == EQ_MENU_STATE) {
                if (w_prawo) op_eq_menu++;
                else op_eq_menu--;
                if (op_eq_menu > 3) op_eq_menu = 0;
                if (op_eq_menu < 0) op_eq_menu = 3;
            } else if (current_state == PWM5_EDIT_STATE) {
                if (w_prawo && vol_l_lvl < 100) vol_l_lvl += 5;
                else if (!w_prawo && vol_l_lvl > 0) vol_l_lvl -= 5;
                if (vol_l_active) update_vol_l(vol_l_lvl);
            } else if (current_state == PWM6_EDIT_STATE) {
                if (w_prawo && vol_r_lvl < 100) vol_r_lvl += 5;
                else if (!w_prawo && vol_r_lvl > 0) vol_r_lvl -= 5;
                if (vol_r_active) update_vol_r(vol_r_lvl);
            } else if (current_state == PWM7_EDIT_STATE) {
                if (w_prawo && eq_lvl < 100) eq_lvl += 5;
                else if (!w_prawo && eq_lvl > 0) eq_lvl -= 5;
                if (eq_active) update_eq(eq_lvl);
            }
            //MIDI menu
            else if (current_state == MIDI_MENU_STATE) {
                if (w_prawo) op_midi_menu++;
                else op_midi_menu--;
                if (op_midi_menu > 1) op_midi_menu = 0;
                if (op_midi_menu < 0) op_midi_menu = 1; // only 2 options : on/off and back to main menu
                if (midi_active) {
                    op_midi_menu = 1;
                }
            }

            update = 1;
        }
        stan_A_poprzedni = stan_A;

        // pezycisk
        if (!(PIND & (1 << PD2))) {
            _delay_ms(50); // Debounce

            if (current_state == MAIN_MANU_STATE) {
                if (main_menu == 0) {
                    current_state = EQ_MENU_STATE;
                    op_eq_menu = 0;
                } else if (main_menu == 1) {
                    current_state = MIDI_MENU_STATE;
                    op_midi_menu = 0;
                } else if (main_menu == 2) blink();
                else if (main_menu == 3) {
                    current_state = PARAMS_MENU_STATE;
                    pwm_menu = 0;
                }
            }
            //funkcjonalnosci menu parametrow
            else if (current_state == PARAMS_MENU_STATE) {
                if (pwm_menu == 0) {
                    // wstecz
                    current_state = MAIN_MANU_STATE;
                } else if (pwm_menu == 1) {
                    // ON/OFF dla wszystkich kanałów PWM
                    par_one_pwm_active = !par_one_pwm_active;
                    if (par_one_pwm_active) {
                        TCCR1A |= (1 << COM1A1) | (1 << COM1B1); // Timer 1 (A i B)
                        TCCR2A |= (1 << COM2A1); // Timer 2
                        upadte_par_one(par_one_val);
                        upadte_par_two(par_two_val);
                        update_par_three(par_three_val);
                    } else {
                        TCCR1A &= ~((1 << COM1A1) | (1 << COM1B1));
                        TCCR2A &= ~(1 << COM2A1);
                        PORTB &= ~((1 << PB1) | (1 << PB2) | (1 << PB3));
                    }
                } else if (pwm_menu == 2) current_state = PAR_ONE_EDIT_STATE;
                else if (pwm_menu == 3) current_state = PAR_TWO_EDIT_STATE;
                else if (pwm_menu == 4) current_state = PAR_THREE_EDIT_STATE;
                else if (pwm_menu == 5) current_state = PRESET_EDIT_STATE;
            }
            //funkcjonalnosci  menu eq
            else if (current_state == EQ_MENU_STATE) {
                if (op_eq_menu == 0) {
                    //wstecz
                    current_state = MAIN_MANU_STATE;
                } else if (op_eq_menu == 1) current_state = PWM5_EDIT_STATE;
                else if (op_eq_menu == 2) current_state = PWM6_EDIT_STATE;
                else if (op_eq_menu == 3) current_state = PWM7_EDIT_STATE;
                update_vol_l(vol_l_lvl);
                update_vol_r(vol_r_lvl);
                update_eq(eq_lvl);
            }
            //funkcjonalnosci menu midi
            else if (current_state == MIDI_MENU_STATE) {
                if (op_midi_menu == 0) {
                    //wstecz
                    current_state = MAIN_MANU_STATE;
                } else if (op_midi_menu == 1) {
                    midi_active = !midi_active;
                }
            }

            //do listy parametrów
            else if (current_state == PAR_ONE_EDIT_STATE ||
                     current_state == PAR_TWO_EDIT_STATE ||
                     current_state == PAR_THREE_EDIT_STATE ||
                     current_state == PRESET_EDIT_STATE) {
                current_state = PARAMS_MENU_STATE;
            } else if (current_state == PWM5_EDIT_STATE ||
                       current_state == PWM6_EDIT_STATE ||
                       current_state == PWM7_EDIT_STATE) {
                current_state = EQ_MENU_STATE;
            }

            update = 1;
            while (!(PIND & (1 << PD2))); //na puszczenie
        }

        if (update) {
            if (current_state == MAIN_MANU_STATE) {
                draw_menu(main_menu);
            } else if (current_state == PARAMS_MENU_STATE ||
                       current_state == PAR_ONE_EDIT_STATE ||
                       current_state == PAR_TWO_EDIT_STATE ||
                       current_state == PAR_THREE_EDIT_STATE ||
                       current_state == PRESET_EDIT_STATE) {
                draw_params_menu(pwm_menu, par_one_val, par_two_val, par_three_val,
                                 current_state, par_one_pwm_active, preset_id);
            } else if (current_state == EQ_MENU_STATE ||
                       current_state == PWM5_EDIT_STATE ||
                       current_state == PWM6_EDIT_STATE ||
                       current_state == PWM7_EDIT_STATE) {
                draw_EQ_menu(op_eq_menu, vol_l_lvl, vol_r_lvl, eq_lvl, current_state);
            } else if (current_state == MIDI_MENU_STATE) {
                draw_MIDI_menu(op_midi_menu, midi_active);
            }
            update = 0;
        }

        if (midi_active) {
            int16_t c = uart_get_char();
            if (c != -1 && (uint8_t)c >= 0xC0 && (uint8_t)c <= 0xCF) {

                int16_t data_byte = -1;
                uint16_t timeout = 0;

                while (data_byte == -1 && timeout < 3000) {
                    data_byte = uart_get_char();
                    _delay_us(10);
                    timeout++;
                }

                if (data_byte != -1) {
                    uint8_t p = (uint8_t)data_byte;

                    if (p == 0) {
                        preset_id = 0;
                    }
                    else if (p == 1) {
                        preset_id = 1;
                    }
                    else if (p == 2) {
                        preset_id = 2;
                    }
                    else if (p == 3) {
                        preset_id = 3;
                    }
                    else if (p == 4) {
                        preset_id = 4;
                    }
                    else {
                        preset_id = p;
                    }
                    set_preset(preset_id);
                    update = 1;
                }
            }
        }
    }
}
