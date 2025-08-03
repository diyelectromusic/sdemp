/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino SP0256A-AL2
//  https://diyelectromusic.com/2025/08/03/arduino-and-sp0256a-al2/
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
  Serial.print(allo);
  Serial.print("\t");

  do {
    // Wait for STBY
  } while (SP_CHK());

  SP_OUT(allo);
  spAddr();
  uint16_t allodel = pgm_read_word_near(&allodelay[allo]);
  Serial.println(allodel);
  delay(allodel);
}

void setup() {
  Serial.begin(9600);
  spSetup();
}

void loop() {
  delay(2000);
  Serial.print("\n");

  Serial.print("Hello\n");
  spAllo(HH1);
  spAllo(EH1);
  spAllo(LL1);
  spAllo(OW1);
  spAllo(PA5);

  Serial.print("World\n");
  spAllo(WW1);
  spAllo(OR1);
  spAllo(LL1);
  spAllo(DD1);
  spAllo(PA5);
}
