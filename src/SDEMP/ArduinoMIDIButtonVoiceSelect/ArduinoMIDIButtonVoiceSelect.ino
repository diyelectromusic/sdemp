/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Keypad Button Voices
//  https://diyelectromusic.wordpress.com/2022/05/17/arduino-midi-button-voice-select/
//
      MIT License
      
      Copyright (c) 2022 diyelectromusic (Kevin)
      
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
    Arduino Button          - https://www.arduino.cc/en/Tutorial/BuiltInExamples/Button
    Arduino USB Transport   - https://github.com/lathoub/Arduino-USBMIDI
*/
#include <MIDI.h>
#include <Keypad.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno/Nano and
// the USB serial port if using a Leonardo/Pro Micro.
MIDI_CREATE_DEFAULT_INSTANCE();

// Uncomment this to include MIDI USB device support.
// This is only available on certain boards (e.g. ATmega32U4)
//#define MIDI_USB_DEV
#ifdef MIDI_USB_DEV
#include <USB-MIDI.h>
USBMIDI_CREATE_INSTANCE(0, MIDI_UD);
#endif

#define MIDI_CHANNEL 1    // Define which MIDI channel to transmit on (1 to 16).

#define NUM_BANKS 8
#define NUM_VOICES 32
int lastbtns[NUM_BANKS+NUM_VOICES];

int ccs[NUM_BANKS][2] = {
  // MSB, LSB
  {    0,   0},
  {    0,   1},
  {    0,   2},
  {    0,   3},
  {    0,   4},
  {    0,   5},
  {    0,   6},
  {    0,   7},
};

// Bank Select related MIDI Control Change message values
#define BANKSELMSB 0
#define BANKSELLSB 32

const byte ROWS = 5; // five keypads
const byte COLS = 8; // eight buttons per keypad

char hexaKeys[ROWS][COLS] = {
  // Order of buttons:
  //    Top Row: 7, 5, 3, 1
  // Bottom Row: 8, 6, 4, 2
  //
  // First keypad = Bank Select
  //
  // Then keypads are in order
  //
  {36, 40, 35, 39, 34, 38, 33, 37},
  { 4, 20,  3, 19,  2, 18,  1, 17},
  { 8, 24,  7, 23,  6, 22,  5, 21},
  {12, 28, 11, 27, 10, 26,  9, 25},
  {16, 32, 15, 31, 14, 30, 13, 29},
};
byte rowPins[ROWS] = {A1, A2, A3, A4, A5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {2,3,4,5,6,7,8,9}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad kp = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

void setup() {
  MIDI.begin(MIDI_CHANNEL);
#ifdef MIDI_USB_DEV
  MIDI_UD.begin(MIDI_CHANNEL);
#endif
}

void loop() {
  char key = kp.getKey();
  if (key) {
    if (key <= NUM_VOICES) {
      // Voice/Patch change
      midiPatchChange (key - 1);
    } else if (key <= (NUM_VOICES+NUM_BANKS)) {
      // Bank select
      midiBankSelect(key - NUM_VOICES - 1);
    }
  }
}

void midiPatchChange (int program) {
  MIDI.sendProgramChange (program, MIDI_CHANNEL);
#ifdef MIDI_USB_DEV
  MIDI_UD.sendProgramChange (program, MIDI_CHANNEL);
#endif
}

void midiBankSelect (int bank) {
  MIDI.sendControlChange (BANKSELMSB, ccs[bank][0], MIDI_CHANNEL);
  MIDI.sendControlChange (BANKSELLSB, ccs[bank][1], MIDI_CHANNEL);
#ifdef MIDI_USB_DEV
  MIDI_UD.sendControlChange (BANKSELMSB, ccs[bank][0], MIDI_CHANNEL);
  MIDI_UD.sendControlChange (BANKSELLSB, ccs[bank][1], MIDI_CHANNEL);
#endif
}
