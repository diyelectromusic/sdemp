/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino SP0256A-AL2 Part 5
//  https://diyelectromusic.com/2025/09/08/arduino-and-sp0256a-al2-part-5/
//
      MIT License
      
      Copyright (c) 2025 diyelectromusic (Kevin)
      
      Permission is hereby granted, free of charge, to any person obtaining a copy of
      this software and associated documentation files (the "Software"), to deal in
      the Software without restriction, including without limitation the rights to
      use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
      the Software, and to permit persons to whom the Software is furnished to do so,
      subject to the following conditions:
      
      The above copyright notice and this permission notice shall be included in all
      copies or substantial portions of the Software.
      
      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
      FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
      COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHERIN
      AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
      WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
  Using principles from the following Arduino tutorials:
    EtherKit SI5351 Library: https://github.com/etherkit/Si5351Arduino
*/
#include <si5351.h>
#include <Wire.h>

// Comment out if no analog input required
//#define ALG_IN A0

// The following code initialises a SI5351 clock module
Si5351 si5351;

void clockSetup () {
  bool i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  if(!i2c_found)
  {
    Serial.println("Device not found on I2C bus!");
  }

  // Set CLK0 to output 3 MHz
  // NB: Value is in 0.01Hz units, so is freq*100
  si5351.set_freq(300000000ULL, SI5351_CLK0);

  // Query a status update and wait a bit to let the Si5351 populate the
  // status flags correctly.
  si5351.update_status();
  delay(500);
}

void setClock (uint64_t freq) {
  si5351.set_freq(freq, SI5351_CLK0);
  si5351.update_status();
  delay(300);
}

#define PA1    0
#define PA2    1
#define PA3    2
#define PA4    3
#define PA5    4
#define OY1    5
#define AY1    6
#define EH1    7
#define KK3    8
#define PP1    9
#define JH1    10
#define NN1    11
#define IH1    12
#define TT2    13
#define RR1    14
#define AX1    15
#define MM1    16
#define TT1    17
#define DH1    18
#define IY1    19
#define EY1    20
#define DD1    21
#define UW1    22
#define AO1    23
#define AA1    24
#define YY2    25
#define AE1    26
#define HH1    27
#define BB1    28
#define TH1    29
#define UH1    30
#define UW2    31
#define AW1    32
#define DD2    33
#define GG3    34
#define VV1    35
#define GG1    36
#define SH1    37
#define ZH1    38
#define RR2    39
#define FF1    40
#define KK2    41
#define KK1    42
#define ZZ1    43
#define NG1    44
#define LL1    45
#define WW1    46
#define XR1    47
#define WH1    48
#define YY1    49
#define CH1    50
#define ER1    51
#define ER2    52
#define OW1    53
#define DH2    54
#define SS1    55
#define NN2    56
#define HH2    57
#define OR1    58
#define AR1    59
#define YR1    60
#define GG2    61
#define EL1    62
#define BB2    63
#define NUM_ALLO 64

const uint16_t PROGMEM allodelay[NUM_ALLO] = {
  10, 30, 50, 100, 200,
  420, 260, 70, 120, 210,
  140, 140, 70, 140, 170,
  70, 180, 100, 290, 250,
  280, 70, 100, 100, 100,
  180, 120, 130, 80, 180,
  100, 260, 370, 160, 140,
  190, 80, 160, 190, 120,
  150, 190, 160, 210, 220,
  110, 180, 360, 200, 130,
  190, 160, 300, 240, 240,
  90, 190, 180, 330, 290,
  350, 40, 190, 50
};

#define pinALD 8
int pinA[6] = {2,3,4,5,6,7}; // Use direct PORT IO
#define SP_OUT(addr) {PORTD = (PIND & 0x03) | ((addr)<<2);}
#define pinSTBY 9
#define SP_CHK() ((PINB & 0x02)!=0x02)

void spSetup() {
  for (int i; i<6; i++) {
    pinMode(pinA[i], OUTPUT);
  }
  SP_OUT(0);
  pinMode(pinALD, OUTPUT);
  digitalWrite(pinALD, HIGH); // Active LOW
  pinMode(pinSTBY, INPUT);
}

void spAddr() {
  digitalWrite(pinALD, LOW);
  delay(1);
  digitalWrite(pinALD, HIGH);
}

void spAllo (int allo) {
  if (allo >= NUM_ALLO) {
    return;
  }
  do {
    // Wait for STBY
  } while (SP_CHK());

  SP_OUT(allo);
  spAddr();
  uint16_t allodel = pgm_read_word_near(&allodelay[allo]);
  delay(allodel);
}

void setup() {
  Serial.begin(9600);
  clockSetup();
  spSetup();
}

uint64_t freq = 1000;  // in kHz
int alglast = 0;
void checkClock (void) {
#ifdef ALG_IN
  int algval = analogRead(ALG_IN);
  if  (algval != alglast) {
    freq = 1000 + 5 * analogRead(ALG_IN);
    setClock(freq*100000); // Convert to 0.01Hz units
  }
  alglast = algval;
#endif
}

void speak (int allo) {
#ifdef ALG_IN
  checkClock();
#endif
  spAllo(allo);
}

void midi2clock (int m) {
  // Calculate frequency based on MIDI note
  //   Freq = 3MHz * 2 ^ ((M-44)/12)
  // MIDI note 44, G#2, seems to be 3MHz

  // Only support range C2 (36) to G3 (55)
  if (m < 36 || m > 56) {
    return;
  }

  freq = 300000000 * pow (2.0, (((double)m-44.0)/12.0));
  Serial.print(m);
  Serial.print(" -> ");
  Serial.print((uint32_t)(freq/100000));
  Serial.println(" kHz");
  Serial.println("------------------");
  setClock (freq);
}

void doReMi (int oct) {
  midi2clock (36+12*oct);
  speak(DD1);
  speak(OW1);
  delay(300);
  speak(PA3);

  midi2clock(38+12*oct);
  speak(RR1);
  speak(EY1);
  delay(300);
  speak(PA3);

  midi2clock(40+12*oct);
  speak(MM1);
  speak(IY1);
  delay(300);
  speak(PA3);

  midi2clock(41+12*oct);
  speak(FF1);
  speak(AR1);
  delay(300);
  speak(PA3);

  midi2clock(43+12*oct);
  speak(SS1);
  speak(OW1);
  delay(300);
  speak(PA3);

  midi2clock(45+12*oct);
  speak(LL1);
  speak(AR1);
  delay(300);
  speak(PA3);

  midi2clock(47+12*oct);
  speak(TT1);
  speak(IY1);
  delay(300);
  speak(PA3);

  midi2clock(48+12*oct);
  speak(DD1);
  speak(OW1);
  delay(300);
  speak(PA3);
}

// 1kHz in 0.01 Hz units
#define KHZ 100000

void mrBlueSky (void) {
  Serial.print("\nMr Blue Sky-eye\n");
  setClock(3000*KHZ);
  speak(MM1);
  speak(IH1);
  speak(SS1);
  delay(200);

  speak(TT1);
  speak(ER1);
  delay(300);
  speak(PA1);

  setClock(3350*KHZ);
  speak(BB1);
  speak(LL1);
  speak(UW1);
  delay(600);

  speak(PA2);
  setClock(3000*KHZ);
  speak(SS1);
  speak(KK3);
  speak(AY1);
  delay(600);

  speak(PA1);
  setClock(6000*KHZ);
  speak(AY1);
  delay(600);

  speak(PA1);
  setClock(5000*KHZ);
  speak(AY1);
  delay(600);

  speak(PA3);
}

void loop() {
#if 0
#ifdef ALG_IN
  checkClock();
#else
  delay(1000);
  setClock(freq*100000); // Convert to 0.01Hz units
  freq += 500; // 500kHz steps
  if (freq > 6000) freq = 1000;
#endif

  Serial.print("\nFreq = ");
  Serial.print((uint32_t)freq);

  Serial.print("\nHello World\n");
  speak(HH1);
  speak(EH1);
  speak(LL1);
  speak(OW1);
  speak(UH1);
  speak(PA3);

  speak(WW1);
  speak(ER2);
  speak(LL1);
  speak(DD1);
  speak(PA5);
#endif

#if 1
  doReMi(0);
  doReMi(1);
  delay(500);
  mrBlueSky();
  delay(1000);
#endif
}
