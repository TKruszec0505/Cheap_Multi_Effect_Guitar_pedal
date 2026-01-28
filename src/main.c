/*
 *
 *
 *
 *
 *
 *
 *
 *
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define LED_PIN    PB1 // 9
#define OLED_ADDR  0x78


const uint8_t char_ARROW[] = {0x08, 0x08, 0x3E, 0x1C, 0x08};
//const uint8_t char_ARROW[] = {0x08, 0x1C, 0x3E, 0x08, 0x08};
//const uint8_t font_N[]     =                       {0x7F, 0x02, 0x04, 0x08, 0x7F};
const uint8_t font_ON[]    = {0x3E, 0x41, 0x3E, 0x00, 0x7F, 0x02, 0x04, 0x08, 0x7F}; // ON
const uint8_t font_OFF[]   = {0x3E, 0x41, 0x3E, 0x00, 0x7F, 0x09, 0x01, 0x00, 0x7F, 0x09, 0x01}; // OFF

// i2c (TWI)
void i2c_init() {
    TWBR = 72; // ustawienie zegara SCL 100kHz przy 16MHz
    TWCR = (1 << TWEN);
}

void i2c_start() {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

void i2c_stop() {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void i2c_write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

//oled(128x64)
void send_cmd(uint8_t cmd) {
    i2c_start();
    i2c_write(OLED_ADDR);
    i2c_write(0x00); //komendy
    i2c_write(cmd);
    i2c_stop();
}

void oled_init() {
    _delay_ms(100);
    send_cmd(0xAE);                     // Display OFF
    send_cmd(0xD5); send_cmd(0x80);
    send_cmd(0xA8); send_cmd(0x3F);     // 64 linie
    send_cmd(0xD3); send_cmd(0x00);
    send_cmd(0x40);
    send_cmd(0x8D); send_cmd(0x14);     // Charge Pump ON
    send_cmd(0xA1);                     // Mirror X
    send_cmd(0xC8);                     // Mirror Y
    send_cmd(0xDA); send_cmd(0x12);     // COM pins
    send_cmd(0x20); send_cmd(0x02);     // Page Mode
    send_cmd(0xAF);                     // Display ON
}

void oled_set_cursor(uint8_t page, uint8_t col) {
    send_cmd(0xB0 | page);
    send_cmd(0x00 | (col & 0x0F));
    send_cmd(0x10 | (col >> 4));
}

void oled_clear() {
    for (uint8_t p = 0; p < 8; p++) {
        oled_set_cursor(p, 0);
        i2c_start();
        i2c_write(OLED_ADDR);
        i2c_write(0x40); // tryb danych
        for (uint16_t i = 0; i < 128; i++) i2c_write(0x00);
        i2c_stop();
    }
}

void draw(const uint8_t* d, uint8_t len) {
    i2c_start();
    i2c_write(OLED_ADDR);
    i2c_write(0x40);
    for (uint8_t i = 0; i < len; i++) i2c_write(d[i]);
    i2c_write(0x00);
    i2c_stop();
}

void rysuj_menu(int8_t wybrana) {
    oled_clear();
    // ON na stronie 2
    oled_set_cursor(2, 40);
    draw(font_ON, 9);
    if(wybrana == 0) { oled_set_cursor(2, 20); draw(char_ARROW, 5); }

    // OFF na stronie 5
    oled_set_cursor(5, 40);
    draw(font_OFF, 11);
    if(wybrana == 1) { oled_set_cursor(5, 20); draw(char_ARROW, 5); }
}

int main(void) {
    //port d konfiguracja
    DDRB |= (1 << LED_PIN); // Pin 9 wyjście
    DDRD &= ~((1 << PD2) | (1 << PD5) | (1 << PD6)); // Enkoder
    PORTD |= (1 << PD2) | (1 << PD5) | (1 << PD6); // Pull-upy

    i2c_init();
    oled_init();

    int8_t opcja = 0;
    rysuj_menu(opcja);

    uint8_t stan_A_poprzedni = (PIND & (1 << PD2));

    while (1) {
        //enkoder
        uint8_t stan_A = (PIND & (1 << PD2));
        if (stan_A != stan_A_poprzedni) {
            if (stan_A == 0) { // opadające zbocze
                if (PIND & (1 << PD5)) opcja = 0; // jedną stronę ON
                else opcja = 1;                  //w drugą OFF
                rysuj_menu(opcja);
            }
        }
        stan_A_poprzedni = stan_A;

        // przycisk enkoder
        if (!(PIND & (1 << PD6))) {
            _delay_ms(50); // Debounce
            if (opcja == 0) PORTB |= (1 << LED_PIN);  // zapal
            else PORTB &= ~(1 << LED_PIN);           //zgas


            //oled_set_cursor(0, 120);
            //draw(char_B, 5);
            _delay_ms(200);

            while (!(PIND & (1 << PD6))); //na puszczenie
        }
    }
}