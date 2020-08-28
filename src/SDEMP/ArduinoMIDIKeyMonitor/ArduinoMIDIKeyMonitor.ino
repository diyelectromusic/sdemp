/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Key Monitor
//  https://diyelectromusic.wordpress.com/2020/06/12/arduino-midi-piano/
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
    Arduino Blink        - https://www.arduino.cc/en/Tutorial/Blink

*/
#include <MIDI.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set up the MIDI codes to respond to by listing the lowest note
#define MIDI_NOTE_START 60

// Set the pins for the LED indications for each note
// Setting it to -1 means "skip" that note
int notes[] = {
  2, -1, 3, -1, 4, 5, -1, 6, -1, 7, -1, 8, 9,
};
int numnotes;

// These are the functions to be called on recieving certain MIDI events.
// Full documentation of how to do this is provided here:
// https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-Callbacks

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  if ((pitch < MIDI_NOTE_START) || (pitch >= MIDI_NOTE_START+numnotes)) {
    // The note is out of range for us so do nothing
    return;
  }

  int note = pitch - MIDI_NOTE_START;

  if (notes[note] != -1) {
    digitalWrite (notes[note], HIGH);
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  if ((pitch < MIDI_NOTE_START) || (pitch >= MIDI_NOTE_START+numnotes)) {
    // The note is out of range for us so do nothing
    return;
  }

  int note = pitch - MIDI_NOTE_START;

  if (notes[note] != -1) {
    digitalWrite (notes[note], LOW);
  }
}

void setup() {
  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to all MIDI channels
  MIDI.begin(MIDI_CHANNEL_OMNI);

  numnotes = sizeof(notes)/sizeof(notes[0]);

  for (int i=0; i<numnotes; i++) {
    if (notes[i] != -1) {
      pinMode (notes[i], OUTPUT);
      digitalWrite(notes[i], HIGH);
      delay(100);
      digitalWrite(notes[i], LOW);
    }
  }
}

void loop() {
  // All the loop has to do is call the MIDI read function
  MIDI.read();
}
