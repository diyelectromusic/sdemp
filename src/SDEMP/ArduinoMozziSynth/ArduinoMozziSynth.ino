/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Mozzi MIDI FM Synthesis
//  https://diyelectromusic.wordpress.com/2020/08/18/arduino-fm-midi-synthesis-with-mozzi/
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
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Mozzi Library        - https://sensorium.github.io/Mozzi/

  Much of this code is based on the Mozzi example Mozzi_MIDI_input (C) Tim Barrass

*/
#include <MIDI.h>
#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator
#include <mozzi_midi.h>
#include <ADSR.h>

MIDI_CREATE_DEFAULT_INSTANCE();

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 256 // Hz, powers of 2 are most reliable

// audio sinewave oscillator
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin2(SIN2048_DATA);

// envelope generator
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;

#define LED 13 // shows if MIDI is being recieved

// Modulation frequency and level
int modfreq;
int modlevel;
int modratio;
int carrierfreq;

//#define TEST_PLAY 200000
long testcount;

void HandleNoteOn(byte channel, byte note, byte velocity) {
   if (velocity == 0) {
      HandleNoteOff(channel, note, velocity);
      return;
  }
  carrierfreq = mtof(note);
  setFrequencies();
  envelope.noteOn();
  digitalWrite(LED, HIGH);
}

void HandleNoteOff(byte channel, byte note, byte velocity) {
  envelope.noteOff();
  digitalWrite(LED, LOW);
}

void setup() {
  pinMode(LED, OUTPUT);

#ifdef TEST_PLAY
  Serial.begin(9600);
#else
  // Connect the HandleNoteOn function to the library, so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(HandleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(HandleNoteOff);  // Put only the name of the function
  MIDI.begin(MIDI_CHANNEL_OMNI);
#endif
  envelope.setADLevels(255, 64);
  envelope.setTimes(50, 200, 10000, 200); // 10000 is so the note will sustain 10 seconds unless a noteOff comes

  aSin1.setFreq(440); // default frequency
  startMozzi(CONTROL_RATE);
}

void setFrequencies() {
  int modfreq = carrierfreq * modratio;
  aSin1.setFreq(carrierfreq);
  aSin2.setFreq(modfreq);
}

void updateControl() {
#ifndef TEST_PLAY
  MIDI.read();
#endif
  envelope.update();

  // Read the potentiometers for the modulation parameters
  modratio = mozziAnalogRead (A0) >> 5; // 0 to 31
  modlevel = mozziAnalogRead (A1);      // 0 to 1023

  setFrequencies();
}


int updateAudio() {
  // The aSin2 value will be between 0 and 255 and modlevel will
  // be a value between 0 and 1023 so we need to allow for larger
  // modulation values as a result.
  //
  // Q15n16 means modulation has a 15 bit "integer" part and a 16
  // bit fractional part when it is interpreted by the phMod() function.
  // According to the documentation the fractional part covers
  // a modulation from -1 to +1 "modulating the phase by one whole
  // table length in each direction".
  //
  // The widest range of effects comes from using the full range
  // of the fractional part - hence scale the result of modlevel*sin
  // to cover 0 to 65535 here (the full 16 bits of the fraction).
  //
  // Maximum level = 1024*256 or 262144.  Divide that by two twice
  // (i.e. >> 2) gives 65536.
  //
  // If you want a different range of effects, experiment with
  // different values here!  << 6 works quite well!
  //
  Q15n16 modulation = (modlevel * aSin2.next()) >> 2;

  // The ADSR envelope will be a value between 0 and 255
  // so need to divide the final value by 256 to get it back into a sensible range
  return (int) ((envelope.next() * aSin1.phMod(modulation)) >> 8);
}


void loop() {
  audioHook(); // required here

#ifdef TEST_PLAY
  testcount++;
  if (testcount == TEST_PLAY) {
    testcount = -TEST_PLAY;
    HandleNoteOn(1, 60, 127);
  } else if (testcount == 0) {
    HandleNoteOff(1, 60, 127);
  }
#endif
}
