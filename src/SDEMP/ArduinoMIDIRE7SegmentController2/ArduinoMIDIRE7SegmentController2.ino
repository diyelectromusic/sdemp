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

// Comment this out if you don't want a "bank select" mode.
// By default this will use the rotary encoder switch
// to switch modes.
#define BANK_SEL 1

// Optional: Use an additional switch as the bank select mode switch.
// If not defined but BANK_SEL above is, then it is assumed the
// rotary encoder switch changes modes (this only really makes sense
// if the code is running in INSTANT_PC mode though!)
//#define BANK_SEL_PIN A5

// Define the MIDI CC messages to be used for the different banks
#define NUM_BANKS 8
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
int banknum;
int last_sw;
int sw_cnt;
int last_bank;
int bank_cnt;
int mode;

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

#ifdef BANK_SEL_PIN
  pinMode(BANK_SEL_PIN, INPUT_PULLUP);
#endif

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
  banknum = 0;
  mode = 0;  // Program
  updateProgDisplay(prognum);
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
      if (mode == 0) { // Program mode
        prognum--;
        prognum &= 0x7F;
      } else { // Bank mode
        banknum--;
        if (banknum < 0) {
          banknum = NUM_BANKS-1;
        }
      }
    } else if (re_dir > 0) {
      if (mode == 0) { // Program mode
        prognum++;
        prognum &= 0x7F;
      } else { // Bank mode
        banknum++;
        if (banknum >= NUM_BANKS) {
          banknum = 0;
        }
      }
    } else {
      // if re_dir == 0; do nothing
    }
    re_pos = new_pos;
    if (mode == 0) { // Program mode
      updateProgDisplay(prognum);
#ifdef INSTANT_PC
      updateProgNum (prognum);
#endif
    } else { // Bank mode
      updateBankDisplay(banknum);
#ifdef INSTANT_PC
      updateBankNum (banknum);
#endif
    }
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
#ifndef INSTANT_PC
    if (mode == 0) { // Program mode
      updateProgNum (prognum);
    } else { // Bank mode
      updateBankNum (banknum);
    }
#else // In "instant mode" so see if we are supporting BANK selection
#ifdef BANK_SEL
#ifndef BANK_SEL_PIN
    // This only makes sense if:
    //   In INSTANT_PC mode
    //   AND In BANK_SEL mode
    //   AND NOT In BANK_SEL_PIN mode
    // So if all this applies, then toggle the mode!
    mode = !mode;
    if (mode == 0) { // Program mode
#ifdef TEST
      Serial.println ("Program Mode via RE");
#endif
      updateProgDisplay(prognum);
    } else { // Bank mode
#ifdef TEST
      Serial.println ("Bank Mode via RE");
#endif
      updateBankDisplay(banknum);
    }
#endif
#endif
#endif
  }

#ifdef BANK_SEL_PIN
  int bank_sw = digitalRead(BANK_SEL_PIN);
  if (bank_sw == HIGH) {
    // No switch pressed yet
    bank_cnt = 0;
    last_bank = HIGH;
  } else {
    bank_cnt++;
  }
  if ((last_bank==HIGH) && (bank_cnt >= 1000)) {
    // Switch is triggered!
    bank_cnt = 0;
    last_bank = LOW;

    // Toggle the mode!
    mode = !mode;
    if (mode == 0) { // Program mode
#ifdef TEST
      Serial.println ("Program Mode via SW");
#endif
      updateProgDisplay(prognum);
    } else { // Bank mode
#ifdef TEST
      Serial.println ("Bank Mode via SW");
#endif
      updateBankDisplay(banknum);
    }
  }
#endif
}

void updateProgDisplay (int progNum) {
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

void updateBankDisplay (int bankNum) {
  // Use negative to show we're in "bank" mode
  // Also show 1 to MAX index on the display
  disp.setNumber(-(bankNum+1),0);
}

void updateBankNum (int bankNum) {
#ifdef TEST
  Serial.print ("Bank Change: ");
  Serial.print (ccs[bankNum][0]);
  Serial.print (",");
  Serial.println (ccs[bankNum][1]);
#else
  MIDI.sendControlChange (BANKSELMSB, ccs[bankNum][0], MIDI_CHANNEL);
  MIDI.sendControlChange (BANKSELLSB, ccs[bankNum][1], MIDI_CHANNEL);
#endif
}
