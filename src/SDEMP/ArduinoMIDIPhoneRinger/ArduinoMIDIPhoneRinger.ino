/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Telephone Ringer
//  https://diyelectromusic.wordpress.com/2022/04/13/arduino-midi-telephone-ringer/
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
  Using principles from the following tutorials:
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Arduino Timer One Library - https://www.arduino.cc/reference/en/libraries/timerone/
*/
#include <MIDI.h>
#include <TimerOne.h>

MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 1
#define NUM_NOTES   3
uint8_t notes[NUM_NOTES] = {60, 62, 64};
uint8_t freqs[NUM_NOTES] = {20, 25, 30};
uint8_t playing;

#define HB_IN1    4
#define HB_IN2    5

bool ringerOn;
int  hb_in;

#define HB_LED LED_BUILTIN

void ledInit() {
#ifdef HB_LED
  pinMode(HB_LED, OUTPUT);
  digitalWrite(HB_LED, LOW);
#endif
}
void ledOn() {
#ifdef HB_LED
  digitalWrite(HB_LED, HIGH);
#endif
}
void ledOff() {
#ifdef HB_LED
  digitalWrite(HB_LED, LOW);
#endif
}

void setRinger (int freq) {
  unsigned long tickperiod = 1000000UL/(((unsigned long)freq)*2);
  Timer1.setPeriod(tickperiod);
}

void setup() {
  ledInit();

  pinMode(HB_IN1, OUTPUT);
  pinMode(HB_IN2, OUTPUT);
  digitalWrite(HB_IN1, LOW);
  digitalWrite(HB_IN2, LOW);

  ringerOn = false;
  hb_in = LOW;

  Timer1.initialize();
  Timer1.attachInterrupt(timerTick);

  MIDI.begin(MIDI_CHANNEL);
}

void loop() {
  if (MIDI.read()) {
    uint8_t cmd = MIDI.getType();
    uint8_t d1 = MIDI.getData1();
    uint8_t d2 = MIDI.getData2();
    switch(cmd) {
      case midi::NoteOn:
        for (int i=0; i<NUM_NOTES; i++) {
          if (d1 == notes[i]) {  // Note
            if (d2 == 0) {       // Velocity
              if (d1 == playing) {
                ledOff();
                ringerOn = false;
              }
            } else {
              ledOn();
              setRinger(freqs[i]);
              ringerOn = true;
              playing = d1;
            }
          }
        }
        break;

      case midi::NoteOff:
        if (d1 == playing) {
          ledOff();
          ringerOn = false;
        }
        break;

      default:
        // Otherwise it is a message we aren't interested in
        break;
    }
  }
}

void timerTick(void) {
  if (ringerOn) {
    // The two pins are always opposites to each other
    digitalWrite(HB_IN1, hb_in);
    digitalWrite(HB_IN2, !hb_in);

    // Toggle the setting for next time
    hb_in = !hb_in;
  } else {
    digitalWrite(HB_IN1, LOW);
    digitalWrite(HB_IN2, LOW);
    hb_in = LOW;
  }
}
