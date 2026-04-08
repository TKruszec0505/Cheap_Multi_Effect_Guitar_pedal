#include <avr/io.h>
#include <util/delay.h>
#include "OledMenuGui.h"
#include <avr/interrupt.h>
#include <stdbool.h>
#include <string.h>

#define LED_PIN PB1

//for mosfet output config
#define PRESET_OUT_1 PC0
#define PRESET_OUT_2 PC1
#define PRESET_OUT_3 PC2
#define PRESET_OUT_4 PC3

#define BAUD 9600
#define MYUBRR (F_CPU/16/BAUD-1)
#define BUFFER_SIZE 32

volatile uint8_t uart_buffer[BUFFER_SIZE];
volatile uint8_t head = 0;
volatile uint8_t tail = 0;

TypStanu aktualny_stan = STAN_MENU_GLOWNE;

int8_t op_glowne = 0;
int8_t op_pwm_menu = 0;
int8_t op_mod_menu = 0;
int8_t op_midi_menu = 0;
int8_t op_eq_menu = 0;
int8_t op_dst_menu = 0;

int8_t wartosc_pwm = 80;
int8_t wartosc_pwm2 = 80;
int8_t wartosc_pwm3 = 80;

int8_t vol_r_lvl = 80;
int8_t vol_l_lvl = 80;
int8_t eq_lvl = 80;

bool pwm_aktywne = 1;
bool pwm_aktywne2 = 1;
bool pwm_aktywne3 = 1;

bool vol_r_active = 0;
bool vol_l_active = 0;
bool eq_active = 1;

bool midi_active = 0;

int8_t preset_id = 5; //5 bc its bypass(test fv-1 program)

uint8_t odswiez = 1;

void uart_init(unsigned int baud) {
    uint32_t ubrr = F_CPU / 16 / baud - 1;
    UBRR0H = (ubrr >> 8);
    UBRR0L = ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << USBS0) | (3 << UCSZ00);
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
    if (head == tail) return -1; // Bufor pusty
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

void setup_vol_stereo() {
    // TCCR0A: Aktywacja obu kanałów A i B w trybie Fast PWM
    // COM0A1 (dla Lewego), COM0B1 (dla Prawego)
    TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);

    // TCCR0B: Ustawienie Preskalera 8
    TCCR0B = (1 << CS01);

    // Ustawienie pinów PD6 (Lewy) i PD5 (Prawy) jako wyjścia
    DDRD |= (1 << PD6) | (1 << PD5);
}

void set_EQ() {
    TCCR2A |= (1 << COM2B1);
    DDRD |= (1 << PD3);
}

void set_preset(uint8_t id) {
    PORTC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2));

    uint8_t fixed_id = id % 8;
    if (~fixed_id & (1 << 0))
        PORTC |= (1 << PC0);
    if (~fixed_id & (1 << 1))
        PORTC |= (1 << PC1);
    if (~fixed_id & (1 << 2))
        PORTC |= (1 << PC2);
}

void setup_pwm() {
    // TIMER1
    // WGM12 = 1 i WGM10 = 1  -> FAST PWM, 8-bit (TOP = 255)
    // CS10 = 1               -> Prescaler 1
    TCCR1A = (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS10);

    // TIMER2
    // WGM21 = 1 i WGM20 = 1  -> FAST PWM, 8-bit (TOP = 255)
    // CS20 = 1               -> Prescaler 1
    TCCR2A = (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS20);

    DDRB |= (1 << PB1) | (1 << PB2) | (1 << PB3);
}

void setup_pwm3() {
    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS20); // CS20 = Prescaler 1
    DDRB |= (1 << PB3);
    // freq: 16MHz / 256 = 62.5 kHz (input rezonator freq/Prescaler*bits)
}

void aktualizuj_pwm(uint8_t v) {
    OCR1A = (uint16_t) (((uint32_t) v * 255) / 100);
}

void aktualizacja_pwm2(uint8_t v) {
    OCR1B = (uint16_t) (((uint32_t) v * 255) / 100);
}

void aktualizacja_pwm3(uint8_t v) {
    //8bit
    OCR2A = (uint16_t) (((uint32_t) v * 255) / 100);
}

void aktualizuj_vol_l(uint8_t v_l) {
    OCR0A = (uint8_t) (((uint32_t) v_l * 255) / 100);
}

void aktualizuj_vol_r(uint8_t v_r) {
    OCR0B = (uint8_t) (((uint32_t) v_r * 255) / 100);
}

void aktualizuj_eq(uint8_t v) {
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

    setup_pwm(); // Timer 1 (PAR1, PAR2)
    setup_pwm3(); // Timer 2 (PAR3)
    set_EQ();

    PORTC |= (1 << PRESET_OUT_4);
    set_preset(5);

    pwm_aktywne = 1;

    draw_menu(op_glowne);


    uint8_t stan_A_poprzedni = (PIND & (1 << PD7));

    while (1) {
        //obrot enkodera
        uint8_t stan_A = (PIND & (1 << PD7));
        if (stan_A != stan_A_poprzedni && stan_A == 0) {
            uint8_t w_prawo = (PIND & (1 << PD4));

            //main menu
            if (aktualny_stan == STAN_MENU_GLOWNE) {
                if (w_prawo) op_glowne++;
                else op_glowne--;
                if (op_glowne > 3) op_glowne = 0;
                if (op_glowne < 0) op_glowne = 3;
            }
            //presets menu
            else if (aktualny_stan == STAN_MENU_PWM) {
                if (w_prawo) op_pwm_menu++;
                else op_pwm_menu--;
                if (op_pwm_menu > 5) op_pwm_menu = 0;
                if (op_pwm_menu < 0) op_pwm_menu = 5;
            } else if (aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM) {
                if (w_prawo && wartosc_pwm < 100) wartosc_pwm += 5;
                else if (!w_prawo && wartosc_pwm > 0) wartosc_pwm -= 5;
                if (pwm_aktywne) aktualizuj_pwm(wartosc_pwm);
            } else if (aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM2) {
                if (w_prawo && wartosc_pwm2 < 100) wartosc_pwm2 += 5;
                else if (!w_prawo && wartosc_pwm2 > 0) wartosc_pwm2 -= 5;
                if (pwm_aktywne) aktualizacja_pwm2(wartosc_pwm2);
            } else if (aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM3) {
                if (w_prawo && wartosc_pwm3 < 100) wartosc_pwm3 += 5;
                else if (!w_prawo && wartosc_pwm3 > 0) wartosc_pwm3 -= 5;
                if (pwm_aktywne) aktualizacja_pwm3(wartosc_pwm3);
            } else if (aktualny_stan == PRESET_EDIT_STATE) {
                if (w_prawo) preset_id++;
                else preset_id--;
                if (preset_id > 15) preset_id = 0; //0-15(16 presets)
                if (preset_id < 0) preset_id = 15;
                if (preset_id > 7) (PORTC &= ~(1 << PRESET_OUT_4)); //0-7 from ROM 8-15 EEPROM(we are switching T1)
                else (PORTC |= (1 << PRESET_OUT_4));
                set_preset(preset_id);
            }
            //EQ-VOL menu
            else if (aktualny_stan == EQ_MENU_STATE) {
                if (w_prawo) op_eq_menu++;
                else op_eq_menu--;
                if (op_eq_menu > 3) op_eq_menu = 0;
                if (op_eq_menu < 0) op_eq_menu = 3;
            } else if (aktualny_stan == PWM5_EDIT_STATE) {
                if (w_prawo && vol_l_lvl < 100) vol_l_lvl += 5;
                else if (!w_prawo && vol_l_lvl > 0) vol_l_lvl -= 5;
                if (vol_l_active) aktualizuj_vol_l(vol_l_lvl);
            } else if (aktualny_stan == PWM6_EDIT_STATE) {
                if (w_prawo && vol_r_lvl < 100) vol_r_lvl += 5;
                else if (!w_prawo && vol_r_lvl > 0) vol_r_lvl -= 5;
                if (vol_r_active) aktualizuj_vol_r(vol_r_lvl);
            } else if (aktualny_stan == PWM7_EDIT_STATE) {
                if (w_prawo && eq_lvl < 100) eq_lvl += 5;
                else if (!w_prawo && eq_lvl > 0) eq_lvl -= 5;
                if (eq_active) aktualizuj_eq(eq_lvl);
            }
            //MIDI menu
            else if (aktualny_stan == MIDI_MENU_STATE) {
                if (w_prawo) op_midi_menu++;
                else op_midi_menu--;
                if (op_midi_menu > 1) op_midi_menu = 0;
                if (op_midi_menu < 0) op_midi_menu = 1; // only 2 options : on/off and back to main menu
                if (midi_active) {
                    op_midi_menu = 1;
                }
            }

            odswiez = 1;
        }
        stan_A_poprzedni = stan_A;

        // pezycisk
        if (!(PIND & (1 << PD2))) {
            _delay_ms(50); // Debounce

            if (aktualny_stan == STAN_MENU_GLOWNE) {
                if (op_glowne == 0) {
                    aktualny_stan = EQ_MENU_STATE;
                    op_eq_menu = 0;
                } else if (op_glowne == 1) {
                    aktualny_stan = MIDI_MENU_STATE;
                    op_midi_menu = 0;
                } else if (op_glowne == 2) blink();
                else if (op_glowne == 3) {
                    aktualny_stan = STAN_MENU_PWM;
                    op_pwm_menu = 0;
                }
            }
            //funkcjonalnosci menu parametrow
            else if (aktualny_stan == STAN_MENU_PWM) {
                if (op_pwm_menu == 0) {
                    // wstecz
                    aktualny_stan = STAN_MENU_GLOWNE;
                } else if (op_pwm_menu == 1) {
                    // ON/OFF dla wszystkich kanałów PWM
                    pwm_aktywne = !pwm_aktywne;
                    if (pwm_aktywne) {
                        TCCR1A |= (1 << COM1A1) | (1 << COM1B1); // Timer 1 (A i B)
                        TCCR2A |= (1 << COM2A1); // Timer 2
                        aktualizuj_pwm(wartosc_pwm);
                        aktualizacja_pwm2(wartosc_pwm2);
                        aktualizacja_pwm3(wartosc_pwm3);
                    } else {
                        TCCR1A &= ~((1 << COM1A1) | (1 << COM1B1));
                        TCCR2A &= ~(1 << COM2A1);
                        PORTB &= ~((1 << PB1) | (1 << PB2) | (1 << PB3));
                    }
                } else if (op_pwm_menu == 2) aktualny_stan = STAN_EDYCJA_WARTOSCI_PWM;
                else if (op_pwm_menu == 3) aktualny_stan = STAN_EDYCJA_WARTOSCI_PWM2;
                else if (op_pwm_menu == 4) aktualny_stan = STAN_EDYCJA_WARTOSCI_PWM3;
                else if (op_pwm_menu == 5) aktualny_stan = PRESET_EDIT_STATE;
            }
            //funkcjonalnosci  menu eq
            else if (aktualny_stan == EQ_MENU_STATE) {
                if (op_eq_menu == 0) {
                    //wstecz
                    aktualny_stan = STAN_MENU_GLOWNE;
                } else if (op_eq_menu == 1) aktualny_stan = PWM5_EDIT_STATE;
                else if (op_eq_menu == 2) aktualny_stan = PWM6_EDIT_STATE;
                else if (op_eq_menu == 3) aktualny_stan = PWM7_EDIT_STATE;
                aktualizuj_vol_l(vol_l_lvl);
                aktualizuj_vol_r(vol_r_lvl);
                aktualizuj_eq(eq_lvl);
            }
            //funkcjonalnosci menu midi
            else if (aktualny_stan == MIDI_MENU_STATE) {
                if (op_midi_menu == 0) {
                    //wstecz
                    aktualny_stan = STAN_MENU_GLOWNE;
                } else if (op_midi_menu == 1) {
                    midi_active = !midi_active;
                }
            }

            //do listy parametrów
            else if (aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM ||
                     aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM2 ||
                     aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM3 ||
                     aktualny_stan == PRESET_EDIT_STATE) {
                aktualny_stan = STAN_MENU_PWM;
            } else if (aktualny_stan == PWM5_EDIT_STATE ||
                       aktualny_stan == PWM6_EDIT_STATE ||
                       aktualny_stan == PWM7_EDIT_STATE) {
                aktualny_stan = EQ_MENU_STATE;
            }

            odswiez = 1;
            while (!(PIND & (1 << PD2))); //na puszczenie
        }

        if (odswiez) {
            if (aktualny_stan == STAN_MENU_GLOWNE) {
                draw_menu(op_glowne);
            } else if (aktualny_stan == STAN_MENU_PWM ||
                       aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM ||
                       aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM2 ||
                       aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM3 ||
                       aktualny_stan == PRESET_EDIT_STATE) {
                draw_params_menu(op_pwm_menu, wartosc_pwm, wartosc_pwm2, wartosc_pwm3,
                                 aktualny_stan, pwm_aktywne, preset_id);
            } else if (aktualny_stan == EQ_MENU_STATE ||
                       aktualny_stan == PWM5_EDIT_STATE ||
                       aktualny_stan == PWM6_EDIT_STATE ||
                       aktualny_stan == PWM7_EDIT_STATE) {
                draw_EQ_menu(op_eq_menu, vol_l_lvl, vol_r_lvl, eq_lvl, aktualny_stan);
            } else if (aktualny_stan == MIDI_MENU_STATE) {
                draw_MIDI_menu(op_midi_menu, midi_active);
            }
            odswiez = 0;
        }

        if (midi_active) {
            int16_t c = uart_get_char();
            if (c != -1) {
                if (c == '\n' || c == '\r') {
                    if (cmd_index > 0) {
                        cmd_buffer[cmd_index] = '\0';

                        if (strcmp(cmd_buffer, "pgm: 1") == 0) {
                            preset_id = 0;
                            set_preset(preset_id);
                            odswiez = 1;
                        } else if (strcmp(cmd_buffer, "pgm: 5") == 0) {
                            preset_id = 5;
                            set_preset(preset_id);
                            odswiez = 1;
                        }

                        cmd_index = 0;
                    }
                } else if (cmd_index < BUFFER_SIZE - 1) {
                    cmd_buffer[cmd_index++] = (char)c;
                }
            }
        }
    }
}
