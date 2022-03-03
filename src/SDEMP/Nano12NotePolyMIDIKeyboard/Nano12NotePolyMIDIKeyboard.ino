/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Nano 12 Note Poly MIDI Keyboard
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

// Uncomment the following to use "INPUT_PULLUPS" internally
// rather than external resistors connected to GND.
//
// This reverses the logic, so that "on" reads as LOW rather than HIGH.
//
//#define PULLUPINPUTS 1

#define SPEAKER  12
// Set up the input and output pins to be used for the keys.
// NB: On the Arduino Nano, pin 13 is attached to the onboard LED
//     and pins 0 and 1 are used for the serial port.
//     We use all free digital pins for keys and two of the analogue pins.
//
// For the MIDI piano, no speaker is required.
//
#define NUM_NOTES 13
int keys[NUM_NOTES] = {
  19, 18, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13
};

// Set the MIDI codes for the notes to be played by each key
int notes[NUM_NOTES] = {
  60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
};

bool playing[NUM_NOTES];

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

  // Initialise the input buttons as piano "keys"
  for (int k = 0; k < NUM_NOTES; k++) {
#ifdef PULLUPINPUTS
    pinMode (keys[k], INPUT_PULLUP);
#else
    pinMode (keys[k], INPUT);
#endif
    // Initialise all keys to "not playing"
    playing[k] = false;
  }
}

void loop() {
  // Check each button and if pressed play the note.
  for (int k = 0; k < NUM_NOTES; k++) {
    int buttonState = digitalRead (keys[k]);
    bool keyplaying = false;
#ifdef PULLUPINPUTS
    // LOW means "pressed" for INPUT_PULLUP mode
    if (buttonState == LOW)
#else
    // HIGH means "pressed" if using external resistors to GND
    if (buttonState == HIGH)
#endif
    {
      keyplaying = true;
    }

    if (keyplaying && !playing[k]) {
      // not-playing to playing transition
      MIDI.sendNoteOn(notes[k], 127, midiChannel);
      playing[k] = true;
    } else if (!keyplaying && playing[k]) {
      // playing to not-playing transition
      MIDI.sendNoteOff(notes[k], 0, midiChannel);
      playing[k] = false;      
    } else {
      // Ignore anything that isn't changing
    }
  }
}
