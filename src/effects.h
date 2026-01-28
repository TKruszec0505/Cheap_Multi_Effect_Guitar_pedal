//
// Created by Tomasz on 28.01.2026.
//

#ifndef EFFECTS_H
#define EFFECTS_H

#include "stdio.h"

void init_audio_system();
void clean();
void delay(int16_t* inpput_buf);
void tremolo();
void chorus();

#endif //EFFECTS_H
