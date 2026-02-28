#include <avr/io.h>
#include <util/delay.h>
#include "OledMenuGui.h"


#define LED_PIN PB1

//for mosfet output config
#define PRESET_OUT_1 PC0
#define PRESET_OUT_2 PC1
#define PRESET_OUT_3 PC2

TypStanu aktualny_stan = STAN_MENU_GLOWNE;

int8_t op_glowne = 0;
int8_t op_pwm_menu = 0;

int8_t wartosc_pwm = 50;
int8_t wartosc_pwm2 = 50;
int8_t wartosc_pwm3 = 50;

uint8_t pwm_aktywne = 0;
uint8_t pwm_aktywne2 = 0;
uint8_t pwm_aktywne3 = 0;

int8_t preset_id = 0;
uint8_t preset_aktywny = 1;

uint8_t odswiez = 1;


void on() {
    TCCR1A &= ~(1 << COM1A1);
    PORTB |= (1 << LED_PIN);
}

void off() {
    TCCR1A &= ~(1 << COM1A1);
    PORTB &= ~(1 << LED_PIN);
}

void blink() {
    TCCR1A &= ~(1 << COM1A1);
    for(uint8_t i=0; i<3; i++) {
        PORTB |= (1 << LED_PIN); _delay_ms(500);
        PORTB &= ~(1 << LED_PIN); _delay_ms(500);
    }
}
void set_preset(uint8_t id) {
    PORTC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2));

    if (id & (1 << 0)) PORTC |= (1 << PC0);
    if (id & (1 << 1)) PORTC |= (1 << PC1);
    if (id & (1 << 2)) PORTC |= (1 << PC2);
}

void setup_pwm() {
    // PWM 10-bit, Prescaler 8
    TCCR1A = (1 << WGM11) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS11);
}
void setup_pwm3() {
    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS21); // Prescaler 8
    DDRB |= (1 << PB3);   //PB3 jako wyjście
}

void aktualizuj_pwm(uint8_t v) {

    OCR1A = (uint16_t)(((uint32_t)v * 1023) / 100);
}
void aktualizacja_pwm2(uint8_t v) {
    OCR1B = (uint16_t)(((uint32_t)v * 1023) / 100);
}
void aktualizacja_pwm3(uint8_t v) {
    // ustawienie 8bit
    OCR2A = (uint16_t)(((uint32_t)v * 255) / 100);
}

int main(void) {
    //PWM: PB1 (OC1A), PB2 (OC1B), PB3 (OC2A)
    DDRB |= (1 << PB1) | (1 << PB2) | (1 << PB3);

    //enkodera i przycisku (PD2, PD5, PD6) z podciągnięciem
    DDRD &= ~((1 << PD2) | (1 << PD5) | (1 << PD6));
    PORTD |= (1 << PD2) | (1 << PD5) | (1 << PD6);

    // PPORT C init
    DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2);

    i2c_init();
    oled_init();


    setup_pwm();   // Timer 1 (PAR1, PAR2)
    setup_pwm3();  // Timer 2 (PAR3)


    draw_menu(op_glowne);

    uint8_t stan_A_poprzedni = (PIND & (1 << PD2));

    while(1) {
        //obrot enkodera
        uint8_t stan_A = (PIND & (1 << PD2));
        if (stan_A != stan_A_poprzedni && stan_A == 0) {
            uint8_t w_prawo = (PIND & (1 << PD5));

            if (aktualny_stan == STAN_MENU_GLOWNE) {
                if (w_prawo) op_glowne++; else op_glowne--;
                if (op_glowne > 3) op_glowne = 0;
                if (op_glowne < 0) op_glowne = 3;
            }
            else if (aktualny_stan == STAN_MENU_PWM) {
                if (w_prawo) op_pwm_menu++; else op_pwm_menu--;
                if (op_pwm_menu > 5) op_pwm_menu = 0;
                if (op_pwm_menu < 0) op_pwm_menu = 5;
            }
            else if (aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM) {
                if (w_prawo && wartosc_pwm < 100) wartosc_pwm += 5;
                else if (!w_prawo && wartosc_pwm > 0) wartosc_pwm -= 5;
                if(pwm_aktywne) aktualizuj_pwm(wartosc_pwm);
            }
            else if (aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM2) {
                if (w_prawo && wartosc_pwm2 < 100) wartosc_pwm2 += 5;
                else if (!w_prawo && wartosc_pwm2 > 0) wartosc_pwm2 -= 5;
                if(pwm_aktywne) aktualizacja_pwm2(wartosc_pwm2);
            }
            else if (aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM3) {
                if (w_prawo && wartosc_pwm3 < 100) wartosc_pwm3 += 5;
                else if (!w_prawo && wartosc_pwm3 > 0) wartosc_pwm3 -= 5;
                if(pwm_aktywne) aktualizacja_pwm3(wartosc_pwm3);
            }
            else if (aktualny_stan == PRESET_EDIT_STATE){
                if (w_prawo) preset_id++; else preset_id--;
                if (preset_id > 6) preset_id = 0;           //od 0 do 6
                if (preset_id < 0) preset_id = 6;
                set_preset(preset_id);
            }

            odswiez = 1;
        }
        stan_A_poprzedni = stan_A;

        // pezycisk
        if (!(PIND & (1 << PD6))) {
            _delay_ms(50); // Debounce

            if (aktualny_stan == STAN_MENU_GLOWNE) {
                if (op_glowne == 0) on();
                else if (op_glowne == 1) off();
                else if (op_glowne == 2) blink();
                else if (op_glowne == 3) {
                    aktualny_stan = STAN_MENU_PWM;
                    op_pwm_menu = 0;
                }
            }
            else if (aktualny_stan == STAN_MENU_PWM) {
                if (op_pwm_menu == 0) { // wstecz
                    aktualny_stan = STAN_MENU_GLOWNE;
                }
                else if (op_pwm_menu == 1) { // ON/OFF dla wszystkich kanałów PWM
                    pwm_aktywne = !pwm_aktywne;
                    if(pwm_aktywne) {
                        TCCR1A |= (1 << COM1A1) | (1 << COM1B1); // Włącz Timer 1 (A i B)
                        TCCR2A |= (1 << COM2A1);                 // Włącz Timer 2
                        aktualizuj_pwm(wartosc_pwm);
                        aktualizacja_pwm2(wartosc_pwm2);
                        aktualizacja_pwm3(wartosc_pwm3);
                        //set_preset(preset_id);
                    } else {
                        TCCR1A &= ~((1 << COM1A1) | (1 << COM1B1));
                        TCCR2A &= ~(1 << COM2A1);
                        PORTB &= ~((1 << PB1) | (1 << PB2) | (1 << PB3));

                    }
                }
                else if (op_pwm_menu == 2) aktualny_stan = STAN_EDYCJA_WARTOSCI_PWM;
                else if (op_pwm_menu == 3) aktualny_stan = STAN_EDYCJA_WARTOSCI_PWM2;
                else if (op_pwm_menu == 4) aktualny_stan = STAN_EDYCJA_WARTOSCI_PWM3;
                else if (op_pwm_menu == 5) aktualny_stan = PRESET_EDIT_STATE;

            }
            //do listy parametrów
            else if (aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM ||
                     aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM2 ||
                     aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM3 ||
                     aktualny_stan == PRESET_EDIT_STATE) {
                aktualny_stan = STAN_MENU_PWM;
            }

            odswiez = 1;
            while (!(PIND & (1 << PD6))); //na puszczenie
        }

        if (odswiez) {
            if (aktualny_stan == STAN_MENU_GLOWNE) {
                draw_menu(op_glowne);
            } else {
                draw_params_menu_1(op_pwm_menu, wartosc_pwm,
                                  wartosc_pwm2,wartosc_pwm3,
                                   aktualny_stan, pwm_aktywne, preset_id);
            }
            odswiez = 0;
        }
    }
}