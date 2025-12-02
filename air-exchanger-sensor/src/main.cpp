#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Wire.h>

#include "display.h"
#include "sensiron-scd30.h"

// Match these to your server code
static NimBLEUUID serviceUuid("b207b7f1-acf6-48eb-8cd7-05fb3fef88f8");
static NimBLEUUID co2LevelCharacteristicUuid("030113cf-4ea5-4c9b-9c9c-e1dd84ce4970");

static const uint8_t I2C_DATA = 21;
static const uint8_t I2C_CLOCK = 22;

static NimBLEAddress targetAddr;
static bool haveTarget = false;

static NimBLEClient* pClient = nullptr;
static NimBLERemoteService* pSvc = nullptr;
static NimBLERemoteCharacteristic* pRemoteChar = nullptr;
static NimBLEScan* pScan = nullptr;

Display *display;
SensironScd30 sensironScd30;

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

void destroyClientConnection() {
    if (pRemoteChar != nullptr) {
        pSvc->deleteCharacteristic(co2LevelCharacteristicUuid);
        pRemoteChar = nullptr;
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

    pClient = NimBLEDevice::createClient();
    if (!pClient->connect(targetAddr)) {
        Serial.println("Connect failed");
        destroyClientConnection();
        return false;
    }

    pSvc = pClient->getService(serviceUuid);
    if (!pSvc) {
        Serial.println("Service not found");
        destroyClientConnection();
        return false;
    }

    pRemoteChar = pSvc->getCharacteristic(co2LevelCharacteristicUuid);
    if (!pRemoteChar) {
        Serial.println("Characteristic not found");
        destroyClientConnection();
        return false;
    }
    Serial.printf("canWriteNoResponse=%d, canWrite=%d\n", pRemoteChar->canWriteNoResponse(), pRemoteChar->canWrite());

    if (!pRemoteChar->canWrite()) {
        Serial.println("Characteristic not writable");
        destroyClientConnection();
        return false;
    }

    Serial.println("Connected");
    return true;
}

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

static const uint16_t msBetweenLoopDelays = 10;
static const uint16_t msBetweenLoops = 1000 * 10;
static const uint16_t numDelaysBetweenLoops = msBetweenLoops / msBetweenLoopDelays;
static uint16_t loopDelayCount = numDelaysBetweenLoops;

static uint8_t currentState = 0;
static uint8_t desiredState = 0;
///static uint16_t targetCo2Level = 1000;
///static uint16_t shutoffCo2Level = 980;
///static uint16_t burstCo2Level = 1100;
static uint16_t targetCo2Level = 800;
static uint16_t shutoffCo2Level = 780;
static uint16_t burstCo2Level = 900;

bool validateConnected() {
    // If we successfully scanned a target but aren't connected, connect
    if (haveTarget && pRemoteChar == nullptr) {
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
    // Check if we entered this loop too early
    if (loopDelayCount < numDelaysBetweenLoops) {
        loopDelayCount++;
        delay(msBetweenLoopDelays);
        return;
    }

    Serial.println("Running loop");
    loopDelayCount = 0;

    // Get the current CO2 levels
    float co2;
    float co2Tempurature;
    float co2Humidity;
    bool success = sensironScd30.getReadings(co2, co2Tempurature, co2Humidity);

    if (success) {
        // Update the display
        char output[50];
        sprintf(output, "CO2 %d ppm", (uint32_t)co2);
        display->setLine(1, output);
        display->setLine(2, "");

        // Update the desired state
        if (co2 > burstCo2Level) {
            desiredState = 3;
        } else if (co2 > targetCo2Level) {
            if (currentState < 2) {
                desiredState = 2;
            }
        } else if (co2 < shutoffCo2Level) {
            desiredState = 1;
        }
    } else {
        display->setLine(2, "Reading stale");
    }

    if (desiredState != currentState && validateConnected()) {
        // If we have a write handle, toggle the write if we are at the right count.
        if (pRemoteChar != nullptr) {
            NimBLEAttValue value;
            if (desiredState == 1) {
                value = NimBLEAttValue((uint8_t*)"ACCEPTABLE", strlen("ACCEPTABLE"));
                display->setLine(3, "Fan off");
                Serial.println("Writing off");
            } else if (desiredState == 2) {
                value = NimBLEAttValue((uint8_t*)"HIGH", strlen("HIGH"));
                Serial.println("Writing on");
                display->setLine(3, "Fan on");
            } else if (desiredState == 3) {
                value = NimBLEAttValue((uint8_t*)"CLIMBING", strlen("CLIMBING"));
                Serial.println("Writing boost");
                display->setLine(3, "Fan boost");
            }
            pRemoteChar->writeValue(value, true);
            currentState = desiredState;
        } else {
            Serial.println("Error communicating with controller");
            display->setLine(2, "Error communicating");
        }
    }

    // Iterate
    delay(msBetweenLoopDelays);
}
