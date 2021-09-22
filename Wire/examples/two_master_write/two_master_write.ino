// Wire Two Masters Write
// by MX682X

// Demonstrates use of the New Wire library
// Writes data to an I2C/TWI slave device
// Refer to the "Wire Slave Multi Address" example for use with this

// Tested with Curiosity Nano - AVR128DA48

// This example takes the input from Serial. If the first element
// is a number, it will write the data on Wire to the slave address 0x54.
// If the first element is anything else, it writes the data on Wire1 to
// the slave address 0x64.

// To use this example with "Wire Slave Dual Address" both interfaces
// of the master have to be on the same I2C bus as the slave
// e.g. for two AVR128DA48: SDA(M): PA2 - PF2    SDA(S): PA2
//                          SCL(M): PA3 - PF3    SCL(S): PA3
#include <Wire.h>

char input[32];
int8_t len = 0;

void setup() {
  Wire.begin();           // initialize master
  Wire1.begin();
  Serial1.begin(9600);
}

void loop() {
  if (Serial1.available() > 0) {    // as soon as the first byte is received on Serial
    readFromSerial();               // read the data from the Serial interface
    sendDataWire();                 // after the while-loop, send the data over I2C
    len = 0;                        // since the data was sent, the position is 0 again
  }
}

void readFromSerial() {
  while (true) {                    // in an endless while-loop
    char c = Serial1.read();        // read the next char
    while (c == -1) {               // when the buffer is empty, Serial.read() returns -1
      c = Serial1.read();           // this avoids that
    }
    if (c == '\n' || c == '\r') {    // until a new line or carriage return is found
      break;                        // if so, break the endless while-loop
    }                               // otherwise
    input[len] = c;                 // save the char
    len++;                          // increment the  position
    if (len > 30) {                 // if there was too much data
      break;                        // break the while-loop to avoid buffer overflow
    }
  }
}

void sendDataWire() {
  uint8_t firstElement = input[0];
  if (firstElement >= '0' && firstElement <= '9') {   // check if the first element is a number
    Wire.beginTransmission(0x54);    // prepare transmission to slave with address 0x54

    for (uint8_t i = 0; i < len; i++) {
      Wire.write(input[i]);          // Write the received data to the bus buffer
    }

    Wire.write("\r\n");              // add new line and carriage return for the Serial monitor
    Wire.endTransmission();          // finish transmission
  } else {
    Wire1.beginTransmission(0x64);   // prepare transmission to slave with address 0x64

    for (uint8_t i = 0; i < len; i++) {
      Wire1.write(input[i]);          // Write the received data to the bus buffer
    }

    Wire1.write("\r\n");              // add new line and carriage return for the Serial monitor
    Wire1.endTransmission();          // finish transmission
  }
}
