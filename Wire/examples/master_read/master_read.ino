// Wire Mixed Bag
// by MX682X

// Demonstrates use of the New Wire library
// Reads data from an I2C/TWI slave device
// Refer to the "Wire Slave Write" example for use with this

// Tested with Curiosity Nano - AVR128DA48

// This example takes the input from Serial. If the serial input is 'm' or 'M',
// this code requests 4 bytes from the slave with the address 0x54. 
// When using together with the complementary example, the slave sends it's millis() value. 
// This value is then sent to the serial monitor

// To use this, you need to connect the slave pins from one device to the
// master pins of a second device.
/* E.g. for two AVR128DA48 (pull-ups omitted)
 SDA(M) PA2 --- PA2 SDA(S)
 SCL(M) PA3 --- PA3 SCL(S)
*/

#include <Wire.h>

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
  char c = Serial1.read();        // read the next char
  while (c == -1) {               // when the buffer is empty, Serial.read() returns -1
    c = Serial1.read();           // this avoids filling the input buffer with gibberish
  }
  if (c == '\n' || c == '\r') {   // if a new line or carriage return is found
    return;                       // if so, return (empty message)
  }                               // otherwise
  if (c == 'm' || c == 'M') {       // check if the char is m or M
    len = 1;  
    return;
  }
}

void sendDataWire() {
  uint32_t ms;
  if (4 == Wire.requestFrom(0x54, 4, 0x01)) {    // request from slave
    while ( Wire.available() ) {
      ms  = (uint32_t)Wire.read();               // read out 32-bit wide data
      ms |= (uint32_t)Wire.read() <<  8;
      ms |= (uint32_t)Wire.read() << 16;
      ms |= (uint32_t)Wire.read() << 24;
      Serial1.println(ms);              // print the milliseconds from Slave
    }
  } else {
    Serial1.println("Wire.requestFrom() timeouted!");
  }
}
