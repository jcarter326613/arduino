#include <Arduino.h>
#include <Wire.h>

#include "senseval-scb4xv1.h"

const uint8_t TEMP_SENSOR_ADDRESS = 0x44;
const uint8_t VOC_SENSOR_ADDRESS = 0x59;

bool SensevalScb4xv1::getReadings(uint16_t &voc, uint16_t &tempurature, uint16_t &humidity) {
    Wire.begin();

    // Get the values
    uint8_t buffer[6];
    bool success = retrieveTemperatureAndHumidity(buffer, tempurature, humidity);
    if (success) {
        voc = retrieveVoc(buffer);
        success = voc >= 0;
    }

    Wire.end();

    return success;
}

uint16_t SensevalScb4xv1::retrieveVoc(uint8_t tempAndHumidityBuffer[6]) {
    uint8_t serialData[3] = {0, 0, 0};
    uint8_t command[2] = {0x26, 0x0F};

    bool checksumSuccess = retrieveI2cValueWithParameters(VOC_SENSOR_ADDRESS, command, 2, tempAndHumidityBuffer, 6, serialData, 3, 30);
    if (checksumSuccess) {
        uint16_t voc = (serialData[0] << 8) | serialData[1];
        return voc;
    } else {
        Serial.println("Checksum is invalid for VOC.");
        return -1;
    }
}

bool SensevalScb4xv1::retrieveTemperatureAndHumidity(uint8_t serialData[6], uint16_t &temperatureOut, uint16_t &humidityOut) {
    uint8_t command[1] = {0xFD};
    bool checksumSuccess = retrieveI2cValue(TEMP_SENSOR_ADDRESS, command, 1, serialData, 6, 10);
    if (checksumSuccess) {
        // Seperate out the values
        uint16_t temperature = (serialData[0] << 8) | serialData[1];
        uint16_t humidity = (serialData[3] << 8) | serialData[4];

        // Convert the values
        temperatureOut = (temperature * 315.0 / 65535.0) - 49.0;
        humidityOut = (humidity * 125.0 / 65535.0) - 6.0;

        return true;
    } else { 
        Serial.println("Checksum is invalid for temperature and humidity.");
        return false;
    }
}

bool SensevalScb4xv1::retrieveI2cValue(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t outputBuffer[], uint8_t outputBufferSize, uint8_t delayMs) {
    uint8_t parameters[0];
    return retrieveI2cValueWithParameters(address, command, commandLength, parameters, 0, outputBuffer, outputBufferSize, delayMs);
}

bool SensevalScb4xv1::retrieveI2cValueWithParameters(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t parameters[], uint8_t parametersLength, uint8_t outputBuffer[], uint8_t outputBufferSize, uint8_t delayMs) {
    // Send the request
    Wire.beginTransmission(address); // Address of the device
    for (uint8_t i = 0; i < commandLength; i++) {
        Wire.write(command[i]);
    }
    for (uint8_t i = 0; i < parametersLength; i++) {
        Wire.write(parameters[i]);
    }
    Wire.endTransmission();

    // Listen for the response
    delay(delayMs);
    Wire.requestFrom(address, outputBufferSize);
    uint8_t i = 0;
    while (Wire.available()) {
        // Read the byte and store it
        char c = Wire.read();
        if (i < outputBufferSize) {
            outputBuffer[i] = c;
        }
        i++;
    }

    return i == outputBufferSize && verifyChecksum(outputBuffer, outputBufferSize);
}

bool SensevalScb4xv1::verifyChecksum(byte data[], uint16_t length) {
    if (length % 3 != 0) {
        Serial.println("Eval board Checksum failed length");
        return false;
    }

    for (uint16_t i = 0; i < length; i += 3) {
        if (!verifyChecksumPiece(data + i, data[i + 2])) {
            char output[50];
            sprintf(output, "Eval board Checksum failed piece %d", i);
            Serial.println(output);
            return false;
        }
    }
    return true;
}

bool SensevalScb4xv1::verifyChecksumPiece(byte data[2], uint8_t checksum) {
    // Compute the checksum
    byte computedCheckshum = computeCrc8Simple(data);

    // Return the results
    return computedCheckshum == checksum;
}

byte SensevalScb4xv1::computeCrc8Simple(byte bytes[2]) {
    const byte generator = 0x31;
    byte crc = 0xFF;

    for (uint8_t i = 0; i < 2; i++) {
        byte currByte = bytes[i];
        crc ^= currByte;

        for (uint8_t j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0) {
                crc = (byte)((crc << 1) ^ generator);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}
