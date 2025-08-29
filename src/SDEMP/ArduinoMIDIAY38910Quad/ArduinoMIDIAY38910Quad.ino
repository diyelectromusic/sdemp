/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Quad AY-3-8910
//  https://diyelectromusic.com/2025/08/29/arduino-and-ay-3-8910-part-5/
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
#include <MIDI.h>

#define NOTE_START  12   // C0
#define NUM_NOTES   108
#define POLYPHONY   12

// Change this to one of channels 1..16 if only require
// single channel operation.
#define MIDI_CHANNEL MIDI_CHANNEL_OMNI

// Comment out to add velocity sensitivity over MIDI.
// Otherwise this is the fixed velocity that will be used.
#define VELOCITY    64

////////////////////////////////////////////////////
//
//   AY-3-8910 Driver Code
//
////////////////////////////////////////////////////

// Assumes a 1MHz clock
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

#define NUM_AY 4
int BC1_pins[NUM_AY] = {11,A0,A2,A4};
int BDIR_pins[NUM_AY] = {12,A1,A3,A5};
//  BC2 is fixed at 5V

// Clock and RESET are common to all chips
#define CLK  10
#define RST  13

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
        PORTB &= 0xF7; // D11 LOW
      } else {
        PORTB |= 0x08; // D11 HIGH
      }
      break;
    case 1:
      if (val == LOW) {
        PORTC &= 0xFE; // A0/D14 LOW
      } else {
        PORTC |= 0x01; // A0/D14 HIGH
      }
      break;
    case 2:
      if (val == LOW) {
        PORTC &= 0xFB; // A2/D16 LOW
      } else {
        PORTC |= 0x04; // A2/D16 HIGH
      }
      break;
    case 3:
      if (val == LOW) {
        PORTC &= 0xEF; // A4/D18 LOW
      } else {
        PORTC |= 0x10; // A4/D18 HIGH
      }
      break;
  }
}

inline void ayBDIR (int ay, int val) {
  switch (ay) {
    case 0:
      if (val == LOW) {
        PORTB &= 0xEF; // D12 LOW
      } else {
        PORTB |= 0x10; // D12 HIGH
      }
      break;
    case 1:
      if (val == LOW) {
        PORTC &= 0xFD; // A1/D15 LOW
      } else {
        PORTC |= 0x02; // A1/D15 HIGH
      }
      break;
    case 2:
      if (val == LOW) {
        PORTC &= 0xF7; // A3/D17 LOW
      } else {
        PORTC |= 0x08; // A3/D17 HIGH
      }
      break;
    case 3:
      if (val == LOW) {
        PORTC &= 0xDF; // A5/D19 LOW
      } else {
        PORTC |= 0x20; // A5/D19 HIGH
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

void aySetFreq (int ay, int ch, int note, int vel) {
  if (note < NOTE_START && note != 0) return;
  if (note >= NOTE_START+NUM_NOTES) return;

  int vol = vel >> 3;  // 0..127->0..15
  uint16_t freq = 0;
  if (note != 0) {
    freq = pgm_read_word(&Notes[note-NOTE_START]);
  }

  switch (ch) {
    case 0:
      ayFastWrite (ay, AY38910Regs::A_TONE_C, freq >> 8);
      ayFastWrite (ay, AY38910Regs::A_TONE_F, freq & 0x0FF);
      ayFastWrite (ay, AY38910Regs::A_AMP, vol);
      break;
    case 1:
      ayFastWrite (ay, AY38910Regs::B_TONE_C, freq >> 8);
      ayFastWrite (ay, AY38910Regs::B_TONE_F, freq & 0x0FF);
      ayFastWrite (ay, AY38910Regs::B_AMP, vol);
      break;
    case 2:
      ayFastWrite (ay, AY38910Regs::C_TONE_C, freq >> 8);
      ayFastWrite (ay, AY38910Regs::C_TONE_F, freq & 0x0FF);
      ayFastWrite (ay, AY38910Regs::C_AMP, vol);
      break;
  }
}

void ayChipSetup(int ay) {
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

  ayFastWrite (ay, AY38910Regs::EN, 0xF8);  // Enable bits active low, so Ch A,B,C = on
}

void aySetup (void) {
  ayResetOn();
  ayClockSetup();
  for (int i=0; i<NUM_AY; i++) {
    ayFastWriteSetup(i);
  }
  ayResetOff();
  delay(200);

  for (int i=0; i<NUM_AY; i++) {
    ayChipSetup(i);
  }
}

void ayNoteOn (int chan, int pitch, int vel) {
  // Split 12 total channels in 4 sets of 3
  int ay = chan / 3;
  int ch = chan % 3;

  // Not doing any clever processing here, assuming it was
  // all done when considering polyphony.
  aySetFreq (ay, ch, pitch, vel);
}

void ayNoteOff (int chan, int pitch) {
  int ay = chan / 3;
  int ch = chan % 3;
  aySetFreq (ay, ch, 0, 0);
}

////////////////////////////////////////////////////
//
//   MIDI Code
//
////////////////////////////////////////////////////

MIDI_CREATE_DEFAULT_INSTANCE();

byte playing[POLYPHONY];
int  playidx;

void midiSetup (void) {
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.begin(MIDI_CHANNEL);
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  if ((pitch < NOTE_START) || (pitch >= NOTE_START+NUM_NOTES)) {
    // The note is out of range for us so do nothing
    return;
  }

  // Find a free tone slot to play. Ignore if no free slots.
  for (int i=0; i<POLYPHONY; i++) {
    if (playing[i] == 0) {
      // We have a free slot so claim it and play the note
      playing[i] = pitch;
#ifdef VELOCITY
      ayNoteOn(i, pitch, VELOCITY);
#else
      ayNoteOn(i, pitch, velocity);
#endif

      // Then stop looking for free slots...
      // (or it will fill them all up with the same note!)
      return;
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  // Find which tone is playing this note and turn it off.
  // If we don't recognise the note, then just ignore it.
  for (int i=0; i<POLYPHONY; i++) {
    if (playing[i] == pitch) {
      // This is our note, turn it off
      ayNoteOff(i, pitch);
      playing[i] = 0;

      return;
    }
  }
}

////////////////////////////////////////////////////
//
//   Arduino Code
//
////////////////////////////////////////////////////

void setup () {
  aySetup();
  midiSetup();
}

void loop() {
  MIDI.read();
}
