/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Rotary Encoder 7 Segment Controller
//  https://diyelectromusic.wordpress.com/2021/09/28/arduino-midi-rotary-encoder-controller/
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
    Rotary Encoder Library  - https://github.com/mathertel/RotaryEncoder
    HT16K33 Library         - https://github.com/RobTillaart/HT16K33
*/
#include <MIDI.h>
#include <RotaryEncoder.h>
#include <HT16K33.h>

// Rotary Encoder
#define RE_A  4  // "CLK"
#define RE_B  3  // "DT"
#define RE_SW 2  // "SW"
RotaryEncoder encoder(RE_A, RE_B, RotaryEncoder::LatchMode::TWO03);

// I2C 7 Segment Display
//   C/CLK = A5/SCL
//   D/DAT = A4/SDA
HT16K33 seg(0x70);  // Pass in I2C address of the display

// MIDI
MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 1    // Define which MIDI channel to transmit on (1 to 16).

uint8_t prognum;
int     last_sw;
int     sw_cnt;

void setup() {
  // Initialise the encoder switch
  pinMode(RE_SW, INPUT_PULLUP);

  // Initialise the I2C display
  seg.begin();
  Wire.setClock(100000);
  seg.displayOn();
  seg.brightness(2);
  seg.displayClear();
  seg.setDigits(1);  // Display minimum of a single digit
  seg.displayInt(prognum+1); // Translate 0-127 into MIDI 1-128

  // Initialise MIDI - listening on all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);
}

int re_pos = 0;
void loop() {
  // Enable the software MIDI THRU functionality
  MIDI.read();

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

    // Update the display
    seg.setDigits(1);  // Display minimum of a single digit
    seg.displayInt(prognum+1); // Translate 0-127 into MIDI 1-128
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
    MIDI.sendProgramChange(prognum, MIDI_CHANNEL);
  }
}
