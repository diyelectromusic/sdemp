/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
// Arduino OPL FM Synth
// https://diyelectromusic.wordpress.com/2023/01/27/arduino-opl-fm-synth/
//
      MIT License
      
      Copyright (c) 2023 diyelectromusic (Kevin)
      
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
    toneMelody           - http://www.arduino.cc/en/Tutorial/Tone
    MD_YM2413 Library    - https://github.com/MajicDesigns/MD_YM2413
*/
#include <MIDI.h>
#include <MD_YM2413.h>
#include "pitches.h"
#include "midi_instruments.h"

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set up the MIDI channel to listen on
#define MIDI_CHANNEL 1

#define MIDI_LED LED_BUILTIN

#define NONOTE (-1)
#define YMCHANNELS 9
int playing[YMCHANNELS];

// Arduino pins conneced to the YM2413:
//   D_PIN = Arduino pins for D0 to D7 of the YM2413
//  WE_PIN = Arduino pin for WE of the YM2413
//  A0_PIN = Arduino pin for A0 of the YM2413
// Note CS of the YM2413 is connected to GND
//
//const uint8_t D_PIN[] = { 8, 9, 7, 6, A0, A1, A2, A3 };
const uint8_t D_PIN[] = { 8, 9, 7, 6, 13, 12, 11, 10 };
const uint8_t WE_PIN = 5;
const uint8_t A0_PIN = 4;

MD_YM2413 SYNTH(D_PIN, WE_PIN, A0_PIN);

#define YMVOLUME  MD_YM2413::VOL_MAX // YM2413 Channel volume

// Set up the MIDI codes to respond to by listing the lowest note
#define MIDI_NOTE_START 23   // B0
#define NUM_NOTES 89
uint16_t notes[NUM_NOTES] = {
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

void ledOff () {
#ifdef MIDI_LED
   digitalWrite(MIDI_LED, LOW);
#endif
}

void ledOn () {
#ifdef MIDI_LED
   digitalWrite(MIDI_LED, HIGH);
#endif
}

// These are the functions to be called on recieving certain MIDI events.
// Full documentation of how to do this is provided here:
// https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-Callbacks

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  if ((pitch < MIDI_NOTE_START) || (pitch >= MIDI_NOTE_START+NUM_NOTES)) {
    // The note is out of range for us so do nothing
    return;
  }

  // Find first free channel to play the note
  for (int i=0; i<YMCHANNELS; i++) {
    if (playing[i] == NONOTE) {
      ledOn();
      SYNTH.noteOn(i, (uint16_t)notes[pitch-MIDI_NOTE_START], YMVOLUME, 0);
      playing[i] = pitch;
      return;
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  for (int i=0; i<YMCHANNELS; i++) {
    if (playing[i] == pitch) {
      SYNTH.noteOff(i);
      playing[i] = NONOTE;

      // NB: LED handling isn't accurate, but it is just an indication of activity
      ledOff();
    }
  }
}

void handleProgramChange(byte channel, byte program) {
  program &= 127;  // ensure in range
  SYNTH.loadInstrumentOPL2(midiInstruments[program], true);
}

void setup() {
#ifdef MIDI_LED
  pinMode (MIDI_LED, OUTPUT);
  ledOff();
#endif

  for (int i=0; i<YMCHANNELS; i++) {
    playing[i] = NONOTE;
  }

  SYNTH.begin();
  for (int i=0; i<YMCHANNELS; i++) {
    SYNTH.setVolume(i, YMVOLUME);
    SYNTH.setInstrument(i, MD_YM2413::I_CUSTOM);
  }
  SYNTH.loadInstrumentOPL2(midiInstruments[0]);

  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleProgramChange(handleProgramChange);

  // This listens to the specified MIDI channel
  MIDI.begin(MIDI_CHANNEL);
}

void loop(void)
{
  MIDI.read();
  SYNTH.run();
}
