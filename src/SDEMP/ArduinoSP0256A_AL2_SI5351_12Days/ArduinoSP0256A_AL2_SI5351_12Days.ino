/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino SP0256A-AL2 - 12 Days of Christmas
//  https://diyelectromusic.com/2025/12/23/sp0256a-al2-sings-the-twelve-days-of-christmas/
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

void ledInit (void) {
  pinMode(LED_BUILTIN, OUTPUT);
  ledOff();
}

void ledOn (void) {
  digitalWrite(LED_BUILTIN, HIGH);
}

void ledOff (void) {
  digitalWrite(LED_BUILTIN, LOW);
}

// The following code initialises a SI5351 clock module
Si5351 si5351;

void clockSetup () {
  bool i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  if(!i2c_found)
  {
//    Serial.println("Device not found on I2C bus!");
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
  delay(10);
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
   10,  30,  50, 100, 200,   // 0-4
  420, 260,  70, 120, 210,   // 5-9
  140, 140,  70, 140, 170,   // 10-14
   70, 180, 100, 290, 250,   // 15-19
  280,  70, 100, 100, 100,   // 20-24
  180, 120, 130,  80, 180,   // 25-29
  100, 260, 370, 160, 140,   // 30-34
  190,  80, 160, 190, 120,   // 35-39
  150, 190, 160, 210, 220,   // 40-44
  110, 180, 360, 200, 130,   // 45-49
  190, 160, 300, 240, 240,   // 50-54
   90, 190, 180, 330, 290,   // 55-59
  350,  40, 190,  50         // 60-63
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

void speak (int allo) {
  if (allo >= NUM_ALLO) {
    return;
  }
  if (allo > PA5) {
    // Non-pause allo
    ledOn();
  } else {
    ledOff();
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
  ledInit();
  clockSetup();
  spSetup();
}

uint64_t freq = 1000;  // in kHz
void midi2clock (int m) {
  // Calculate frequency based on MIDI note
  //   Freq = 3MHz * 2 ^ ((M-44)/12)
  // MIDI note 44, G#2, seems to be 3MHz

  // Only support range C2 (36) to G3 (55)
  if (m < 36 || m > 56) {
    return;
  }

  freq = 300000000 * pow (2.0, (((double)m-44.0)/12.0));
  setClock (freq);
}

#define MIDI_BASE 41  // F2. In F major, highest note is G3
void note (int note) {
  midi2clock (MIDI_BASE+note);
}

void numberth (int verse) {
  switch (verse) {
    case 1:
      // First
      speak(FF1);
      speak(FF1);
      speak(AX1);
      speak(RR1);
      speak(SS1);
      speak(TT1);
      speak(PA4);
      break;
    
    case 2:
      // Second
      speak(SS1);
      speak(SS1);
      speak(EH1);
      speak(KK3);
      speak(AX1);
      speak(NN1);
      speak(DD1);
      speak(PA2);
      break;

    case 3:
      // Third
      speak(TH1);
      speak(ER2);
      speak(DD1);
      speak(PA5);
      break;

    case 4:
      // Fourth
      speak(FF1);
      speak(FF1);
      speak(OR1);
      speak(TH1);
      speak(PA3);
      break;

    case 5:
      // Fifth
      speak(FF1);
      speak(FF1);
      speak(IH1);
      speak(TH1);
      speak(PA3);
      break;

    case 6:
      // Sixth
      speak(SS1);
      speak(SS1);
      speak(IH1);
      speak(KK1);
      speak(SS1);
      speak(TH1);
      speak(PA3);
      break;

    case 7:
      // Seventh
      speak(SS1);
      speak(SS1);
      speak(EH1);
      speak(VV1);
      speak(AX1);
      speak(NN1);
      speak(TH1);
      break;

    case 8:
      // Eigth
      speak(EY1);
      speak(PA3);
      speak(TH1);
      break;

    case 9:
      // Ninth
      speak(NN1);
      speak(AY1);
      speak(NN1);
      speak(TH1);
      break;

    case 10:
      // Tenth
      speak(TT1);
      speak(EH1);
      speak(NN1);
      speak(TH1);
      break;

    case 11:
      // Eleventh
      speak(IH1);
      speak(LL1);
      speak(EH1);
      speak(VV1);
      speak(IH1);
      speak(NN1);
      speak(TH1);
      break;

    case 12:
      // Twelfth
      speak(TT1);
      speak(WW1);
      speak(EH1);
      speak(LL1);
      speak(FF1);
      speak(TH1);
      break;
  }
}

void intro (int verse){
  // On
  note(0);
  speak(AO1);
  speak(NN1);
  speak(PA1);
  // The
  speak(DH1);
  speak(AX1);
  speak(PA1);
  // Nth
  numberth(verse);
  speak(PA2);
  // Day
  note(5);
  speak(DD1);
  speak(EY1);
  speak(IH1);
  speak(PA2);
  // Of
  speak(AX1);
  speak(VV1);
  speak(PA2);
  // Christmas
  speak(PA1);
  speak(KK3);
  speak(RR1);
  speak(IH1);
  speak(SS1);
  delay(200);
  note(4);
  speak(MM1);
  speak(AX1);
  speak(SS1);
  speak(PA5);
  // My
  note(5);
  speak(MM1);
  speak(AY1);
  speak(PA4);
  // True
  note(7);
  speak(TT1);
  speak(RR1);
  speak(UW1);
  speak(PA5);
  // Love
  note(9);
  speak(LL1);
  speak(AX1);
  speak(VV1);
  speak(PA4);
  // Gave
  note(10);
  speak(GG1);
  speak(EY1);
  speak(VV1);
//  speak(PA4);
  // To
  note(7);
  speak(TT1);
  speak(IH1);
  speak(PA5);
  // Me
  note(9);
  speak(MM1);
  speak(IY1);
  speak(PA4);
}

void one (bool firsttime, bool lasttime) {
  if (!firsttime) {
    // And
    note(9);
    speak(AX1);
    speak(NN1);
    speak(DD1);
    if (lasttime) delay(150);
    speak(PA3);
  } else {
    // short pause
    speak(PA5);
    speak(PA5);
    speak(PA5);
  }
  // A
  note(10);
  speak(AX1);
  if (lasttime) delay(150);
  speak(PA3);
  // Partridge
  note(12);
  speak(PP1);
  speak(AA1);
  speak(RR1);
  if (lasttime) delay(200);
  speak(PA3);
  note(14);
  speak(TT2);
  speak(RR1);
  speak(IH1);
  if (lasttime) delay(200);
  note(10);
  speak(JH1);
  if (lasttime) delay(200);
  speak(PA5);
  // In
  note(9);
  speak(IH1);
  speak(NN1);
  if (lasttime) delay(300);
  speak(PA4);
  // A
  note(5);
  speak(AX1);
  if (lasttime) delay(300);
  speak(PA4);
  // Pear
  note(7);
  speak(PP1);
  speak(EH1);
  speak(RR1);
  if (lasttime) delay(500);
  speak(PA4);
  // Tree
  note(5);
  speak(TT1);
  speak(RR1);
  speak(IY1);
  if (lasttime) delay(500);
  speak(PA4);
  speak(PA4);
  speak(PA5);
}

void two (bool postrings) {
  int notes[4] = {12, 7, 9, 10};
  if (postrings) {
    notes[0] = 7;
    notes[1] = 4;
    notes[2] = 2;
    notes[3] = 0;
  }
  // Two
  note(notes[0]);
  speak(TT2);
  speak(UW2);
  speak(PA5);
  // Turtle
  note(notes[1]);
  speak(TT2);
  speak(UH1);
  speak(RR1);
  note(notes[2]);
  speak(TT1);
  speak(UH1);
  speak(LL1);
  speak(PA1);
  // Doves
  note(notes[3]);
  speak(DD1);
  speak(UH1);
  speak(VV1);
  speak(PA5);
  speak(PA5);
}

void three (bool postrings) {
  int notes[3] = {12, 7, 10};
  if (postrings) {
    notes[0] = 10;
    notes[1] = 2;
    notes[2] = 5;
  }
  // Three
  note(notes[0]);
  speak(TH1);
  speak(RR1);
  speak(IY1);
  speak(PA3);
  // French
  note(notes[1]);
  speak(FF1);
  speak(RR1);
  speak(EH1);
  speak(NN1);
  speak(PA1);
  speak(PA3);
  // Hens
  note(notes[2]);
  speak(HH1);
  speak(EH1);
  speak(NN1);
  speak(ZZ1);
  speak(PA3);
}

void four (bool postrings) {
  int notes[4] = {12, 7, 9, 10};
  if (postrings) {
    notes[0] = 12;
    notes[1] = 9;
    notes[2] = 7;
    notes[3] = 5;
  }
  // Four
  note(notes[0]);
  speak(FF1);
  speak(FF1);
  speak(OR1);
  speak(PA3);
  // Calling
  note(notes[1]);
  speak(PA1);
  speak(KK3);
  speak(AO1);
  note(notes[2]);
  speak(LL1);
  speak(IH1);
  speak(NG1);
  speak(PA2);
  // Birds
  note(notes[3]);
  speak(BB1);
  speak(AX1);
  speak(RR1);
  speak(DD1);
  speak(ZZ1);
  speak(PA1);
}

void five (int verse) {
  int rall = 0;
  if (verse > 5) {
    rall = 40 * (verse - 5);
  }
  // Five
  note(12);
  speak(FF1);
  speak(FF1);
  speak(AY1);
  delay(300+rall);
  speak(VV1);
  speak(PA3);
  // Go--
  note(14);
  speak(GG1);
  speak(OW1);
  delay(rall);
  // --old
  note(11);
  speak(OW1);
  speak(UH1);
  speak(LL1);
  delay(200+rall);
  speak(DD1);
  speak(PA3);
  // Rings
  note(12);
  speak(RR1);
  speak(IH1);
  speak(NG1);
  delay(150+rall);
  speak(ZZ1);
  delay(150+rall);
  speak(PA3);
}

void six () {
  // Six
  note(12);
  speak(SS1);
  speak(IH1);
  speak(KK3);
  speak(SS1);
  speak(PA5);
  // Geese
  note(7);
  speak(GG1);
  speak(IY1);
  speak(SS1);
  speak(PA4);
  // A
  note(9);
  speak(AX1);
  speak(PA3);
  // Laying
  note(10);
  speak(LL1);
  speak(EY1);
  note(7);
  speak(IH1);
  speak(NG1);
  speak(PA3);
}

void seven () {
  // Seven
  note(12);
  speak(PA1);
  speak(SS1);
  speak(EH1);
  speak(VV1);
  speak(IH1);
  speak(NN1);
  speak(PA3);
  // Swans
  note(7);
  speak(SS1);
  speak(WW1);
  speak(AO1);
  speak(NN1);
  speak(ZZ1);
  speak(PA3);
  // A
  note(9);
  speak(AX1);
  speak(PA3);
  // Swimming
  note(10);
  speak(SS1);
  speak(PA1);
  speak(WW1);
  speak(IH1);
  speak(MM1);
  note(7);
  speak(IH1);
  speak(NG1);
  speak(PA1);
  speak(PA3);
}

void eight () {
  // Eight
  note(12);
  speak(EY1);
  speak(TT2);
  speak(PA5);
  // Maids
  note(7);
  speak(MM1);
  speak(EY1);
  speak(DD1);
  speak(ZZ1);
  speak(PA2);
  // A
  note(9);
  speak(AX1);
  speak(PA3);
  // Milking
  note(10);
  speak(PA1);
  speak(MM1);
  speak(IH1);
  speak(LL1);
  note(7);
  speak(KK3);
  speak(IH1);
  speak(NG1);
  speak(PA3);
}

void nine () {
  // Nine
  note(12);
  speak(NN1);
  speak(AY1);
  speak(NN1);
  speak(PA5);
  // Ladies
  note(7);
  speak(LL1);
  speak(EY1);
  speak(IH1);
  note(9);
  speak(DD1);
  speak(IY1);
  speak(ZZ1);
  speak(PA3);
  // Dancing
  note(10);
  speak(PA1);
  speak(DD1);
  speak(AE1);
  speak(NN1);
  note(7);
  speak(SS1);
  speak(IH1);
  speak(NG1);
  speak(PA3);
}

void ten () {
  // Ten
  note(12);
  speak(TT1);
  speak(EH1);
  speak(NN1);
  speak(PA3);
  speak(PA5);
  // Lords
  note(7);
  speak(LL1);
  speak(AO1);
  speak(RR1);
  speak(DD1);
  speak(ZZ1);
  speak(PA3);
  // A
  note(9);
  speak(AX1);
  speak(PA3);
  // Leaping
  note(10);
  speak(LL1);
  speak(IY1);
  note(7);
  speak(PP1);
  speak(IH1);
  speak(NG1);
  speak(PA3);
}

void eleven () {
  // Eleven
  note(12);
  speak(PA1);
  speak(IH1);
  speak(LL1);
  speak(EH1);
  speak(VV1);
  speak(IH1);
  speak(PA2);
  // Pipers
  note(7);
  speak(PA1);
  speak(PP1);
  speak(AY1);
  note(9);
  speak(PP1);
  speak(AX1);
  speak(RR1);
  speak(ZZ1);
  speak(PA1);
  // Piping
  note(10);
  speak(PA1);
  speak(PP1);
  speak(AY1);
  note(7);
  speak(PP1);
  speak(IH1);
  speak(NG1);
  speak(PA3);
}

void twelve () {
  // Twelve
  note(12);
  speak(TT1);
  speak(WH1);
  speak(EH1);
  speak(LL1);
  speak(VV1);
  speak(PA4);
  // Drummers
  note(7);
  speak(PA1);
  speak(PA1);
  speak(DD1);
  speak(RR1);
  speak(AX1);
  note(9);
  speak(MM1);
  speak(AX1);
  speak(RR1);
  speak(ZZ1);
  speak(PA3);
  // Drumming
  note(10);
  speak(PA1);
  speak(DD1);
  speak(RR1);
  speak(AX1);
  note(7);
  speak(MM1);
  speak(IH1);
  speak(NG1);
  speak(PA3);
}

// 1kHz in 0.01 Hz units
#define KHZ 100000

void loop() {
  uint16_t phrase=1;
  midi2clock (MIDI_BASE);

  for (int verse=1; verse<=12 ; verse++) {
    intro(verse);
    delay(200);
    if (phrase & 0x0800) twelve();
    if (phrase & 0x0400) eleven();
    if (phrase & 0x0200) ten();
    if (phrase & 0x0100) nine();
    if (phrase & 0x0080) eight();
    if (phrase & 0x0040) seven();
    if (phrase & 0x0020) six();
    if (phrase & 0x0010) five(verse);
    if (phrase & 0x0008) four(phrase & 0x0FF0);
    if (phrase & 0x0004) three(phrase & 0x0FF0);
    if (phrase & 0x0002) two(phrase & 0x0FF0);
    if (phrase & 0x0001) one(phrase == 1, verse == 12);
    phrase = (phrase<<1) + 1;
    delay(1000);
  }

  delay(30000);
}
