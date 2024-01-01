// SDEMP Simple Arduino EPROM Reader.
// https://diyelectromusic.wordpress.com/
//
// Based on Ben Eater's eeprom-programmer.ino
// https://github.com/beneater/eeprom-programmer
//
// But for reading only.
//
// Originally designed for 28C256 where pin 1
// is A14 and pin 27 is /WE.
// But will work for 27C256 if pin 1 is
// mapped to VCC and pin 27 is A14.
//
// Uses two 595 shift registers for A0 to A14.
//
// Updated to include independent /OE pin.
// (original used last bit of the 2nd shift register).
//
#define EEPROM_SIZE 0x8000 // 32K
#define SHIFT_DATA  2
#define SHIFT_CLK   3
#define SHIFT_LATCH 4
#define OE_PIN      13
int datapins[8] = {5, 6, 7, 8, 9, 10, 11, 12};

//#define DATA_TEST 1

void setAddress(unsigned address) {
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, (address >> 8));
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, address);

  digitalWrite(SHIFT_LATCH, LOW);
  digitalWrite(SHIFT_LATCH, HIGH);
  digitalWrite(SHIFT_LATCH, LOW);
}

void outputEnable (bool en) {
  // OE is active LOW
  if (en) {
    digitalWrite(OE_PIN, LOW);
  } else {
    digitalWrite(OE_PIN, HIGH);
  }
}

byte readEEPROM(unsigned address) {
  setAddress(address);
  outputEnable(true);

  byte data = 0;
  for (int p=0; p<8; p++) {
    // Read in D7 down to D0
    data = (data << 1) + digitalRead(datapins[7-p]);
  }
  outputEnable(false);
  return data;
}

void setup() {
  // put your setup code here, to run once:
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  pinMode(OE_PIN, OUTPUT);
  outputEnable(false);
  for (int p=0; p<8; p++) {
    pinMode(datapins[p], INPUT);
  }
  Serial.begin(9600);

#ifdef DATA_TEST
  return;
#endif

  // Read and print out the contents of the EERPROM
  Serial.println("Reading EEPROM");
  for (unsigned base = 0; base < EEPROM_SIZE; base += 16) {
    byte data[16];
    for (int i=0; i<16; i++) {
      data[i] = readEEPROM(base + i);
    }

    char buf[100];
    sprintf(buf, "%05x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.print(buf);
    Serial.print("\t");
    for (int i=0; i<16; i++) {
      if (data[i]>=32 && data[i]<127) {
        Serial.write(data[i]);
      } else {
        Serial.print(".");
      }
    }
    Serial.print("\n");
  }
}


void loop() {
  // put your main code here, to run repeatedly:
#ifdef DATA_TEST
  unsigned base = 0;
  byte data[16];
  for (int i=0; i<16; i++) {
    data[i] = readEEPROM(base + i);
  }

  char buf[100];
  sprintf(buf, "%05x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
          base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
          data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

  Serial.print(buf);
  Serial.print("\t");
  for (int i=0; i<16; i++) {
    if (data[i]>=32 && data[i]<127) {
      Serial.write(data[i]);
    } else {
      Serial.print(".");
    }
  }
  Serial.print("\n");
#endif
}
