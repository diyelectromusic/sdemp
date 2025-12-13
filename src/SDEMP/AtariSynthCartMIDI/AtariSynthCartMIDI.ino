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
uint8_t row[ROWS];
uint8_t colbits[COLS] = {
  0x02,  // C0 = A1  PORTC
  0x01,  // C1 = A0  PORTC
  0x10   // C2 = D12 PORTB
};

void keyOn (int r, int c) {
  if (r < ROWS && c < COLS) {
    // Clear the bit as need active LOW
    row[r] &= (~colbits[c]);
  }
}

void keyOff (int r, int c) {
  if (r < ROWS && c < COLS) {
    // Set the bit as need active LOW
    row[r] |= colbits[c];
  }
}

// Define key presses in terms of index into key[] array
char keys[ROWS][COLS] = {
  {60,61,62},
  {63,64,65},
  {66,67,68},
  {69,70,71},
};

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  for (int r=0; r<ROWS; r++) {
    for (int c=0; c<COLS; c++) {
      if (keys[r][c] == pitch) {
        keyOn(r, c);
      }
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  for (int r=0; r<ROWS; r++) {
    for (int c=0; c<COLS; c++) {
      if (keys[r][c] == pitch) {
        keyOff(r, c);
      }
    }
  }
}

void scanKeypad (void) {
  PORTB = (PORTB & (~0x20)) | 0x20;

  // ROWS: D11-D8 = ROW1-ROW4
  if ((PINB & 0x08) == 0) {  // D11
    PORTB = (PORTB & (~0x10)) | (row[0] & 0x10); // COL D12
    PORTC = (PORTC & (~0x03)) | (row[0] & 0x03); // COL A0, A1
  }
  if ((PINB & 0x04) == 0) {  // D10
    PORTB = (PORTB & (~0x10)) | (row[1] & 0x10);
    PORTC = (PORTC & (~0x03)) | (row[1] & 0x03);
  }
  if ((PINB & 0x02) == 0) {  // D9
    PORTB = (PORTB & (~0x10)) | (row[2] & 0x10);
    PORTC = (PORTC & (~0x03)) | (row[2] & 0x03);
  }
  if ((PINB & 0x01) == 0) {  // D8
    PORTB = (PORTB & (~0x10)) | (row[3] & 0x10);
    PORTC = (PORTC & (~0x03)) | (row[3] & 0x03);
  }

  PORTB = (PORTB & (~0x20));
}

void setup() {
  for (int r=0; r<ROWS; r++) {
    pinMode(rowPins[r], INPUT);
  }
  for (int c=0; c<COLS; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c],HIGH);
  }

  // D13 for timing of interrupt routine
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  for (int r=0; r<ROWS; r++) {
    for (int c=0; c<COLS; c++) {
      keyOff(r, c);
    }
  }

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.begin(MIDI_CHANNEL);

  Timer1.initialize(500);  // 0.1mS
  Timer1.attachInterrupt(scanKeypad);
}

void loop() {
  MIDI.read();
}
