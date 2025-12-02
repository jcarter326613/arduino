#include <Arduino.h>
#include <NimBLEDevice.h>
#include <esp_adc_cal.h>

static NimBLEUUID serviceUuid("b207b7f1-acf6-48eb-8cd7-05fb3fef88f8");

// Read/Write characteristics
static NimBLEUUID fanSpeedCharacteristicUuid("061f0262-3fa8-4b48-a609-444658a8ba7e");
static NimBLEUUID buildingLevelCharacteristicUuid("c05ca475-979f-44d3-be0b-7988ef98a170");

// Read characteristics
static NimBLEUUID radonLevelCharacteristicUuid("a5e32290-d139-43f5-acdb-8ec6043b7368");

// Notify characteristics
static NimBLEUUID freshBootCharacteristicUuid("69f5f863-424e-47dd-a408-2d0dc45f7720");

static uint8_t OPERATION_MODE_OFF = 0;
static uint8_t OPERATION_MODE_ON = 1;
static uint8_t OPERATION_MODE_BOOST = 2;
static uint8_t DEVICE_ON = HIGH;
static uint8_t DEVICE_OFF = LOW;
static uint8_t LED_PIN = 2;
static uint8_t FAN_PIN = 18;
static uint8_t BOOST_PIN = 19;
static uint8_t RADON_READING_PIN = 34;
static uint8_t AIR_IN_LEVEL1_OPEN = 32;
static uint8_t AIR_IN_BASEMENT_OPEN = 33;
static uint8_t AIR_OUT_LEVEL1_OPEN = 25;
static uint8_t AIR_OUT_BASEMENT_OPEN = 26;

static uint8_t FAN_SPEED_OFF = 0;
static uint8_t FAN_SPEED_MEDIUM = 1;
static uint8_t FAN_SPEED_HIGH = 2;

static uint8_t BUILD_LEVEL_BASEMENT = 0;
static uint8_t BUILD_LEVEL_1 = 1;

static uint8_t fanSpeed = FAN_SPEED_MEDIUM;
static uint8_t buildingLevel = BUILD_LEVEL_1;
static const float radonVoltageDivision = (10000.0+5600.0) / ((10000.0+5600.0) + (22000.0+47000));

static NimBLECharacteristic* gRadonLevelChar = nullptr;
static NimBLECharacteristic* pFreshBootChar = nullptr;


class FanSpeedCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo &connInfo) {
        std::string value = pCharacteristic->getValue();
        Serial.print("FanSpeedCallbacks onWrite from client: ");
        Serial.println(value.c_str());

        if (value == "OFF") {
            fanSpeed = FAN_SPEED_OFF;
        } else if (value == "MEDIUM") {
            fanSpeed = FAN_SPEED_MEDIUM;
        } else if (value == "HIGH") {
            fanSpeed = FAN_SPEED_HIGH;
        }
    }
};

class BuildingLevelCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo &connInfo) {
        std::string value = pCharacteristic->getValue();
        Serial.print("BuildingLevelCallbacks onWrite from client: ");
        Serial.println(value.c_str());

        if (value == "BASEMENT") {
            buildingLevel = BUILD_LEVEL_BASEMENT;
        } else if (value == "1") {
            buildingLevel = BUILD_LEVEL_1;
        }
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

String generateUUID() {
    uint8_t uuid[16];

    for (int i = 0; i < 16; i++) {
        uuid[i] = (uint8_t)(esp_random() & 0xFF);
    }

    // Set version (4) and variant (10xx)
    uuid[6] = (uuid[6] & 0x0F) | 0x40;   // version 4
    uuid[8] = (uuid[8] & 0x3F) | 0x80;   // variant 10xxxxxx

    char buf[37]; // 36 chars + null
    snprintf(
        buf,
        sizeof(buf),
        "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        uuid[0], uuid[1], uuid[2], uuid[3],
        uuid[4], uuid[5],
        uuid[6], uuid[7],
        uuid[8], uuid[9],
        uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]
    );

    return String(buf);
}

void setup() {
    // Setup debug   writing
    Serial.begin(9600);

    // Create the bluetooth service
    NimBLEDevice::init("Vent_Controller");

    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService *pService = pServer->createService(serviceUuid);

    // Add the fan speed characteristic
    NimBLECharacteristic *pFanSpeedCharacteristic = pService->createCharacteristic(
        fanSpeedCharacteristicUuid,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pFanSpeedCharacteristic->setCallbacks(new FanSpeedCallbacks());

    // Add the building level characteristic
    NimBLECharacteristic *pBuildingLevelCharacteristic = pService->createCharacteristic(
        buildingLevelCharacteristicUuid,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pBuildingLevelCharacteristic->setCallbacks(new BuildingLevelCallbacks());

    // Add the radon characteristic
    gRadonLevelChar = pService->createCharacteristic(
        radonLevelCharacteristicUuid,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    float initialRadon = 0.0f;
    gRadonLevelChar->setValue((uint8_t*)&initialRadon, sizeof(initialRadon));

    // Add the fresh boot characteristic
    pFreshBootChar = pService->createCharacteristic(
        freshBootCharacteristicUuid,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    String bootGuid = generateUUID();
    pFreshBootChar->setValue((uint8_t*)bootGuid.c_str(), bootGuid.length());

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
    pinMode(AIR_IN_LEVEL1_OPEN, OUTPUT);
    pinMode(AIR_IN_BASEMENT_OPEN, OUTPUT);
    pinMode(AIR_OUT_LEVEL1_OPEN, OUTPUT);
    pinMode(AIR_OUT_BASEMENT_OPEN, OUTPUT);

    // Setup Analog Input pins
    analogSetWidth(ADC_WIDTH_BIT_12);
    analogSetPinAttenuation(RADON_READING_PIN, ADC_11db); 
}

static uint16_t loopDelayTimeMs = 10;
static uint16_t msBetweenOperationModeChanges = 1000 * 30;
static uint16_t loopsBetweenOperationModeChanges = msBetweenOperationModeChanges / loopDelayTimeMs;
static uint16_t currentLoopNumber = 0;

void loop() {
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
        sprintf(output, "FanSpeed %d.  BuildingLevel %d.  RadonLevel %.2f", fanSpeed, buildingLevel, radonValue);
        Serial.println(output);

        // Update the fan pins
        if (fanSpeed == FAN_SPEED_OFF) {
            digitalWrite(LED_PIN, LOW);
            digitalWrite(FAN_PIN, DEVICE_OFF);
            digitalWrite(BOOST_PIN, DEVICE_OFF);
        } else if (fanSpeed == FAN_SPEED_MEDIUM) {
            digitalWrite(LED_PIN, HIGH);
            digitalWrite(FAN_PIN, DEVICE_ON);
            digitalWrite(BOOST_PIN, DEVICE_OFF);
        } else if (fanSpeed == FAN_SPEED_HIGH) {
            digitalWrite(LED_PIN, HIGH);
            digitalWrite(FAN_PIN, DEVICE_ON);
            digitalWrite(BOOST_PIN, DEVICE_ON);
        }

        // Update the building level
        if (buildingLevel == BUILD_LEVEL_BASEMENT) {
            digitalWrite(AIR_IN_LEVEL1_OPEN, DEVICE_OFF);
            digitalWrite(AIR_IN_BASEMENT_OPEN, DEVICE_ON);
            digitalWrite(AIR_OUT_LEVEL1_OPEN, DEVICE_OFF);
            digitalWrite(AIR_OUT_BASEMENT_OPEN, DEVICE_ON);
        } else if (buildingLevel == BUILD_LEVEL_1) {
            digitalWrite(AIR_IN_LEVEL1_OPEN, DEVICE_ON);
            digitalWrite(AIR_IN_BASEMENT_OPEN, DEVICE_OFF);
            digitalWrite(AIR_OUT_LEVEL1_OPEN, DEVICE_ON);
            digitalWrite(AIR_OUT_BASEMENT_OPEN, DEVICE_OFF);
        }
    }

    // Iterate
    currentLoopNumber = (currentLoopNumber + 1) % loopsBetweenOperationModeChanges;
    delay(loopDelayTimeMs);
}
