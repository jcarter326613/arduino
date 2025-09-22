#pragma once
#ifndef _NETWORK_COMMUNICATION_H_
#define _NETWORK_COMMUNICATION_H_

#include <string>
#include "measurement.h"

// ---- Platform selection ----
// Define one of these in your sketch or build flags:
//#define USE_ARDUINO_UNO_R4
#define USE_ESP32

#if defined(USE_ARDUINO_UNO_R4)
    // Wifi documentation
    // https://docs.arduino.cc/language-reference/en/functions/wifi/overview/
    // Wifi example
    // https://www.mathworks.com/help/instrument/read-data-from-arduino-using-tcpip-communication.html

    #include <WiFiS3.h>
    #include <WiFiSSLClient.h>
    using NetworkClient = WiFiSSLClient;

#elif defined(USE_ESP32)
    #include <WiFi.h>
    #include <WiFiClientSecure.h>
    using NetworkClientt = WiFiClientSecure;

#else
    #error "You must define either USE_ARDUINO_UNO_R4 or USE_ESP32 before including this header."
#endif


class NetworkCommunication {
    private:
        NetworkClientt client;
        std::string sensorId;

    public:
        NetworkCommunication(const char sensorId[]);
        ~NetworkCommunication();

        bool connect();
        bool isConnected() const;

        bool sendMeasurement(const Measurement& measurement);

    private:
        static void printWifiStatus();
};

#endif