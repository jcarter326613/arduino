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

    Serial.println("Starting test in 5 seconds");

    delay(5000);

    digitalWrite(ledPin, HIGH);
    digitalWrite(signalPin, HIGH);

    Serial.println("Timer triggered");
    
    delay(2000);
    
    digitalWrite(ledPin, LOW);
    digitalWrite(signalPin, LOW);

    Serial.println("Trigger disabled");

    // Indicate the 10 seconds is over
    delay(8000);
    for (int i = 0; i < 5; i++) {
        digitalWrite(ledPin, HIGH);
        delay(500);
        digitalWrite(ledPin, LOW);
        delay(500);
    }

    // Loop forever
    while(true) {
        delay(10000);
    }
}
