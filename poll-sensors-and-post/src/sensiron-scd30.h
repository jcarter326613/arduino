#include <Arduino.h>

class SensironScd30 {
    private:
        bool isInitialized;

    public:
        SensironScd30();
        bool getReadings(float &voc, float &tempurature, float &humidity);

    private:
        void initialize();

        bool waitForReady();
        bool retrieveCo2(        
            float &co2,
            float &temp,
            float &hum
        );

        bool writeI2cValue(uint8_t address, uint8_t command[], uint8_t commandLength);
        bool writeI2cValueWithParameters(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t parameters[], uint8_t parametersLength);
        bool retrieveI2cValue(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t outputBuffer[], uint8_t outputBufferSize, uint8_t &outputBytesAvailable);
        bool retrieveI2cValueWithParameters(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t parameters[], uint8_t parametersLength, uint8_t outputBuffer[], uint8_t outputBufferSize, uint8_t &outputBytesAvailable);

        bool verifyChecksum(byte data[6], uint16_t length);
        bool verifyChecksumPiece(byte data[2], byte checksum);
        byte computeCrc8Simple(byte bytes[2]);

        
        float sensirion_common_bytes_to_float(const uint8_t* bytes);
        uint32_t sensirion_common_bytes_to_uint32_t(const uint8_t* bytes);
};
