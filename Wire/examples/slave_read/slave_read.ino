// Wire Slave Read
// by MX682X

// Demonstrates use of the Wire library
// Receives data as an I2C/TWI slave device
// Refer to the "Wire Master Write" example for use with this

// Tested with Curiosity Nano - AVR128DA48
#include <Wire.h>

void setup() {
  Wire.begin(0x54);                 // join i2c bus with address 0x54
  Wire.onReceive(receiveDataWire);  // give the Wire library the name of the function
                                    // that will be called on a master write event
  Serial1.begin(9600);
}

void loop() {
  delay(100);
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveDataWire(int16_t numBytes) {      // the Wire library tells us how many bytes
  for (uint8_t i = 0; i < numBytes; i++) {    // were received so we can for loop for that
    char c = Wire.read();                     // amount and read the received data
    Serial1.write(c);                         // to print it to the Serial Monitor
  }
}
