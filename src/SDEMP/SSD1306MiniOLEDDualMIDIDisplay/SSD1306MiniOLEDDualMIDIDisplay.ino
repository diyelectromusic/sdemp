/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  SSD1306 Mini OLED Dual MIDI Display
//  https://diyelectromusic.wordpress.com/2020/10/30/oled-midi-display-part-2/
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
    Arduino MIDI Library     - https://github.com/FortySevenEffects/arduino_midi_library
    Adafruit SSD1306 Library - https://learn.adafruit.com/monochrome-oled-breakouts/arduino-library-and-examples
    Adafruit GFX Library     - https://learn.adafruit.com/adafruit-gfx-graphics-library

*/
#include <MIDI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

//#define TEST 1

// MIDI Channel to listen on
#define MIDI_CHANNEL MIDI_CHANNEL_OMNI

// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_LED LED_BUILTIN

// Set the screen direction, relative to use of the DIY shield.
//   0 = landscape (pins at top)
//   1 = portrait (pins on the left)
//   2 = landscape (pins at bottom)
//   3 = portrait (pins on the right)
#define OLED_ROT 0

// Dimensions of the OLED screen
#define OLED_W  128
#define OLED_H   32
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
#define OLED_ADDR1 0x3C
#define OLED_ADDR2 0x3D
Adafruit_SSD1306 display1 = Adafruit_SSD1306(OLED_W, OLED_H, &Wire, OLED_RST);
Adafruit_SSD1306 display2 = Adafruit_SSD1306(OLED_W, OLED_H, &Wire, OLED_RST);

// Used to refer to the width and height of the display
uint16_t oled_w;
uint16_t oled_h;

// Default font dimensions
// See: https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#define FONT_X (5+1)  // Characters are 5 wide, with a space
#define FONT_Y 8

// Character positions for text writing
uint8_t oled_rows;
uint8_t oled_cols;
uint8_t col,row;
uint8_t testnote=21;

void setup(void) {
  display1.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR1);
  display2.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR2);

  // Leave the Adafruit library logo on screen for a while
  display1.display();
  display2.display();
  delay(2000);

  // We are assuming the two displays are the same
  display1.setRotation(OLED_ROT);
  display2.setRotation(OLED_ROT);
  oled_w = display1.width();
  oled_h = display1.height();
  oled_cols = oled_w/FONT_X;
  oled_rows = oled_h/FONT_Y;

#ifdef TEST
  display1.clearDisplay();
  display1.setTextColor(SSD1306_WHITE);
  display1.setCursor(0,0);
  display1.print ("W: ");
  display1.println (oled_w);
  display1.print ("H: ");
  display1.println (oled_h);
  display1.print ("C: ");
  display1.println (oled_cols);
  display1.print ("R: ");
  display1.println (oled_rows);
  display1.display();
  display2.clearDisplay();
  display2.setTextColor(SSD1306_WHITE);
  display2.setCursor(0,0);
  display2.print ("W: ");
  display2.println (oled_w);
  display2.print ("H: ");
  display2.println (oled_h);
  display2.print ("C: ");
  display2.println (oled_cols);
  display2.print ("R: ");
  display2.println (oled_rows);
  display2.display();
  delay(10000);
#endif
  
  display1.clearDisplay();
  display1.display();
  display2.clearDisplay();
  display2.display();

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
  if (col < oled_cols) {
    // We are still on display 1
    display1.print(notes[note%12]);
    display1.print(sharps[note%12]);
    display1.print((note/12)-1,DEC);
    display1.print(" ");
  } else {
    // Print on display 2
    display2.print(notes[note%12]);
    display2.print(sharps[note%12]);
    display2.print((note/12)-1,DEC);
    display2.print(" ");
  }
  col = col+NEXTCOL;
  if ((col+NEXTCOL-1 >= oled_cols) && (col < oled_cols)) {
    // We are at the end of display 1 so
    // skip ahead to display 2
    col = oled_cols;
  }
  if (col+NEXTCOL-1 >= oled_cols*2) {
    col = 0;
    display1.println();
    display2.println();
    row++;
    if (row == oled_rows/2) {
      printClear(2);
    }
    if (row >= oled_rows) {
      // wrap back around to the top
      row = 0;
      display1.setCursor(0,0);
      display2.setCursor(0,0);
      printClear(1);
    }
  }
  display1.display();
  display2.display();
}

void printInit (void) {
  col = 0;
  row = 0;
  display1.setTextColor(SSD1306_WHITE,SSD1306_BLACK);
  display1.setTextWrap(false);
  display1.setTextSize(1);
  display1.setCursor(0,0);
  display1.clearDisplay();
  display2.setTextColor(SSD1306_WHITE,SSD1306_BLACK);
  display2.setTextWrap(false);
  display2.setTextSize(1);
  display2.setCursor(0,0);
  display2.clearDisplay();
}

void printClear (int half) {
  // Clears half the screen
  if (half == 1) {
    // first half
    display1.fillRect (0,0, oled_w, oled_h/2, 0);
    display2.fillRect (0,0, oled_w, oled_h/2, 0);
  } else {
    // second half
    display1.fillRect (0,oled_h/2, oled_w, oled_h/2, 0);
    display2.fillRect (0,oled_h/2, oled_w, oled_h/2, 0);
  }
}
