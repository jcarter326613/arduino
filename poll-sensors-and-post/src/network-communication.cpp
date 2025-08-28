#include <Arduino.h>

#include "network-communication.h"
#include "arduino-secrets.h"

NetworkCommunication::NetworkCommunication(const char sensorId[]):
    sensorId(sensorId)
{
}

NetworkCommunication::~NetworkCommunication() {
}

bool NetworkCommunication::connect() {
    // Check that wifi exists
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        return false;
    }
    
    // Connect to the network
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    
    int status = WiFi.begin(SECRET_SSID, SECRET_PASS);
    while (status == WL_IDLE_STATUS) {
        delay(1000);
        status = WiFi.status();
    }

    // Wait for an IP address to be assigned
    if (status == WL_CONNECTED) {
        IPAddress ip = WiFi.localIP();
        while (ip == "0.0.0.0" && status == WL_CONNECTED) {
            delay(1000);
            ip = WiFi.localIP();
            status = WiFi.status();
        }
    }

    if (!isConnected()) {
        return false;
    }

    // Debug output
    printWifiStatus();

    return true;
}

bool NetworkCommunication::isConnected() const {
    int status = WiFi.status();
    if (status != WL_CONNECTED) {
        return false;
    }

    IPAddress ip = WiFi.localIP();
    if (ip == "0.0.0.0") {
        return false;
    }

    return true;
}

bool NetworkCommunication::sendMeasurement(const Measurement& measurement) {
    // Create the body
    char emptyBuffer[0];
    int bodyLength = snprintf(emptyBuffer, 0, 
        "{\"sensorId\":\"%s\",\"readingType\":\"%s\",\"unit\":\"%s\",\"value\":%s}",
        sensorId.data(),
        measurement.type,
        measurement.unit,
        measurement.value
    );
    char *body = new char[bodyLength + 1];
    snprintf(body, bodyLength + 1, 
        "{\"sensorId\":\"%s\",\"readingType\":\"%s\",\"unit\":\"%s\",\"value\":%s}",
        sensorId.data(),
        measurement.type,
        measurement.unit,
        measurement.value
    );
    /*
    std::string body = std::format(
        "\{\"sensorId\":\"{}\",\"readingType\":\"{}\",\"unit\":\"{}\",\"value\":{}\}",
        sensorId,
        measurement.type,
        measurement.unit,
        measurement.value
    );
    */

    // Send the measurement
    if (client.connect("ghs.skipthedevops.com", 443)) {
        Serial.println("connected to server");

        //const uint16_t bodyLength = body.size();
        char contentLengthBuffer[30];
        sprintf(contentLengthBuffer, "Content-Length: %d", bodyLength);

        client.println("POST /v1/sensor-readings HTTP/1.1");
        client.println("Host: ghs.skipthedevops.com");
        client.println("Cache-Control: no-cache");
        client.println("Connection: close");
        client.println("Content-Type: application/json");
        client.println(contentLengthBuffer);
        client.println();

        Serial.println(body);
        Serial.println(contentLengthBuffer);

        client.print(body);
    }
    
    // Read the response
    while (client.connected()) {
        while (client.available()) {
            char c = client.read();
            Serial.print(c);
        }
    }

    return true;
}

void NetworkCommunication::printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}