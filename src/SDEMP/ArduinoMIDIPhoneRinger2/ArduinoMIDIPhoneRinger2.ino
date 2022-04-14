/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Telephone Ringer - Part 2
//  https://diyelectromusic.wordpress.com/2022/04/14/arduino-midi-telephone-ringer-part-2/
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
#define FREQ_HZ     25
uint8_t notes1[NUM_NOTES] = {48, 60, 72};
uint8_t notes2[NUM_NOTES] = {50, 62, 74};
uint8_t playing1, playing2;

#define HB_IN1    4
#define HB_IN2    5
#define HB_IN3    6
#define HB_IN4    7

bool ringer1On, ringer2On;
int  hb1_in, hb2_in;

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
  pinMode(HB_IN3, OUTPUT);
  pinMode(HB_IN4, OUTPUT);
  digitalWrite(HB_IN1, LOW);
  digitalWrite(HB_IN2, LOW);
  digitalWrite(HB_IN3, LOW);
  digitalWrite(HB_IN4, LOW);

  ringer1On = false;
  ringer2On = false;
  hb1_in = LOW;
  hb2_in = LOW;

  Timer1.initialize();
  Timer1.attachInterrupt(timerTick);
  setRinger(FREQ_HZ);

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
          if (d1 == notes1[i]) {  // Note
            if (d2 == 0) {        // Velocity
              if (d1 == playing1) {
                ledOff();
                ringer1On = false;
              }
            } else {
              ledOn();
              ringer1On = true;
              playing1 = d1;
            }
          } else if (d1 == notes2[i]) {
            if (d2 == 0) {
              if (d1 == playing2) {
                ledOff();
                ringer2On = false;
              }
            } else {
              ledOn();
              ringer2On = true;
              playing2 = d1;
            }
          }
        }
        break;

      case midi::NoteOff:
        if (d1 == playing1) {
          ledOff();
          ringer1On = false;
        } else if (d1 == playing2) {
          ledOff();
          ringer2On = false;
        }
        break;

      default:
        // Otherwise it is a message we aren't interested in
        break;
    }
  }
}

void timerTick(void) {
  if (ringer1On) {
    // The two pins are always opposites to each other
    digitalWrite(HB_IN1, hb1_in);
    digitalWrite(HB_IN2, !hb1_in);

    // Toggle the setting for next time
    hb1_in = !hb1_in;
  } else {
    digitalWrite(HB_IN1, LOW);
    digitalWrite(HB_IN2, LOW);
    hb1_in = LOW;
  }
  // Repeat above logic for the second ringer
  if (ringer2On) {
    digitalWrite(HB_IN3, hb2_in);
    digitalWrite(HB_IN4, !hb2_in);
    hb2_in = !hb2_in;
  } else {
    digitalWrite(HB_IN3, LOW);
    digitalWrite(HB_IN4, LOW);
    hb2_in = LOW;
  }
}
