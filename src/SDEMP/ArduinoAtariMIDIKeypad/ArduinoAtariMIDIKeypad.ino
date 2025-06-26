/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Atari MIDI Keypads
//  https://diyelectromusic.com/2025/06/26/arduini-atari-midi-keypad/
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
    Arduino Atari PCB - https://diyelectromusic.com/2025/05/22/atari-2600-controller-shield-pcb-build-guide/
*/
#include <Keypad.h>
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 1  // MIDI Channel number (1 to 16, or MIDI_CHANNEL_OMNI)

#define NUMNOTES 24
uint8_t notes[NUMNOTES] = {
  84, 88, 89, 91, 93, 94,  // C6...
  72, 76, 77, 79, 81, 82,  // C5, E5, F5, G5, A5, A#5
  60, 64, 65, 67, 69, 70,  // C4...
  48, 52, 53, 55, 57, 58,  // C3...
//  36, 40, 41, 43, 45, 46,  // C2...
};
bool playing[NUMNOTES];

// For Atari, the COLs have hardware pull-ups.
// See: https://atariage.com/2600/archives/schematics/Schematic_2600_Accessories_Low.html
// But in Keypad Lib, it is ROWS that are inputs with
// pull-ups so need to swap ROWS/COLS otherwise get issues.
//
// This works best for an Atari controller circuit that
// doesn't include the RC timing circuit of the original
// console, for scanning paddles.
//
// If the RC circuit is included, the the non-reversed
// version works best, but certain two-key combinations
// will not be scanned properly.
//
// Uncomment this to swap ROWS/COLS on the Keypad
#define REVERSE_KEYPAD

#ifdef REVERSE_KEYPAD
const byte ROWS = 3;
const byte COLS = 4;
char keys1[ROWS][COLS] = {
  { 1, 7,13,19},
  { 2, 8,14,20},
  { 3, 9,15,21},
};
byte rowPins1[ROWS] = {A1,A0,12}; //connect to the row pinouts of the keypad
byte colPins1[COLS] = {11,10,9,8}; //connect to the column pinouts of the keypad

// Port 2
char keys2[ROWS][COLS] = {
  { 4,10,16,22},
  { 5,11,17,23},
  { 6,12,18,24},
};
byte rowPins2[ROWS] = {A3,A2,2}; //connect to the row pinouts of the keypad
byte colPins2[COLS] = {6,5,4,3}; //connect to the column pinouts of the keypad

#else

const byte ROWS = 4;
const byte COLS = 3;
char keys1[ROWS][COLS] = {
  { 1, 2, 3},
  { 7, 8, 9},
  {13,14,15},
  {19,20,21}
};
byte rowPins1[ROWS] = {11,10,9,8}; //connect to the row pinouts of the keypad
byte colPins1[COLS] = {A1,A0,12}; //connect to the column pinouts of the keypad

// Port 2
char keys2[ROWS][COLS] = {
  { 4, 5, 6},
  {10,11,12},
  {16,17,18},
  {22,23,24}
};
byte rowPins2[ROWS] = {6,5,4,3}; //connect to the row pinouts of the keypad
byte colPins2[COLS] = {A3,A2,2}; //connect to the column pinouts of the keypad
#endif

//initialize an instance of class NewKeypad
Keypad kp1 = Keypad( makeKeymap(keys1), rowPins1, colPins1, ROWS, COLS);
Keypad kp2 = Keypad( makeKeymap(keys2), rowPins2, colPins2, ROWS, COLS);

void midiNoteOn (int note) {
  // note is the value from the keypads: 1..24
  int idx = note-1;
  uint8_t midiNote = notes[idx];
  if (!playing[idx]) {
    // Start playing the note
    MIDI.sendNoteOn(midiNote, 127, MIDI_CHANNEL);
    playing[idx] = true;
  }
}

void midiNoteOff (int note) {
  // note is the value from the keypads: 1..24
  int idx = note-1;
  uint8_t midiNote = notes[idx];
  if (playing[idx]) {
    // Stop playing the note
    MIDI.sendNoteOff(midiNote, 0, MIDI_CHANNEL);
    playing[idx] = false;
  }
}

void setup(){
  MIDI.begin(MIDI_CHANNEL);
  for (int i=0; i<NUMNOTES; i++) {
    playing[i] = false;
  }
}
  
void loop(){
  MIDI.read();

  if (kp1.getKeys()) {
    for (int i=0; i<LIST_MAX; i++) {
      if (kp1.key[i].stateChanged) {
        switch (kp1.key[i].kstate) {
          case PRESSED:
            midiNoteOn(kp1.key[i].kchar);
            break;
          case RELEASED:
            midiNoteOff(kp1.key[i].kchar);
            break;
        }
      }
    }
  }

  if (kp2.getKeys()) {
    for (int i=0; i<LIST_MAX; i++) {
      if (kp2.key[i].stateChanged) {
        switch (kp2.key[i].kstate) {
          case PRESSED:
            midiNoteOn(kp2.key[i].kchar);
            break;
          case RELEASED:
            midiNoteOff(kp2.key[i].kchar);
            break;
        }
      }
    }
  }
}
