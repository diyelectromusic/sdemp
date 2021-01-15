/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI 7 Segment Controller
//  https://diyelectromusic.wordpress.com/2021/01/15/arduino-midi-7-segment-controller/
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
    Arduino Button          - https://www.arduino.cc/en/Tutorial/BuiltInExamples/Button
    Arduino 7-Segment Tutorial - https://www.circuitbasics.com/arduino-7-segment-display-tutorial/
*/
#include <MIDI.h>
#include <SevSeg.h>

// Uncomment this to print to the Serial port instead of sending MIDI
//#define TEST 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1    // Define which MIDI channel to transmit on (1 to 16).

// Set up the LED display
SevSeg disp;

// Pin definitions for the display
//
//   = A =
//  F     B
//   = G =
//  E     C
//   = D =
//
// Pattern repeated for three digits (1,2,3,4) each with a common cathode.
// Note: Cathode should be connected to the "digit" pins via a resistor (1k).
//
// Digit Order:  D1-D2-D3-D4
//
// Pinout of my 12-pin display.
//
//   D1--A--F-D2-D3--B
//    |              |
//    E--D-DP--C--G-D4
//
#define NUM_DIGITS   4
#define NUM_SEGMENTS 8
byte digitPins[NUM_DIGITS]     = {2,3,4,5};             // Order: D1,D2,D3,D4
byte segmentPins[NUM_SEGMENTS] = {6,7,8,9,10,11,12,13}; // Order: A,B,C,D,E,F,G,DP
#define RESISTORS_ON_SEGMENTS false          // Resistors on segments (true) or digitals (false)
#define HARDWARE_CONFIG       COMMON_CATHODE // COMMON_CATHODE or COMMON_ANODE (or others)
#define UPDATE_WITH_DELAYS    false          // false recommended apparently
#define LEADING_ZEROS         true           // Set true to show leading zeros
#define NO_DECIMAL_POINT      false          // Set true if no DP or not connected

#define BTN_UP   A0
#define BTN_DOWN A1

#define MIN_PROG 1
#define MAX_PROG 128
int progNum;
int lastBtnUp;
int lastBtnDown;

// ------------------------------------------------------
//   The display needs to be updated continually, which
//   means that we can't use delay() anywhere in the code
//   without interrupting the display.
//
//   This function can be used as a replacement for the
//   delay() function which will do the "waiting" whilst
//   at the same time keeping the display updated.
//
// ------------------------------------------------------
void delayWithDisplay(unsigned long mS) {
  unsigned long end_mS = mS + millis();
  unsigned long time_mS  = millis();
  while (time_mS < end_mS) {
    disp.refreshDisplay();
    time_mS = millis();
  }
}

void setup() {
  disp.begin(HARDWARE_CONFIG, NUM_DIGITS, digitPins, segmentPins,
               RESISTORS_ON_SEGMENTS, UPDATE_WITH_DELAYS, LEADING_ZEROS, NO_DECIMAL_POINT);
  disp.setBrightness(90);

  pinMode (BTN_UP, INPUT_PULLUP);
  pinMode (BTN_DOWN, INPUT_PULLUP);
  progNum = MIN_PROG;
  lastBtnUp = HIGH;
  lastBtnDown = HIGH;

#ifdef TEST
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL);
#endif
  updateDisplay();
}

void loop() {
  // Note: Need to keep the display updated continually.
  //       This means we cannot use delay(), so have to use
  //       the replacement delayWithDisplay()
  //
  disp.refreshDisplay();

  int btnUp = digitalRead(BTN_UP);
  int btnDown = digitalRead(BTN_DOWN);
  if ((btnUp == LOW) && (lastBtnUp == HIGH)) {
    // Button has been pressed
    progNum++;
    if (progNum > MAX_PROG) progNum = MAX_PROG;
    updateDisplay();
    delayWithDisplay (300); // simple "manual" switch debouncing
  }
  if ((btnUp == LOW) && (lastBtnUp == LOW)) {
    // Button is held down, so update more quickly
    progNum++;
    if (progNum > MAX_PROG) progNum = MAX_PROG;
    updateDisplay();
    delayWithDisplay (100);
  }
  if ((btnUp == HIGH) && (lastBtnUp == LOW)) {
    // Button has been released
    updateProgNum();
  }
  if ((btnDown == LOW) && (lastBtnDown == HIGH)) {
    // Button has been pressed
    progNum--;
    if (progNum < MIN_PROG) progNum = MIN_PROG;
    updateDisplay();
    delayWithDisplay (300); // simple "manual" switch debouncing
  }
  if ((btnDown == LOW) && (lastBtnDown == LOW)) {
    // Button is held down, so update more quickly
    progNum--;
    if (progNum < MIN_PROG) progNum = MIN_PROG;
    updateDisplay();
    delayWithDisplay (100);
  }
  if ((btnDown == HIGH) && (lastBtnDown == LOW)) {
    // Button has been released
    updateProgNum();
  }
  lastBtnUp = btnUp;
  lastBtnDown = btnDown;
}

void updateDisplay () {
  disp.setNumber(progNum,0);
}

void updateProgNum () {
#ifdef TEST
  Serial.print ("Program Change: ");
  Serial.println (progNum);
#else
  // MIDI programs are defined as 1 to 128, but "on the wire"
  // have to be converted to 0 to 127.
  MIDI.sendProgramChange (progNum-1, MIDI_CHANNEL);
  delayWithDisplay(500);
// Optional: play a note after the program change to confirm it worked
//  playTestNote(60);
#endif
}

void playTestNote (byte note) {
  MIDI.sendNoteOn(note, 127, MIDI_CHANNEL);
  delayWithDisplay(100);
  MIDI.sendNoteOff(note, 0, MIDI_CHANNEL);
  delayWithDisplay(100);
}
