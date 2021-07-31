/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI R2R Slider Waveform Generator
//  https://diyelectromusic.wordpress.com/2021/07/31/arduino-midi-slider-r2r-waveform-generator/
//
      MIT License
      
      Copyright (c) 2020 diyelectromusic (Kevin)
      
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
    Potentiometer        - https://www.arduino.cc/en/Tutorial/Potentiometer
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Arduino R2R DAC - https://makezine.com/2008/05/29/makeit-protodac-shield-fo/
    Arduino Direct Port Manipulation - https://www.arduino.cc/en/Reference/PortManipulation
    Arduino Timer1  - https://playground.arduino.cc/Code/Timer1/
    Direct Digital Synthesis - https://www.electronics-notes.com/articles/radio/frequency-synthesizer/dds-direct-digital-synthesis-synthesizer-what-is-basics.php
    "Audio Output and Sound Synthesis" from "Arduino for Musicians" Brent Edstrom
*/
#include <MIDI.h>
#include "pitches.h"
#include <TimerOne.h>

// Uncomment this line to test updating the wavetable
//#define TEST 1
//#define SERPLOT 1
#define TESTNOTE 69 // A4 - "Concert A" @ 440Hz

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set up the MIDI channel to listen on
#define MIDI_CHANNEL 1

// Set up the MIDI codes to respond to by listing the lowest note
#define MIDI_NOTE_START 23   // B0

// Set the notes to be played by each key
int notes[] = {
  NOTE_B0,
  NOTE_C1, NOTE_CS1, NOTE_D1, NOTE_DS1, NOTE_E1, NOTE_F1, NOTE_FS1, NOTE_G1, NOTE_GS1, NOTE_A1, NOTE_AS1, NOTE_B1,
  NOTE_C2, NOTE_CS2, NOTE_D2, NOTE_DS2, NOTE_E2, NOTE_F2, NOTE_FS2, NOTE_G2, NOTE_GS2, NOTE_A2, NOTE_AS2, NOTE_B2,
  NOTE_C3, NOTE_CS3, NOTE_D3, NOTE_DS3, NOTE_E3, NOTE_F3, NOTE_FS3, NOTE_G3, NOTE_GS3, NOTE_A3, NOTE_AS3, NOTE_B3,
  NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
  NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
  NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
  NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7,
  NOTE_C8, NOTE_CS8, NOTE_D8, NOTE_DS8
};
int numnotes;

// This project runs code on the command of one of the Arduino Timers.
// This determines the length of the "tick" of the timer.
//
// Using a TICK of 10 means our code runs every
// 10 microseconds, i.e. at a frequency of 100kHz.
//
// A TICK of 50 is 20kHz and so on.
//
// The TICK essentially defines the period of the
// sample rate to use. So to use a sample rate of 16,384Hz
// we use a TICK of 61uS as 61uS = 1/16384.
//
// If we output one sample from the wavetable on
// every TICK, then the frequency we achieve is:
//     freq = Sample Rate / Size of wavetable
//
// So for a 20kHz sample rate, freq = 20000/256 = 78.125Hz
//
// If we want to output a specific audio frequency then
// we need to work out how quickly to step through the wavetable
// when running at our base sample frequency.
//
// We do that as follows:
//   The required frequency of out sample outputs = required audio freq * wavetable size.
//   So the period of that required frequency of sample output = 1/freq of sample outputs.
//   So the desired increment on every TICK of our sample rate (i.e. how
//      much we step through the wave table on each TICK)
//         = Sample period / Required sample output period.
//
// So to produce a 440Hz note, using a 20kHz sample rate (i.e. TICK of 50uS) and a 256 step wavetable:
//   Freq of output   = 440 * 256 = 112.64kHz
//   Period of output = 1/112640 = 8.878uS
//   Increment step   = 50uS / 8.878uS = 5.632
//
// Combining the above, we can say:
//   Increment step = (1/Sample Rate) / (1 / (freq * wavetable size))
// Or simplifying:
//   Increment step = Required Frequency * wavetable size / Sample rate
// Or
//   Increment step = Required Frequency * wavetable size * TICK(Seconds)
//
// So to re-run the example:
//   Increment step = 440 * 256 / 20000 = 5.632
//
// I'm using a Sample Rate of 16384 to make these calculations simple
// for a 256 value wave table and for another reason (see later).
//
#define TICK   61    // 61uS = 16384 Hz Sample Rate

#define FREQ_MIN 55   // A1
#define FREQ_MAX 1760 // A6

// The accumulator is a 16 bit value, but we only need
// to refer to one of 256 values in the wavetable.  However
// the calculations involving the frequency often turn up decimals
// which are awkward to handle in code like this (and you lose accuracy).
//
// One way round this is to use "fixed point maths" where
// we take, say, a 16 bit value and use the top 8 bits
// for the whole number part and the low 8 bits for the
// decimal part.
//
// This is what I do with the accumulator.  This means
// that when it comes to actually using the accumulator
// to access the wavetable, it is 256 times bigger than
// we need it to be, so will always use (accumulator>>8)
// to calculate the index into the wavetable.
//
// This needs to be taken into account when calculating
// accumulator increments which will be using the formulas
// discussed above, but with several pre-set constant values.
//
// As we are using constant values involving 256 and a sample
// rate of 16,384Hz it is very convenient to pre-calculate the
// values to be used in the frequency to increment calculation,
// especially making use of the 8.8 fixed point maths we've already mentioned.
//
// Recall:
//     Inc = freq * wavetable size / Sample rate
//
// We also want this 256 times larger to support the 8.8 FP maths.
// So for a sample rate of 16384 and wavetable size of 256
//     Inc = 256 * freq * 256 / 16384
//     Inc = freq * 65535/16384
//     Inc = freq * 4
//
// This makes calculating the (256 times too large) increment
// from the frequency pretty simple.
//
#define FREQ2INC(freq) (freq*4)
uint16_t accumulator;
uint16_t lastaccumulator;
uint16_t frequency;
uint16_t nextfrequency;
uint8_t  wave;
int      playnote;

// This is the actual wavetable used. If this is changed
// then updateWavetable() will need re-writing.
#define WTSIZE 256
unsigned char wavetable[WTSIZE];

// Need to set up the single analog pin to use for the MUX
#define MUXPIN A0
// And the digital lines to be used to control it.
// Four lines are required for 16 pots on the MUX.
// You can drop this down to three if only using 8 pots.
#define MUXLINES 4
int muxpins[MUXLINES]={A5,A4,A3,A2};
// Uncomment this if you want to control the MUX EN pin
// (optional, only required if you have several MUXs or
// are using the IO pins for other things too).
// This is active LOW.
//#define MUXEN    3

// Using 16 pots to define the wave.  If this changes or
// the size of the wavetable changes, then updateWavetable()
// will need re-writing.
#define NUMPOTS 16
// Use this to define the order for reading the pots and
// to decide if any of them need "mirroring" (i.e. 1023-analogRead()).
int pots[NUMPOTS] = {7,6,5,4,3,2,1,0,8,9,10,11,12,13,14,15};
int mirrorpots[NUMPOTS] = {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0};

int pot;
int potReading[NUMPOTS];
int lastReading[NUMPOTS];

int muxAnalogRead (int mux) {
  if (mux >= NUMPOTS) {
    return 0;
  }

#ifdef MUXEN
  digitalWrite(MUXEN, LOW);  // Enable MUX
#endif

  // Translate mux pot number into the digital signal
  // lines required to choose the pot.
  for (int i=0; i<MUXLINES; i++) {
    if (mux & (1<<i)) {
      digitalWrite(muxpins[i], HIGH);
    } else {
      digitalWrite(muxpins[i], LOW);
    }
  }

  // And then read the analog value from the MUX signal pin
  int algval = analogRead(MUXPIN);

#ifdef MUXEN
  digitalWrite(MUXEN, HIGH);  // Disable MUX
#endif
  
  return algval;
}

void muxInit () {
  for (int i=0; i<MUXLINES; i++) {
    pinMode(muxpins[i], OUTPUT);
    digitalWrite(muxpins[i], LOW);
  }

#ifdef MUXEN
  pinMode(MUXEN, OUTPUT);
  digitalWrite(MUXEN, HIGH); // start disabled
#endif
}

// These are the functions to be called on recieving certain MIDI events.
// Full documentation of how to do this is provided here:
// https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-Callbacks

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  if ((pitch < MIDI_NOTE_START) || (pitch >= MIDI_NOTE_START+numnotes)) {
    // The note is out of range for us so do nothing
    return;
  }

  playnote = pitch;
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  if (playnote == pitch) {
    // This is our note, so turn it off
    playnote = 0;
  }
}

void setup() {
#ifdef TEST
  Serial.begin(9600);
#endif
  // Initialise our output pins
  DDRD |= 0b11111100;
  DDRB |= 0b00000011;

  muxInit();

  // Build the initial wavetable
  for (int i=0; i<NUMPOTS; i++) {
    if (mirrorpots[i]) {
      potReading[i] = 1023-muxAnalogRead(pots[i]);
    } else {
      potReading[i] = muxAnalogRead(pots[i]);
    }
    updateWavetable(i, potReading[i]);
  }

  // Use an Arduino Timer1 to generate a regular "tick"
  // to run our code every TICK microseconds.
  Timer1.initialize (TICK);
  Timer1.attachInterrupt (ddsOutput);

  wave = 0;
  accumulator = 0;
  lastaccumulator = 0;
  frequency = 0;
  nextfrequency = 0;

#ifdef TEST
  // Dump the values from the wavetable out to the serial port
  // for use with the Arduino serial plotter
  if (pot == 0) {
#ifdef SERPLOT
    for (int i=0; i<WTSIZE; i++) {
      Serial.println(wavetable[i]);
    }
#endif
    delay(1000);
  }
#else
  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to the specified MIDI channel
  MIDI.begin(MIDI_CHANNEL);
#endif

  numnotes = sizeof(notes)/sizeof(notes[0]);
}

void loop() {
#ifdef TEST
  playnote = TESTNOTE;
#else
  MIDI.read();
#endif
  // Need to read the potentiometers and update the wavetable
  int algval;
  if (mirrorpots[pot]) {
    algval = 1023-muxAnalogRead(pots[pot]);
  } else {
    algval = muxAnalogRead(pots[pot]);
  }
  if (algval != potReading[pot]) {
    // Need to update the reading and rebuild the wavetable
    potReading[pot] = algval;
    updateWavetable(pot, algval);
  }
  pot++;
  if (pot >= NUMPOTS) pot = 0;

  // Convert the playing note to a frequency
  if (playnote != 0) {
    nextfrequency = notes[playnote-MIDI_NOTE_START];
  } else {
    // turn the sound generation off
    nextfrequency = 0;
  }

#ifdef TEST
  // Dump the values from the wavetable out to the serial port
  // for use with the Arduino serial plotter
  if (pot == 0) {
#ifdef SERPLOT
    for (int i=0; i<WTSIZE; i++) {
      Serial.println(wavetable[i]);
    }
#endif
    delay(1000);
  }
#endif
}

// This is the code that is run on every "tick" of the timer.
void ddsOutput () {
  // Output the last calculated value first to reduce "jitter"
  // then go on to calculate the next value to be used.

  // Ideally this would be a single write action, but
  // I didn't really want to use the TX/RX pins on the Arduino
  // as outputs - these are pins 0 and 1 of PORTD.
  //
  // There is the possibility that the slight delay between
  // setting the two values might create a bit of a "glitch"
  // in the output, but the last two bits (PORTB) are pretty
  // much considered minor adjustments really, so it shouldn't
  // be too noticeable.
  //
  uint8_t pd = wave & 0b11111100;
  uint8_t pb = wave & 0b00000011;
  PORTD = pd;
  PORTB = pb;

  // Avoid jumps part way through the waveform on change of frequency.
  // Only update the frequency when the accumulator wraps around.
  if (nextfrequency != frequency) {
    if ((accumulator == 0) || (lastaccumulator > accumulator)) {
      frequency = nextfrequency;
      accumulator = 0;
    } else if (frequency == 0) {
      // Exception - if the frequency is already zero then
      // can move to a new frequency straight away as long as we
      // reset the accumulator.
      frequency = nextfrequency;
      accumulator = 0;
    }
  }
  lastaccumulator = accumulator;

  // Recall that the accumulator is as 16 bit value, but
  // representing an 8.8 fixed point maths value, so we
  // only need the top 8 bits here.
  wave = wavetable[accumulator>>8];
  accumulator += (FREQ2INC(frequency));
}

// This function rebuilds the wavetable from potentiometer readings.
// It is designed for incremental updates, so will just adjust one
// value and the interpolated values between it and its neighbours.
//
// As written it assumes a wavetable of 256 values and 16 step updates.
// If the size of the wavetable or the number of potentiometers changes
// then this function will need re-writing.
//
uint16_t tempwt[33];
void updateWavetable (int idx, int newval) {
  if ((idx < 0) || (idx >= NUMPOTS) || (NUMPOTS != 16) || (WTSIZE != 256)) {
    // Values are out of range
    return;
  }

  // There are 256 values in the wavetable, but only 16 potentiometers
  // to define them, so there is one real value for every 16 values in the table.
  //
  // This means that we need to interpolate between values to fill in the gaps.
  //
  // We need to take care that the start and end of the table link up too, so
  // need to do something special for idx=0 and idx=15.
  //
  // I am treating each idx as being the first value in the table, then will work
  // on the following 15 values up until the next idx.
  //
  // I'm just using basic linear interpolation here.  More accurate algorithms
  // are available that would perform actual curve fitting, but I'm keeping it simple
  // for now.
  //
  // Note that although the actual wavetable has only 8-bit resolution, I'm using
  // 16-bit integers in the calculation, relying on the full 1023 bit range from
  // the analog readings, and only dropping back to 8-bits at the end.
  //
  // I'm using a 8-bit.8-bit fixed point numerical value for the calculation.
  // This means each value is 256 times bigger than it needs to be but anything
  // in the last 8 bits is treated as a fractional component.
  //
  // This also goes for the 10 bit analog reading, so the 0-1023 is turned into
  // and 8.2 fixed point value - i.e. 8 significant bits and 2 "decimal points".

  // Calculate the indexes into the wavetable to use:
  //    (idx-1)*16 .... idx*16 .... (idx+1)*16
  // As idx=0 is the start of the wavetable, we using count up from the lower
  // index to get the right part of the table.
  //
  int lastidx = (idx-1)*16;
  if (idx == 0) lastidx = 15*16;
  int nextidx = (idx+1)*16;
  if (idx == 15) nextidx = 0;
  int curidx = idx*16;

  // Turn the 10-bit analog value into a fixed point 8.2 value and
  // store it in our temporary wavetable
  tempwt[16] = newval << 6;

  // Now do the same for the last and first values from the real
  // wavetable, this time converting from 8-bit to 8.8 fixed point.
  tempwt[0] = wavetable[lastidx] << 8;
  tempwt[32] = wavetable[nextidx] << 8;

#ifdef TEST
#ifndef SERPLOT
  Serial.print(idx);
  Serial.print("\t");
  for (int i=0; i<33; i++) {
    Serial.print(tempwt[i]);
    Serial.print("\t");
  }
  Serial.print("\n");
#endif
#endif
  // Now calculate the 15 new values for the wavetable for the range
  //    lastidx to idx, represented by 0 to 16
  //    idx to nextidx, represented by 16 to 32
  //
  // Linear interpolation gives the equation for a point on a line
  // between two other points (x1,y1) and (x2,y2) as:
  //
  //   y = y1 + (x - x1) * gradient of the line
  //
  // Where the gradient of the line  = (y2 - y1) / (x2 - x1)
  //
  // In our case, x1 = 0 and x2 = 16 for each section of the wavetable.
  //
  // NB: Don't need to do the calculation for i=0 as that is defined
  //     by the existing value each time.
  //
  long lastgrad = ((long)tempwt[16] - (long)tempwt[0]) / 16;
  long nextgrad = ((long)tempwt[32] - (long)tempwt[16]) / 16;
  for (long i=1L; i<16; i++) {
    // lastidx to idx
    tempwt[i] = (uint16_t)((long)tempwt[0] + i*lastgrad);

    // idx to nextidx
    tempwt[16+i] = (uint16_t)((long)tempwt[16] + i*nextgrad);
  }

#ifdef TEST
#ifndef SERPLOT
  Serial.print(idx);
  Serial.print("\t");
  for (int i=0; i<33; i++) {
    Serial.print(tempwt[i]);
    Serial.print("\t");
  }
  Serial.print("\n");
#endif
#endif
  // Now write this section of the wavetable back, converting it
  // back from 8.8 fixed point into an 8-bit only value.
  for (int i=0; i<16; i++) {
    wavetable[lastidx+i] = tempwt[i]>>8;
    wavetable[curidx+i] = tempwt[16+i]>>8;
  }
}
