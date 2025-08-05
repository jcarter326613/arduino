class EepromWriter {
    public:
        EepromWriter();
        void writeShort(short value);

    private:
        const unsigned int minimumDataAddress;
        const unsigned int eepromSize;

        unsigned int highestWrittenAddress;
        unsigned int firstWrittenAddress;
        unsigned int nextAddress;
};