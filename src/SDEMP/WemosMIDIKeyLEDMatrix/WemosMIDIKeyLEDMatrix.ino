/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Wemos MIDI Key LED Matrix
//  https://diyelectromusic.wordpress.com/2020/06/13/wemos-midi-led-matrix/
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
    Arduino MIDI Library    - https://github.com/FortySevenEffects/arduino_midi_library
    Wemos Matrix LED Shield - https://docs.wemos.cc/en/latest/d1_mini_shiled/matrix_led.html

*/
#include <MIDI.h>
#include <WEMOS_Matrix_LED.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port
// which is pin 1 for transmitting on the Wemos D1 Mini.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set up the LED Matrix library
MLED matrix(3);  //set intensity=7 (maximum)

// Set up the MIDI codes to respond to by listing the lowest note
#define MIDI_NOTE_START 36   // C2
#define MIDI_NOTE_END   (MIDI_NOTE_START+64) // E7

// These are the functions to be called on recieving certain MIDI events.
// Full documentation of how to do this is provided here:
// https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-Callbacks

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
  }

  if ((pitch < MIDI_NOTE_START) || (pitch > MIDI_NOTE_END)) {
    // The note is out of range for us so do nothing
    return;
  }

  int note = pitch - MIDI_NOTE_START;

  // "draw" the dot corresponding to the note
  //
  // As this is a 8x8 matrix, we want every eight
  // notes to be a new line in the matrix.
  //
  // We can therefore get the line number by
  // takeing the note and dividing by 8 (which is
  // the same as dividing by 2, 3 times, so we can
  // do a bit shift to the right by 3 to get the answer).
  //
  // The remainder is the column number, which
  // we can get by pulling out the bottom three bits
  // of the value - we can do by "AND"ing with the
  // value 7 (or 0b00000111 in binary).
  //
  matrix.dot ((note&0x07), (note>>3));
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  if ((pitch < MIDI_NOTE_START) || (pitch > MIDI_NOTE_END)) {
    // The note is out of range for us so do nothing
    return;
  }

  int note = pitch - MIDI_NOTE_START;

  // clear the dot corresponding to the note
  matrix.dot ((note&0x07), (note>>3), 0);
}

void setup() {
  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to all MIDI channels
  MIDI.begin(MIDI_CHANNEL_OMNI);

}

void loop() {
  // All the loop has to do is call the MIDI read function
  MIDI.read();

  // and update the display
  matrix.display();
}
