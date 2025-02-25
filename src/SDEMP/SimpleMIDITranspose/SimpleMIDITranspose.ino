/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Simple MIDI Transpose
//  https://diyelectromusic.com/2025/02/25/simple-midi-transpose/
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
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
*/
#include <MIDI.h>

#define TRANSPOSE -12 // Number of semitones to transpose.  Negative numbers to go down.
#define MIDI_CHANNEL MIDI_CHANNEL_OMNI // 1 to 16 or MIDI_CHANNEL_OMNI

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// For special, or alternative, cases, use:
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

#define MIDI_LED LED_BUILTIN

void setup() {
  // This listens to all MIDI channels
  MIDI.begin(MIDI_CHANNEL);
  MIDI.turnThruOff(); // Disable automatic MIDI Thru function

  // Use the built-in LED as a MIDI note indicator
  pinMode (MIDI_LED, OUTPUT);
}

void loop() {
  if (MIDI.read()) {
    byte pitch;
    switch(MIDI.getType()) {
      case midi::NoteOn:
        digitalWrite (MIDI_LED, HIGH);
        pitch = MIDI.getData1();
        if (((TRANSPOSE < 0) && (pitch >= -TRANSPOSE)) ||
            ((TRANSPOSE > 0) && (pitch <= 127-TRANSPOSE))  ||
            (TRANSPOSE == 0)) {
          pitch += TRANSPOSE;
          MIDI.send(MIDI.getType(), pitch, MIDI.getData2(), MIDI.getChannel());
        }
        break;

      case midi::NoteOff:
        digitalWrite (MIDI_LED, LOW);
        pitch = MIDI.getData1();
        if (((TRANSPOSE < 0) && (pitch >= -TRANSPOSE)) ||
            ((TRANSPOSE > 0) && (pitch <= 127-TRANSPOSE))  ||
            (TRANSPOSE == 0)) {
          pitch += TRANSPOSE;
          MIDI.send(MIDI.getType(), pitch, MIDI.getData2(), MIDI.getChannel());
        }
        break;

      default:
        MIDI.send(MIDI.getType(), MIDI.getData1(), MIDI.getData2(), MIDI.getChannel());
        break;
    }
  }
}
