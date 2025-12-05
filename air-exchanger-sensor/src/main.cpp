#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Wire.h>

#include "display.h"
#include "sensiron-scd30.h"

// Match these to your server code
static NimBLEUUID serviceUuid("b207b7f1-acf6-48eb-8cd7-05fb3fef88f8");

// Read/Write characteristics
static NimBLEUUID fanSpeedCharacteristicUuid("061f0262-3fa8-4b48-a609-444658a8ba7e");
static NimBLEUUID buildingLevelCharacteristicUuid("c05ca475-979f-44d3-be0b-7988ef98a170");

// Read characteristics
static NimBLEUUID radonLevelCharacteristicUuid("a5e32290-d139-43f5-acdb-8ec6043b7368");

// Notify characteristics
static NimBLEUUID freshBootCharacteristicUuid("69f5f863-424e-47dd-a408-2d0dc45f7720");

#define NOT_SET 0
#define FAN_SPEED_OFF 1
#define FAN_SPEED_MEDIUM 2
#define FAN_SPEED_HIGH 3
#define BUILDING_LEVEL_BASEMENT 1
#define BUILDING_LEVEL_1 2
#define MAX_RADON_LEVEL (1.3 * 1000.0)
#define RADON_VERY_HIGH (4.0 * 1000.0)

static const uint8_t I2C_DATA = 21;
static const uint8_t I2C_CLOCK = 22;

static NimBLEAddress targetAddr;
static bool haveTarget = false;

static NimBLEClient* pClient = nullptr;
static NimBLERemoteService* pSvc = nullptr;
static NimBLERemoteCharacteristic* pFanSpeedChar = nullptr;
static NimBLERemoteCharacteristic* pBuildingLevelChar = nullptr;
static NimBLERemoteCharacteristic* pRadonChar = nullptr;
static NimBLERemoteCharacteristic* pFreshBootChar = nullptr;
static NimBLEScan* pScan = nullptr;

Display *display;
SensironScd30 sensironScd30;
static float radonValue = 0;
static float displayedRadon = 0;
static uint8_t currentFanSpeed = NOT_SET;
static uint8_t currentBuildingLevel = NOT_SET;

void destroyClientConnection();

class MyScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* adv) override {
        bool svcMatch = adv->isAdvertisingService(serviceUuid);

        if (svcMatch) {
            Serial.print("Target found: ");
            Serial.println(adv->toString().c_str());
            targetAddr = adv->getAddress();
            haveTarget = true;
            NimBLEDevice::getScan()->stop();
        } else {
            Serial.print("Non matching service ");
            Serial.println(adv->toString().c_str());
        }
    }
};

class MyClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* client) override {
        Serial.println("Client connected");
    }

    void onDisconnect(NimBLEClient* client, int reason) override {
        Serial.printf("Client disconnected, reason = %d\n", reason);

        currentFanSpeed = NOT_SET;
        currentBuildingLevel = NOT_SET;

        destroyClientConnection();
    }
};

static void onRadonNotify(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
    if (len == sizeof(float)) {
        float v;
        memcpy(&v, data, sizeof(float));
        radonValue = v;
    } else {
        Serial.println("Radon notify: unexpected payload size");
    }
}

static void onFreshBootNotify(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
    currentFanSpeed = NOT_SET;
    currentBuildingLevel = NOT_SET;
}

void destroyClientConnection() {
    if (pFanSpeedChar != nullptr) {
        pSvc->deleteCharacteristic(fanSpeedCharacteristicUuid);
        pFanSpeedChar = nullptr;
    }
    if (pBuildingLevelChar != nullptr) {
        pSvc->deleteCharacteristic(fanSpeedCharacteristicUuid);
        pFanSpeedChar = nullptr;
    }
    if (pRadonChar != nullptr) {
        pSvc->deleteCharacteristic(radonLevelCharacteristicUuid);
        pRadonChar = nullptr;
    }
    if (pFreshBootChar != nullptr) {
        pSvc->deleteCharacteristic(freshBootCharacteristicUuid);
        pFreshBootChar = nullptr;
    }
    if (pSvc != nullptr) {
        pClient->deleteService(serviceUuid);
        pSvc = nullptr;
    }
    if (pClient != nullptr) {
        if (pClient->isConnected()) {
            pClient->disconnect();
        }
        NimBLEDevice::deleteClient(pClient);
        pClient = nullptr;
    }

    haveTarget = false;
}

bool connectToServer() {
    Serial.println("Connecting...");

    // Client
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallbacks(), false);
    if (!pClient->connect(targetAddr)) {
        Serial.println("Connect failed");
        destroyClientConnection();
        return false;
    }

    // Service
    pSvc = pClient->getService(serviceUuid);
    if (!pSvc) {
        Serial.println("Service not found");
        destroyClientConnection();
        return false;
    }

    // Fan Speed
    pFanSpeedChar = pSvc->getCharacteristic(fanSpeedCharacteristicUuid);
    if (!pFanSpeedChar) {
        Serial.println("Characteristic not found");
        destroyClientConnection();
        return false;
    }
    Serial.printf("fan speed canWriteNoResponse=%d, canWrite=%d\n", pFanSpeedChar->canWriteNoResponse(), pFanSpeedChar->canWrite());

    if (!pFanSpeedChar->canWrite()) {
        Serial.println("Fan Speed characteristic not writable");
        destroyClientConnection();
        return false;
    }

    // Building Level
    pBuildingLevelChar = pSvc->getCharacteristic(buildingLevelCharacteristicUuid);
    if (!pBuildingLevelChar) {
        Serial.println("Characteristic not found");
        destroyClientConnection();
        return false;
    }
    Serial.printf("fan speed canWriteNoResponse=%d, canWrite=%d\n", pBuildingLevelChar->canWriteNoResponse(), pBuildingLevelChar->canWrite());

    if (!pBuildingLevelChar->canWrite()) {
        Serial.println("Building Level characteristic not writable");
        destroyClientConnection();
        return false;
    }

    // Radon
    pRadonChar = pSvc->getCharacteristic(radonLevelCharacteristicUuid);
    if (!pRadonChar) {
        Serial.println("Radon characteristic not found");
        destroyClientConnection();
        return false;
    }
    Serial.printf("radon canRead=%d, canNotify=%d\n", pRadonChar->canRead(), pRadonChar->canNotify());

    if (!pRadonChar->canRead() || !pRadonChar->canNotify()) {
        Serial.println("Radon characteristic not readable or not notifiable");
        destroyClientConnection();
        return false;
    }
    if (!pRadonChar->subscribe(true, onRadonNotify)) {
        Serial.println("Failed to subscribe to radon notifications");
        destroyClientConnection();
        return false;
    }
    std::string rv = pRadonChar->readValue();
    if (rv.size() == sizeof(float)) {
        memcpy(&radonValue, rv.data(), sizeof(float));
    }

    // Fresh Boot
    pFreshBootChar = pSvc->getCharacteristic(freshBootCharacteristicUuid);
    if (!pFreshBootChar) {
        Serial.println("Frsh boot characteristic not found");
        destroyClientConnection();
        return false;
    }
    Serial.printf("fresh boot canRead=%d, canNotify=%d\n", pFreshBootChar->canRead(), pFreshBootChar->canNotify());

    if (!pFreshBootChar->canRead() || !pFreshBootChar->canNotify()) {
        Serial.println("Fresh boot characteristic not readable or not notifiable");
        destroyClientConnection();
        return false;
    }
    if (!pFreshBootChar->subscribe(true, onFreshBootNotify)) {
        Serial.println("Failed to subscribe to fresh boot notifications");
        destroyClientConnection();
        return false;
    }

    // Done
    Serial.println("Connected");
    return true;
}

static const uint16_t msBetweenLoopDelays = 10;     // 10 milliseconds that the chip is completely frozen
static const uint16_t msBetweenLoops = 1000 * 10;   // 10 seconds between loop runs
static const uint16_t numDelaysBetweenLoops = msBetweenLoops / msBetweenLoopDelays;     // Number of times the loop function needs to be entered before we actually do any work
static uint16_t loopDelayCount = numDelaysBetweenLoops; // Start with the initial state of "run a loop"
static const uint32_t mSecondsPerMinute = 1000 * 60;
static const uint32_t mSecondsPerHour = mSecondsPerMinute * 60;
static const uint32_t loopsPerHour = mSecondsPerHour / msBetweenLoops;
static const uint32_t loopsAllowedForRadonPerHour = (mSecondsPerMinute / msBetweenLoops) * 10;  // 10 minutes allowed per hour
static const uint32_t maxLoopsBankedForRadon = loopsPerHour - loopsAllowedForRadonPerHour;
static const uint32_t loopsSpentPerRadonLoop = maxLoopsBankedForRadon / loopsAllowedForRadonPerHour;
//static const uint32_t maxLoopsBankedForRadon = 200;
//static const uint32_t loopsSpentPerRadonLoop = 2;

static uint8_t desiredFanSpeed = NOT_SET;
static uint8_t desiredLevel1FanSpeed = NOT_SET;
static uint8_t desiredBuildingLevel = NOT_SET;
static uint16_t targetCo2Level = 800;
static uint16_t shutoffCo2Level = 780;
static uint16_t burstCo2Level = 900;
static float co2 = 0;
static uint32_t radonPoints = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("Booting up");
    
    // Bluetooth
    NimBLEDevice::init("");

    pScan = NimBLEDevice::getScan();
    pScan->setActiveScan(true);
    pScan->setInterval(45);
    pScan->setWindow(15);
    pScan->setScanCallbacks(new MyScanCallbacks(), false);

    // I2C
    Wire.setPins(I2C_DATA, I2C_CLOCK);

    // Display
    display = new Display();
}

bool validateConnected() {
    // If we successfully scanned a target but aren't connected, connect
    if (
        haveTarget &&
        (
            pFanSpeedChar == nullptr ||
            pBuildingLevelChar == nullptr ||
            pRadonChar == nullptr ||
            pFreshBootChar == nullptr
        )
    ) {
        return connectToServer();
    }

    // If we were connected but aren't now, connect
    else if (pClient != nullptr && !pClient->isConnected()) {
        Serial.println("Disconnected. Rescanning...");
        destroyClientConnection();
        NimBLEDevice::getScan()->start(0, true);

        return false;
    }

    // If we don't have a target re-try scanning
    else if (!haveTarget) {
        Serial.println("Starting scan");
        pScan->start((uint32_t)msBetweenLoops);

        return false;
    }

    return true;
}

void loop() {
    char output[50];
    
    // Check if we should continue with the sensor reading
    if (loopDelayCount < numDelaysBetweenLoops) {
        loopDelayCount++;
        delay(msBetweenLoopDelays);
        return;
    }

    loopDelayCount = 0;

    // Update the radon display if needed
    if (displayedRadon != radonValue) {
        displayedRadon = radonValue;

        if (radonValue == NAN || radonValue == 0) {
            display->setLine(1, "");
            radonValue = 0;
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "Radon %.1f pCi/L", displayedRadon / 1000.0);
            if (display)  {
                display->setLine(1, buf);
            }
        }
    }

    const bool radonTooHigh = radonValue > MAX_RADON_LEVEL;
    const bool radonExtremelyHigh = radonValue > RADON_VERY_HIGH;

    sprintf(output, "radonTooHigh %d, radonExtremelyHigh %d, radonValue %f", radonTooHigh, radonExtremelyHigh, radonValue);
    Serial.println(output);
    sprintf(output, "MAX_RADON_LEVEL %f, RADON_VERY_HIGH %f", MAX_RADON_LEVEL, RADON_VERY_HIGH);
    Serial.println(output);

    // Get the current CO2 levels
    float newCo2;
    float co2Tempurature;
    float co2Humidity;
    bool success = sensironScd30.getReadings(newCo2, co2Tempurature, co2Humidity);

    if (success) {
        co2 = newCo2;
    }

    // Update the display
    sprintf(output, "CO2 %d ppm", (uint32_t)co2);
    display->setLine(0, output);

    // Update the desired fan speed based on CO2 levels
    if (co2 > burstCo2Level) {
        desiredLevel1FanSpeed = FAN_SPEED_HIGH;
    } else if (co2 > targetCo2Level) {
        if (currentFanSpeed < FAN_SPEED_HIGH) {
            desiredLevel1FanSpeed = FAN_SPEED_MEDIUM;
        }
    } else if (co2 < shutoffCo2Level) {
        desiredLevel1FanSpeed = FAN_SPEED_OFF;
    }

    // Determine the desired building level
    if (radonTooHigh) {
        if (radonExtremelyHigh && co2 <= burstCo2Level) {
            radonPoints = maxLoopsBankedForRadon;
        }

        if (
            desiredLevel1FanSpeed == FAN_SPEED_OFF ||
            radonPoints >= maxLoopsBankedForRadon
        ) {
            desiredBuildingLevel = BUILDING_LEVEL_BASEMENT;
        } else if (radonPoints == 0) {
            desiredBuildingLevel = BUILDING_LEVEL_1;
        }
    } else {
        desiredBuildingLevel = BUILDING_LEVEL_1;
    }
    if (desiredBuildingLevel == NOT_SET) {
        desiredBuildingLevel = BUILDING_LEVEL_1;
    }

    // Determine the final fan speed
    if (desiredBuildingLevel == BUILDING_LEVEL_BASEMENT) {
        desiredFanSpeed = FAN_SPEED_HIGH;
    } else {
        desiredFanSpeed = desiredLevel1FanSpeed;
    }

    // Update the fan speed and building level
    if (validateConnected()) {
        bool errorCommunicating = false;
        if (desiredBuildingLevel != currentBuildingLevel) {
            if (pBuildingLevelChar != nullptr) {
                NimBLEAttValue value;
                if (desiredBuildingLevel == BUILDING_LEVEL_BASEMENT) {
                    value = NimBLEAttValue((uint8_t*)"BASEMENT", strlen("BASEMENT"));
                    display->setLine(3, "Venting Basement");
                    Serial.println("Writing basement");
                } else if (desiredBuildingLevel == BUILDING_LEVEL_1) {
                    value = NimBLEAttValue((uint8_t*)"1", strlen("1"));
                    display->setLine(3, "Venting Living Space");
                    Serial.println("Writing living space");
                }
                pBuildingLevelChar->writeValue(value, true);
                currentBuildingLevel = desiredBuildingLevel;
            } else {
                errorCommunicating = true;
            }
        }

        if (desiredFanSpeed != currentFanSpeed) {
            if (pFanSpeedChar != nullptr) {
                NimBLEAttValue value;
                if (desiredFanSpeed == FAN_SPEED_OFF) {
                    value = NimBLEAttValue((uint8_t*)"OFF", strlen("OFF"));
                    display->setLine(2, "Fan off");
                    Serial.println("Writing off");
                } else if (desiredFanSpeed == FAN_SPEED_MEDIUM) {
                    value = NimBLEAttValue((uint8_t*)"MEDIUM", strlen("MEDIUM"));
                    display->setLine(2, "Fan on");
                    Serial.println("Writing on");
                } else if (desiredFanSpeed == FAN_SPEED_HIGH) {
                    value = NimBLEAttValue((uint8_t*)"HIGH", strlen("HIGH"));
                    display->setLine(2, "Fan boost");
                    Serial.println("Writing boost");
                }
                pFanSpeedChar->writeValue(value, true);
                currentFanSpeed = desiredFanSpeed;
            } else {
                errorCommunicating = true;
            }
        }

        // Accrue or spend radon points and update the display on error
        if (errorCommunicating) {
            Serial.println("Error communicating with controller");
            display->setLine(0, "Error communicating");
            display->setLine(1, "");
            display->setLine(2, "");
            display->setLine(3, "");
        } else if (currentBuildingLevel == BUILDING_LEVEL_BASEMENT) {
            if (radonPoints < loopsSpentPerRadonLoop) {
                radonPoints = 0;
            } else {
                radonPoints -= loopsSpentPerRadonLoop;
            }
        } else if (currentBuildingLevel == BUILDING_LEVEL_1) {
            if (radonPoints < maxLoopsBankedForRadon) {
                radonPoints++;
            }
        }
        char output[100];
        sprintf(output, "radonPoints %d, maxLoopsBankedForRadon %d, loopsSpentPerRadonLoop %d", radonPoints, maxLoopsBankedForRadon, loopsSpentPerRadonLoop);
        Serial.println(output);
    } else {
        Serial.println("Error communicating with controller");
        display->setLine(0, "Error communicating");
        display->setLine(1, "");
        display->setLine(2, "");
        display->setLine(3, "");
    }

    // Iterate
    delay(msBetweenLoopDelays);
}
