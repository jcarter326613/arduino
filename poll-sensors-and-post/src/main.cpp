#include <Arduino.h>
#include <Wire.h>
#include <string>

#include "network-communication.h"
#include "senseval-scb4xv1.h"
#include "sensiron-scd30.h"

// Convert SSL certs to ECP32 variable
// https://unreeeal.github.io/ssl_esp.html
// Test TLS attributes
// https://www.ssllabs.com/ssltest/analyze.html

void sendMeasurementExample();

bool hasEvenConnected = false;
NetworkCommunication networkCommunication("11111111-1111-1111-1111-111111111111");
SensevalScb4xv1 sensevalScb4xv1;
SensironScd30 sensironScd30;

const uint16_t signalPin = 2;

void setup() {
    pinMode(signalPin, OUTPUT);

    Serial.begin(9600);
    Serial.println();
    Serial.println();
}

void loop() {
    // Connect to the network
    while (!networkCommunication.isConnected()) {
        Serial.println("t3");
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
    float co2;
    float co2Tempurature;
    float co2Humidity;
    uint16_t voc;
    uint16_t temperature;
    uint16_t humidity;

    bool success1 = sensironScd30.getReadings(co2, co2Tempurature, co2Humidity);
    delay(100);
    bool success2 = sensevalScb4xv1.getReadings(voc, temperature, humidity);

    if (success1) {
        Serial.println("sensironScd30 success");
    } else {
        Serial.println("sensironScd30 failed");
    }
    if (success2) {
        Serial.println("sensevalScb4xv1 success");
    } else {
        Serial.println("sensevalScb4xv1 failed");
    }
    
    bool success = success1 && success2;

    // Report the data
    if (success) {
        // Send the Temperature
        std::string tempString = std::to_string(temperature);
        Measurement measurementTemp("Temperature", "fahrenheit", tempString.data());
        if (!networkCommunication.sendMeasurement(measurementTemp)) {
            Serial.println("Failed to send measurement.");
        } else {
            Serial.println("Temperature measurement sent.");
        }

        // Send the VOC
        std::string humString = std::to_string(humidity);
        Measurement measurementHumidity("Humidity", "%RH", humString.data());
        if (!networkCommunication.sendMeasurement(measurementHumidity)) {
            Serial.println("Failed to send measurement.");
        } else {
            Serial.println("Humidity measurement sent.");
        }

        // Send the VOC
        std::string vocString = std::to_string(voc);
        Measurement measurement("VOC", "none", vocString.data());
        if (!networkCommunication.sendMeasurement(measurement)) {
            Serial.println("Failed to send measurement.");
        } else {
            Serial.println("VOC measurement sent.");
        }

        // Send temp 2        
        std::string temp2String = std::to_string(co2Tempurature);
        Measurement measurementTemp2("Temperature2", "fahrenheit", temp2String.data());
        if (!networkCommunication.sendMeasurement(measurementTemp2)) {
            Serial.println("Failed to send measurement.");
        } else {
            Serial.println("Temperature 2 measurement sent.");
        }
        
        // Send hum 2        
        std::string hum2String = std::to_string(co2Humidity);
        Measurement measurementHum2("Humidity2", "%RH", hum2String.data());
        if (!networkCommunication.sendMeasurement(measurementHum2)) {
            Serial.println("Failed to send measurement.");
        } else {
            Serial.println("Humidity 2 measurement sent.");
        }
        
        // Send co2        
        std::string co2String = std::to_string(co2);
        Measurement measurementCo2("CO2", "ppm", co2String.data());
        if (!networkCommunication.sendMeasurement(measurementCo2)) {
            Serial.println("Failed to send measurement.");
        } else {
            Serial.println("CO2 measurement sent.");
        }
        
        // Wait for 10 minutes
        //delay(10 * 60 * 1000);
    } else {
        // We failed, so only wait a half second before retrying
        Serial.println("t0");
        //delay(500);
        Serial.println("t1");
    }
    
    digitalWrite(signalPin, HIGH);
    delay(5000);
}
