/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  MIDI In-Out Half-Shield
//  https://diyelectromusic.wordpress.com/2020/08/08/midi-in-out-half-shield/
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

*/
#include <MIDI.h>

#define MIDI_CHANNEL 1

//#define MIDI_OFFSET 12   // One octave higher
#define MIDI_OFFSET 7    // Fifth higher
//#define MIDI_OFFSET -12  // Octave lower

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// These are the functions to be called on recieving certain MIDI events.
// Full documentation of how to do this is provided here:
// https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-Callbacks

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  // Pass on the MIDI code for the note and a new note
  MIDI.sendNoteOn (pitch, velocity, channel);
  MIDI.sendNoteOn (pitch+MIDI_OFFSET, velocity, channel);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  // Pass on the MIDI code for the note and a new note
  MIDI.sendNoteOff (pitch, velocity, channel);
  MIDI.sendNoteOff (pitch+MIDI_OFFSET, velocity, channel);
}

void setup() {
  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to the specified MIDI channel
  MIDI.begin(MIDI_CHANNEL);
  MIDI.turnThruOff();  // Disable automatic MIDI Thru function

}

void loop() {
  // All the loop has to do is call the MIDI read function
  MIDI.read();
}
