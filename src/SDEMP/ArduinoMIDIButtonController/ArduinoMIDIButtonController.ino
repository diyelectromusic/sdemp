/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Button MIDI Controller
//  https://diyelectromusic.wordpress.com/2021/01/11/arduino-midi-button-controller/
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
    Adafruit LED Backpack   - https://learn.adafruit.com/adafruit-led-backpack/0-dot-56-seven-segment-backpack-arduino-setup

*/
#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <MIDI.h>

// Uncomment this to print to the Serial port instead of sending MIDI
//#define TEST 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1    // Define which MIDI channel to transmit on (1 to 16).

// Set up the LED display
Adafruit_7segment matrix = Adafruit_7segment();

#define BTN_UP   2
#define BTN_DOWN 3

#define MIN_PROG 1
#define MAX_PROG 128
int progNum;
int lastBtnUp;
int lastBtnDown;

void setup() {
  pinMode (BTN_UP, INPUT_PULLUP);
  pinMode (BTN_DOWN, INPUT_PULLUP);
  progNum = MIN_PROG;
  lastBtnUp = HIGH;
  lastBtnDown = HIGH;

  matrix.begin(0x70);
  updateDisplay();

#ifdef TEST
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL);
#endif
}

void loop() {
  int btnUp = digitalRead(BTN_UP);
  int btnDown = digitalRead(BTN_DOWN);
  if ((btnUp == LOW) && (lastBtnUp == HIGH)) {
    // Button has been pressed
    progNum++;
    if (progNum > MAX_PROG) progNum = MAX_PROG;
    updateDisplay();
    delay (300); // simple "manual" switch debouncing
  }
  if ((btnUp == LOW) && (lastBtnUp == LOW)) {
    // Button is held down, so update more quickly
    progNum++;
    if (progNum > MAX_PROG) progNum = MAX_PROG;
    updateDisplay();
    delay (100);
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
    delay (300); // simple "manual" switch debouncing
  }
  if ((btnDown == LOW) && (lastBtnDown == LOW)) {
    // Button is held down, so update more quickly
    progNum--;
    if (progNum < MIN_PROG) progNum = MIN_PROG;
    updateDisplay();
    delay (100);
  }
  if ((btnDown == HIGH) && (lastBtnDown == LOW)) {
    // Button has been released
    updateProgNum();
  }
  lastBtnUp = btnUp;
  lastBtnDown = btnDown;
}

void updateDisplay () {
  matrix.print(progNum, DEC);
  matrix.writeDisplay();
}

void updateProgNum () {
#ifdef TEST
  Serial.print ("Program Change: ");
  Serial.println (progNum);
#else
  // MIDI programs are defined as 1 to 128, but "on the wire"
  // have to be converted to 0 to 127.
  MIDI.sendProgramChange (progNum-1, MIDI_CHANNEL);
  delay(500);
// Optional: play a note after the program change to confirm it worked
//  playTestNote(60);
#endif
}

void playTestNote (byte note) {
  MIDI.sendNoteOn(note, 127, MIDI_CHANNEL);
  delay(100);
  MIDI.sendNoteOff(note, 0, MIDI_CHANNEL);
  delay(100);
}
