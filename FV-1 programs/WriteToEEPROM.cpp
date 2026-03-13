//
// Created by Tomasz on 13.03.2026.
//
//arduino bc i will not waste my time on pure C fancy eeprom loader :)
#include <Arduino.h>
#include <Wire.h>
#define EEPROM_ADR 0x50

//eeprom cell addres
uint16_t addr = 0;

void setup() {
    Wire.begin();
    Wire.setClock(400000);
    Serial.begin(115200);
}

void loop() {
    if (Serial.available() > 0) {
        byte data = Serial.read();


        Wire.beginTransmission(EEPROM_ADR);
        Wire.write((byte)(addr >> 8));
        Wire.write((byte)(addr & 0xFF));
        Wire.write(data);

        if (Wire.endTransmission() == 0) {
            Serial.write('K'); // writes k so we can send another bytes
            addr++;            // to another cell
        } else {
            // i2c error
            Serial.write('E');
        }

        delay(5);
    }
}