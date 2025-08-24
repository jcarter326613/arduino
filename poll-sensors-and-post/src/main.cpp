#include <Arduino.h>
#include <Wire.h>
#include <string>

#include "network-communication.h"
#include "senseval-scb4xv1.h"

// Convert SSL certs to ECP32 variable
// https://unreeeal.github.io/ssl_esp.html
// Test TLS attributes
// https://www.ssllabs.com/ssltest/analyze.html

void sendMeasurementExample();

bool hasEvenConnected = false;
NetworkCommunication networkCommunication("11111111-1111-1111-1111-111111111111");
SensevalScb4xv1 sensevalScb4xv1;

void setup() {
    Serial.begin(9600);
    Serial.println();
    Serial.println();
}

void loop() {
    // Connect to the network
    while (!networkCommunication.isConnected()) {
        if (!hasEvenConnected) {
            Serial.println("Connecting to network.");
        } else {
            Serial.println("Reconnecting to network.");
        }

        while (!networkCommunication.connect()) {
            delay(5000);   // Wait 5 seconds between connection attempts
        }

        Serial.println("Connection complete.");

        hasEvenConnected = true;
    }

    // Read the sensors
    uint16_t voc;
    uint16_t temperature;
    uint16_t humidity;
    if (sensevalScb4xv1.getReadings(voc, temperature, humidity)) {
        // Send the VOC
        std::string vocString = std::to_string(voc);
        Measurement measurement("VOC", "none", vocString.data());
        if (!networkCommunication.sendMeasurement(measurement)) {
            Serial.println("Failed to send measurement.");
        } else {
            Serial.println("VOC measurement sent.");
        }
    }

    // Wait for 10 minutes
    //delay(10 * 60 * 1000);
    delay(10 * 1000);
}
