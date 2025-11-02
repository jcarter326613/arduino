#include <Arduino.h>

class Display {
    public:
        Display();
        void setLine(uint8_t line, char *value);

    private:
        void command(uint8_t command);
        void write(uint8_t value);
        void nibble();
        void setDataPins(uint8_t value);
};
