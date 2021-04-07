/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MT-32 Program Changer
//  https://diyelectromusic.wordpress.com/2021/04/07/arduino-mt-32-program-changer/
//
      MIT License
      
      Copyright (c) 2021 diyelectromusic (Kevin)
      
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
    Arduino Analog Input    - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput/
    Adafruit SSD1306        - https://learn.adafruit.com/monochrome-oled-breakouts/arduino-library-and-examples
    Adafruit GFX Library    - https://learn.adafruit.com/adafruit-gfx-graphics-library
*/
#include <MIDI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "mt32voices.h"

//#define TEST 1

// MIDI Channel to listen on
#define MIDI_CHANNEL 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port.
MIDI_CREATE_DEFAULT_INSTANCE();

// Potentiometer definitions.
// NB: Can't use A4 and A5 as these are the I2C interface used for the display.
//
#define POT_G A0   // Sound Group POT
#define POT_S A1   // Sound POT

// Set the screen direction, relative to use of the DIY shield.
//   0 = landscape (pins at top)
//   1 = portrait (pins on the left)
//   2 = landscape (pins at bottom)
//   3 = portrait (pins on the right)
#define OLED_ROT 2

// Dimensions of the OLED screen
#define OLED_W  128
#define OLED_H   64
#define OLED_RST -1 // If your screen has a RESET pin set that here

// Set the address on the I2C bus for the display.
// SSD1306 based displays can be set to one of two
// addresses 0x3C or 0x3D depending on a jumper
// on the back of the boards.  By default mine is set
// to 0x3C.
//
// Note that I2C addresses are 7-bit only and the final
// bit indicates a read or a write going on.
//
// 0x3C = b0111100
// 0x3D = b0111101
//
// On the bus these are shifted up 1 bit to make
//    0x3C -> 0x78 (b01111000)
//    0x3D -> 0x7A (b01111010)
//
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display = Adafruit_SSD1306(OLED_W, OLED_H, &Wire, OLED_RST);

// Used to refer to the width and height of the display
uint16_t oled_w;
uint16_t oled_h;

// Default font dimensions
// See: https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#define FONT_X (5+1)  // Characters are 5 wide, with a space
#define FONT_Y 8

int pot_group;
int pot_sound;
int mt32voice;

int calcMT32Voice () {
  // Now calculate the MT32 Voice index
  // NB: This will return a zero-indexed voice number
  int voice = 0;
  for (int i=0; i<pot_group; i++) {
    voice += mt32VinG[i];
  }
  voice += pot_sound;

  return voice;
}

void updateDisplay (int group, int sound) {
  display.clearDisplay();
  display.setCursor(0,0);

  printGroup(group);
  printSound(mt32voice);
}

void printGroup (int group) {
  for (int i=0; i<MT32SIZE; i++) {
    char c = pgm_read_byte_near(&mt32groups[group][i]);
    if (c==0) break;
    display.print(c);
  }
  display.println();
}

void printSound (int sound) {
  for (int i=0; i<MT32SIZE; i++) {
    char c = pgm_read_byte_near(&mt32voices[sound][i]);
    if (c==0) break;
    display.print(c);
  }
  display.println();  
}

#ifdef TEST
void dumpMT32Voice() {
  Serial.print(pot_group);
  Serial.print("\t");
  Serial.print(pot_sound);
  Serial.print("\t");
  Serial.print(mt32voice);
  Serial.print("\n");  
}
#endif

void mt32ProgramChange (byte voice) {
#ifndef TEST
  MIDI.sendProgramChange(voice, MIDI_CHANNEL);
#endif
}

void setup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    for (;;); // No display - stop
  }

  display.setRotation(OLED_ROT);
  oled_w = display.width();
  oled_h = display.height();

  display.display();
  delay(1000);

  // Initialise the display for text
  display.setTextColor(SSD1306_WHITE,SSD1306_BLACK);
  display.setTextWrap(true);
  display.setTextSize(2);
  updateDisplay(0,0);

#ifdef TEST
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL_OFF);
#endif
}

void loop() {
  // Convert the potentiometers from 0 to 1023 down to
  // represent the number of MT-32 Groups and the max
  // number of MT-32 voices per group.
  int pot1 = analogRead(POT_G) / (1024/MT32GROUPS);
  int pot2 = analogRead(POT_S) / (1024/MT32MAXVING);
  
  // Check that the sound is in range for the group
  // NB: Need 0-indexed group and sound
  // NB: Calc before testing pot_group/pot_sound otherwise
  //     when the pot readings are out of range they will
  //     continually be calculated as being different to
  //     pot_group/pot_sound and so continuously trigger updates.
  //
  if (pot1 >= MT32GROUPS) pot1 = MT32GROUPS-1;
  if (pot2 >= mt32VinG[pot1]) pot2 = mt32VinG[pot1]-1;
  
  if ((pot_group != pot1) || (pot_sound != pot2)) {
  // Update the group and sound
    pot_group = pot1;
    pot_sound = pot2;
    mt32voice = calcMT32Voice();
    updateDisplay(pot_group, pot_sound);
    mt32ProgramChange(mt32voice);

#ifdef TEST
    dumpMT32Voice();
#endif
  }

  display.display();
}
