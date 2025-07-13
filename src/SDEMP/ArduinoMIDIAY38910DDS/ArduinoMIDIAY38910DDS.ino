/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino AY-3-8910 MIDI DDS
//  https://diyelectromusic.com/2025/07/13/arduino-and-ay-3-8910-part-3/
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
  Using principles from the following tutorials:
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Arduino AY-3-8910 - https://github.com/Andy4495/AY3891x
    Arduino Timer1  - https://playground.arduino.cc/Code/Timer1/
    Direct Digital Synthesis - https://www.electronics-notes.com/articles/radio/frequency-synthesizer/dds-direct-digital-synthesis-synthesizer-what-is-basics.php
*/
#include <MIDI.h>
#include "AY3891x.h"
#include "AY3891x_sounds.h"  // Contains the divisor values for the musical notes

//MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 1
#define POLYPHONY 3
byte playing[POLYPHONY];
int playidx;

// Comment this out if don't want new notes
// to overwrite already playing notes.
#define POLYOVERWRITE

// Define the range covered by the AY-3-8910 library
#define MIDI_NOTE_START 12  // C0
#define MIDI_NOTE_END   119 // B8

// Comment out to ignore button
#define BUTTON A5
int lastbtn;

#define TIMER_PIN 10

// DDS Notes:
//   Use 8.8 fixed point maths, so values are 256 bigger than will use.
//     Increment = Freq * wavetable size / Sample Rate * 256
//     Increment = Freq * 256 * 256 / 8192 = Freq * 8
//
#define FREQ2INC(freq) (freq*8)
uint16_t accumulator;
uint16_t frequency;
uint8_t  wave;
uint16_t freq;

#define WTSIZE 256
unsigned char wavetable[WTSIZE];

// This is the only wave we set up "by hand"
// Note: this is the first half of the wave,
//       the second half is just this mirrored.
const unsigned char PROGMEM sinetable[128] = {
    0,   0,   0,   0,   1,   1,   1,   2,   2,   3,   4,   5,   5,   6,   7,   9,
   10,  11,  12,  14,  15,  17,  18,  20,  21,  23,  25,  27,  29,  31,  33,  35,
   37,  40,  42,  44,  47,  49,  52,  54,  57,  59,  62,  65,  67,  70,  73,  76,
   79,  82,  85,  88,  90,  93,  97, 100, 103, 106, 109, 112, 115, 118, 121, 124,
  128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173,
  176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215,
  218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244,
  245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255,
};

//-------------------------------------------------------------------
//
//   AY-3-8910 Driver
//
//-------------------------------------------------------------------

#define BC1  14  // A0
#define BC2  15  // A1
#define BDIR 16  // A2

// DA7-DA0, BDIR, BC1, BC2
AY3891x psg( 17, 8, 7, 6, 5, 4, 3, 2, BDIR, BC2, BC1);
// NOTE: D2:D7 = PORTD[2:7]
//       D8 = PORTB[0]
//       D17/A3 = PORTC[3]

#define BC1LOW  {PORTC &= 0xFE;} // A0 LOW
#define BC1HIGH {PORTC |= 0x01;} // A0 HIGH
#define BC2LOW  {PORTC &= 0xFD;} // A1 LOW
#define BC2HIGH {PORTC |= 0x02;} // A1 HIGH
#define BDIRLOW  {PORTC &= 0xF7;} // A2 LOW
#define BDIRHIGH {PORTC |= 0x04;} // A2 HIGH

void ayFastWriteSetup () {
  // Fix output modes for all pins too
  DDRD |= 0xFC;  // DA0-DA5
  DDRC |= 0x0F;  // DA7, BC1, BC2, BDIR
  DDRB |= 0x01;  // DA6

  // Fix BC2 high for later fast writes
  BC2HIGH;
}

void ayFastWrite (byte reg, byte val) {
  // Mode=Addr Latch
  BC1HIGH;
  BDIRHIGH;

  // Latch address
  // NB: Addresses are all in range 0..15 so don't need to
  //     worry about writing out bits 6,7 - just ensure set to zero
  PORTD = (PORTD & 0x03) | ((reg & 0xCF)<<2); // Bits 0-5 map to PORTD[2:7]
  PORTB = (PORTB & 0xFE); // Bit 6 maps to PORTB[0]
  PORTC = (PORTC & 0xF7); // Bit 7 maps to PORTC[3]

  // Need 400nS Min
  delayMicroseconds(1);

  // Mode = Inactive
  BC1LOW;
  BDIRLOW;

  // Need 100nS settle then 50nS preamble
  delayMicroseconds(1);

  // Mode = Write
  BC1LOW;
  BDIRHIGH;

  // Write data
  PORTD = (PORTD & 0x03) | ((val & 0xCF)<<2); // Shift bits 0:5 to 2:7
  PORTB = (PORTB & 0xFE) | ((val & 0x40)>>6); // Shift bit 6 to 0
  PORTC = (PORTC & 0xF7) | ((val & 0x80)>>4); // Shift bit 7 to 3

  // Need 500nS min
  delayMicroseconds(1);

  // Mode = Inactive
  BC1LOW;
  BDIRLOW;

  // Need 100nS min
  // Assume the normal machine of running other code will give this
}

// Taken from AY3891x library examples...
//
// The following code generates a 1 MHz 50% duty cycle output to be used
// as the clock signal for the AY-3-891x chip.
#define AY_CLOCK 9 // D9
void aySetup () {
  pinMode(AY_CLOCK, OUTPUT);
  digitalWrite(AY_CLOCK, LOW);

  // Timer 1
  // Use CTC mode: WGM13..0 = 0100
  // Toggle OC1A on Compare Match: COM1A1..0 = 01
  // Use ClkIO with no prescaling: CS12..0 = 001
  // Interrupts off: TIMSK1 = 0
  // OCR1A = interval value
  //
  // NB: As OC1A toggles on match, need counter 
  //     running at 2MHz for a 1MHz clock signal.
  TCCR1A = (1 << COM1A0);
  TCCR1B = (1 << WGM12) | (1 << CS10);
  TCCR1C = 0;
  TIMSK1 = 0;
  OCR1AH = 0;
  OCR1AL = 7; // 16MHz / 8 = 2MHz Counter

  psg.begin();

  // Output highest frequency on each channel, but set level to 0
  // Highest freq = 1000000 / (16 * 1) = 62500
  psg.write(AY3891x::ChA_Amplitude, 0);
  psg.write(AY3891x::ChA_Tone_Period_Coarse_Reg, 0);
  psg.write(AY3891x::ChA_Tone_Period_Fine_Reg,   0);
  psg.write(AY3891x::ChB_Amplitude, 0);
  psg.write(AY3891x::ChB_Tone_Period_Coarse_Reg, 0);
  psg.write(AY3891x::ChB_Tone_Period_Fine_Reg,   0);
  psg.write(AY3891x::ChC_Amplitude, 0);
  psg.write(AY3891x::ChC_Tone_Period_Coarse_Reg, 0);
  psg.write(AY3891x::ChC_Tone_Period_Fine_Reg,   0);

  // LOW = channel is in the mix.
  // Turn everything off..
  psg.write(AY3891x::Enable_Reg, 0xFF);

  ayFastWriteSetup();
}

void ayOutput (byte ch, byte level) {
  // Convert from 8-bit value to a 4-bit level
  switch (ch) {
    case 0:
      ayFastWrite(AY3891x::ChA_Amplitude, level>>4);
      break;
    case 1:
      ayFastWrite(AY3891x::ChB_Amplitude, level>>4);
      break;
    case 2:
      ayFastWrite(AY3891x::ChC_Amplitude, level>>4);
      break;
    default:
      // ignore
      break;
  }
}

void ayNoteOn (byte ch, byte note, byte vel) {
}

void ayNoteOff (byte ch, byte note) {
}

//-------------------------------------------------------------------
//
//   MIDI
//
//-------------------------------------------------------------------

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  if (velocity == 0) {
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  if ((pitch < MIDI_NOTE_START) || (pitch > MIDI_NOTE_END)) {
    // The note is out of range for us so do nothing
    return;
  }

#ifdef POLYOVERWRITE
  if (playing[playidx] != 0) {
    // Find next slot free to play, overwriting oldest playing notes
    playidx++;
    if (playidx >= POLYPHONY) playidx = 0;
  }
  ayNoteOn(playidx, pitch-MIDI_NOTE_START, velocity>>4);
  playing[playidx] = pitch;
#else
  // Find a free slot.
  // Ignore if all notes already sounding.
  for (int i=0; i<POLYPHONY; i++) {
    if (playing[i] == 0) {
      ayNoteOn(i, pitch-MIDI_NOTE_START, velocity>>4);
      playing[i] = pitch;
      // Once a free slot is claimed, stop looking for more
      return;
    }
  }
#endif
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  if ((pitch < MIDI_NOTE_START) || (pitch > MIDI_NOTE_END)) {
    // The note is out of range for us so do nothing
    return;
  }

  // If we receive a note off event for the playing note, turn it off
  for (int i=0; i<POLYPHONY; i++) {
    if (pitch == playing[i]) {
      ayNoteOff(i, pitch-MIDI_NOTE_START);
      playing[i] = 0;
    }
  }
}

//-------------------------------------------------------------------
//
//   DDS
//
//-------------------------------------------------------------------

volatile int timerToggle;
void ddsSetup () {
#ifdef TIMER_PIN
  pinMode(TIMER_PIN, OUTPUT);
  digitalWrite(TIMER_PIN, LOW);
  timerToggle = HIGH;
#endif
  // Setup an interrupt at the desired sample rate to run DDS.
  // Use the 8-bit Timer 2 compare A match interrupt.
  // Run at clock/32 speed (500kHz).
  // CTC mode, clear on OCR2A match.
  // TOP = SysClk / N.Freq - 1
  // TOP = 16MHz / (32 x 8192) - 1 = 61 - 1
  // Sample Rate = 8192 Hz
  //
  // COM2A[1:0] = 00  No output
  //  WGM2[2:0] = 010 CTC mode
  //   CS2[2:0] = 011 Prescalar=32
  pinMode(11,OUTPUT);
  ASSR = 0;
  TCCR2A = _BV(WGM21) | _BV(COM2A0);
  TCCR2B = _BV(CS21) | _BV(CS20);
  TCNT2 = 0;
  OCR2A = 60;
  TIMSK2 = _BV(OCIE2A);
}

void ddsUpdate() {
  // Output the last calculated value first to reduce "jitter"
  // then go on to calculate the next value to be used.
  ayOutput(0, wave);

  // Recall that the accumulator is as 16 bit value, but
  // representing an 8.8 fixed point maths value, so we
  // only need the top 8 bits here.
  wave = wavetable[accumulator>>8];
  accumulator += (FREQ2INC(frequency));
}

ISR(TIMER2_COMPA_vect) {
#ifdef TIMER_PIN
#if 0
  if (timerToggle==HIGH) {
    timerToggle = LOW;
  } else {
    timerToggle = HIGH;
  }
  digitalWrite(TIMER_PIN, timerToggle);
#endif
#endif

  digitalWrite(TIMER_PIN, HIGH);
  ddsUpdate();
  digitalWrite(TIMER_PIN, LOW);
}

// This function will initialise the wavetable
void setWavetable (int wavetype) {
  if (wavetype == 0) {
    // Sine wave
    for (int i = 0; i < 128; i++) {
      // Read out the first half the wave directly
      wavetable[i] = pgm_read_byte_near(sinetable + i);
    }
    wavetable[128] = 255;  // the peak of the wave
    for (int i = 1; i < 128; i++) {
      // Need to read the second half in reverse missing out
      // sinetable[0] which will be the start of the next one
      wavetable[128 + i] = pgm_read_byte_near(sinetable + 128 - i);
    }
  }
  else if (wavetype == 1) {
    // Sawtooth wave
    for (int i = 0; i < 256; i++) {
      wavetable[i] = i;
    }
  }
  else if (wavetype == 2) {
    // Triangle wave
    for (int i = 0; i < 128; i++) {
      // First half of the wave is increasing from 0 to almost full value
      wavetable[i] = i * 2;
    }
    for (int i = 0; i < 128; i++) {
      // Second half is decreasing from full down to just above 0
      wavetable[128 + i] = 255 - (i*2);
    }
  }
  else { // Default also if (wavetype == 3)
    // Square wave
    for (int i = 0; i < 128; i++) {
      wavetable[i] = 255;
    }
    for (int i = 128; i < 256; ++i) {
      wavetable[i] = 0;
    }
  }
}

//-------------------------------------------------------------------
//
//   Arduino setup/loop
//
//-------------------------------------------------------------------

void setup() {
  frequency = 440;
  setWavetable(0);

  aySetup();
  ddsSetup();
//  MIDI.begin(MIDI_CHANNEL);
//  MIDI.setHandleNoteOn(handleNoteOn);
//  MIDI.setHandleNoteOff(handleNoteOff);
  Serial.begin(9600);

#ifdef BUTTON
  pinMode(BUTTON, INPUT_PULLUP);
  lastbtn = HIGH;
#endif
}

void loop() {
//  MIDI.read();
  Serial.print(accumulator>>8);
  Serial.print("\t");
  Serial.println(wave);

  int btn = digitalRead(BUTTON);
  if (lastbtn == HIGH && btn == LOW) {
    // Button pressed
    delay(100);
  }
  lastbtn = btn;
}
