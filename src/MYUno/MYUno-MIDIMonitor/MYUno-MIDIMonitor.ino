/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Make Your Uno Synth - MIDI Monitor
//  https://diyelectromusic.wordpress.com/2023/01/05/arduino-make-your-uno-synth/
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

*/
#include <MIDI.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set up the MIDI channel to listen on.
// 1 to 16 or MIDI_CHANNEL_OMNI to listen on all.
#define MIDI_CHANNEL MIDI_CHANNEL_OMNI

#define MIDI_LED LED_BUILTIN

void ledOn() {
  digitalWrite (MIDI_LED, HIGH);  
}

void ledOff() {
  digitalWrite (MIDI_LED, LOW);
}

void signalMidiActivity () {
  ledOn();
  delay(100);
  ledOff();
}

void setup() {
  MIDI.begin(MIDI_CHANNEL);

  // Use the LED as a MIDI indicator
  pinMode (MIDI_LED, OUTPUT);
}

void loop() {
  if (MIDI.read()) {
    switch(MIDI.getType()) {
      // Add specific cases to filter on specific MIDI messages.
      // or specific clauses to drive the activity.
      case midi::NoteOn:
        ledOn();
        break;

      case midi::NoteOff:
        ledOff();
        break;

      case midi::ActiveSensing:
        // Ignore active sensing messages
        break;
 
      default:
        // Comment this out if you only want to signal specific activity
        signalMidiActivity();
        break;
    }
  }
}
