/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MPR121 MIDI Touch Piano
//  https://diyelectromusic.wordpress.com/2021/07/04/arduino-mpr121-midi-touch-piano/
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
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Adafruit MPR121 Demo - https://learn.adafruit.com/adafruit-mpr121-12-key-capacitive-touch-sensor-breakout-tutorial/overview
    Sparkfun MPR121 Hookup Guide - https://learn.sparkfun.com/tutorials/mpr121-hookup-guide

*/
#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <MIDI.h>

// Uncomment for test information to the serial port instead of real MIDI
//#define TEST 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

int midiChannel = 1;  // Define which MIDI channel to transmit on (1 to 16).

#define MIDI_FIRST_NOTE 48

// It is possible to have up to four of these connected.
// But it needs the "addresses" to be set correctly.
Adafruit_MPR121 cap1 = Adafruit_MPR121();
Adafruit_MPR121 cap2 = Adafruit_MPR121();
Adafruit_MPR121 cap3 = Adafruit_MPR121();
Adafruit_MPR121 cap4 = Adafruit_MPR121();

#define NUM_NOTES 48
bool playing[NUM_NOTES];
bool lastplaying[NUM_NOTES];
int numcaps;

#define MIDI_LED LED_BUILTIN
#define LED_FLASH  200
#define LED_STATUS 1000

void ledOn() {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, HIGH);
#endif
}

void ledOff() {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, LOW);
#endif
}

void ledFlash (int flashes) {
  for (int i=0; i<flashes; i++) {
    ledOn();
    delay(LED_FLASH);
    ledOff();
    delay(LED_FLASH);
  }
}

void setup() {
#ifdef TEST
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL_OFF);
#endif

#ifdef MIDI_LED
  pinMode (MIDI_LED, OUTPUT);
#endif

  // Some notes on addressing.
  //
  // The default address is 0x5A if ADDR is tied to GND.
  // If ADDR is tied to 3.3V its 0x5B.
  // If ADDR is tied to SDA its 0x5C.
  // If ADDR is tied to SCL its 0x5D.
  //
  // IMPORTANT: Most MPR121 clone breakout boards (those based
  // on a design from Sparkfun) have ADDR shorted direct to GND
  // via a solder jumper.
  //
  // To change the address, the track across this solder jumper
  // must first be cut or you risk shorting SDA, SCL or worse 3.3V
  // directly to GND!.
  //
  // The Adafruit modules do not have this problem, ADDR is linked
  // to GND via a large enough resistor that it can be rewired across
  // to any of 3.3V, SDA or SCL quite happily.
  //
  // Also note that non-Adafruit modules need direct 3.3V logic levels
  // and power.  Driving via 5V logic may work (it seems to, but don't
  // take my word for it), but putting 5V into ADDR from SDA or SCL does
  // not seem to work.  The use of a 5v-3.3V level shifter is highly
  // recommended.
  //
  // Adafruit modules include a regulator so will automatically convert
  // 5V logic to 3.3V within the module.
  //
  numcaps = 0;
  if (cap1.begin(0x5A)) {
#ifdef TEST
    Serial.println("MPR121 #1 found at 0x5A.");
#endif
    ledFlash(1);
    delay(LED_STATUS);
    numcaps |= 1;
  }
  if (cap2.begin(0x5B)) {
#ifdef TEST
    Serial.println("MPR121 #2 found at 0x5B.");
#endif
    ledFlash(2);
    delay(LED_STATUS);
    numcaps |= 2;
  }
  if (cap3.begin(0x5C)) {
#ifdef TEST
    Serial.println("MPR121 #3 found at 0x5C.");
#endif
    ledFlash(3);
    delay(LED_STATUS);
    numcaps |= 4;
  }
  if (cap4.begin(0x5D)) {
#ifdef TEST
    Serial.println("MPR121 #4 found at 0x5D.");
#endif
    ledFlash(4);
    numcaps |= 8;
  }
  if (numcaps == 0) {
#ifdef TEST
    Serial.println("ERROR: No MPR121s found.");
#endif
    while (1) {
      ledFlash(1);
    }
  }
}

void loop() {
  // Check each sensor and if touched play the note.

  // Loop through all possible cap devices
  for (int c=0; c<4; c++) {
    uint16_t currtouched;
    // Get the currently touched pads for all found cap devices
    if (numcaps & (1<<c)) {
      switch (c) {
      case 0:
        currtouched = cap1.touched(); break;
      case 1:
        currtouched = cap2.touched(); break;
      case 2:
        currtouched = cap3.touched(); break;
      case 3:
        currtouched = cap4.touched(); break;
      }
      
      for (int i=0; i<12; i++) {
        if (currtouched & (1<<i)) {
          playing[i+c*12] = true;
        } else {
          playing[i+c*12] = false;          
        }
      }
    }
  }

  for (int i=0; i<NUM_NOTES; i++) {
    // Now look for transitions and do the right MIDI thing
    if (playing[i] && !lastplaying[i]) {
      // Wasn't playing and now is, so send a MIDI Note On
#ifdef TEST
      Serial.print("MIDI NoteOn: ");
      Serial.println(MIDI_FIRST_NOTE+i);
#else
      MIDI.sendNoteOn(MIDI_FIRST_NOTE+i, 127, midiChannel);
#endif
      ledOn();
    } else if (!playing[i] && lastplaying[i]) {
      // Was playing but now isn't, so send a MIDI Note Off
#ifdef TEST
      Serial.print("MIDI NoteOff: ");
      Serial.println(MIDI_FIRST_NOTE+i);
#else
      MIDI.sendNoteOff(MIDI_FIRST_NOTE+i, 0, midiChannel);
#endif
      ledOff();
    } else {
      // Whatever it was doing, it is still doing, so do nothing
    }
    // Now update the "last playing" indicator
    lastplaying[i] = playing[i];
  }
}
