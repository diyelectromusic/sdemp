// SPDX-License-Identifier: MIT.
// With the added proviso that this code MUST NOT be used for training of AI systems.
//
// Copyright (c) 2026 Kevin (emalliab)
//
#define DSIZE 8
#define ASIZE 7
const int databus[DSIZE] = {8,9,2,3,4,5,6,7};
const int addrbus[ASIZE] = {A0,A1,A2,A3,A4,A5,13};
const int not_oe = 12;
const int not_we = 11;
const int clck = 10;

//#define PRINTOUTPUT
//#define CHECKDATA

uint16_t addr;
void addrinc() {
  addr++;
  if ((addr & 0x7F) == 0) {
    // Pulse the clock LOW to increment the 393 counter
    digitalWrite(clck, LOW);
    delayMicroseconds(10);
    digitalWrite(clck, HIGH);
  }
}

uint8_t ramDataRead () {
  // Set data pins to INPUT
  for (int i=0; i<DSIZE; i++) {
    pinMode(databus[i],INPUT);
  }
  digitalWrite(not_oe, LOW);   // RAM Output
  delayMicroseconds(2);

  // Use PORT IO for PORTD[2:7] | PORTB[0:1]
  uint8_t data = (PIND & 0xFC) | (PINB & 0x03);
  digitalWrite(not_oe, HIGH);   // RAM Output off

  return data;
}

void ramDataWrite (uint8_t data, bool leave) {
  digitalWrite(not_oe, HIGH);   // RAM Input

  // Set data pins to OUTPUT
  for (int i=0; i<DSIZE; i++) {
    pinMode(databus[i],OUTPUT);
  }

  // Write via direct PORT IO
  PORTD = (PORTD & 0x03) | (data & 0xFC);
  PORTB = (PORTB & 0xFC) | (data & 0x03);
  digitalWrite(not_we, LOW);    // Write
  delayMicroseconds(2);
  digitalWrite(not_we, HIGH);   // Write complete
#ifdef CHECKDATA
  delay(200);
#endif
  if (!leave) {
    // Set data pins to INPUT
    for (int i=0; i<DSIZE; i++) {
      pinMode(databus[i],INPUT_PULLUP);
    }
  }
}

void ramAddrSet (uint16_t address) {
  // Can only write bottom 7 bits
  PORTC = address & 0x3F;
  if ((address & (1<<6)) == 0) {
    digitalWrite(addrbus[6], LOW);
  } else {
    digitalWrite(addrbus[6], HIGH);
  }
}

uint8_t datavalue;
void setup() {
  Serial.begin(9600);
  Serial.println("62256 SRAM Tester");
  addr = 0;
  pinMode(not_oe,OUTPUT);
  digitalWrite(not_oe,HIGH);
  pinMode(not_we,OUTPUT);
  digitalWrite(not_we,HIGH);
  pinMode(clck,OUTPUT);
  digitalWrite(clck, HIGH);
  for (int i=0; i<ASIZE; i++) {
    pinMode(addrbus[i],OUTPUT);
    digitalWrite(addrbus[i],LOW);
  }
  datavalue = 0x55;
}

void loop() {
  ramAddrSet(addr);
  ramDataWrite (datavalue, false);

  uint16_t notaddr = (addr & 0x40) ? (addr & (~(0x40))) : (addr | 0x40);
  uint8_t notdatavalue = ~datavalue;
  ramAddrSet(notaddr);
  ramDataWrite (notdatavalue, false);

  ramAddrSet(addr);
  uint8_t val1 = ramDataRead();

  ramAddrSet(notaddr);
  uint8_t val2 = ramDataRead();

#ifdef PRINTOUTPUT
  if (addr < 0x10) Serial.print("0");
  if (addr < 0x100) Serial.print("0");
  if (addr < 0x1000) Serial.print("0");
  Serial.print(addr,HEX);
  Serial.print(":\t");
  if (datavalue < 0x10) Serial.print("0");
  Serial.print(datavalue,HEX);
  Serial.print(" = ");
  if (val1 < 0x10) Serial.print("0");
  Serial.print(val1,HEX);
  Serial.print("\t|||\t");
  if (notaddr < 0x10) Serial.print("0");
  if (notaddr < 0x100) Serial.print("0");
  if (notaddr < 0x1000) Serial.print("0");
  Serial.print(notaddr,HEX);
  Serial.print(":\t");
  if (notdatavalue < 0x10) Serial.print("0");
  Serial.print(notdatavalue,HEX);
  Serial.print(" = ");
  if (val2 < 0x10) Serial.print("0");
  Serial.println(val2,HEX);
#endif

  if ((val1 != datavalue) || (val2 != notdatavalue)) {
    // Stop
    for (;;) {
      // Leave data lines with last value used
      ramDataWrite (datavalue, true);
      digitalWrite(13, HIGH);
      delay(500);
      digitalWrite(13, LOW);
      delay(500);
    }
  }
  addrinc();
  datavalue++;
  datavalue = datavalue % 251;  // 251 is prime
}
