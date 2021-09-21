// Wire Slave Dual Address
// by MX682X

// Demonstrates use of the New Wire library
// Receives data as an I2C/TWI slave device
// Refer to the "Wire Two Masters Write" example for use with this

// Tested with Curiosity Nano - AVR128DA48

// This example prints the address which triggered the receive function
// and the data that was sent to the slave on the Serial Monitor

// To use this example with "Wire Two Masters Write" the interface has to
// be connected to both I2C master pins
// e.g. for two AVR128DA48: SDA(M): PA2 - PF2    SDA(S): PA2
//                          SCL(M): PA3 - PF3    SCL(S): PA3
#include <Wire.h>

char input[32];
int8_t len = 0;

void setup() {
  // Initializing Dual Address match slave
  // 1st argument: 1st address to listen to
  // 2nd argument: listen to general broadcast (address 0x00)
  // 3rd argument: bits 7-1: second address if bit 0 is set true
  //               or bit mask of an address if bit 0 is set false
  Wire.begin(0x54, false, ((0x64 << 1) | 1));

  Wire.onReceive(receiveDataWire);
  Serial1.begin(9600);
}

void loop() {
  delay(100);
}



// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveDataWire(int16_t numBytes) {
  uint8_t addr = Wire.getIncomingAddress();   // get the address that triggered this function
                                              // the incoming address is leftshifted though
  if (addr == (0x54 << 1)) {                  // if the address was 0x54, do this
    Serial1.print("Addr 0x54: ");
    for (uint8_t i = 0; i < numBytes; i++) {
      char c = Wire.read();
      Serial1.write(c);
    }
  } else if (addr == (0x64 << 1)) {           // if the address was 0x64, do that
    Serial1.print("Addr 0x64: ");
    for (uint8_t i = 0; i < numBytes; i++) {
      char c = Wire.read();
      Serial1.write(c);
    }
  }
}
