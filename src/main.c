#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#include "OledMenuGui.h"

#define LED_PIN    PB1 // 9

int main(void) {
    //port d konfiguracja
    DDRB |= (1 << LED_PIN); // Pin 9 wyjście
    DDRD &= ~((1 << PD2) | (1 << PD5) | (1 << PD6)); // Enkoder
    PORTD |= (1 << PD2) | (1 << PD5) | (1 << PD6); // Pull-upy

    i2c_init();
    oled_init();

    int8_t opcja = 0;
    draw_menu(opcja);

    uint8_t stan_A_poprzedni = (PIND & (1 << PD2));

    while (1) {
        //enkoder
        uint8_t stan_A = (PIND & (1 << PD2));
        if (stan_A != stan_A_poprzedni) {
            if (stan_A == 0) { // opadające zbocze
                if (PIND & (1 << PD5)) opcja = 0; // jedną stronę ON
                else opcja = 1;                  //w drugą OFF
                draw_menu(opcja);
            }
        }
        stan_A_poprzedni = stan_A;

        // przycisk enkoder
        if (!(PIND & (1 << PD6))) {
            _delay_ms(50); // Debounce
            if (opcja == 0) PORTB |= (1 << LED_PIN);  // zapal
            else PORTB &= ~(1 << LED_PIN);           //zgas

            _delay_ms(200);

            while (!(PIND & (1 << PD6))); //na puszczenie
        }
    }
}