#include <Arduino.h>

class SensevalScb4xv1 {
    public:
        bool getReadings(uint16_t &voc, uint16_t &tempurature, uint16_t &humidity);

    private:
        bool retrieveVoc(uint8_t buffer[6], uint16_t &voc_out);
        bool retrieveTemperatureAndHumidity(uint8_t serialData[6], uint16_t &tempurature, uint16_t &humidity);
        bool retrieveI2cValue(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t outputBuffer[], uint8_t outputBufferSize, uint8_t &outputBytesAvailable, uint8_t delayMs);
        bool retrieveI2cValueWithParameters(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t parameters[], uint8_t parametersLength, uint8_t outputBuffer[], uint8_t outputBufferSize, uint8_t &outputBytesAvailable, uint8_t delayMs);

        bool verifyChecksum(byte data[6], uint16_t length);
        bool verifyChecksumPiece(byte data[2], byte checksum);
        byte computeCrc8Simple(byte bytes[2]);

};
