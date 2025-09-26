#include <Arduino.h>

#include "display.h"

/**
 * Device: NHD-0420H1Z-FL-GBW-33V3
 */

/**
 * Wiring instructions:
 * - Pin 1 Ground
 * - Pin 2 3.3v
 * - Pin 3 Ground
 * - Pin 4 pinRs
 * - Pin 5 pinRW
 * - Pin 6 pinE
 * - Pin 11 pinData4
 * - Pin 12 pinData5
 * - Pin 13 pinData6
 * - Pin 14 pinData7
 * - Pin 15 3.3v
 * - Pin 16 Ground
 */

//const uint16_t pinContrast = 3;
const uint16_t pinRs = 18;
const uint16_t pinRW = 19;
const uint16_t pinE = 23;
const uint16_t pinData4 = 14;
const uint16_t pinData5 = 27;
const uint16_t pinData6 = 26;
const uint16_t pinData7 = 25;

Display::Display() {
    // Initialize the display pins
    //pinMode(pinContrast, OUTPUT);
    pinMode(pinRs, OUTPUT);
    pinMode(pinRW, OUTPUT);
    pinMode(pinE, OUTPUT);
    pinMode(pinData4, OUTPUT);
    pinMode(pinData5, OUTPUT);
    pinMode(pinData6, OUTPUT);
    pinMode(pinData7, OUTPUT);

    // Set the contrast
    //analogWrite(pinContrast, 7);

    // Clear the rest of the pins
    digitalWrite(pinRs, LOW);
    digitalWrite(pinRW, LOW);
    digitalWrite(pinE, LOW);
    digitalWrite(pinData4, LOW);
    digitalWrite(pinData5, LOW);
    digitalWrite(pinData7, LOW);
    digitalWrite(pinData6, LOW);

    // Initialize the display;
    setDataPins(0x00);
    delay(100);
    setDataPins(0x30);
    delay(30);
    nibble();
    delay(10);
    nibble();
    delay(10);
    nibble();
    delay(10);
    setDataPins(0x20);
    nibble();
    command(0x28);
    command(0x10);
    command(0x0C);
    command(0x06);
    command(0x02);
    command(0x01);
}

void Display::setLine(uint8_t line, char *value) {
    // Set the cursor to the beginning of the line
    if (line == 0) {
        command(0x80);
    } else if (line == 1) {
        command(0x80 | 0x40);
    } else if (line == 2) {
        command(0x80 | 0x14);
    } else if (line == 3) {
        command(0x80 | 0x54);
    }

    // Write the line
    for (int i = 0; i < 20; i++) {
        uint8_t c = value[i];
        if (c == '\0') {
            for (i < 20; i++) {
                write(0);
            }
        } else {
            write(c);
        }
    }
}

void Display::command(uint8_t command) {
    // Set the command mode
    digitalWrite(pinRs, LOW);
    digitalWrite(pinRW, LOW);

    // Set the data pins
    setDataPins(command);
    nibble();
    setDataPins(command << 4);
    nibble();
    delay(1);
}

void Display::write(uint8_t value) {
    // Set the data mode
    digitalWrite(pinRs, HIGH);
    digitalWrite(pinRW, LOW);

    // Set the data pins
    setDataPins(value);
    nibble();
    value = value << 4;
    setDataPins(value);
    nibble();
    delay(1);
}

void Display::nibble() {
    // Pulse the enable pin
    digitalWrite(pinE, HIGH);
    delayMicroseconds(1);
    digitalWrite(pinE, LOW);
    delayMicroseconds(1);
}

void Display::setDataPins(uint8_t value) {
    // Set the data pins based on the value
    digitalWrite(pinData4, (value & 0x10) != 0);
    digitalWrite(pinData5, (value & 0x20) != 0);
    digitalWrite(pinData6, (value & 0x40) != 0);
    digitalWrite(pinData7, (value & 0x80) != 0);
    delay(1);
}