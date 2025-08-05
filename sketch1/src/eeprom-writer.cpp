#include "eeprom-writer.h"

#include <EEPROM.h>

EepromWriter::EepromWriter():
    minimumDataAddress(sizeof(highestWrittenAddress) + sizeof(firstWrittenAddress) + sizeof(nextAddress)),
    eepromSize(EEPROM.length())
{
    EEPROM.get(0, highestWrittenAddress);
    EEPROM.get(sizeof(highestWrittenAddress), firstWrittenAddress);
    EEPROM.get(sizeof(highestWrittenAddress) + sizeof(firstWrittenAddress), nextAddress);

    if (highestWrittenAddress <= firstWrittenAddress && highestWrittenAddress <= nextAddress) {
        highestWrittenAddress = 0;
        firstWrittenAddress = 0;
        nextAddress = 0;
    }
}

void EepromWriter::writeShort(short value) {
    bool shouldUpdateFirst = true;

    // If we are initializing
    if (nextAddress == 0) {
        highestWrittenAddress = 0;
        nextAddress = sizeof(highestWrittenAddress) + sizeof(firstWrittenAddress) + sizeof(nextAddress);
        firstWrittenAddress = nextAddress;
        shouldUpdateFirst = false;
    }

    // See if we need to wrap
    if (nextAddress + sizeof(value) > eepromSize) {
        highestWrittenAddress = nextAddress - 1;
        nextAddress = minimumDataAddress;
    }

    // Write the value
    EEPROM.put(nextAddress, value);

    // Update the highest address if needed
    if (highestWrittenAddress < nextAddress + sizeof(value) - 1) {
        highestWrittenAddress = nextAddress + sizeof(value) - 1;
    }
    if (shouldUpdateFirst) {
        firstWrittenAddress = nextAddress + sizeof(value);
        if (firstWrittenAddress > highestWrittenAddress) {
            firstWrittenAddress = minimumDataAddress;
        }
    }
}
