/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Telephone Ringer Revisited
//  https://diyelectromusic.wordpress.com/2022/11/15/arduino-midi-telephone-ringer-revisited/
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

#define FR_PIN 2
#define RM_PIN 3
int FR_level;
bool ringer;

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

void ringerOn () {
  digitalWrite (RM_PIN, HIGH);
  ringer = true;
}

void ringerOff () {
  digitalWrite (RM_PIN, LOW);
  ringer = false;
}

void setRinger (int freq) {
  unsigned long tickperiod = 1000000UL/(((unsigned long)freq)*2);
  Timer1.setPeriod(tickperiod);
}

void setup() {
  ledInit();

  pinMode (FR_PIN, OUTPUT);
  FR_level = false;
  digitalWrite (FR_PIN, FR_level);
  pinMode (RM_PIN, OUTPUT);
  digitalWrite (RM_PIN, LOW);

  ringer = false;

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
                ringerOff();
              }
            } else {
              ledOn();
              setRinger(freqs[i]);
              ringerOn();
              playing = d1;
            }
          }
        }
        break;

      case midi::NoteOff:
        if (d1 == playing) {
          ledOff();
          ringerOff();
        }
        break;

      default:
        // Otherwise it is a message we aren't interested in
        break;
    }
  }
}

void timerTick(void) {
  if (ringer) {
    FR_level = !FR_level;
    digitalWrite(FR_PIN, FR_level);
  } else {
    FR_level = LOW;
    digitalWrite(FR_PIN, FR_level);
  }
}
