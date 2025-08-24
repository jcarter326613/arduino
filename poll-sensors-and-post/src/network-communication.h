#include <string>
#include <WiFiS3.h>
#include <WiFiSSLClient.h>

#include "measurement.h"

// Wifi documentation
// https://docs.arduino.cc/language-reference/en/functions/wifi/overview/
// Wifi example
// https://www.mathworks.com/help/instrument/read-data-from-arduino-using-tcpip-communication.html

class NetworkCommunication {
    private:
        WiFiSSLClient client;
        std::string sensorId;
        //const char sensorIdTemplate

    public:
        NetworkCommunication(const char sensorId[]);
        ~NetworkCommunication();

        bool connect();
        bool isConnected() const;

        bool sendMeasurement(const Measurement& measurement);

    private:
        static void printWifiStatus();
};
