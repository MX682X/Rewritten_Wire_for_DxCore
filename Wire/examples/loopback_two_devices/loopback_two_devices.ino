// Wire Loop Back
// by MX682X

// Demonstrates use of the New Wire library
// Writes data to and
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
  Wire.enableDualMode(false);  // set argument true if FastMode+ speeds are expected on slave
  Wire.begin();                                 // initialize master
  Wire.begin(0x54, false, ((0x64 << 1) | 1));   // initialize slave
  Wire.onReceive(receiveDataWire);
  Wire.onRequest(transmitDataWire);
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
  if (rxLen > 0) {
    sendDataBack();
    rxLen = 0;
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
  uint32_t ms;
  if (firstElement == 'r' || firstElement == 'R') {   // check if the first element is an 'r' or 'R'
    Wire.requestFrom(0x54, 4, true);              // request from slave
    while(Wire.available()){            
      ms  = (uint32_t)Wire.read();                // read out 32-bit wide data
      ms |= (uint32_t)Wire.read() <<  8;
      ms |= (uint32_t)Wire.read() << 16;
      ms |= (uint32_t)Wire.read() << 24;
      Serial1.println(ms);              // print the milli seconds
    }
  } else {
    Wire.beginTransmission(0x54);       // prepare transmission to slave with address 0x54

    for (uint8_t i = 0; i < len; i++) {
      Wire.write(input[i]);             // Write the received data to the bus buffer
    }

    Wire.write("\r\n");                 // add new line and carriage return for the Serial monitor
    Wire.endTransmission();             // finish transmission
  }
}

void sendDataBack() {
  Wire.beginTransmission(0x64);     // prepare transmission to slave with address 0x64

  for (uint8_t i = 0; i < rxLen; i++) {
    Wire.write(rxWire[i]);          // Write the received data to the bus buffer
  }
  Wire.endTransmission();           // finish transmission
}


void receiveDataWire(int16_t numBytes) {
  uint8_t i = 0;
  uint8_t addr = Wire.getIncomingAddress();   // get the address that triggered this function
                                              // the incoming address is leftshifted though
  if (addr == (0x54 << 1)) {                  // if the address was 0x54, do this
    for (; i < numBytes; i++) {
      rxWire[i] = Wire.read();
    }
    rxLen = i;
  } else if (addr == (0x64 << 1)) {           // if the address was 0x64, do that
    Serial1.print("Addr 0x64: ");
    for (; i < numBytes; i++) {
      char c = Wire.read();
      Serial1.write(c);
    }
  }
}

void transmitDataWire() {
  uint32_t ms = millis();
  Wire.write((uint8_t) ms);
  Wire.write((uint8_t)(ms >> 8));
  Wire.write((uint8_t)(ms >> 16));
  Wire.write((uint8_t)(ms >> 24));
}
