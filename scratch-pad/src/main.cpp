#include <Arduino.h>
#include <Wire.h>

const uint16_t ledPin = LED_BUILTIN;
const uint16_t signalPin = 2;

void setup() {
    pinMode(ledPin, OUTPUT);
    pinMode(signalPin, OUTPUT);

    Serial.begin(9600);
    Serial.println();
    Serial.println();
}

void loop() {
    digitalWrite(ledPin, LOW);
    digitalWrite(signalPin, LOW);

    delay(5000);

    digitalWrite(ledPin, HIGH);
    digitalWrite(signalPin, HIGH);
    
    delay(5000);
}
