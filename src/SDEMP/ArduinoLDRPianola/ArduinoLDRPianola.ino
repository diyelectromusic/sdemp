/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino LDR Pianola
//  https://diyelectromusic.wordpress.com/2020/06/06/arduino-ldr-pianola/
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
    Arduino LDR          - https://www.tweaking4all.com/hardware/arduino/arduino-light-sensitive-resistor/
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
*/
#include <MIDI.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

int midiChannel = 1;  // Define which MIDI channel to transmit on (1 to 16).

// Set up the input and output pins to be used for the keys.
//
// First test uses just two keys for now.
//
int keys[] = {
  2, 3, 
};

// There needs to be at least one of these for each key above
int lastkeys[] = {
  LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW,
};

// Optional, if you list a pin here it will be used as an LED pin
// to indicate a note "on" or playing.
//
// Pin 13 is connected to the on-board LED for the Arduino Uno,
// so is useful for testing.
//
// Set to -1 to disable LED for that key.
int leds[] = {
  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

// Set the MIDI codes for the notes to be played by each key
int notes[] = {
//  60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
   60, 64,
};

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
//  Serial.begin (115200);

  int numkeys = sizeof(keys)/sizeof(keys[0]);     // Programming trick to get the number of keys
  // Initialise the input buttons as piano "keys"
  for (int k = 0; k < numkeys; k++) {
    pinMode (keys[k], INPUT);
    if (leds[k] != -1) {
      pinMode (leds[k], OUTPUT);
      lastkeys[k] = HIGH;
    }
  }
}

void loop() {
  // Read the LDR "pin" for each note and if it changes dark to light
  // then turn on the note and if it changes light to dark, turn
  // it off.
  
  int numkeys = sizeof(keys)/sizeof(keys[0]);
  for (int k = 0; k < numkeys; k++) {

    // Note that ideally, we want a dark part to play a note
    // and a light part to stop a note.  This allows us to
    // define the "pianola roll" by colouring black where
    // we want notes to play.
    //
    int ldrState = digitalRead (keys[k]);
    if ((ldrState == LOW) && (lastkeys[k] == HIGH)) {
      if (leds[k] != -1) {
        digitalWrite (leds[k], HIGH);
      }
      MIDI.sendNoteOn(notes[k], 127, midiChannel);
      lastkeys[k] = ldrState;
    } else if ((ldrState == HIGH) && (lastkeys[k] == LOW)) {
      if (leds[k] != -1) {
        digitalWrite (leds[k], LOW);
      }
      MIDI.sendNoteOff(notes[k], 0, midiChannel);
      lastkeys[k] = ldrState;
    } else {
      // Nothing to do - either still high or still low
    }
  }
}
