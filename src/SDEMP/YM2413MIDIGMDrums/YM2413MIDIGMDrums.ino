/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
// Arduino OPL FM Drum Machine
// https://diyelectromusic.wordpress.com/2023/05/05/arduino-opl-fm-drum-machine/
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

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set up the MIDI channel to listen on.
// Channel 10 is the General MIDI Drum channel.
#define MIDI_CHANNEL 10

#define MIDI_LED LED_BUILTIN

#define NONOTE (-1)
#define YMCHANNELS 5  // 5 Drum channels
int playing[YMCHANNELS];

// Arduino pins conneced to the YM2413:
//   D_PIN = Arduino pins for D0 to D7 of the YM2413
//  WE_PIN = Arduino pin for WE of the YM2413
//  A0_PIN = Arduino pin for A0 of the YM2413
// Note CS of the YM2413 is connected to GND
//
//const uint8_t D_PIN[] = { 8, 9, 7, 6, A0, A1, A2, A3 };
//const uint8_t D_PIN[] = { 8, 9, 7, 6, 13, 12, 11, 10 };
//const uint8_t WE_PIN = 5;
//const uint8_t A0_PIN = 4;
const uint8_t D_PIN[] = { 3, 2, 4, 5, 6, 7, 8, 9 };
const uint8_t WE_PIN = 10;
const uint8_t A0_PIN = 11;

MD_YM2413 SYNTH(D_PIN, WE_PIN, A0_PIN);

// Short definitions for the Drum instruments.
// NB: The chip works on channels for percussion, the actual
//     note played on the channel is largely irrelevant!
#define BD MD_YM2413::CH_BD
#define SD MD_YM2413::CH_SD
#define TT MD_YM2413::CH_TOM
#define HH MD_YM2413::CH_HH
#define CY MD_YM2413::CH_TCY
#define NO (255)

// Convert between above definitions and a 0 to YMCHANNELS-1 index
// NB: Doesn't check for x=NO...
#define DRUM2IDX(x) ((x)-MD_YM2413::PERC_CHAN_BASE)
#define IDX2DRUM(x) ((x)+MD_YM2413::PERC_CHAN_BASE)

// Set up the MIDI codes to respond to from the General MIDI Drum
// Instrument map.  Note: starts at Base Drum (35) and finishes at Low Timbale (66).
// Anything outside this range is ignored.
//
// Drum numbers are assumed to be 0-indexed MIDI note numbers.
// However, if 1-indexed MIDI note numbers are required, then
// uncomment this line!
//#define GM_MIDI_1_INDEX_DRUMS 1
#define MIDI_DRUM_START 35   // Bass Drum = Idx 0
#define NUM_DRUMS 32         // Any before or after are not supported
const uint8_t drums[NUM_DRUMS] = {
/* 35-40 */  BD, BD, NO, SD, NO, SD,
/* 41-50 */  TT, HH, TT, HH, TT, HH, TT, TT, CY, TT,
/* 51-60 */  CY, CY, NO, NO, CY, NO, CY, NO, SD, SD,
/* 61-66 */  SD, SD, SD, SD, SD, SD
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

#ifdef GM_MIDI_1_INDEX_DRUMS
  // Convert from 0-indxed "on wire" drums to 1-indexed drums
  pitch++;
#endif

  if ((pitch < MIDI_DRUM_START) || (pitch >= MIDI_DRUM_START+NUM_DRUMS)) {
    // The note is out of range for us so do nothing
    return;
  }

  // Note: have to convert the MIDI Drum note to a YM2413 Drum channel to play it
  uint8_t drum = drums[pitch-MIDI_DRUM_START];
  if (drum != NO) {
    ledOn();
    SYNTH.noteOn(drum, MD_YM2413::MIN_OCTAVE, 0, MD_YM2413::VOL_MAX, 0);
    playing[DRUM2IDX(drum)] = pitch;
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
#ifdef GM_MIDI_1_INDEX_DRUMS
  // Convert from 0-indxed "on wire" drums to 1-indexed drums
  pitch++;
#endif

  if ((pitch < MIDI_DRUM_START) || (pitch >= MIDI_DRUM_START+NUM_DRUMS)) {
    // The note is out of range for us so do nothing
    return;
  }

  // Note: have to convert the MIDI Drum note to a YM2413 Drum channel to play it
  uint8_t drum = drums[pitch-MIDI_DRUM_START];
  if ((drum != NO) && (playing[DRUM2IDX(drum)] == pitch)) {
    SYNTH.noteOff(drum);
    playing[DRUM2IDX(drum)] = NONOTE;

    // NB: LED handling isn't accurate, but it is just an indication of activity
    ledOff();
  }
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
  SYNTH.setPercussion(true);
  for (int i=0; i<YMCHANNELS; i++) {
    SYNTH.setVolume(IDX2DRUM(i), MD_YM2413::VOL_MAX);
  }

  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to the specified MIDI channel
  MIDI.begin(MIDI_CHANNEL);
}

void loop(void)
{
  MIDI.read();
  SYNTH.run();
}
