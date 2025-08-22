#include "eeprom-writer.h"

#include <Arduino.h>
#include <EEPROM.h>

EepromWriter::EepromWriter():
    minimumDataAddress(sizeof(highestWrittenAddress) + sizeof(nextAddress)),
    eepromSize(EEPROM.length())
{
    EEPROM.get(0, highestWrittenAddress);
    EEPROM.get(sizeof(highestWrittenAddress), nextAddress);
}

void EepromWriter::writeShort(uint16_t value) {
    // If we are initializing
    if (nextAddress == 0) {
        nextAddress = sizeof(highestWrittenAddress) + sizeof(nextAddress);
    }

    // See if we need to wrap
    if (nextAddress + sizeof(short) * 2 > eepromSize) {
        highestWrittenAddress = nextAddress - 1;
        nextAddress = minimumDataAddress;
    }

    // Write the value
    EEPROM.put(nextAddress, value);

    // Write the time
    EEPROM.put(nextAddress + sizeof(value), (short)(millis() / 1000));

    // Move to the next address
    nextAddress += sizeof(value) + sizeof(short);

    // Update the highest address if needed
    if (highestWrittenAddress < nextAddress - 1) {
        highestWrittenAddress = nextAddress - 1;
    }

    // Update all the address records
    EEPROM.put(0, highestWrittenAddress);
    EEPROM.put(sizeof(highestWrittenAddress), nextAddress);
}

const void EepromWriter::printMemory() {
    uint16_t currentAddress = nextAddress;
    uint8_t loopCount = 0;

    do {
        // Read out the values
        int16_t value;
        int16_t time;
        EEPROM.get(currentAddress, value);
        EEPROM.get(currentAddress + sizeof(short), time);

        // Print out the adresss
        char output[50];
        snprintf(output, sizeof(output), "%d, %d", time, value);
        Serial.println(output);

        // Iterate
        currentAddress += sizeof(short) * 2;
        if (currentAddress > highestWrittenAddress) {
            currentAddress = minimumDataAddress;
            loopCount++;
        }
    } while (currentAddress != nextAddress && loopCount < 2);
}
