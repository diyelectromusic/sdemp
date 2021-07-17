/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Multi Slider MIDI Controller
//  https://diyelectromusic.wordpress.com/2021/07/17/arduino-multi-slider-midi-controller/
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
    Potentiometer        - https://www.arduino.cc/en/Tutorial/Potentiometer
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
*/
#include <MIDI.h>

// Uncomment this to use the serial port as a test rather than real MIDI
//#define TEST 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 1

// Uncomment this to use for the "slider board" with an Arduino Nano
#define SLIDER_BOARD_NANO 1

#define NUMPOTS 8
int pots[NUMPOTS] = {A0,A1,A2,A3,A4,A5,A6,A7};
int cc[NUMPOTS] = {
  0x01,  // Modulation wheel
//  0x07,  // Channel volume
//  0x08,  // Balance
//  0x0A,  // Pan
  0x0B,  // Expression control
  0x0C,  // Effect control 1
  0x0D,  // Effect control 2
  0x10,  // General purpose control 1
  0x11,  // General purpose control 2
  0x12,  // General purpose control 3
  0x13,  // General purpose control 4
//  0x14,  // General purpose control 5
//  0x15,  // General purpose control 6
//  0x16,  // General purpose control 7
//  0x17,  // General purpose control 8
};
int last[NUMPOTS];

void setup() {
#ifdef TEST
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL_OMNI);
#endif

  // Preset the "last" values to something invalid to
  // force an update on the first time through the loop.
  for (int i=0; i<NUMPOTS; i++) {
    last[i] = -1;
  }

#ifdef SLIDER_BOARD_NANO
  // This is a 'hack' to use two of the Arduino's IO
  // pins as VCC and GND for the potentiometers.
  //
  // This is specific to my "slider" board, so you
  // probably don't need to worry about doing this.
  pinMode(6,OUTPUT);
  pinMode(7,OUTPUT);
  digitalWrite(6,LOW);  // GND
  digitalWrite(7,HIGH); // VCC
#endif
}

int pot;
void loop() {
  // As this is a controller, we process any MIDI messages
  // we receive and allow the MIDI library to enact it's THRU
  // functionality to pass them on.
  MIDI.read();

  pot++;
  if (pot >= NUMPOTS) pot = 0;

  // We want to turn it into a number between 0 and 127
  // from the analog range of 0 to 1023. The simplest
  // way to do that is to divide it by two, three times,
  // which can be trivially done with a bit-shift of three places.
  //
  // But there are some inaccuracies in the readings here when
  // using my slider board, so instead (as this isn't particularly
  // time critical) I actually map the 0 to 127 range onto a smaller
  // analog range. In my case between 20 and 1000.
  //
  // For any other pots, this just means that the zero and highest
  // readers occur for a slighter larger portion of the physical
  // distance travelled by the pot.
  //
  // Also note the additional use of constrain() to ensure the values
  // are within the range passed to the map() function.
  //
  int val = analogRead(pots[pot]);
  val = map(constrain(val, 20, 1000), 20,1000,0,127);
  if (val != last[pot]) {
    // The value for this pot has changed so send a new MIDI message
#ifdef TEST
    Serial.print(pot);
    Serial.print(" CC 0x");
    if (cc[i]<16) Serial.print("0");
    Serial.print(cc[pot],HEX);
    Serial.print(" -> ");
    Serial.println(val);
#else
    MIDI.sendControlChange (cc[pot], val, MIDI_CHANNEL);
#endif
    last[pot] = val;
  }
}
