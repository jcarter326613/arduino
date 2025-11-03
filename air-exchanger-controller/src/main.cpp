#include <Arduino.h>
#include <NimBLEDevice.h>
#include <esp_adc_cal.h>

static NimBLEUUID serviceUuid("b207b7f1-acf6-48eb-8cd7-05fb3fef88f8");
static NimBLEUUID co2LevelCharacteristicUuid("030113cf-4ea5-4c9b-9c9c-e1dd84ce4970");
static NimBLEUUID radonLevelCharacteristicUuid("7e1b9f3a-5f6f-4b2c-a4db-8f8cb2d7f2a1");

static uint8_t OPERATION_MODE_OFF = 0;
static uint8_t OPERATION_MODE_ON = 1;
static uint8_t OPERATION_MODE_BOOST = 2;
static uint8_t DEVICE_ON = LOW;
static uint8_t DEVICE_OFF = HIGH;
static uint8_t LED_PIN = 2;
static uint8_t FAN_PIN = 18;
static uint8_t BOOST_PIN = 19;
static uint8_t RADON_READING_PIN = 34;

static uint8_t CO2_LEVEL_ACCEPTABLE = 0;
static uint8_t CO2_LEVEL_HIGH = 1;
static uint8_t CO2_LEVEL_CLIMBING = 2;

static uint8_t co2Level = CO2_LEVEL_HIGH;
static const float radonVoltageDivision = (10000.0+5600.0) / ((10000.0+5600.0) + (22000.0+47000));

static NimBLECharacteristic* gRadonLevelChar = nullptr;


class Co2LevelCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo &connInfo) {
        std::string value = pCharacteristic->getValue();
        Serial.print("onWrite from client: ");
        Serial.println(value.c_str());

        if (value == "ACCEPTABLE") {
            co2Level = CO2_LEVEL_ACCEPTABLE;
        } else if (value == "HIGH") {
            co2Level = CO2_LEVEL_HIGH;
        } else if (value == "CLIMBING") {
            co2Level = CO2_LEVEL_CLIMBING;
        }
    }
    
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo &connInfo) {
        std::string value = pCharacteristic->getValue();
        Serial.print("onRead from client: ");
        Serial.println(value.c_str());
    }
    
    void onStatus(NimBLECharacteristic* pCharacteristic, int code) {
        std::string value = pCharacteristic->getValue();
        Serial.print("onStatus from client: ");
        Serial.println(value.c_str());
    }
    
    void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue) {
        std::string value = pCharacteristic->getValue();
        Serial.print("onSubscribe from client: ");
        Serial.println(value.c_str());
    }
};

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* s, NimBLEConnInfo &connInfo) {
        Serial.println("Client connected");
    }
    void onDisconnect(NimBLEServer* s, NimBLEConnInfo &connInfo, int reason) {
        Serial.println("Client disconnected, restarting advertising");
        NimBLEDevice::startAdvertising();
    }
};

float currentRadonLevel() {
    float voltage = analogReadMilliVolts(RADON_READING_PIN) / radonVoltageDivision;
    return ((voltage - 1) / 9.0) * 11;  // Current pCi/L
}

void setup() {
    // Setup debug   writing
    Serial.begin(9600);

    // Create the bluetooth service
    NimBLEDevice::init("Vent_Controller");

    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService *pService = pServer->createService(serviceUuid);

    // Add the co2 characteristic
    NimBLECharacteristic *pCharacteristic = pService->createCharacteristic(
        co2LevelCharacteristicUuid,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );

    pCharacteristic->setCallbacks(new Co2LevelCallbacks());

    // Add the radon characteristic
    gRadonLevelChar = pService->createCharacteristic(
        radonLevelCharacteristicUuid,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    float initialRadon = 0.0f;
    gRadonLevelChar->setValue((uint8_t*)&initialRadon, sizeof(initialRadon));

    // Start the service
    pService->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(serviceUuid);
    NimBLEDevice::startAdvertising();
    Serial.println("BLE server is advertising");

    // Setup IO Output pins
    pinMode(LED_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);
    pinMode(BOOST_PIN, OUTPUT);

    // Setup Analog Input pins
    analogSetWidth(ADC_WIDTH_BIT_12);
    analogSetPinAttenuation(RADON_READING_PIN, ADC_11db); 
}

static uint16_t loopDelayTimeMs = 10;
static uint16_t msBetweenOperationModeChanges = 1000 * 30;
static uint16_t loopsBetweenOperationModeChanges = msBetweenOperationModeChanges / loopDelayTimeMs;
static uint16_t currentLoopNumber = 0;

static uint8_t operationMode = OPERATION_MODE_ON;

void loop() {
    // Update the operation mode based on the CO2
    if (co2Level == CO2_LEVEL_ACCEPTABLE) {
        operationMode = OPERATION_MODE_OFF;
    } else if (co2Level == CO2_LEVEL_HIGH) {
        operationMode = OPERATION_MODE_ON;
    } else if (co2Level == CO2_LEVEL_CLIMBING) {
        operationMode = OPERATION_MODE_BOOST;
    }

    // Update the fans
    if (currentLoopNumber == 0) {
        // Update the radon value
        float radonValue = currentRadonLevel();
        if (gRadonLevelChar) {
            gRadonLevelChar->setValue((uint8_t*)&radonValue, sizeof(radonValue));
            gRadonLevelChar->notify();
        }

        // Output our current readings
        char output[50];
        sprintf(output, "Setting mode %d.  Co2Level %d.  RadonLevel %.2f", operationMode, co2Level, radonValue);
        Serial.println(output);

        // Update the fan pins
        if (operationMode == OPERATION_MODE_OFF) {
            digitalWrite(LED_PIN, LOW);
            digitalWrite(FAN_PIN, DEVICE_OFF);
            digitalWrite(BOOST_PIN, DEVICE_OFF);
        } else if (operationMode == OPERATION_MODE_ON) {
            digitalWrite(LED_PIN, HIGH);
            digitalWrite(FAN_PIN, DEVICE_ON);
            digitalWrite(BOOST_PIN, DEVICE_OFF);
        } else if (operationMode == OPERATION_MODE_BOOST) {
            digitalWrite(LED_PIN, HIGH);
            digitalWrite(FAN_PIN, DEVICE_ON);
            digitalWrite(BOOST_PIN, DEVICE_ON);
        }
    }

    // Iterate
    currentLoopNumber = (currentLoopNumber + 1) % loopsBetweenOperationModeChanges;
    delay(loopDelayTimeMs);
}
