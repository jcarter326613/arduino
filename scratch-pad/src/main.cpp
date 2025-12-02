#include <Arduino.h>
#include "esp_adc_cal.h"

static esp_adc_cal_characteristics_t adc_chars;
static const adc_unit_t unit = ADC_UNIT_1;
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const adc1_channel_t ch = ADC1_CHANNEL_4;     // GPIO32

static const uint8_t inputPin = 34;

void setup() {
    Serial.begin(9600);

    analogSetWidth(width);
    analogSetPinAttenuation(inputPin, ADC_11db); 
}

static const float voltageDivider = (10000.0+5600.0) / ((10000.0+5600.0) + (22000.0+47000));

float readVolts(int pin) {
    int raw = analogRead(pin);              // 0..4095
    return (raw / 4095.0f) * 1.1f * voltageDivider;          // approx; ESP32 ADC is non-linear
}

void loop() {
    int mv = analogReadMilliVolts(inputPin); 
    Serial.print("Analog read mV: "); 
    Serial.println(mv);

    Serial.print("Analog read mV corrected: "); 
    Serial.println(mv / voltageDivider);
    Serial.print("analogRead: "); 
    Serial.println(analogRead(inputPin));

    delay(1000);
}
