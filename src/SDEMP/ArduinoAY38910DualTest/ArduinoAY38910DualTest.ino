/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino AY-3-8910 Shield
//  https://diyelectromusic.com/2025/09/25/arduino-ay-3-8910-shield-build-guide/
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

// Assumes a 1MHz clock
#define NOTE_START  12   // C0
#define NUM_NOTES   108

// Taken from https://github.com/Andy4495/AY3891x/blob/main/src/AY3891x_sounds.h
const uint16_t PROGMEM Notes[NUM_NOTES] = {
  3823, 3609, 3406, 3213, 3034, 2863, 2703, 2551, 2408, 2273, 2145, 2025,  // Octave 0
  1911, 1804, 1703, 1607, 1517, 1432, 1351, 1276, 1204, 1136, 1073, 1012,  // Octave 1
   956,  902,  851,  804,  758,  716,  676,  638,  602,  568,  536,  506,  // Octave 2
   478,  451,  426,  402,  379,  358,  338,  319,  301,  284,  268,  253,  // Octave 3
   239,  225,  213,  201,  190,  179,  169,  159,  150,  142,  134,  127,  // Octave 4
   119,  113,  106,  100,   95,   89,   84,   80,   75,   71,   67,   63,  // Octave 5
    60,   56,   53,   50,   47,   45,   42,   40,   38,   36,   34,   32,  // Octave 6
    30,   28,   27,   25,   24,   22,   21,   20,   19,   18,   17,   16,  // Octave 7
    15,   14,   13,   13,   12,   11,   11,   10,    9,    9,    8,    8   // Octave 8
};

enum AY38910Regs {
  A_TONE_F = 0,
  A_TONE_C,
  B_TONE_F,
  B_TONE_C,
  C_TONE_F,
  C_TONE_C,
  NOISE,
  EN,
  A_AMP,
  B_AMP,
  C_AMP,
  ENV_F,
  ENV_C,
  ENC_S,
  A_IO,
  B_IO,
};

#define NUM_AY 2
int BC1_pins[NUM_AY] = {A0,A2};
int BDIR_pins[NUM_AY] = {A1,A3};
//  BC2 is fixed at 5V

// Clock and RESET are common to all chips
#define CLK  10
#define RST  11

// AY38910 Data Pin Mappings
//      AY = Arduino
// DA0:DA5 = PORTD[2:7] D2-D7
// DA6:DA7 = PORTB[0:1] D8-D9
int dbus[8] = {2,3,4,5,6,7,8,9};

// NB: Addresses are all in range 0..15 so don't need to
//     worry about writing out bits 6,7 - just ensure set to zero
#define AYADDR(x) {PORTD = (PORTD & 0x03) | (((x) & 0x3F)<<2); \
                   PORTB = (PORTB & 0xFC); }

#define AYDATA(x) {PORTD = (PORTD & 0x03) | (((x) & 0x3F)<<2); \
                   PORTB = (PORTB & 0xFC) | (((x) & 0xC0)>>6); }

#define DATAOUT() {DDRD = (DDRD | 0xFC); DDRB = (DDRB | 0x03);}
#define DATAIN()  {DDRD = (DDRD & 0x03); DDRB = (DDRB & 0xFC);}

inline void ayBC (int ay, int val) {
  switch (ay) {
    case 0:
      if (val == LOW) {
        PORTC &= 0xFE; // A0/D14 LOW
      } else {
        PORTC |= 0x01; // A0/D14 HIGH
      }
      break;
    case 1:
      if (val == LOW) {
        PORTC &= 0xFB; // A2/D16 LOW
      } else {
        PORTC |= 0x04; // A2/D16 HIGH
      }
      break;
  }
}

inline void ayBDIR (int ay, int val) {
  switch (ay) {
    case 0:
      if (val == LOW) {
        PORTC &= 0xFD; // A1/D15 LOW
      } else {
        PORTC |= 0x02; // A1/D15 HIGH
      }
      break;
    case 1:
      if (val == LOW) {
        PORTC &= 0xF7; // A3/D17 LOW
      } else {
        PORTC |= 0x08; // A3/D17 HIGH
      }
      break;
  }
}

void ayFastWriteSetup (int ay) {
  if (ay >= NUM_AY) return;

  pinMode (BC1_pins[ay], OUTPUT);
  pinMode (BDIR_pins[ay], OUTPUT);
  pinMode (CLK, OUTPUT);

  // Fix output modes for all pins too
  // Note: this assumes we're only ever writing to the chip...
  DATAOUT();
  AYDATA(0);
  ayBC(0, LOW);
  ayBDIR(0, LOW);
}

void ayResetOn (void) {
  pinMode(RST, OUTPUT);
  // To reset all chips - hold low for a least 5uS min
  digitalWrite(RST, LOW);
  delayMicroseconds(6);
}

void ayResetOff (void) {
  digitalWrite(RST, HIGH);
}

void ayFastWrite (int ay, AY38910Regs reg, byte val) {
  if (ay >= NUM_AY) return;

  // Mode=Addr Latch
  ayBC(ay, HIGH);
  ayBDIR(ay, HIGH);

  // Latch address
  AYADDR((byte)reg);

  // Need 400nS Min
  delayMicroseconds(1);

  // Mode = Inactive
  ayBC(ay, LOW);
  ayBDIR(ay, LOW);

  // Need 100nS settle then 50nS preamble
  delayMicroseconds(1);

  // Mode = Write
  ayBC(ay, LOW);
  ayBDIR(ay, HIGH);

  // Write data
  AYDATA(val);

  // Need 500nS min
  delayMicroseconds(1);

  // Mode = Inactive
  ayBC(ay, LOW);
  ayBDIR(ay, LOW);

  // Need 100nS min
  // Assume the normal machine of running other code will give this
}

void ayClockSetup () {
  pinMode(CLK, OUTPUT);
  digitalWrite(CLK, HIGH);

  // Timer 1
  // Use CTC mode: WGM13..0 = 0100
  // Toggle OC1B on Compare Match: COM1B[1:0] = 01
  // Use ClkIO with no prescaling: CS1[2:0] = 001
  // Interrupts off: TIMSK1 = 0
  // OCR1A = Fclk / (2 * PreScale * Freq) - 1
  //       = 16MHz / (2 * 1 * 1MHz) - 1
  // OCR1A = 8-1 for 2MHz counter operation, hence 1MHz clock
  //
  // NB: As OC1B toggles on match, need counter 
  //     running at 2MHz for a 1MHz clock signal.
#if (CLK!=10)
#error "CLOCK has to be D10 to use OC1B"
#endif
  TCCR1A = (1 << COM1B0);  // D10/OC1B
  TCCR1B = (1 << WGM12) | (1 << CS10);
  TCCR1C = 0;
  TIMSK1 = 0;
  OCR1AH = 0;
  OCR1AL = 7; // 16MHz / 8 = 2MHz Counter/1MHz clock
}

void aySetFreq (int ay, int ch, int note) {
  if (note < NOTE_START && note != 0) return;
  if (note >= NOTE_START+NUM_NOTES) return;

  uint16_t freq = 0;
  if (note != 0) {
    freq = pgm_read_word(&Notes[note-NOTE_START]);
  }

  switch (ch) {
    case 0:
      ayFastWrite (ay, AY38910Regs::A_TONE_C, freq >> 8);
      ayFastWrite (ay, AY38910Regs::A_TONE_F, freq & 0x0FF);
      break;
    case 1:
      ayFastWrite (ay, AY38910Regs::B_TONE_C, freq >> 8);
      ayFastWrite (ay, AY38910Regs::B_TONE_F, freq & 0x0FF);
      break;
    case 2:
      ayFastWrite (ay, AY38910Regs::C_TONE_C, freq >> 8);
      ayFastWrite (ay, AY38910Regs::C_TONE_F, freq & 0x0FF);
      break;
  }
}

void aySetup(int ay) {
  if (ay >= NUM_AY) return;
  ayFastWrite (ay, AY38910Regs::EN, 0xFF);  // Enable bits active low, so all off
  ayFastWrite (ay, AY38910Regs::A_AMP, 0);
  ayFastWrite (ay, AY38910Regs::A_TONE_C, 0);
  ayFastWrite (ay, AY38910Regs::A_TONE_F, 0);
  ayFastWrite (ay, AY38910Regs::B_AMP, 0);
  ayFastWrite (ay, AY38910Regs::B_TONE_C, 0);
  ayFastWrite (ay, AY38910Regs::B_TONE_F, 0);
  ayFastWrite (ay, AY38910Regs::C_AMP, 0);
  ayFastWrite (ay, AY38910Regs::C_TONE_C, 0);
  ayFastWrite (ay, AY38910Regs::C_TONE_F, 0);
}

void ayTestToneOn (int ay) {
  if (ay >= NUM_AY) return;
  aySetFreq(ay, 0, 48+ay*12);
  aySetFreq(ay, 1, 52+ay*12);
  aySetFreq(ay, 2, 55+ay*12);
  ayFastWrite (ay, AY38910Regs::EN, 0xF8);  // Enable bits active low, so Ch A,B,C = on

  ayFastWrite (ay, AY38910Regs::A_AMP, 15);
  delay(250);
  ayFastWrite (ay, AY38910Regs::B_AMP, 15);
  delay(250);
  ayFastWrite (ay, AY38910Regs::C_AMP, 15);
}

void ayTestToneOff (int ay) {
  ayFastWrite (ay, AY38910Regs::A_AMP, 0);
  ayFastWrite (ay, AY38910Regs::B_AMP, 0);
  ayFastWrite (ay, AY38910Regs::C_AMP, 0);
  aySetFreq (ay, 0, 0);
  aySetFreq (ay, 1, 0);
  aySetFreq (ay, 2, 0);

  ayFastWrite (ay, AY38910Regs::EN, 0xFF);
}

void setup () {
  ayResetOn();
  ayClockSetup();
  for (int i=0; i<NUM_AY; i++) {
    ayFastWriteSetup(i);
  }
  ayResetOff();
  delay(200);

  for (int i=0; i<NUM_AY; i++) {
    aySetup(i);
  }
}

void loop() {
  for (int i=0; i<NUM_AY; i++) {
    ayTestToneOn(i);
    delay(500);
  }
  for (int i=0; i<NUM_AY; i++) {
    ayTestToneOff(i);
  }
  delay(1000);
}
