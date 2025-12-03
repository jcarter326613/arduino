#include <Arduino.h>
#include <Wire.h>

#include "sensiron-scd30.h"

// GitHub reference
// https://github.com/Sensirion/embedded-i2c-scd30/blob/master/sensirion_common.c

const uint8_t SENSOR_ADDRESS = 0x61;

SensironScd30::SensironScd30():
    isInitialized(false) 
{  
}

bool SensironScd30::getReadings(float &co2, float &tempurature, float &humidity) {

    if (!initialize()) {
        return false;
    }

    // Get the values
    uint8_t buffer[6];
    Wire.begin();
    bool success = waitForReady();
    Wire.end();
    if (!success) {
        return false;
    }

    Wire.begin();
    success = retrieveCo2(co2, tempurature, humidity);
    Wire.end();
    if (success) {
        tempurature = tempurature * 9 / 5 + 32;
    }


    return success;
}

bool SensironScd30::initialize() {
    if (isInitialized) {
        return true;
    }

    Serial.println("Initializing");

    Wire.begin();
    uint8_t command[2] = {0x00, 0x10};
    uint8_t parameters[2] = {0x00, 0x00};
    bool result = writeI2cValueWithParameters(SENSOR_ADDRESS, command, 2, parameters, 2);
    if (result) {
        isInitialized = true;
        Serial.println("Initializing complete");
    } else {
        Serial.println("Initializing failed");
    }
    Wire.end();

    return isInitialized;
}

bool SensironScd30::waitForReady() {
    uint8_t command[2] = {0x02, 0x02};
    uint8_t buffer[3];
    uint8_t bufferSize = 3;
    uint8_t bytesRead;
    bool success;
    bool isReady;
    while (true) {
        success = retrieveI2cValue(SENSOR_ADDRESS, command, 2, buffer, bufferSize, bytesRead);

        // Debug output
        if (!success || bytesRead < bufferSize) {
            return false;
        }

        isReady = true;
        if (bufferSize >= 1) {
            isReady = buffer[0];
        }
        if (bufferSize >= 2) {
            isReady = isReady || buffer[1];
        }
        if (isReady) {
            return true;
        } else {
            delay(10);
        }
    }
}

bool SensironScd30::retrieveCo2(
    float &co2,
    float &temp,
    float &hum
) {
    uint8_t command[2] = {0x03, 0x00};
    uint8_t buffer[18];
    uint8_t bufferSize = 18;
    uint8_t bytesRead;
    bool success;
    bool isReady;
        
    success = retrieveI2cValue(SENSOR_ADDRESS, command, 2, buffer, bufferSize, bytesRead);
    
    // Convert the output
    if (success) {
        uint8_t conversionBuffer[4];
        conversionBuffer[0] = buffer[0];
        conversionBuffer[1] = buffer[1];
        conversionBuffer[2] = buffer[3];
        conversionBuffer[3] = buffer[4];
        co2 = sensirion_common_bytes_to_float(conversionBuffer);

        conversionBuffer[0] = buffer[6];
        conversionBuffer[1] = buffer[7];
        conversionBuffer[2] = buffer[9];
        conversionBuffer[3] = buffer[10];
        temp = sensirion_common_bytes_to_float(conversionBuffer);
        
        conversionBuffer[0] = buffer[12];
        conversionBuffer[1] = buffer[13];
        conversionBuffer[2] = buffer[15];
        conversionBuffer[3] = buffer[16];
        hum = sensirion_common_bytes_to_float(conversionBuffer);
    }

    return success;
}

bool SensironScd30::writeI2cValue(uint8_t address, uint8_t command[], uint8_t commandLength) {
    uint8_t parameters[0];
    return writeI2cValueWithParameters(address, command, commandLength, parameters, 0);
}

bool SensironScd30::writeI2cValueWithParameters(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t parameters[], uint8_t parametersLength) {
    char output[50];
    uint8_t bytesWritten;
    uint8_t tryCount = 0;
    while (true) {
        Wire.beginTransmission(address); // Address of the device

        bytesWritten = Wire.write(command, commandLength);
        if (bytesWritten != commandLength) {
            sprintf(output, "Only %d command bytes witten.", bytesWritten);
            Serial.println(output);
        }
        for (uint8_t i = 0; i < parametersLength; i++) {
            Wire.write(parameters[i]);

            if (i % 2 != 0) {
                Wire.write(computeCrc8Simple(parameters + i));
            }
        }

        uint8_t result = Wire.endTransmission();

        if (result == 0) {
            break;
        }

        sprintf(output, "End result %d.", result);
        Serial.println(output);
        tryCount++;
        if (tryCount > 3) {
            return false;
        }
        delay(100);
    }

    return true;
}

bool SensironScd30::retrieveI2cValue(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t outputBuffer[], uint8_t outputBufferSize, uint8_t &outputBytesAvailable) {
    uint8_t parameters[0];
    return retrieveI2cValueWithParameters(address, command, commandLength, parameters, 0, outputBuffer, outputBufferSize, outputBytesAvailable);
}

bool SensironScd30::retrieveI2cValueWithParameters(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t parameters[], uint8_t parametersLength, uint8_t outputBuffer[], uint8_t outputBufferSize, uint8_t &outputBytesAvailable) {
    char output[50];
    
    uint8_t bytesWritten;
    uint8_t tryCount = 0;
    while (true) {
        Wire.beginTransmission(address); // Address of the device

        bytesWritten = Wire.write(command, commandLength);
        if (bytesWritten != commandLength) {
            sprintf(output, "Only %d command bytes witten.", bytesWritten);
            Serial.println(output);
        }
        for (uint8_t i = 0; i < parametersLength; i++) {
            Wire.write(parameters[i]);

            if (i % 2 != 0) {
                Wire.write(computeCrc8Simple(parameters + i));
            }
        }

        uint8_t result = Wire.endTransmission();

        if (result == 0) {
            break;
        }

        sprintf(output, "End result %d.", result);
        Serial.println(output);
        tryCount++;
        if (tryCount > 3) {
            return false;
        }
        delay(100);
    }

    // Listen for the response
    Wire.requestFrom(address, outputBufferSize);
    outputBytesAvailable = 0;
    uint16_t failureCount = 0;
    while (true) {
        // Terminate the loop or wait for the first byte
        if (!Wire.available()) {
            if (failureCount < 10) {
                delay(100);
                failureCount++;
                continue;
            } else {
                break;
            }
        }

        // Read the byte and store it
        failureCount = 0;
        char c = Wire.read();
        if (outputBytesAvailable < outputBufferSize) {
            outputBuffer[outputBytesAvailable] = c;
        }
        outputBytesAvailable++;
    }

    if (outputBytesAvailable != outputBufferSize) {
        sprintf(output, "Read %d of %d", outputBytesAvailable, outputBufferSize);
        Serial.println(output);

        for (int i = 0; i < outputBufferSize; i++) {
            sprintf(output, "%02X", outputBuffer[i]);
            Serial.print(output);
        }
        Serial.println();
    }

    return outputBytesAvailable == outputBufferSize && verifyChecksum(outputBuffer, outputBytesAvailable);
}

bool SensironScd30::verifyChecksum(byte data[], uint16_t length) {
    if (length % 3 != 0) {
        Serial.println("SCD30 Checksum failed length");
        return false;
    }

    /*
    char output[50];
    for (int i = 0; i < length; i++) {
        sprintf(output, "%02X", data[i]);
        Serial.print(output);
    }
    Serial.println();
    */

    for (uint16_t i = 0; i < length; i += 3) {
        if (!verifyChecksumPiece(data + i, data[i + 2])) {
            char output[50];
            sprintf(output, "SCD30 Checksum failed piece %d", i);
            Serial.println(output);
            return false;
        }
    }

    return true;
}

bool SensironScd30::verifyChecksumPiece(byte data[2], uint8_t checksum) {
    // Compute the checksum
    byte computedCheckshum = computeCrc8Simple(data);

    // Return the results
    return computedCheckshum == checksum;
}

byte SensironScd30::computeCrc8Simple(byte bytes[2]) {
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

float SensironScd30::sensirion_common_bytes_to_float(const uint8_t* bytes) {
    union {
        uint32_t u32_value;
        float float32;
    } tmp;

    tmp.u32_value = sensirion_common_bytes_to_uint32_t(bytes);
    return tmp.float32;
}

uint32_t SensironScd30::sensirion_common_bytes_to_uint32_t(const uint8_t* bytes) {
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
}