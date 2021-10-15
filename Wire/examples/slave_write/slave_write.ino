// Wire Mixed Bag
// by MX682X

// Demonstrates use of the New Wire library
// Writes data to an I2C/TWI slave and
// Reads data from an I2C/TWI slave device

// Tested with Curiosity Nano - AVR128DA48

// This example takes the input from Serial. This code can run on both MCUs
// If the first element is a 'r' or 'R', it will read the data on Wire
// from the slave address 0x54, in this example the millis(), and print
// it on the serial monitor.
// If the first element is anything else, it writes the data on Wire to
// the slave address 0x54. It is them copied in a buffer and sent back
// to the sender, but with the address 0x64, to avoid an endless loop

// To use this, you need to connect the slave pins from one device to the
// master pins of a second device.


#include <Wire.h>

char input[32];
char rxWire[32];
int8_t rxLen = 0;
int8_t len = 0;

void setup() {
  Wire.begin(0x54, false, ((0x64 << 1) | 1));   // initialize slave
  Wire.onRequest(transmitDataWire);
  Serial1.begin(9600);
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
