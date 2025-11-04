#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Wire.h>

#include "display.h"
#include "sensiron-scd30.h"

// Match these to your server code
static NimBLEUUID serviceUuid("b207b7f1-acf6-48eb-8cd7-05fb3fef88f8");
static NimBLEUUID co2LevelCharacteristicUuid("030113cf-4ea5-4c9b-9c9c-e1dd84ce4970");
static NimBLEUUID radonLevelCharacteristicUuid("7e1b9f3a-5f6f-4b2c-a4db-8f8cb2d7f2a1");

static const uint8_t I2C_DATA = 21;
static const uint8_t I2C_CLOCK = 22;

static NimBLEAddress targetAddr;
static bool haveTarget = false;

static NimBLEClient* pClient = nullptr;
static NimBLERemoteService* pSvc = nullptr;
static NimBLERemoteCharacteristic* pCo2RemoteChar = nullptr;
static NimBLERemoteCharacteristic* pRadonRemoteChar = nullptr;
static NimBLEScan* pScan = nullptr;

Display *display;
SensironScd30 sensironScd30;
static float radonValue = 0;
static float displayedRadon = 0;
static uint8_t currentState = 0;
static uint8_t desiredState = 0;

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

static void onRadonNotify(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
    if (len == sizeof(float)) {
        float v;
        memcpy(&v, data, sizeof(float));
        radonValue = v;
    } else {
        Serial.println("Radon notify: unexpected payload size");
    }
}

void destroyClientConnection() {
    if (pCo2RemoteChar != nullptr) {
        pSvc->deleteCharacteristic(co2LevelCharacteristicUuid);
        pCo2RemoteChar = nullptr;
    }
    if (pRadonRemoteChar != nullptr) {
        pSvc->deleteCharacteristic(radonLevelCharacteristicUuid);
        pRadonRemoteChar = nullptr;
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

    // Co2
    pCo2RemoteChar = pSvc->getCharacteristic(co2LevelCharacteristicUuid);
    if (!pCo2RemoteChar) {
        Serial.println("Characteristic not found");
        destroyClientConnection();
        return false;
    }
    Serial.printf("co2 canWriteNoResponse=%d, canWrite=%d\n", pCo2RemoteChar->canWriteNoResponse(), pCo2RemoteChar->canWrite());

    if (!pCo2RemoteChar->canWrite()) {
        Serial.println("Co2 characteristic not writable");
        destroyClientConnection();
        return false;
    }

    // Radon
    pRadonRemoteChar = pSvc->getCharacteristic(radonLevelCharacteristicUuid);
    if (!pRadonRemoteChar) {
        Serial.println("Radon characteristic not found");
        destroyClientConnection();
        return false;
    }
    Serial.printf("radon canRead=%d, canNotify=%d\n", pRadonRemoteChar->canRead(), pRadonRemoteChar->canNotify());

    if (!pRadonRemoteChar->canRead() || !pRadonRemoteChar->canNotify()) {
        Serial.println("Radon characteristic not readable or not notifiable");
        destroyClientConnection();
        return false;
    }
    if (!pRadonRemoteChar->subscribe(true, onRadonNotify)) {
        Serial.println("Failed to subscribe to radon notifications");
        destroyClientConnection();
        return false;
    }
    std::string rv = pRadonRemoteChar->readValue();
    if (rv.size() == sizeof(float)) {
        memcpy(&radonValue, rv.data(), sizeof(float));
    }

    Serial.println("Connected");
    currentState = 0;   // Reset current state so we update it
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

static uint16_t targetCo2Level = 800;
static uint16_t shutoffCo2Level = 780;
static uint16_t burstCo2Level = 900;

bool validateConnected() {
    // If we successfully scanned a target but aren't connected, connect
    if (haveTarget && pCo2RemoteChar == nullptr && pRadonRemoteChar == nullptr) {
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
    // Update the radon display if needed
    if (displayedRadon != radonValue) {
        displayedRadon = radonValue;

        if (radonValue == NAN) {
            display->setLine(2, "");
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "Radon %.1f pCi/L", displayedRadon / 1000.0);
            if (display)  {
                display->setLine(2, buf);
            }
        }
        Serial.println("In radon display loop");
    }

    // Check if we should continue with the sensor reading
    if (loopDelayCount < numDelaysBetweenLoops) {
        loopDelayCount++;
        delay(msBetweenLoopDelays);
        return;
    }

    Serial.println("Running sensor reading");
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
        display->setLine(0, "");

        // Update the desired state
        if (co2 > burstCo2Level) {
            desiredState = 3;
        } else if (co2 > targetCo2Level) {
            if (currentState < 2) {
                desiredState = 2;
            }
        } else if (co2 < shutoffCo2Level) {
            desiredState = 1;
        } else {
            Serial.print("Invalid co2 level ");
            Serial.println(co2);
        }
        Serial.print("desired state ");
        Serial.print(desiredState);
        Serial.print("  Current state ");
        Serial.println(currentState);
    } else {
        display->setLine(0, "Reading stale");
    }

    if (desiredState != currentState && validateConnected()) {
        // If we have a write handle, toggle the write if we are at the right count.
        if (pCo2RemoteChar != nullptr) {
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
            pCo2RemoteChar->writeValue(value, true);
            currentState = desiredState;
        } else {
            Serial.println("Error communicating with controller");
            display->setLine(1, "Error communicating");
        }
    }

    // Iterate
    delay(msBetweenLoopDelays);
}
