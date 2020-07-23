/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  MIDI Channel Monitor
//  https://diyelectromusic.wordpress.com/2020/07/20/arduino-midi-channel-monitor/
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
    Arduino Blink        - https://www.arduino.cc/en/Tutorial/Blink
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library

*/
#include <MIDI.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define COUNTDOWN 2000
#define NUMLEDS 16
int leds[NUMLEDS] = {
   14, 15, 16, 17, 18, 19, 11, 12,
   9, 8, 7, 6, 5, 4, 3, 2,
};

int countdown[NUMLEDS];

void setup() {
  // This listens to all MIDI channels
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Initialise the LED pins
  for (int i=0; i<NUMLEDS; i++) {
    pinMode (leds[i], OUTPUT);
    countdown[i] = COUNTDOWN;
  }

  for (int i=0; i<NUMLEDS; i++) {
    digitalWrite(leds[i], HIGH);
    delay (500);
    digitalWrite(leds[i], LOW);
  }
}

void loop() {
  if (MIDI.read()) {
    byte cmd = MIDI.getType();
    if ((cmd >= 0x80) && (cmd <= 0xE0)) {
       byte ch = MIDI.getChannel();
       digitalWrite (leds[ch-1], HIGH);
       countdown[ch-1] = COUNTDOWN;
    }
  }

  // Service the counters for each channel
  for (int i=0; i<NUMLEDS; i++) {
    if (countdown[i] == 0) {
      digitalWrite (leds[i], LOW);
      countdown[i] = -1;
    }
    else if (countdown[i] > 0) {
      countdown[i]--;
    }
    else {
      // If less than 0 then do nothing
    }
  }
}
