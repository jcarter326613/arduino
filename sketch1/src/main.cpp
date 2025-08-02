#include <Arduino.h>
#include <Wire.h>

const int ledPin = LED_BUILTIN;
const uint8_t TEMP_SENSOR_ADDRESS = 0x44;

void retrieveSerialNumber();
void retrieveTemperatureAndHumidity();
bool retrieveI2cValue(uint8_t address, byte command, byte data[6]);

bool verifyChecksum(byte data[6]);
bool verifyChecksumPiece(byte data[2], byte checksum);
byte computeCrc8Simple(byte bytes[2]);

void setup() {
  // Initialize the onboard LED pin as an output
  pinMode(ledPin, OUTPUT);

  Serial.begin(9600);
  Wire.begin();
}

void loop() {
  // Turn the LED on
  digitalWrite(ledPin, HIGH);

  // Get the values
  retrieveSerialNumber();
  retrieveTemperatureAndHumidity();

  // Turn the LED off
  Serial.println();
  digitalWrite(ledPin, LOW);

  // Wait until we do it again
  delay(10000);
}

void retrieveTemperatureAndHumidity() {
  uint8_t serialData[6];
  bool checksumSuccess = retrieveI2cValue(TEMP_SENSOR_ADDRESS, 0xFD, serialData);

  // Check the checksums
  char temperatureLabel[] = "Temperature: ";
  char humidityLabel[] = "Humidity: ";
  
  // Print out the temperature and humidity
  if (checksumSuccess) {
    // Seperate out the values
    uint16_t temperature = (serialData[0] << 8) | serialData[1];
    uint16_t humidity = (serialData[3] << 8) | serialData[4];

    // Convert the values
    temperature = (temperature * 315.0 / 65535.0) - 49.0;
    humidity = (humidity * 125.0 / 65535.0) - 6.0;

    // Print the values
    char output[50];
    Serial.print(temperatureLabel);
    sprintf(output, "%d F", temperature);
    Serial.println(output);
    Serial.print(humidityLabel);
    sprintf(output, "%d%%", humidity);
    Serial.println(output);
  } else {
    Serial.print(temperatureLabel);
    Serial.println("Checksum is invalid.");
    Serial.print(humidityLabel);
    Serial.println("Checksum is invalid.");
  }
}

void retrieveSerialNumber() {
  uint8_t serialData[6];
  bool checksumSuccess = retrieveI2cValue(TEMP_SENSOR_ADDRESS, 0x89, serialData);

  // Check the serial number cheksums
  Serial.print("Serial Number: ");
  if (checksumSuccess) {
    char output[200];
    sprintf(output, "%02x%02x%02x%02x", 
            serialData[0], serialData[1], serialData[3], serialData[4]);
    Serial.println(output);
  } else {
    Serial.println("Checksum is invalid.");
  }
}

bool retrieveI2cValue(uint8_t address, uint8_t command, uint8_t data[6]) {
  // Send the request
  Wire.beginTransmission(address); // Address of the device
  Wire.write(command); // Command to send
  Wire.endTransmission();

  // Listen for the response
  delay(10);
  Wire.requestFrom(address, 6u);
  byte i = 0;
  while (Wire.available()) {
    // Read the byte and store it
    char c = Wire.read();
    if (i < 6) {
      data[i] = c;
    }
    i++;
  }

  return i == 6 && verifyChecksum(data);
}

bool verifyChecksum(byte data[6]) {
  return verifyChecksumPiece(data, data[2]) &&
         verifyChecksumPiece(data + 3, data[5]);
}

bool verifyChecksumPiece(byte data[2], byte checksum) {
  // Compute the checksum
  byte computedCheckshum = computeCrc8Simple(data);

  // Return the results
  return computedCheckshum == checksum;
}

byte computeCrc8Simple(byte bytes[2]) {
    const byte generator = 0x31;
    byte crc = 0xFF;

    for (int i = 0; i < 2; i++) {
      byte currByte = bytes[i];
      crc ^= currByte;

      for (int j = 0; j < 8; j++) {
        if ((crc & 0x80) != 0) {
          crc = (byte)((crc << 1) ^ generator);
        } else {
          crc <<= 1;
        }
      }
    }

    return crc;
}