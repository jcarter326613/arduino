#include <Arduino.h>

#include "display.h"
#include "sensiron-scd30.h"

Display *display;
SensironScd30 sensironScd30;

void setup() {
    Serial.begin(9600);
    Serial.println();
    Serial.println();

    display = new Display();
}

void loop() {
    float co2;
    float co2Tempurature;
    float co2Humidity;
    bool success1 = sensironScd30.getReadings(co2, co2Tempurature, co2Humidity);

    display->setLine(1, "Hello World");

    delay(10000);
}
