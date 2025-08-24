struct Measurement {
    char *type;
    char *unit;
    char *value;
    
    Measurement(char type[], char unit[], char value[]);
    ~Measurement();
};