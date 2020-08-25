/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MPR121 Touch Piano
//  https://diyelectromusic.wordpress.com/2020/08/25/arduino-mpr121-touch-piano/
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
    toneMelody - http://www.arduino.cc/en/Tutorial/Tone
    Adafruit MPR121 Demo - https://learn.adafruit.com/adafruit-mpr121-12-key-capacitive-touch-sensor-breakout-tutorial/overview
    Sparkfun MPR121 Hookup Guide - https://learn.sparkfun.com/tutorials/mpr121-hookup-guide

*/
#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "pitches.h"

// Set up the output pin to be used for the speaker
#define SPEAKER  12

// Set the notes to be played by each key.
// Pitch values taken from toneMedly pitches.h
int notes[] = {
  NOTE_C3, NOTE_CS3, NOTE_D3, NOTE_DS3, NOTE_E3, NOTE_F3, NOTE_FS3, NOTE_G3, NOTE_GS3, NOTE_A3, NOTE_AS3, NOTE_B3,
  NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
  NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
  NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
};

// It is possible to have up to four of these connected.
// But it needs the "addresses" to be set correctly.
Adafruit_MPR121 cap1 = Adafruit_MPR121();
Adafruit_MPR121 cap2 = Adafruit_MPR121();
Adafruit_MPR121 cap3 = Adafruit_MPR121();
Adafruit_MPR121 cap4 = Adafruit_MPR121();

int numcaps;

void setup() {
  Serial.begin(9600);
  Serial.print("\n\nArduino MPR121 Touch Piano\n");

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
    Serial.println("MPR121 #1 found at 0x5A.");
    numcaps |= 1;
  }
  if (cap2.begin(0x5B)) {
    Serial.println("MPR121 #2 found at 0x5B.");
    numcaps |= 2;
  }
  if (cap3.begin(0x5C)) {
    Serial.println("MPR121 #3 found at 0x5C.");
    numcaps |= 4;
  }
  if (cap4.begin(0x5D)) {
    Serial.println("MPR121 #4 found at 0x5D.");
    numcaps |= 8;
  }
  if (numcaps == 0) {
    Serial.println("ERROR: No MPR121s found.");
    while (1);
  }
}

void loop() {
  // Check each sensor and if touched play the note.
  // If no sensors are touched, turn off all notes.
  int playing = -1;

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
      
      for (uint8_t i=0; i<12; i++) {
        if (currtouched & (1<<i)) {
          Serial.print ("1\t");
          playing = i+c*12;
        } else {
          Serial.print ("0\t");
        }
      }
      Serial.println();
    }
  }

  if (playing != -1) {
    tone (SPEAKER, notes[playing]);
  } else {
    // If we didn't find a button pressed, make sure everything is off
    noTone(SPEAKER);
  }
}
