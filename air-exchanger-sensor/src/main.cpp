#include <Arduino.h>
#include <NimBLEDevice.h>

// Match these to your server code
static NimBLEUUID serviceUuid("b207b7f1-acf6-48eb-8cd7-05fb3fef88f8");
static NimBLEUUID onOffCharacteristicUuid("030113cf-4ea5-4c9b-9c9c-e1dd84ce4970");

static NimBLEAddress targetAddr;
static bool haveTarget = false;

static NimBLEClient* pClient = nullptr;
static NimBLERemoteService* pSvc = nullptr;
static NimBLERemoteCharacteristic* pRemoteChar = nullptr;
static NimBLEScan* pScan = nullptr;

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
        pSvc->deleteCharacteristic(onOffCharacteristicUuid);
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

    pRemoteChar = pSvc->getCharacteristic(onOffCharacteristicUuid);
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
    NimBLEDevice::init("");

    pScan = NimBLEDevice::getScan();
    pScan->setActiveScan(true);
    pScan->setInterval(45);
    pScan->setWindow(15);
    pScan->setScanCallbacks(new MyScanCallbacks(), false);
}

static const uint8_t msBetweenLoops = 100;
static uint8_t cycleCount = 0;
static const uint8_t targetMaxCount = (2 * 1000) / msBetweenLoops;
static bool currentMode = 0;

void loop() {
    // If we successfully scanned a target, connect
    if (haveTarget && pRemoteChar == nullptr) {
        Serial.println("t2");
        connectToServer();
    }

    // If we were connected but aren't now, connect
    else if (pClient != nullptr && !pClient->isConnected()) {
        Serial.println("Disconnected. Rescanning...");
        destroyClientConnection();
        NimBLEDevice::getScan()->start(0, true);
    }

    // At the beginning
    if (cycleCount == 0) {
        // If we have a write handle, toggle the write if we are at the right count.
        if (pRemoteChar != nullptr) {
            NimBLEAttValue value;
            if (currentMode) {
                value = NimBLEAttValue((uint8_t*)"Off", strlen("Off"));
                Serial.println("Writing off");
            } else {
                value = NimBLEAttValue((uint8_t*)"On", strlen("On"));
                Serial.println("Writing on");
            }
            currentMode = !currentMode;
            pRemoteChar->writeValue(value, true);
        }
        
        // If we don't have a target, scan for one.
        else if (!haveTarget) {
            Serial.println("Starting scan");
            pScan->start(((uint32_t)msBetweenLoops) * targetMaxCount);  // continuous, non-blocking scan
        }

        else {
            Serial.println("Nothing happening");
        }
    }
    cycleCount = (cycleCount + 1) % targetMaxCount;

    delay(msBetweenLoops);
}
