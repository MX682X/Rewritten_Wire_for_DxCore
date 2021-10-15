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
/* E.g. for two AVR128DA48 (pull-ups omitted)
 SDA(M) PA2 ----\    /---- PA2 SDA(M)
 SCL(M) PA3 ----\\  //---- PA3 SCL(M)
                 \\//
                  \\
                 //\\
 SDA(S) PC2 ----//  \\---- PC2 SDA(S)
 SCL(S) PC3 ----/    \---- PC3 SCL(S)
*/

#include <Wire.h>

char input[32];
char rxWire[32];
int8_t rxLen = 0;
int8_t len = 0;

void setup() {
  Wire.begin();                                 // initialize master
  Serial1.begin(9600);
}

void loop() {
  if (Serial1.available() > 0) {    // as soon as the first byte is received on Serial
    readFromSerial();               // read the data from the Serial interface
    if (len > 0) {                  // after the while-loop, if there was useful data,
      sendDataWire();               // send the data over I2C
    }
    len = 0;                        // since the data was sent, the position is 0 again
  }
}

void readFromSerial() {
  while (true) {                    // in an endless while-loop
    char c = Serial1.read();        // read the next char
    while (c == -1) {               // when the buffer is empty, Serial.read() returns -1
      c = Serial1.read();           // this avoids filling the input buffer with gibberish
    }
    if (c == '\n' || c == '\r') {   // until a new line or carriage return is found
      break;                        // if so, break the endless while-loop
    }                               // otherwise
    input[len] = c;                 // save the char
    len++;                          // increment the position
    if (len > 30) {                 // if there was too much data
      break;                        // break the while-loop to avoid buffer overflow
    }
  }
}

void sendDataWire() {
  uint32_t ms;
  if (4 == Wire.requestFrom(0x54, 4, 0x01)) {              // request from slave
    while ( Wire.available() ) {
      ms  = (uint32_t)Wire.read();                // read out 32-bit wide data
      ms |= (uint32_t)Wire.read() <<  8;
      ms |= (uint32_t)Wire.read() << 16;
      ms |= (uint32_t)Wire.read() << 24;
      Serial1.println(ms);              // print the milli seconds
    }
  } else {
    Serial1.println("Wire.requestFrom() timeouted!");
  }
}
