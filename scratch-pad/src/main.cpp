#include <Arduino.h>
#include "esp_sleep.h"
#include "driver/uart.h"

RTC_DATA_ATTR int bootCount = 0;
const int LED_PIN = 2;  // On most ESP32 DevKit boards the onboard LED is GPIO 2

void setup() {
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(9600);
  delay(1000); // wait for serial

  bootCount++;
  Serial.printf("Boot number: %d\n", bootCount);

  esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("Reset reason: %d\n", reason);

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  Serial.printf("Wakeup cause: %d\n", cause);
  Serial.printf("Wakeup status touchpad: %d\n", esp_sleep_get_touchpad_wakeup_status());
  Serial.printf("Wakeup status ext1: %d\n", esp_sleep_get_ext1_wakeup_status());
  

  Serial.println("Sleeping for 10 minutes...");
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_sleep_enable_timer_wakeup(10ULL * 60ULL * 1000000ULL);
  //esp_sleep_enable_timer_wakeup(10ULL * 1000000ULL);
}

void loop() {

  Serial.flush();                                // empty Arduino Serial buffer
  uart_wait_tx_idle_polling((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM); // wait for hardware TX FIFO


  esp_deep_sleep_start();

  // Never reached
  Serial.println("This will never print");
}
