#include <avr/io.h>
#include <util/delay.h>
#include "OledMenuGui.h"


#define LED_PIN PB1

TypStanu aktualny_stan = STAN_MENU_GLOWNE;

int8_t op_glowne = 0;   // 0:ON, 1:OFF, 2:BLINK, 3:PWM
int8_t op_pwm_menu = 0; // 0:WSTECZ, 1:ON/OFF_PWM, 2:WARTOSC_PWM
int8_t wartosc_pwm = 50;
uint8_t pwm_aktywne = 0;
uint8_t odswiez = 1;


void on() {
    TCCR1A &= ~(1 << COM1A1); // Wyłącz PWM, by PORTB działał
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
void setup_pwm() {
    // PWM 10-bit, Prescaler 8
    TCCR1A = (1 << WGM11) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS11);
}

void aktualizuj_pwm(uint8_t v) {
    // Skalowanie 0-100% na 0-1023 (10-bit)
    OCR1A = (uint16_t)(((uint32_t)v * 1023) / 100);
}

int main(void) {
    // Konfiguracja I/O
    DDRB |= (1 << LED_PIN);
    DDRD &= ~((1 << PD2) | (1 << PD5) | (1 << PD6));
    PORTD |= (1 << PD2) | (1 << PD5) | (1 << PD6);

    i2c_init();
    oled_init();
    setup_pwm();

    // Pierwsze rysowanie (z logo) obsłużone w draw_menu
    draw_menu(op_glowne);

    uint8_t stan_A_poprzedni = (PIND & (1 << PD2));

    while (1) {
        // enkoder
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
                if (op_pwm_menu > 2) op_pwm_menu = 0;
                if (op_pwm_menu < 0) op_pwm_menu = 2;
            }
            else if (aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM) {
                if (w_prawo && wartosc_pwm < 100) wartosc_pwm += 5;
                else if (!w_prawo && wartosc_pwm > 0) wartosc_pwm -= 5;

                if(pwm_aktywne) aktualizuj_pwm(wartosc_pwm);
            }
            odswiez = 1;
        }
        stan_A_poprzedni = stan_A;


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
                if (op_pwm_menu == 0) { // WSTECZ
                    aktualny_stan = STAN_MENU_GLOWNE;
                }
                else if (op_pwm_menu == 1) { //ON/OFF w PWM
                    pwm_aktywne = !pwm_aktywne;
                    if(pwm_aktywne) {
                        TCCR1A |= (1 << COM1A1);
                        aktualizuj_pwm(wartosc_pwm);
                    } else {
                        TCCR1A &= ~(1 << COM1A1);
                        PORTB &= ~(1 << LED_PIN);
                    }
                }
                else if (op_pwm_menu == 2) { // edycja
                    aktualny_stan = STAN_EDYCJA_WARTOSCI_PWM;
                }
            }
            else if (aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM) {
                aktualny_stan = STAN_MENU_PWM;
            }

            odswiez = 1;
            while (!(PIND & (1 << PD6))); //na puszczenie
        }

        // odswierzanie
        if (odswiez) {
            if (aktualny_stan == STAN_MENU_GLOWNE) {
                draw_menu(op_glowne);
            } else {
                // Rysuje podmenu: (strzałka, wartość, switch)
                draw_params_menu(op_pwm_menu, wartosc_pwm, (aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM), pwm_aktywne);
            }
            odswiez = 0;
        }
    }
}