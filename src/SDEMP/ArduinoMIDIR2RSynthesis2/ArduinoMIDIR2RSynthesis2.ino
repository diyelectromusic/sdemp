/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI R2R Synthesis - Part 2
//  https://diyelectromusic.wordpress.com/2022/10/11/arduino-midi-r2r-digital-audio-part-2/
//
      MIT License
      
      Copyright (c) 2022 diyelectromusic (Kevin)
      
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
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Arduino R2R DAC - https://makezine.com/2008/05/29/makeit-protodac-shield-fo/
    Arduino Direct Port Manipulation - https://www.arduino.cc/en/Reference/PortManipulation
    Arduino Timer1  - https://playground.arduino.cc/Code/Timer1/
    Direct Digital Synthesis - https://www.electronics-notes.com/articles/radio/frequency-synthesizer/dds-direct-digital-synthesis-synthesizer-what-is-basics.php
    "Audio Output and Sound Synthesis" from "Arduino for Musicians" Brent Edstrom
*/
#include <MIDI.h>
#include "pitches.h"
#include "TimerOne.h"

//#define TEST 1
//#define DDSDBG 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set up the MIDI channel to listen on
#define MIDI_CHANNEL 1

// Set up the MIDI codes to respond to by listing the lowest note
#define MIDI_NOTE_START 23   // B0

// Uncomment this to prefix the waveform to be used
//#define DEFAULT_WAVE 0

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
uint8_t wave;
int     env;
int     wavetype;
int     playnote;
int     fade;
bool    gate, lastgate;
#define ZBIAS 128 // "Zero" value for a 0 to 255 biased signal
#define FADEINC 2 // 1 to 255 depending how large an increment is required

// This is the only wave we set up "by hand"
// Note: this is the first half of the wave,
//       the second half is just this mirrored.
const int PROGMEM sinetable[128] = {
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
int8_t wavetable[256];

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

#ifdef TEST
#ifndef DDSDBG
  Serial.print("Note ON: ");
  Serial.println(pitch);
#endif
#endif
  if ((pitch < MIDI_NOTE_START) || (pitch >= MIDI_NOTE_START+numnotes)) {
    // The note is out of range for us so do nothing
    return;
  }

  playnote = pitch;
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
#ifdef TEST
#ifndef DDSDBG
  Serial.print("Note OFF: ");
  Serial.println(pitch);
#endif
#endif
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

#ifndef DDSDBG
  // Use an Arduino Timer1 to generate a regular "tick"
  // to run our code every TICK microseconds.
  Timer1.initialize (TICK);
  Timer1.attachInterrupt (ddsOutput);
#endif

  wave = 0;
  accumulator = 0;
  lastaccumulator = 0;
  frequency = 0;
  nextfrequency = 0;
  env = 0;
  fade = 0;
  gate = false;
  lastgate = false;

  // Set a default wavetable to get us started
  wavetype = 0;
  setWavetable(wavetype);

  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

#ifndef TEST
  // This listens to the specified MIDI channel
  MIDI.begin(MIDI_CHANNEL);
#endif

  numnotes = sizeof(notes)/sizeof(notes[0]);
}

void loop() {
#ifdef TEST
  testNote();
#ifdef DDSDBG
  ddsOutput();
  Serial.print(frequency);
  Serial.print("\t");
  Serial.print(nextfrequency);
  Serial.print("\t");
  Serial.print(accumulator>>8);
  Serial.print("\t");
  Serial.print(wave);
  Serial.print("\t");
  Serial.print(env);
  Serial.print("\n");
#endif
#else
  MIDI.read();
#endif

  // Use the value read from the potentiometer to set
  // the required audio frequency.
  //
  // Use the map() function to map the potentiometer
  // reading onto our supported frequency range.
  //
  // int pot = analogRead(A0);
  // frequency = map (pot, 0, 1023, FREQ_MIN, FREQ_MAX);

  // Convert the playing note to a frequency
  if (playnote != 0) {
    nextfrequency = notes[playnote-MIDI_NOTE_START];
    gate = true;
  } else {
    // turn the sound generation off
    gate = false;
  }

  // Use the second potentiometer to change the wavetype
  // As there are only 4 wavetypes, we only recognise
  // the top 2 bits of the reading the choose the type!
  //
  // Note: We only actually change the wavetype if
  //       the potentiometer shows it has been changed.
  //
#ifdef DEFAULT_WAVE
  int pot2 = DEFAULT_WAVE;
#else
  int pot2 = analogRead(A1)>>8;
#endif
  if (pot2 != wavetype) {
    wavetype = pot2;
    gate = false;
    setWavetable(wavetype);
  }
}

uint16_t getfrequency () {
  uint16_t newfreq = frequency;
  // See if a change of frequency is required
  if (nextfrequency != frequency) {
    // Only change frequency when the accumulator wraps around
    // to avoid blips
    if ((accumulator == 0) || (lastaccumulator > accumulator)) {
      frequency = nextfrequency;
      newfreq = frequency;
    }
  }

  lastaccumulator = accumulator;

  return newfreq;
}

int getenvelope () {
  if (gate && !lastgate) {
    // Start the envelope and fade in
    fade = FADEINC;
  } else if (!gate && lastgate) {
    // Stop the envelope and fade out
    fade = -FADEINC;
  } else if (fade != 0) {
    // Process the changing envelope
    env = env + fade;
  } else {
    // Leave the envelope as it was
  }
  // Check the values...
  if (env < 0) {
    env = 0;
    fade = 0;
  }
  if (env > 255) {
    env = 255;
    fade = 0;
  }

  lastgate = gate;
  return env;
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

  frequency = getfrequency();

  // Recall that the accumulator is as 16 bit value, but
  // representing an 8.8 fixed point maths value, so we
  // only need the top 8 bits here.
  int16_t nwave = wavetable[accumulator>>8];
  accumulator += (FREQ2INC(frequency));

  // if we have no frequency/note to play then output "zero"
  if (frequency == 0) {
    nwave = 0;
  }

  // Apply any envelope shaping and rescale
  wave = ZBIAS + (uint8_t)(nwave * getenvelope() >> 8);
}

// This function will initialise the wavetable
void setWavetable (int wavetype) {
  if (wavetype == 0) {
    // Sine wave
    for (int i = 0; i < 64; i++) {
      // Read out the first half the wave directly
      // Starting at position 64
      wavetable[i] = (int8_t)pgm_read_byte_near(sinetable + 64 + i) - ZBIAS;
    }
    for (int i = 0; i < 128; i++) {
      // Need to read the second half in reverse
      wavetable[64 + i] = (int8_t)pgm_read_byte_near(sinetable + 127 - i) - ZBIAS;
    }
    for (int i = 0; i < 64; i++) {
      // Read out the first part of the first half of the wave directly
      wavetable[64 + 128 + i] = (int8_t)pgm_read_byte_near(sinetable + i) - ZBIAS;
    }
  }
  else if (wavetype == 1) {
    // Sawtooth wave
    for (int i = 0; i < 256; i++) {
      wavetable[i] = i - ZBIAS;
    }
  }
  else if (wavetype == 2) {
    // Triangle wave
    for (int i = 0; i < 128; i++) {
      // First half of the wave is increasing from 0 to almost full value
      wavetable[i] = (i - 64) * 2;
    }
    for (int i = 0; i < 128; i++) {
      // Second half is decreasing from full down to just above 0
      wavetable[128 + i] = 255 - ((i-64)*2);
    }
  }
  else { // Default also if (wavetype == 3)
    // Square wave
    for (int i = 0; i < 128; i++) {
      wavetable[i] = 127;
    }
    for (int i = 128; i < 256; ++i) {
      wavetable[i] = -128;
    }
  }
#ifdef TEST
  // Print the waveform to the serial port
  for (int i=0; i<256; i++) {
    Serial.println(wavetable[i] + ZBIAS);
  }
  for (int i=0; i<256; i++) {
    Serial.println(wavetable[i] + ZBIAS);
  }
#endif
}


#ifdef DDSDBG
#define TESTPERIOD 5000
#else
#define TESTPERIOD 500
#endif

#ifdef TEST
unsigned long t;
int test;
void testNote () {
  unsigned long ttime = millis();
  if (ttime >= t) {
    t = millis() + TESTPERIOD;
    test++;
    switch (test) {
      case 1:
        handleNoteOn (1, 60, 64);
        break;
      case 2:
        handleNoteOff(1, 60, 0);
        break;
      case 3:
        handleNoteOn (1, 72, 64);
        break;
      case 4:
        handleNoteOff(1, 72, 0);
        break;
      case 5:
        handleNoteOn (1, 60, 64);
        break;      
      case 6:
        handleNoteOn (1, 48, 64);
        break;      
      case 7:
        handleNoteOn (1, 60, 64);
        break;      
      case 8:
        handleNoteOn (1, 72, 64);
        break;      
      case 9:
        handleNoteOff (1, 72, 0);
        handleNoteOff (1, 60, 0);
        handleNoteOff (1, 48, 0);
        break;      
      default:
        test = 0;
    }
  }
}
#endif
