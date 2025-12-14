/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Atari Synth Cart MIDI Controller
//  https://diyelectromusic.com/2025/12/13/atari-synth-cart-controller/
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
#include <MIDI.h>
#include <TimerOne.h>
#include <Keypad.h>

MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 1

// For the 2600, the ROWS are outputs that are scanned
// in turn, and the COLS are inputs that are tested for
// each ROW scan.
//
// To emulate a controller, the ROWS needs to be
// inputs and the COLS outputs.
//
// Need to wait for ROW to be active then assert
// the appropriate COL corresponding to the required key.
//
#define COLS 3
#define ROWS 4
int colPins[COLS] = {A1, A0, 12};
int rowPins[ROWS] = {11, 10, 9, 8};

// Note: Above are hard-coded in the scanning
//       routine to use PORT IO for performance.
//
//       Each bit has an entry in the following
//       array which gets written directly out.
//
// There are two copies - one for the MIDI
// input and one for any additionally attached
// keypad in passthrough
//
uint8_t row[2][ROWS];
uint8_t colbits[COLS] = {
  0x02,  // C0 = A1  PORTC
  0x01,  // C1 = A0  PORTC
  0x10   // C2 = D12 PORTB
};

void keyOn (int kp, int r, int c) {
  if (r < ROWS && c < COLS && kp < 2) {
    // Clear the bit as need active LOW
    row[kp][r] &= (~colbits[c]);
  }
}

void keyOff (int kp, int r, int c) {
  if (r < ROWS && c < COLS && kp < 2) {
    // Set the bit as need active LOW
    row[kp][r] |= colbits[c];
  }
}

// Define key presses in terms of index into key[] array
char keys[ROWS][COLS] = {
  {60,61,62},
  {63,64,65},
  {66,67,68},
  {69,70,71},
};

// Set up a Keypad routine to handle passthrough from
// any additional plugged in keypad.
//
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
// This code is assuming the reversed format.
// Note: MIDI values here must match those in the
//       main keypad map, but with transposed r/c.
//
const byte KPROWS = 3;
const byte KPCOLS = 4;
char kpkeys[KPROWS][KPCOLS] = {
  {60,63,66,69},
  {61,64,67,70},
  {62,65,68,71},
};
byte kpRowPins[KPROWS] = {A3,A2,2};
byte kpColPins[KPCOLS] = {6,5,4,3};
Keypad kp = Keypad( makeKeymap(kpkeys), kpRowPins, kpColPins, KPROWS, KPCOLS);

void midiNoteOn(byte channel, byte pitch, byte velocity) {
  if (velocity == 0) {
    // Handle this as a "note off" event
    midiNoteOff(channel, pitch, velocity);
    return;
  }

  ledOn();

  for (int r=0; r<ROWS; r++) {
    for (int c=0; c<COLS; c++) {
      if (keys[r][c] == pitch) {
        keyOn(0, r, c);
      }
    }
  }
}

void keypadNoteOn(byte pitch) {
  ledOn();
  for (int r=0; r<ROWS; r++) {
    for (int c=0; c<COLS; c++) {
      if (keys[r][c] == pitch) {
        keyOn(1, r, c);
      }
    }
  }
}

void midiNoteOff(byte channel, byte pitch, byte velocity) {
  ledOff();
  for (int r=0; r<ROWS; r++) {
    for (int c=0; c<COLS; c++) {
      if (keys[r][c] == pitch) {
        keyOff(0, r, c);
      }
    }
  }
}

void keypadNoteOff(byte pitch) {
  ledOff();
  for (int r=0; r<ROWS; r++) {
    for (int c=0; c<COLS; c++) {
      if (keys[r][c] == pitch) {
        keyOff(1, r, c);
      }
    }
  }
}

void scanKeypad (void) {
  // ROWS: D11-D8 = ROW1-ROW4
  // NB: All signals are active LOW
  if ((PINB & 0x08) == 0) {  // D11
    PORTB = (PORTB & (~0x10)) | (row[0][0] & row[1][0] & 0x10); // COL D12
    PORTC = (PORTC & (~0x03)) | (row[0][0] & row[1][0] & 0x03); // COL A0, A1
  }
  if ((PINB & 0x04) == 0) {  // D10
    PORTB = (PORTB & (~0x10)) | (row[0][1] & row[1][1] & 0x10);
    PORTC = (PORTC & (~0x03)) | (row[0][1] & row[1][1] & 0x03);
  }
  if ((PINB & 0x02) == 0) {  // D9
    PORTB = (PORTB & (~0x10)) | (row[0][2] & row[1][2] & 0x10);
    PORTC = (PORTC & (~0x03)) | (row[0][2] & row[1][2] & 0x03);
  }
  if ((PINB & 0x01) == 0) {  // D8
    PORTB = (PORTB & (~0x10)) | (row[0][3] & row[1][3] & 0x10);
    PORTC = (PORTC & (~0x03)) | (row[0][3] & row[1][3] & 0x03);
  }
}

void ledInit() {
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

void ledOn() {
  digitalWrite(13, HIGH);
}

void ledOff() {
  digitalWrite(13, LOW);
}

void setup() {
  for (int r=0; r<ROWS; r++) {
    pinMode(rowPins[r], INPUT);
  }
  for (int c=0; c<COLS; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c],HIGH);
  }

  ledInit();

  for (int r=0; r<ROWS; r++) {
    for (int c=0; c<COLS; c++) {
      keyOff(0, r, c);
      keyOff(1, r, c);
    }
  }

  MIDI.setHandleNoteOn(midiNoteOn);
  MIDI.setHandleNoteOff(midiNoteOff);
  MIDI.begin(MIDI_CHANNEL);

  Timer1.initialize(500);  // 0.1mS
  Timer1.attachInterrupt(scanKeypad);
}

void loop() {
  MIDI.read();

  if (kp.getKeys()) {
    for (int i=0; i<LIST_MAX; i++) {
      if (kp.key[i].stateChanged) {
        switch (kp.key[i].kstate) {
          case PRESSED:
            keypadNoteOn(kp.key[i].kchar);
            break;
          case RELEASED:
            keypadNoteOff(kp.key[i].kchar);
            break;
        }
      }
    }
  }
}
