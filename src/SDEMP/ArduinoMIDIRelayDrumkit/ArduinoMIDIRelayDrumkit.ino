/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Relay Drumkit
//  https://diyelectromusic.wordpress.com/2020/06/14/arduino-midi-relay-drumkit/
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

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set the relay pins to be played for each drum.
// And set up a list to remember the state of each drum.
int drums[] = {
  2, 3, 4, 5, 6, 7, 8
};
int drumstates[] = {
  LOW, LOW, LOW, LOW, LOW, LOW, LOW
};
int numdrums;
int numdrumstates;

// Set the MIDI notes that correspond to each drum
// Use the "General MIDI" mappings - https://www.midi.org/specifications-old/item/gm-level-1-sound-set
//
// As there is some duplication in the GM specification
// compared the number number drums we have, this allows
// for each our our drums to respond to up to three different
// MIDI drums.
//
// If there are less than three, then "-1" means 'ignore'.
//
int mididrums[] = {
  35,  // Acoustic bass drum
  36,  // Bass drum 1
  33,  // Bass drum soft (Yamaha) 
  
  31,  // Snare H soft (Yamaha)
  38,  // Acoustic snare
  40,  // Electric snare
  
  41,  // Low floor tom
  43,  // High floor tom
  45,  // Low tom

  42,  // Closed hi-hat
  44,  // Pedal hi-hat
  46,  // Open hi-hat

  47,  // Low-mid tom
  48,  // Hi-mid tom
  50,  // Hi tom

  49,  // Crash cymbal 1
  55,  // Splash cymbal
  57,  // Crash cymbal 2

  51,  // Ride cymbal 1
  53,  // Ride bell
  59,  // Ride cymbal 2
};
int nummididrums;

// Drum kits listen on channel 10
#define MIDI_DRUM_CHANNEL 10

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

  for (int i=0; i<nummididrums; i++) {
    if (pitch == mididrums[i]) {
      // Play this drum.
      //
      // As there are up to three MIDI drums for every real
      // drum we play, the drum to play = midi drum/3
      // This will automatically round down
      int drum = i/3;

      // We don't mind if the relay is opening or closing - 
      // we just want a change to make a sound.
      //
      // So for this code, we just toggle the state of
      // of the relay, which happens here.
      drumstates[drum] = !drumstates[drum];

      // Then set the relay to this state
      digitalWrite (drums[drum], drumstates[drum]);
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  // As this is a single "click" for each note, we don't actually
  // have any concept of "note off", so just ignore these messages.
}

void setup() {
  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to all MIDI channels
  MIDI.begin(MIDI_DRUM_CHANNEL);

  numdrums = sizeof(drums)/sizeof(drums[0]);
  numdrumstates = sizeof(drumstates)/sizeof(drumstates[0]);
  nummididrums = sizeof(mididrums)/sizeof(mididrums[0]);
  
  // A programming trick to pick the lowest of two numbers.
  // This ensures that we will always have an equivalent number
  // of drum state values for drums specified.
  numdrums = (numdrums < numdrumstates) ? numdrums : numdrumstates;
  if (nummididrums < numdrums*3) {
    // make sure we don't have more MIDI drums than actual drums
    nummididrums = numdrums * 3;
  }

  // Initialise our relay drums
  for (int i=0; i<numdrums; i++) {
    pinMode (drums[i], OUTPUT);
    digitalWrite (drums[i], drumstates[i]);
  }
}

void loop() {
  // All the loop has to do is call the MIDI read function
  MIDI.read();
}