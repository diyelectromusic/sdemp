/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  LOL Shield MIDI Waterfall
//  https://diyelectromusic.wordpress.com/2020/07/06/lolshield-midi-waterfall-display/
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
    LOL Shield Examples  - https://github.com/jprodgers/LoLshield

*/
#include <MIDI.h>
#include <Charliplexing.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port
// which is pin 1 for transmitting on the Wemos D1 Mini.
MIDI_CREATE_DEFAULT_INSTANCE();

// Size of the LolShield display
#define LOL_MAX_H 14
#define LOL_MAX_V 9
bool notes[LOL_MAX_H];

// Set up the MIDI codes to respond to by listing the lowest note
//#define MIDI_NOTE_START 46  // A2#
#define MIDI_NOTE_START 60  // C4
//#define MIDI_NOTE_START 74  // D5
#define MIDI_NOTE_END   (MIDI_NOTE_START + LOL_MAX_H - 1)

// Set up how quickly we want the scroll to run
#define SCROLL_DELAY 10000
int scrollcount;

bool leddisplay[LOL_MAX_H][LOL_MAX_V];

// These are the functions to be called on recieving certain MIDI events.
// Full documentation of how to do this is provided here:
// https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-Callbacks

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  if ((pitch < MIDI_NOTE_START) || (pitch > MIDI_NOTE_END)) {
    // The note is out of range for us so do nothing
    return;
  }

  int note = pitch - MIDI_NOTE_START;

  // "draw" the dot corresponding to the note
  //
  notes[note] = HIGH;
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  if ((pitch < MIDI_NOTE_START) || (pitch > MIDI_NOTE_END)) {
    // The note is out of range for us so do nothing
    return;
  }

  int note = pitch - MIDI_NOTE_START;

  // clear the dot corresponding to the note
  //
  notes[note] = LOW;
}

void setup() {
  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to all MIDI channels
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Initialise the LolShield too allow us to 
  // use shades (i.e. varying the brightness)
  // starting with full brightness (127).
  //
  LedSign::Init(GRAYSCALE);
  LedSign::SetBrightness(127);

  // Set up our display, starting with a single line at the bottom
  for (int v=0; v<LOL_MAX_V; v++) {
    for (int h=0; h<LOL_MAX_H; h++) {
      if (v == 0) {
        leddisplay[h][v] = HIGH;
        notes[h] = LOW;
      }
      else {
        leddisplay[h][v] = LOW;
      }
    }
  }
  updateDisplay();

  scrollcount = 0;
}

void loop() {
  // All the loop has to do is call the MIDI read function
  MIDI.read();

  // Scroll the display
  scrollcount++;
  if (scrollcount > SCROLL_DELAY) {
    scrollcount = 0;
    
    // For each row in the display, we replace it with the one
    // beneath it, starting from the top, stopping at the last
    // but one row.
    //
    // While we are updating the leddisplay, also actually draw
    // each line out to the real display afterwards.
    //
    for (int v=LOL_MAX_V-1; v>=1 ; v--) {
      for (int h=0; h<LOL_MAX_H; h++) {
        leddisplay[h][v] = leddisplay[h][v-1];
      }
    }
    // Set the bottom row to whatever the notes are telling us need to be set
    for (int h=0; h<LOL_MAX_H; h++) {
      leddisplay[h][0] = notes[h];
    }

    // Finally update the status of the LEDs
    updateDisplay();
  }
}

void updateDisplay () {
  for (int v=0; v<LOL_MAX_V ; v++) {
    for (int h=0; h<LOL_MAX_H; h++) {
      if (leddisplay[h][v] == HIGH) {
        // Set the LED
        //
        // The lower the row, the brighter we want it to be.
        // Brightness goes from 0 (off) to 7 (bright)
        // so we use the formula:
        //    brightness = 9 - row
        //
        // It doesn't matter that our first couple of rows
        // are "over brightness" in value, it seems to cope fine.
        //
        // The shield itself has coordinate (0,0) at the top
        // left, so we need to reverse the order of the rows,
        // so when we ask for row 0, we want LOL_MAX_V-1.
        //
        LedSign::Set(h, LOL_MAX_V-1-v, (9-v));
      }
      else {
        // Clear the LED
        LedSign::Set(h, LOL_MAX_V-1-v, 0);
      }
    }
  }
}
