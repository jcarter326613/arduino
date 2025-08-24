#include <Arduino.h>
#include <Wire.h>
#include <WiFiS3.h>
#include <WiFiSSLClient.h>

#include "arduino_secrets.h"

// Wifi documentation
// https://docs.arduino.cc/language-reference/en/functions/wifi/overview/
// Wifi example
// https://www.mathworks.com/help/instrument/read-data-from-arduino-using-tcpip-communication.html
// Convert SSL certs to ECP32 variable
// https://unreeeal.github.io/ssl_esp.html
// Test TLS attributes
// https://www.ssllabs.com/ssltest/analyze.html

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

int status = WL_IDLE_STATUS;

WiFiSSLClient client;

void sendMeasurementExample();
void printWifiStatus();

void setup() {
    Serial.begin(9600);
    Serial.println();
    Serial.println();

    // Check that wifi exists
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        while (true);
    }

    // Connect to the network
    while (true) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        bool isConnected = false;
        while (status == WL_CONNECTED) {
            IPAddress ip = WiFi.localIP();
            if (ip == "0.0.0.0"){
                delay(1000);
            } else {
                isConnected = true;
                break;
            }
        }

        if (isConnected) {
            break;
        }

        // wait 10 seconds for connection:
        delay(10000);
    }

    printWifiStatus();

    Serial.println("\nStarting readings...");
}

bool readingResponse = false;

void loop() {
    // Send the measurement
    if (!readingResponse) {
        sendMeasurementExample();
    }

    // Read the response
    while (client.available()) {
        char c = client.read();
        Serial.print(c);
    }

    // if the server's disconnected, wait the required delay:
    if (!client.connected()) {
        readingResponse = false;
        delay(10 * 60 * 1000);  // Delay for 10 minutes
        Serial.println();
        Serial.println();
    }
}

void sendMeasurementExample() {
    if (client.connect("ghs.skipthedevops.com", 443)) {
        Serial.println("connected to server");

        const char body[] = "{\"sensorId\":\"11111111-1111-1111-1111-111111111111\",\"readingType\":\"test\",\"unit\":\"none\",\"value\":43}";
        const uint16_t bodyLength = strlen(body);
        char contentLengthBuffer[50];
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

        readingResponse = true;
    }
}

void printWifiStatus() {
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