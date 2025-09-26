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
    bool success = sensironScd30.getReadings(co2, co2Tempurature, co2Humidity);

    if (success) {
        char output[50];
        sprintf(output, "CO2 %d ppm", (uint32_t)co2);
        display->setLine(1, output);
        display->setLine(2, "");
    } else {
        display->setLine(2, "Reading stale");
    }

    delay(10000);
}
