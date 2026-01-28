//
// Created by Tomasz on 28.01.2026.
//
#include "effects.h"
#include <avr/io.h>

void init_audio_system() {
    //wejście
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2);

    //wyjście
    TCCR1A |= (1 << COM1A1) | (1 << WGM10);
    TCCR1B |= (1 << WGM12) | (1 << CS10);
}

void clean() {

}
void delay(int16_t* inpput_buf) {

}
void tremolo() {

}
void chorus() {

}