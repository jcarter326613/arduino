#include <Arduino.h>
#include <Wire.h>

const int ledPin = LED_BUILTIN;
const uint8_t TEMP_SENSOR_ADDRESS = 0x44;

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

  // Send the request
  Wire.beginTransmission(TEMP_SENSOR_ADDRESS); // Address of the device
  byte command[1] = {0x89}; // Command to send
  Wire.write(command, 1); // Command
  Wire.endTransmission();

  // Listen for the response
  delay(1);
  Wire.requestFrom(TEMP_SENSOR_ADDRESS, 6u);
  byte serialData[6];
  byte i = 0;
  while (Wire.available()) {
    // Read the byte and store it
    char c = Wire.read();
    if (i < 6) {
      serialData[i] = c;
      i++;
    }

    // Output the byte
    char output[6];
    sprintf(output, "%02X ", c); // Format it as a hex string
    Serial.print(output);      // Print it to the Serial Monitor
  }
  Serial.println();

  // Check the serial number cheksums
  if (i == 6 && verifyChecksum(serialData)) {
    Serial.println("Checksum is valid.");
  } else {
    Serial.println("Checksum is invalid.");
  }

  // Turn the LED off
  Serial.println();
  digitalWrite(ledPin, LOW);

  // Wait until we do it again
  delay(10000);
}

bool verifyChecksum(byte data[6]) {
  return verifyChecksumPiece(data, data[2]) &&
         verifyChecksumPiece(data + 3, data[5]);
}

bool verifyChecksumPiece(byte data[2], byte checksum) {
  // Compute the checksum
  byte computedCheckshum = computeCrc8Simple(data);

  // Print out
  char output[6];
  sprintf(output, "%02X ", data[0]);
  Serial.print(output);
  sprintf(output, "%02X ", data[1]);
  Serial.print(output);
  Serial.print("= ");
  sprintf(output, "%02X ", computedCheckshum);
  Serial.print(output);
  Serial.print(".  Expected checksum ");
  sprintf(output, "%02X ", checksum);
  Serial.print(output);
  Serial.println();

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