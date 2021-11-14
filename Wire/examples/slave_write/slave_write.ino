// Wire Mixed Bag
// by MX682X

// Demonstrates use of the New Wire library
// Writes back data to an TWI/I2C Master
// Refer to the "Wire Master Read" example for use with this

// Tested with Curiosity Nano - AVR128DA48

/* E.g. for two AVR128DA48 (pull-ups omitted)
 SDA(M) PA2 --- PA2 SDA(S)
 SCL(M) PA3 --- PA3 SCL(S)
*/


#include <Wire.h>

void setup() {
  Wire.begin(0x54, false, ((0x64 << 1) | 1));   // initialize slave
  Wire.onRequest(transmitDataWire);
}

void loop() {
}


void transmitDataWire() {
  uint32_t ms = millis();
  Wire.write((uint8_t) ms);
  Wire.write((uint8_t)(ms >> 8));
  Wire.write((uint8_t)(ms >> 16));
  Wire.write((uint8_t)(ms >> 24));
}
