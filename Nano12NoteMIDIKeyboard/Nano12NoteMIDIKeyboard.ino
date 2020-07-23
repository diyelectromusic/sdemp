/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Nano 12 Note MIDI Keyboard
//  https://diyelectromusic.wordpress.com/2020/06/05/arduino-nano-midi-keyboard/
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
    Button               - http://www.arduino.cc/en/Tutorial/Button
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library

*/
#include <MIDI.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

int midiChannel = 1;  // Define which MIDI channel to transmit on (1 to 16).

// Set up the input and output pins to be used for the keys.
// NB: On the Arduino Nano, pin 13 is attached to the onboard LED
//     and pins 0 and 1 are used for the serial port.
//     We use all free digital pins for keys and two of the analogue pins.
//
// For the MIDI piano, no speaker is required.
//
#define SPEAKER  12
int keys[] = {
  19, 18, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13
};

// Set the MIDI codes for the notes to be played by each key
int notes[] = {
  60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
};

int lastKey;

void setup() {
  // put your setup code here, to run once:
  MIDI.begin(MIDI_CHANNEL_OFF);

  // This updates speed of the serial channel so that it can be used
  // with a PC rather than a MIDI port.
  //
  // This can be used with software such as the Hairless MIDI Bridge
  // and loopMidi during testing.
  //
  // It is commented out for "real" use.
  //
  //Serial.begin (115200);

  int numkeys = sizeof(keys)/sizeof(keys[0]);     // Programming trick to get the number of keys
  // Initialise the input buttons as piano "keys"
  for (int k = 0; k < numkeys; k++) {
    pinMode (keys[k], INPUT);
  }

  lastKey = -1;
}

void loop() {
  // Check each button and if pressed play the note.
  // If no buttons are pressed, turn off all notes.
  // Note - the last button to be checked will be
  // the note that ends up being played.
  int numkeys = sizeof(keys)/sizeof(keys[0]);
  int playing = -1;
  for (int k = 0; k < numkeys; k++) {
    int buttonState = digitalRead (keys[k]);
    if (buttonState == HIGH) {
      playing = k;
    }
  }

  if (playing != -1) {
    // Only want go send a MIDI message if something has changed
    if (playing != lastKey){
      if (lastKey != -1) {
        MIDI.sendNoteOff(notes[lastKey], 0, midiChannel);
      }
      MIDI.sendNoteOn(notes[playing], 127, midiChannel);
      lastKey = playing;
    }
  } else {
    // If not buttons pressed then turn off the note if necessary
    if (lastKey != -1) {
      MIDI.sendNoteOff(notes[lastKey], 0, midiChannel);
      lastKey = -1;
    }
  }
}
