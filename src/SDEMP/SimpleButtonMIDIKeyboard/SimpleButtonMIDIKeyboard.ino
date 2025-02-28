/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Simple Button MIDI Keyboard
//  https://diyelectromusic.com/2025/02/28/simple-midi-touch-button-keyboard/
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
  Using principles from the following Arduino tutorials:
    Button               - http://www.arduino.cc/en/Tutorial/Button
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library

*/
#include <MIDI.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1  // Define which MIDI channel to transmit on (1 to 16).

// Set up the input and output pins to be used for the keys.
// NB: On the Arduino Uno/Nano, pin 13 is attached to the onboard LED
//     and pins 0 and 1 are used for the serial port.
#define NUM_KEYS 4
int keys[] = {
  2, 3, 4, 5
};

// Set the MIDI codes for the notes to be played by each key
int notes[NUM_KEYS] = {
  60, 62, 64, 65
};

int lastbtn[NUM_KEYS];

void setup() {
  // put your setup code here, to run once:
  MIDI.begin(MIDI_CHANNEL_OFF);

  // Initialise the input buttons as piano "keys"
  for (int k = 0; k < NUM_KEYS; k++) {
    pinMode (keys[k], INPUT);
    lastbtn[k] = LOW;
  }
}

void loop() {
  // Check each button and if pressed play the note.
  for (int k = 0; k < NUM_KEYS; k++) {
    int buttonState = digitalRead (keys[k]);
    if (buttonState == HIGH && lastbtn[k] == LOW) {
      // Low->High means key just pressed
      MIDI.sendNoteOn(notes[k], 127, MIDI_CHANNEL);
    } else if (buttonState == LOW && lastbtn[k] == HIGH) {
      // High->Low means key just released
      MIDI.sendNoteOff(notes[k], 0, MIDI_CHANNEL);
    }

    lastbtn[k] = buttonState;
  }
}
