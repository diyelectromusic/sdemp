t/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  ST7735 Mini TFT MIDI Display 3
//  https://diyelectromusic.wordpress.com/2020/11/03/tft-midi-display-part-3/
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

// Colours are defined using a 16-bit pattern:
//    RRRRR-GGGGGG-BBBBB i.e. 5-6-5 bits
//     Red   = 0 to 31
//     Green = 0 to 63
//     Blue  = 0 to 31
//
// The following is a MACRO which creates a short-hand piece
// of code to create the right colour bit-pattern from the
// R, G and B parts.  There is no checking here that RGB are
// in the above ranges though.
//
#define C_RGB(r,g,b) ((uint16_t)(((r)<<11)+((g)<<5)+(b)))
#define C_LINES       C_RGB(10,20,10)
#define C_BACKGROUND  C_RGB(0,0,0)

// Used to refer to the width and height of the display
uint16_t tft_w;
uint16_t tft_h;

// Range of notes
#define LOWEST_NOTE  21  // A0
#define HIGHEST_NOTE 104 // G#7

uint8_t testnote=LOWEST_NOTE-1;

// Display is a 6x12 array of coloured rectangles.
// Following the colouring in the film doesn't seem to make
// musical sense though, so we're just using a display that is
// "inspired by" the film...
//
#define ROWS      7   // Number of colour rows
#define COLS      12  // Number of colour columns
#define ROW_S     30  // Row start
#define ROW_H     5   // Row height
#define ROW_H_SP  1   // Row spacing
#define COL_S     0   // Column start
#define COL_W     11  // Column width
#define COL_W_SP  2   // Column spacing

void drawRectangles (void) {
  tft.fillRect (COL_S, ROW_S, COL_W_SP+(COL_W+COL_W_SP)*COLS, ROW_H_SP+(ROW_H+ROW_H_SP)*ROWS, C_LINES);
  for (int r=0; r<ROWS; r++) {
    for (int c=0; c<COLS; c++) {
      drawNoteRect (r, c, C_BACKGROUND);
    }
  }
}

void drawNoteRect (int r, int c, uint16_t col) {
  tft.fillRect (COL_S+COL_W_SP+(COL_W+COL_W_SP)*c, ROW_S+ROW_H_SP+(ROW_H+ROW_H_SP)*r, COL_W, ROW_H, col);
}

void setup(void) {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(TFT_ROT);
  tft_w = tft.width();
  tft_h = tft.height();
  tft.fillScreen(C_BACKGROUND);

  drawRectangles();
  for (uint8_t note=LOWEST_NOTE; note<=HIGHEST_NOTE; note++) {
    printNote(note);
  }
  delay(10000);
  tft.fillScreen(C_BACKGROUND);
  drawRectangles();

  pinMode(MIDI_LED, OUTPUT);
  digitalWrite(MIDI_LED, LOW);

  // This listens to all MIDI channels
  MIDI.begin(MIDI_CHANNEL);
}

void loop(void) {
  bool changed = false;
#ifdef TEST
  delay(500);
  clearNote(testnote);
  testnote++;
  if (testnote > HIGHEST_NOTE) testnote=LOWEST_NOTE;
  printNote(testnote);
#endif
  if (MIDI.read()) {
    switch(MIDI.getType()) {
      case midi::NoteOn:
        digitalWrite(MIDI_LED, HIGH);
        printNote(MIDI.getData1());
        digitalWrite(MIDI_LED, LOW);
        break;

      case midi::NoteOff:
        clearNote(MIDI.getData1());
        break;

      default:
        // Otherwise it is a message we aren't interested in
        break;
    }
  }
}

void printNote (uint8_t note) {
  if ((note < LOWEST_NOTE) || (note > HIGHEST_NOTE)) {
    return;
  }

  // Calculate the row and column for the note.
  // Note: As we want the lowest notes at the bottom,
  //       need to "invert" the row number by taking
  //       it away from the number of rows.
  //
  int r = ROWS-1-(note-LOWEST_NOTE)/COLS;
  int c = (note-LOWEST_NOTE)%COLS;

  // Calculate the colour as follows:
  //   Red = based on X position (>X more red)
  //   Blue = based on X position (<X more blue)
  //   Green = based on Y position
  //
  // All scaled to 0 to 31 (RB) or 0 to 63 (G)
  //
  uint16_t cr = c*31/COLS;
  uint16_t cg = r*63/ROWS;
  uint16_t cb = 31-(c*31/COLS);
  drawNoteRect(r, c, C_RGB(cr,cg,cb));
}

void clearNote (uint8_t note) {
  if ((note < LOWEST_NOTE) || (note > HIGHEST_NOTE)) {
    return;
  }
  
  // Calculate the row and column for the note
  int r = ROWS-1-(note-LOWEST_NOTE)/COLS;
  int c = (note-LOWEST_NOTE)%COLS;
  drawNoteRect(r, c, C_BACKGROUND);
}
