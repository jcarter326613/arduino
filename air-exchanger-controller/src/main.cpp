#include <Arduino.h>
#include <NimBLEDevice.h>

static NimBLEUUID serviceUuid("b207b7f1-acf6-48eb-8cd7-05fb3fef88f8");
static NimBLEUUID onOffCharacteristicUuid("030113cf-4ea5-4c9b-9c9c-e1dd84ce4970");

class OnOffCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo &connInfo) {
        std::string value = pCharacteristic->getValue();
        Serial.print("onWrite from client: ");
        Serial.println(value.c_str());
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

void setup() {
    Serial.begin(9600);

    NimBLEDevice::init("ESP32_BLE_Server");

    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService *pService = pServer->createService(serviceUuid);
    NimBLECharacteristic *pCharacteristic = pService->createCharacteristic(
        onOffCharacteristicUuid,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );

    pCharacteristic->setCallbacks(new OnOffCallbacks());
    pService->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(serviceUuid);
    NimBLEDevice::startAdvertising();
    Serial.println("BLE server is advertising");
}

void loop() {
}
