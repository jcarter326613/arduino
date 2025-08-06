#include <Arduino.h>

class EepromWriter {
    public:
        EepromWriter();
        void writeShort(uint16_t value);
        const void printMemory();

    private:
        const uint16_t minimumDataAddress;
        const uint16_t eepromSize;

        uint16_t highestWrittenAddress;
        uint16_t nextAddress;
};