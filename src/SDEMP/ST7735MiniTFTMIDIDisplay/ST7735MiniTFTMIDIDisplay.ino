/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  ST7735 Mini TFT MIDI Display
//  https://diyelectromusic.wordpress.com/2020/10/18/tft-midi-display/
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
    Adafruit ST7735 Library - https://learn.adafruit.com/1-8-tft-display
    Adafruit GFX Library    - https://learn.adafruit.com/adafruit-gfx-graphics-library

*/
#include <MIDI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

//#define TEST 1

// MIDI Channel to listen on
#define MIDI_CHANNEL MIDI_CHANNEL_OMNI

// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_LED LED_BUILTIN

// All settings are defined for the DIY TFT shield using a 
// cheap ST7735 based 1.8" 128x160 16-bit colour display as
// described here:
//    https://emalliab.wordpress.com/2020/10/10/arduino-and-a-cheap-tft-display/

// Set the screen direction, relative to use of the DIY shield.
//   0 = portrait (USB top)
//   1 = landscape (USB left)
//   2 = portrait (USB bottom)
//   3 = landscape (USB right)
#define TFT_ROT 1

// Simplest option is to use the built-in hardware SPI - it has the best
// performance and will be assumed by default.
//
// There is an option to use any IO pins in a software SPI mode.  It isn't
// as fast as hardware SPI but you can use whatever pins work best for you.
//
// (for example, my cheap ST7735 display pin out matches the Arduino headers
// from 5V to A5 if the pins are configured as follows:
//
//   TFT_CS   A5
//   TFT_RST  A4
//   TFT_DC   A3
//   TFT_MOSI A2
//   TFT_SCLK A1
//   (then four unused pins)
//   GND
//   5V
//
#define TFT_CS   10
#define TFT_RST   9
#define TFT_DC    8  // DC/RS
#define TFT_MOSI  11 // MOSI/SDA - not required for hardware SPI
#define TFT_SCLK  13 // SCLK/CLK - not required for hardware SPI

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Used to refer to the width and height of the display
uint16_t tft_w;
uint16_t tft_h;

// Default font dimensions
// See: https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#define FONT_X (5+1)  // Characters are 5 wide, with a space
#define FONT_Y 8

// Character positions for text writing
uint8_t tft_rows;
uint8_t tft_cols;
uint8_t col,row;
uint8_t testnote=21;

void setup(void) {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(TFT_ROT);
  tft_w = tft.width();
  tft_h = tft.height();
  tft_cols = tft_w/FONT_X;
  tft_rows = tft_h/FONT_Y;
  tft.fillScreen(ST77XX_BLACK);

  printInit();

  pinMode (MIDI_LED, OUTPUT);
  digitalWrite (MIDI_LED, LOW);
  
  // This listens to all MIDI channels
  MIDI.begin(MIDI_CHANNEL);
}

void loop(void) {
  if (MIDI.read()) {
    switch(MIDI.getType()) {
      case midi::NoteOn:
        digitalWrite (MIDI_LED, HIGH);
        printNote (MIDI.getData1());
        digitalWrite (MIDI_LED, LOW);
        break;

      default:
        // Otherwise it is a message we aren't interested in
        break;
    }
  }

#ifdef TEST
  printNote (testnote);
  testnote++;
  if (testnote > 127) testnote=21;
  delay(100);
#endif
}

// Allow for each display element to be 4 characters wide as follows:
//    Note-Sharp-Octave-Space
//
// Example:
//    A#1 C 2 D#3 D 3
//
#define NEXTCOL 4
char notes[12]  = {' ','C',' ','D',' ',' ','F',' ','G',' ','A',' '};
char sharps[12] = {'C','#','D','#','E','F','#','G','#','A','#','B'};
void printNote (uint8_t note) {
  // MIDI notes go from A0 = 21 up to G9 = 127
  tft.print(notes[note%12]);
  tft.print(sharps[note%12]);
  tft.print((note/12)-1,DEC);
  tft.print(" ");
  col = col+NEXTCOL;
  if (col >= tft_cols) {
    col = 0;
    tft.println();
    row++;
    if (row == tft_rows/2) {
      printClear(2);
    }
    if (row >= tft_rows) {
      // wrap back around to the top
      row = 0;
      tft.setCursor(0,0);
      printClear(1);
    }
  }
}

void printInit (void) {
  tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setTextSize(1);
  tft.setCursor(0,0);
  col = 0;
  row = 0;
  tft.fillScreen(ST77XX_BLACK);
}

void printClear (int half) {
  // Clears half the screen
  if (half == 1) {
    // first half
    tft.fillRect (0,0, tft_w, tft_h/2, ST77XX_BLACK);
  } else {
    // second half
    tft.fillRect (0,tft_h/2, tft_w, tft_h/2, ST77XX_BLACK);
  }
}
