//
// Created by Tomasz on 28.01.2026.
//
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "OledMenuGui.h"
#include "chars.h"

#define F_CPU 16000000UL
#define OLED_ADDR  0x78
#define DATA_MODE  0x40

uint8_t buffer[1024];

const char* const FX_names[] PROGMEM = {
	"REV-CHOR",
	"REV-FLANG",
	"REV-TREM",
	"PTCH-SHFT",
	"PTCH-ECHO",
	"BYPASS",
	"REVERB1",
	"REVERB2",

	"EPPROM1",
	"EPPROM2",
	"EPPROM3",
	"EPPROM4",
	"EPPROM5",
	"EPPROM6",
	"EPPROM7",
	"EPPROM8",
};

// i2c (TWI)
void i2c_init() {
    TWBR = 12; //SCL 400kHz w/16MHz
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
    i2c_write(0x00);               //com
    i2c_write(cmd);
    i2c_stop();
}

void oled_init() {
    _delay_ms(10);
    send_cmd(0xAE);                     // Display OFF
    send_cmd(0xD5); send_cmd(0x80);
    send_cmd(0xA8); send_cmd(0x3F);     // 64 lines
    send_cmd(0xD3); send_cmd(0x00);
    send_cmd(0x40);
    send_cmd(0x8D); send_cmd(0x14);     // Charge Pump ON
    send_cmd(0xA1);                     // Mirror X
    send_cmd(0xC8);                     // Mirror Y
    send_cmd(0xDA); send_cmd(0x12);     // COM pins
    send_cmd(0x20); send_cmd(0x02);     // Page Mode
    send_cmd(0xAF);                     // Display ON
}


void update_display() {
	send_cmd(0x20); send_cmd(0x02);

	for (uint8_t page = 0; page < 8; page++) {
		send_cmd(0xB0 | page); // Ustaw stronę (wiersz)
		send_cmd(0x00);        // Kolumna niższa
		send_cmd(0x10);        // Kolumna wyższa

		i2c_start();
		i2c_write(OLED_ADDR);
		i2c_write(DATA_MODE); 
		for (uint16_t i = 0; i < 128; i++) {
			i2c_write(buffer[page * 128 + i]);
		}
		i2c_stop();
	}
}

void buffer_clear() {  
	for (uint16_t i = 0; i < 1024; i++) {
		buffer[i] = 0x00;
	}
}
void draw_to_buffer(uint8_t row, uint8_t col, const uint8_t* data, uint8_t len, uint8_t is_progmem) {
	if (row > 7 || col > 127) return;

	for (uint8_t i = 0; i < len; i++) {
		if (col + i > 127) break; 

		uint8_t byte = is_progmem ? pgm_read_byte(&data[i]) : data[i];
		buffer[(row * 128) + col + i] = byte;
	}
}

void draw_menu(int8_t state) {
	static uint8_t logo_shown = 0;

	if (!logo_shown) {
		buffer_clear();
		for (uint16_t i = 0; i < 1024; i++) {
			buffer[i] = pgm_read_byte(&epd_bitmap_logo1[i]);
		}
		update_display();
		_delay_ms(3000);
		logo_shown = 1;
	}

	buffer_clear();

	print(0, 40, "VOL EQ");
	if(state == 0) print(0, 20, ">");

	print(2, 40, "MIDI");
	if(state == 1) print(2, 20, ">");

	print(4, 40, "BLINK");
	if(state == 2) print(4, 20, ">");

	print(6, 40, "PRESETS");
	if(state == 3) print(6, 20, ">");


	update_display();
}

void print(uint8_t row, uint8_t col, const char* str) {
    uint8_t current_col = col;

    while (*str) {
        if (current_col > 122) break; 

        uint16_t offset = (*str - 32) * 5;

        draw_to_buffer(row, current_col, OLED_ASCII_FONT + offset, 5, 1);

        current_col += 6; 

        str++;
    }
}

void draw_pwm_to_buffer(uint8_t row, uint8_t col, uint8_t val) {
	uint8_t s = val / 100;
	uint8_t d = (val / 10) % 10;
	uint8_t j = val % 10;

	// Setki
	if (s > 0) draw_to_buffer(row, col, nums[s], 5, 1);

	// Dziesiątki (col + 6 pikseli odstępu)
	if (s > 0 || d > 0) draw_to_buffer(row, col + 6, nums[d], 5, 1);

	// Jedności (col + 12 pikseli odstępu)
	draw_to_buffer(row, col + 12, nums[j], 5, 1);

	//% (col + 18 pikseli odstępu)
	print(row, col + 18, "%");
}

void draw_params_menu(int8_t wybrana, int8_t pwm_val, int8_t pwm_val2, int8_t pwm_val3,
						uint8_t aktualny_stan, uint8_t sw,uint8_t preset_id) {
	buffer_clear();

	// WSTECZ
	print(0, 40, "WSTECZ");
	if(wybrana == 0) print(0, 20, ">");

	if(sw) print(1, 40, "ON");
	else   print(1, 40, "OFF");
	if(wybrana == 1) print(1, 20, ">");
	if (preset_id == 5) {
		print(3, 40, "PAR1");
		print(3, 70, "-");
		if (wybrana == 2) {
			print(3, 20, ">");
		}

		print(4, 40, "PAR2");
		print(4, 70, "-");
		if (wybrana == 3) {
			print(4, 20, ">");
		}

		print(5, 40, "PAR3");
		print(5, 70, "-");
		if (wybrana == 4) {
			print(5, 20, ">");
		}
	} else {
		// PAR1
		print(3, 40, "PAR1");
		draw_pwm_to_buffer(3, 70, pwm_val);
		if(wybrana == 2) {
			if(aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM) print(3, 20, "ON");
			else print(3, 20, ">");
		}

		// PAR2
		print(4, 40, "PAR2");
		draw_pwm_to_buffer(4, 70, pwm_val2);
		if(wybrana == 3) {
			if(aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM2) print(4, 20, "ON");
			else print(4, 20, ">");
		}

		// PAR3
		print(5, 40, "PAR3");
		draw_pwm_to_buffer(5, 70, pwm_val3);
		if(wybrana == 4) {
			if(aktualny_stan == STAN_EDYCJA_WARTOSCI_PWM3) print(5, 20, "ON");
			else print(5, 20, ">");
		}
	}

	//PRESET
	print(7,40, "FX:");
	print(7, 57, (const char*)pgm_read_ptr(&FX_names[preset_id]));
	if(wybrana == 5){
		if(aktualny_stan == PRESET_EDIT_STATE) print(7, 20, "ON");
		else print(7, 20, ">");
	}

	update_display();
}

void draw_MIDI_menu(uint8_t wybrana, uint8_t sw) {
	buffer_clear();

	print(0, 40, "WSTECZ");
	if(wybrana == 0) print(0, 20, ">");

	if(sw) print(1, 40, "ON");
	else   print(1, 40, "OFF");
	if(wybrana == 1) print(1, 20, ">");
	update_display();

}
void draw_mod_menu(uint8_t wybrana) {
	buffer_clear();

	// WSTECZ
	print(0, 40, "WSTECZ");
	if(wybrana == 0) print(0, 20, ">");
}
void draw_EQ_menu(uint8_t wybrana,uint8_t pwm_val, uint8_t pwm_val2,uint8_t pwm_val3, uint8_t aktualny_stan) {
	buffer_clear();

	// WSTECZ
	print(0, 40, "WSTECZ");
	if(wybrana == 0) print(0, 20, ">");

	//params settings
	print(3, 40, "PAR1");
	draw_pwm_to_buffer(3, 70, pwm_val);
	if(wybrana == 1) {	//vol l
		if(aktualny_stan == PWM5_EDIT_STATE) print(3, 20, "ON");
		else print(3, 20, ">");
	}
	print(4, 40, "PAR2");
	draw_pwm_to_buffer(4, 70, pwm_val2);
	if (wybrana == 2) { // vol r
		if (aktualny_stan == PWM6_EDIT_STATE) print(4, 20, "ON");
		else print(4, 20, ">");
	}
	print(5, 40, "PAR3");
	draw_pwm_to_buffer(5, 70, pwm_val3);
	if (wybrana == 3) { // eq
		if (aktualny_stan == PWM7_EDIT_STATE) print(5, 20, "ON");
		else print(5, 20, ">");
	}
	update_display();
}
void draw_distortion_menu(uint8_t wybrana,uint8_t sw,uint8_t aktualny_stan,uint8_t pwm_val){

	buffer_clear();

	// WSTECZ
	print(0, 40, "WSTECZ");
	if(wybrana == 0) print(0, 20, ">");

	if(sw) print(1, 40, "ON");
	else   print(1, 40, "OFF");
	if(wybrana == 1) print(1, 20, ">");

	//params settings
	print(3, 40, "PAR1");
	draw_pwm_to_buffer(3, 70, pwm_val);

	if(wybrana == 2) {
		if(aktualny_stan == PWM4_EDIT_STATE) print(3, 20, "ON");
		else print(3, 20, ">");
	}
}