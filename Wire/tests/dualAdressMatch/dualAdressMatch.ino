#include "Wire.h"



// This example shows how to use the dual address match functionality
// and how to see which address was used.





void rxFunction(int numBytes);

uint8_t wire0_data[64];
uint8_t wire1_data[64];
uint8_t wire0_len = 0;
uint8_t wire1_len = 0;

void setup() {
  Wire.begin();  // Enable Master to send data back

  // First parameter: Normal address.
  // Second parameter: Respond to broadcast (address 0x00)
  // Third parameter: Bit-mask or second address, a address in this case
  Wire.begin(0x10, false, (0x30 <<  1) | 1);  // Address 0x10 and Address 0x30 will trigger the interrupt
  Wire.onReceive(rxFunction);
}

void loop() {
  if (wire0_len > 0) {
    Wire.beginTransmission(0x20);
    Wire.write(wire0_data, wire0_len);
    Wire.endTransmission(true);
    wire0_len = 0;
  }

  if (wire1_len > 0) {
    Wire.beginTransmission(0x40);
    Wire.write(wire1_data, wire1_len);
    Wire.endTransmission(true);
    wire1_len = 0;
  }
}





void rxFunction(int numBytes) {
  uint8_t j = Wire.available();
  if (Wire.getIncomingAddress() == (0x10 << 1)) {  // Data was sent with the address 0x10
    uint8_t i = 0;
    for ( ; i < j; i++) {
      wire0_data[i] = Wire.read();
    }
    wire0_len = i;
  } else if (Wire.getIncomingAddress() == (0x30 << 1)) {  // Data was sent with the address 0x30
    uint8_t i = 0;
    for ( ; i < j; i++) {
      wire1_data[i] = Wire.read();
    }
    wire1_len = i;
  }
}
