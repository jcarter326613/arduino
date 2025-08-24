#include <Arduino.h>

#include "measurement.h"

Measurement::Measurement(char type[], char unit[], char value[]) {
    this->type = new char[strlen(type) + 1];
    strcpy(this->type, type);
    
    this->unit = new char[strlen(unit) + 1];
    strcpy(this->unit, unit);
    
    this->value = new char[strlen(value) + 1];
    strcpy(this->value, value);
}

Measurement::~Measurement() {
    delete type;
    delete unit;
    delete value;
}