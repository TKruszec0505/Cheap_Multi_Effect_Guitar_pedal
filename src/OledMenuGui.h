//
// Created by Tomasz on 28.01.2026.
//

#ifndef OLEDMENUGUI_H
#define OLEDMENUGUI_H
void i2c_init();
void i2c_start();
void i2c_stop();
void i2c_write(uint8_t data);

void send_cmd(uint8_t cmd);
void oled_init();
void oled_set_cursor(uint8_t row, uint8_t col);
void oled_clear();

void draw(const uint8_t* d, uint8_t len);
void draw_menu(int8_t state);
#endif //OLEDMENUGUI_H
