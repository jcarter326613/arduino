#include <Arduino.h>
#include <Wire.h>

const int ledPin = LED_BUILTIN;
const uint8_t TEMP_SENSOR_ADDRESS = 0x44;
const uint8_t VOC_SENSOR_ADDRESS = 0x59;

void retrieveVoc(uint8_t buffer[6]);
bool retrieveTemperatureAndHumidity(uint8_t serialData[6]);
void retrieveSerialNumber();
bool retrieveI2cValue(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t outputBuffer[], uint8_t outputBufferSize, uint8_t delayMs);
bool retrieveI2cValueWithParameters(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t parameters[], uint8_t parametersLength, uint8_t outputBuffer[], uint8_t outputBufferSize, uint8_t delayMs);

bool verifyChecksum(byte data[6], int length);
bool verifyChecksumPiece(byte data[2], byte checksum);
byte computeCrc8Simple(byte bytes[2]);

void setup() {
  // Initialize the onboard LED pin as an output
  pinMode(ledPin, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  // Turn the LED on
  digitalWrite(ledPin, HIGH);
  Wire.begin();

  // Get the values
  retrieveSerialNumber();
  uint8_t buffer[6];
  bool success = retrieveTemperatureAndHumidity(buffer);
  if (success) {
    retrieveVoc(buffer);
  }

  // Turn the LED off
  Serial.println();
  digitalWrite(ledPin, LOW);
  Wire.end();

  // Wait until we do it again
  delay(10000);
}

void retrieveVoc(uint8_t tempAndHumidityBuffer[6]) {
  uint8_t serialData[3] = {0, 0, 0};
  uint8_t command[2] = {0x26, 0x0F};

  bool checksumSuccess = retrieveI2cValueWithParameters(VOC_SENSOR_ADDRESS, command, 2, tempAndHumidityBuffer, 6, serialData, 3, 30);

  // Check the checksums
  char vocLabel[] = "VOC: ";
  
  // Print out the VOC
  if (checksumSuccess) {
    // Seperate out the value
    uint16_t voc = (serialData[0] << 8) | serialData[1];

    // Print the value
    char output[50];
    Serial.print(vocLabel);
    sprintf(output, "%d", voc);
    Serial.println(output);
  } else {
    Serial.print(vocLabel);
    char output[50];
    sprintf(output, "%02x%02x%02x ", serialData[0], serialData[1], serialData[2]);
    Serial.print(output);
    Serial.println("Checksum is invalid.");
  }
}

bool retrieveTemperatureAndHumidity(uint8_t serialData[6]) {
  uint8_t command[1] = {0xFD};
  bool checksumSuccess = retrieveI2cValue(TEMP_SENSOR_ADDRESS, command, 1, serialData, 6, 10);

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

    return true;
  } else {
    Serial.print(temperatureLabel);
    Serial.println("Checksum is invalid.");
    Serial.print(humidityLabel);
    Serial.println("Checksum is invalid.");

    return false;
  }
}

void retrieveSerialNumber() {
  uint8_t serialData[6];
  uint8_t command[1] = {0x89};
  bool checksumSuccess = retrieveI2cValue(TEMP_SENSOR_ADDRESS, command, 1, serialData, 6, 1);

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

bool retrieveI2cValue(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t outputBuffer[], uint8_t outputBufferSize, uint8_t delayMs) {
  uint8_t parameters[0];
  return retrieveI2cValueWithParameters(address, command, commandLength, parameters, 0, outputBuffer, outputBufferSize, delayMs);
}

bool retrieveI2cValueWithParameters(uint8_t address, uint8_t command[], uint8_t commandLength, uint8_t parameters[], uint8_t parametersLength, uint8_t outputBuffer[], uint8_t outputBufferSize, uint8_t delayMs) {
  // Send the request
  Wire.beginTransmission(address); // Address of the device
  for (uint8_t i = 0; i < commandLength; i++) {
    Wire.write(command[i]);
  }
  for (uint8_t i = 0; i < parametersLength; i++) {
    Wire.write(parameters[i]);
  }
  Wire.endTransmission();

  // Listen for the response
  delay(delayMs);
  Wire.requestFrom(address, outputBufferSize);
  uint8_t i = 0;
  while (Wire.available()) {
    // Read the byte and store it
    char c = Wire.read();
    if (i < outputBufferSize) {
      outputBuffer[i] = c;
    }
    i++;
  }

  return i == outputBufferSize && verifyChecksum(outputBuffer, outputBufferSize);
}

bool verifyChecksum(byte data[], int length) {
  if (length % 3 != 0) {
    return false;
  }

  for (int i = 0; i < length; i += 3) {
    if (!verifyChecksumPiece(data + i, data[i + 2])) {
      return false;
    }
  }
  return true;
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