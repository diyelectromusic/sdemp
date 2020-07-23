/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino R2R Synthesis - Part 3
//  https://diyelectromusic.wordpress.com/2020/06/28/arduino-r2r-digital-audio-part-3/
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
    Arduino R2R DAC - https://makezine.com/2008/05/29/makeit-protodac-shield-fo/
    Arduino Direct Port Manipulation - https://www.arduino.cc/en/Reference/PortManipulation
    Arduino Timer1  - https://playground.arduino.cc/Code/Timer1/
    Direct Digital Synthesis - https://www.electronics-notes.com/articles/radio/frequency-synthesizer/dds-direct-digital-synthesis-synthesizer-what-is-basics.php
    "Audio Output and Sound Synthesis" from "Arduino for Musicians" Brent Edstrom
*/
#include "TimerOne.h"

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
uint16_t frequency;
uint8_t  wave;
int      wavetype;

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

// This is the actual wavetable used
unsigned char wavetable[256];

void setup() {
  // put your setup code here, to run once:

  // Initialise our output pins
  DDRD |= 0b11111100;
  DDRB |= 0b00000011;

  // Use an Arduino Timer1 to generate a regular "tick"
  // to run our code every TICK microseconds.
  Timer1.initialize (TICK);
  Timer1.attachInterrupt (ddsOutput);

  wave = 0;
  accumulator = 0;
  frequency = 0;

  // Set a default wavetable to get us started
  wavetype = 0;
  setWavetable(wavetype);
}

void loop() {
  // put your main code here, to run repeatedly:

  // Use the value read from the potentiometer to set
  // the required audio frequency.
  //
  // Use the map() function to map the potentiometer
  // reading onto our supported frequency range.
  //
  int pot = analogRead(A0);
  frequency = map (pot, 0, 1023, FREQ_MIN, FREQ_MAX);

  // Use the second potentiometer to change the wavetype
  // As there are only 4 wavetypes, we only recognise
  // the top 2 bits of the reading the choose the type!
  //
  // Note: We only actually change the wavetype if
  //       the potentiometer shows it has been changed.
  //
  int pot2 = analogRead(A1)>>8;
  if (pot2 != wavetype) {
    wavetype = pot2;
    setWavetable(wavetype);
  }
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

  // Recall that the accumulator is as 16 bit value, but
  // representing an 8.8 fixed point maths value, so we
  // only need the top 8 bits here.
  wave = wavetable[accumulator>>8];
  accumulator += (FREQ2INC(frequency));
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
