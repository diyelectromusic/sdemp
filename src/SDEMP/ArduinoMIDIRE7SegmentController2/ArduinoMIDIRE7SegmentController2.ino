/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI 7 Segment Controller - Part 2
//  https://diyelectromusic.wordpress.com/2022/09/15/arduino-midi-rotary-encoder-controller-part-2/
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
    Rotary Encoder Library  - https://github.com/mathertel/RotaryEncoder
    Arduino 7-Segment Tutorial - https://www.circuitbasics.com/arduino-7-segment-display-tutorial/
*/
#include <MIDI.h>
#include <SevSeg.h>
#include <RotaryEncoder.h>

// Uncomment this to print to the Serial port instead of sending MIDI
//#define TEST 1

// Comment this out if you don't want the program change messages to
// be instant, but would prefer them to require a button press.
#define INSTANT_PC 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1    // Define which MIDI channel to transmit on (1 to 16).

// Rotary Encoder
#define RE_A  A1  // "CLK"
#define RE_B  A0  // "DT"
#define RE_SW A2  // "SW"
RotaryEncoder encoder(RE_A, RE_B, RotaryEncoder::LatchMode::FOUR3);

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
// Pattern repeated for three digits (1,2,3) each with a common cathode.
// Note: Cathode should be connected to the "digit" pins via a resistor (1k).
//
// Digit Order:  D1-D2-D3
//
// Pinout of my 10-pin display.
//
//   D1--B-D2--A-D3
//    |           |
//    F--C--D--E--G
//
#define NUM_DIGITS   3
#define NUM_SEGMENTS 7
byte digitPins[NUM_DIGITS]     = {4,3,2};           // Order: D1,D2,D3
byte segmentPins[NUM_SEGMENTS] = {5,6,7,8,9,10,11}; // Order: A,B,C,D,E,F,G
#define RESISTORS_ON_SEGMENTS false          // Resistors on segments (true) or digits (false)
#define HARDWARE_CONFIG       COMMON_CATHODE // COMMON_CATHODE or COMMON_ANODE (or others)
#define UPDATE_WITH_DELAYS    false          // false recommended apparently
#define LEADING_ZEROS         true           // Set true to show leading zeros
#define NO_DECIMAL_POINT      true           // Set true if no DP or not connected

int prognum;
int last_sw;
int sw_cnt;

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
  // Initialise the encoder switch
  pinMode(RE_SW, INPUT_PULLUP);

  // Initialise the 7-Segment display
  disp.begin(HARDWARE_CONFIG, NUM_DIGITS, digitPins, segmentPins,
             RESISTORS_ON_SEGMENTS, UPDATE_WITH_DELAYS, LEADING_ZEROS, NO_DECIMAL_POINT);
  disp.setBrightness(90);

#ifdef TEST
  Serial.begin(9600);
#else
  // Initialise MIDI - listening on all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);
#endif
  prognum = 0;
  updateDisplay(prognum);
}

int re_pos = 0;
void loop() {
  // Enable the software MIDI THRU functionality
  MIDI.read();

  // Note: Need to keep the display updated continually.
  //       This means we cannot use delay(), so have to use
  //       the replacement delayWithDisplay()
  //
  disp.refreshDisplay();

  // Read the rotary encoder
  encoder.tick();

  int new_pos = encoder.getPosition();
  if (new_pos != re_pos) {
    int re_dir = (int)encoder.getDirection();
    if (re_dir < 0) {
      prognum--;
      prognum &= 0x7F;
    } else if (re_dir > 0) {
      prognum++;
      prognum &= 0x7F;
    } else {
      // if re_dir == 0; do nothing
    }
    re_pos = new_pos;
    updateDisplay(prognum);
#ifdef INSTANT_PC
    updateProgNum (prognum);
#endif
  }

  // Check the encoder switch with some software debouncing
  int re_sw = digitalRead(RE_SW);
  if (re_sw == HIGH) {
    // No switch pressed yet
    sw_cnt = 0;
    last_sw = HIGH;
  } else {
    sw_cnt++;
  }
  if ((last_sw==HIGH) && (sw_cnt >= 1000)) {
    // Switch is triggered!
    sw_cnt = 0;
    last_sw = LOW;
    updateProgNum (prognum);
  }
}

void updateDisplay (int progNum) {
  // Change to user-friendly 1 to 128 range
  disp.setNumber(progNum+1,0);
}

void updateProgNum (int progNum) {
#ifdef TEST
  Serial.print ("Program Change: ");
  Serial.println (progNum);
#else
  // MIDI programs are defined as 1 to 128, but "on the wire"
  // have to be converted to 0 to 127.
  MIDI.sendProgramChange (progNum, MIDI_CHANNEL);
#endif
}
