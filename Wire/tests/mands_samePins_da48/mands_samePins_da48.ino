#include "Arduino.h"
#include "Wire.h"

void rxFunction(int numBytes);
void txFunction(void);

void rxFunction1(int numBytes);
void txFunction1(void);

uint8_t wire0_data[64];
uint8_t wire1_data[64];
uint8_t wire0_len = 0;
uint8_t wire1_len = 0;



void setup() {
  Wire.begin();
  Wire.begin(0x40);
  Wire1.begin();
  Wire1.begin(0x20);

  Serial1.begin(115200);

  Wire1.onReceive(rxFunction1);
}

void loop() {
  if (Serial1.available() > 0) {
    wire0_len = 0;
    while (true) {
      while (!Serial1.available()) {}
      uint8_t c = Serial1.read();
      if (c == '\n' || c == '\r') break;
      if (wire0_len < 16) {
        wire0_data[wire0_len++] = c;
      }
    }
    Wire.beginTransmission(0x10);
    Wire.write(wire0_data, wire0_len);
    Wire.endTransmission(1);

    wire0_len = 0;
    Serial1.println();
  }

  if (wire1_len > 0) {
    Wire1.beginTransmission(0x40);
    Wire1.write(wire1_data, wire1_len);
    Wire1.endTransmission(1);
    wire1_len = 0;
  }
}

void rxFunction(int numBytes) {
  uint8_t i = 0;
  Serial1.print("WS0RX ");
  Serial1.print(Wire.getIncomingAddress());
  Serial1.print(":");
  for ( ; i < numBytes; i++) {
    Serial1.write(Wire.read());
  }
  Serial1.println();
}


void rxFunction1(int numBytes) {
  uint8_t i = 0;
  Serial1.print("WS1RX ");
  Serial1.print(Wire1.getIncomingAddress());
  Serial1.print(":");
  for ( ; i < numBytes; i++) {
    wire1_data[i] = Wire1.read();
    Serial1.write(wire1_data[i]);
  }
  wire1_len = i;
  Serial1.println();
}
