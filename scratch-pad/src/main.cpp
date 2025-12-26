#include <Arduino.h>

const uint8_t REST_PIN = 27;

void setup() {
    Serial.begin(9600);
    Serial.println("Starting up");

    pinMode(REST_PIN, OUTPUT);
    digitalWrite(REST_PIN, LOW);
}

const uint32_t loopDelayMs = 100;
const uint32_t secondsBeforeReset = 5;
const uint32_t loopsTillReset = secondsBeforeReset * 1000 / loopDelayMs;
uint32_t currentLoop = 0;

void loop() {
    currentLoop++;
    if (currentLoop == loopsTillReset) {
        Serial.println("Releasing reset");
    }
    if (currentLoop == loopsTillReset + 1) {
        digitalWrite(REST_PIN, HIGH);
    }
    
    delay(100);
}
