/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  ST7735 Mini TFT MIDI Display 2
//  https://diyelectromusic.wordpress.com/2020/10/25/tft-midi-display-part-2/
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

// Bitmaps generated using "Paint" and loading the result into
//    http://javl.github.io/image2cpp/
//
#define CLEFS_W  26  // Width of the following bitmap
#define CLEFS_H  84  // Height of the following bitmap
// 'Clefs_26x84', 26x84px
const uint8_t clefs [] PROGMEM = {
  0x00, 0x02, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x1f, 0x80, 0x00, 
  0x00, 0x1c, 0x80, 0x00, 0x00, 0x18, 0xc0, 0x00, 0x00, 0x38, 0x40, 0x00, 0x00, 0x30, 0x40, 0x00, 
  0x00, 0x30, 0x40, 0x00, 0x00, 0x30, 0xc0, 0x00, 0x00, 0x20, 0xc0, 0x00, 0x00, 0x20, 0xc0, 0x00, 
  0x00, 0x21, 0xc0, 0x00, 0x00, 0x21, 0x80, 0x00, 0x00, 0x13, 0x80, 0x00, 0x00, 0x17, 0x80, 0x00, 
  0x00, 0x1f, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 
  0x00, 0xf8, 0x00, 0x00, 0x01, 0xf8, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x00, 
  0x07, 0x80, 0x00, 0x00, 0x0f, 0x0f, 0xc0, 0x00, 0x0f, 0x1f, 0xe0, 0x00, 0x1e, 0x3f, 0xf0, 0x00, 
  0x1c, 0x7f, 0xf8, 0x00, 0x1c, 0xf0, 0x3c, 0x00, 0x38, 0xc2, 0x1c, 0x00, 0x38, 0xc2, 0x0e, 0x00, 
  0x38, 0x82, 0x0e, 0x00, 0x18, 0x82, 0x06, 0x00, 0x18, 0x82, 0x06, 0x00, 0x18, 0x81, 0x06, 0x00, 
  0x0c, 0x41, 0x0c, 0x00, 0x06, 0x21, 0x0c, 0x00, 0x07, 0x01, 0x18, 0x00, 0x01, 0x80, 0x30, 0x00, 
  0x00, 0xe1, 0xe0, 0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xe0, 0x80, 0x00, 0x01, 0xf0, 0x80, 0x00, 0x03, 0xf8, 0x80, 0x00, 0x03, 0xf8, 0x80, 0x00, 
  0x03, 0xf8, 0x80, 0x00, 0x01, 0xf1, 0x00, 0x00, 0x01, 0xc3, 0x00, 0x00, 0x00, 0xe4, 0x00, 0x00, 
  0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x03, 0x07, 0x00, 0x00, 0x06, 0x01, 0xc1, 0x00, 
  0x04, 0x01, 0xc3, 0x80, 0x0c, 0x00, 0xe3, 0x80, 0x0f, 0xc0, 0x60, 0x00, 0x0f, 0xe0, 0x70, 0x00, 
  0x0f, 0xe0, 0x70, 0x00, 0x0f, 0xe0, 0x70, 0x00, 0x07, 0xe0, 0x71, 0x80, 0x03, 0xc0, 0x73, 0x80, 
  0x00, 0x00, 0x71, 0x80, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0xf0, 0x00, 
  0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x03, 0xc0, 0x00, 
  0x00, 0x03, 0x80, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 
  0x00, 0x70, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00
};

// Bitmap for the sharps
#define SHARP_W   14
#define SHARP_H   17
// 'Sharp', 14x17px
const uint8_t sharp [] PROGMEM = {
  0x00, 0x60, 0x0c, 0x60, 0x0c, 0x60, 0x0c, 0x60, 0x0c, 0x7c, 0x0f, 0xfc, 0x7f, 0xe0, 0x7c, 0x60, 
  0x0c, 0x60, 0x0c, 0x7c, 0x0f, 0xfc, 0x7f, 0xe0, 0x7c, 0x60, 0x0c, 0x60, 0x0c, 0x60, 0x0c, 0x60, 
  0x0c, 0x00
};

// Definitions for the positions of notes, sharp notes, and the sharps themselves
#define CLEFS_XO  5  // X offset to place the clefs bitmap
#define CLEFS_YO 12  // Y offset to place the clefs bitmap
#define STAVE_YS 20  // Y offset of the start of the stave
#define STAVE_YO  8  // Y offset between each line in the stave
#define NOTE_YS  12  // Y offset for the first (highest) note on the stave
#define NOTE_YO   4  // Y offset between each note on the stave (should be half the stave distance)
#define NOTE_XO  50  // X offset for the centre of drawn notes
#define NOTE_SXO 80  // X offset for the centre of drawn sharp notes
#define SHARP_XO 60  // X offset for the sharp sign
#define SHARP_YO -8  // Y offset from the note Y coord for the sharp sign
#define NOTE_RAD  3  // Radius for the drawn notes

// Range of notes
// If this changes, there are structures for each note
// later in the code that will need to change too.
#define LOWEST_NOTE  40 // E2
#define HIGHEST_NOTE 81 // A5

// List of active notes
#define MAX_ACTIVE_NOTES 16
uint8_t notes[MAX_ACTIVE_NOTES];

uint8_t testnote=LOWEST_NOTE-4;

void setup(void) {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(TFT_ROT);
  tft_w = tft.width();
  tft_h = tft.height();
  tft_cols = tft_w/FONT_X;
  tft_rows = tft_h/FONT_Y;
  tft.fillScreen(ST77XX_WHITE);

  drawClefs();
  drawStave();

  pinMode(MIDI_LED, OUTPUT);
  digitalWrite(MIDI_LED, LOW);
  
  // This listens to all MIDI channels
  MIDI.begin(MIDI_CHANNEL);
}

void loop(void) {
  bool changed = false;
#ifdef TEST
  delay(500);
  deactivateNote(testnote);
  clearNote(testnote);
  testnote++;
  if (testnote > HIGHEST_NOTE+4) testnote=LOWEST_NOTE-4;
  activateNote(testnote);
  changed = true;
#endif
  if (MIDI.read()) {
    switch(MIDI.getType()) {
      case midi::NoteOn:
        digitalWrite(MIDI_LED, HIGH);
        activateNote(MIDI.getData1());
        digitalWrite(MIDI_LED, LOW);
        changed = true;
        break;

      case midi::NoteOff:
        deactivateNote(MIDI.getData1());
        clearNote(MIDI.getData1());
        changed = true;
        break;

      default:
        // Otherwise it is a message we aren't interested in
        break;
    }
  }
  // Redraw the screen if something has changed
  if (changed) {
    drawStave();
    drawNotes();
  }
}

void activateNote (uint8_t note) {
  for (int i=0; i<MAX_ACTIVE_NOTES; i++) {
    if (notes[i] == 0) {
      // Store the note in the first space slot.
      notes[i] = note;
      return;
    }
  }
}

void deactivateNote (uint8_t note) {
  for (int i=0; i<MAX_ACTIVE_NOTES; i++) {
    if (notes[i] == note) {
      // Remove this note from any slots.
      // Don't stop when we find one, incase we've got out of sync
      // and have old notes lying around.
      notes[i] = 0;
    }
  }
}

void drawStave (void) {
  // draw the stave
  for (int i=0; i<11; i++) {
    if (i!= 5) tft.drawFastHLine(0, STAVE_YS+i*STAVE_YO, tft_w, ST77XX_BLACK);
  }
}

void drawClefs (void) {
  tft.drawBitmap(CLEFS_XO,CLEFS_YO,clefs,CLEFS_W,CLEFS_H,ST77XX_BLACK);
}

void drawNotes (void) {
  for (int i=0; i<MAX_ACTIVE_NOTES; i++) {
    if (notes[i] != 0) {
      printNote(notes[i]);
    }
  }
}

// The calculation for the position of the note will depend
// on the note and octave (naturally), but also the accidental.
//
// This lookup table contains the relative positions for each note.
// As Y coordinates start at the top with 0, the lowest note will
// have the largest offset from the top of the stave.
//
uint8_t notecoords[] = {
  // E,  F, F#,  G, G#,  A, A#,  B, C,  C#, D,  D#,
    24, 23, 23, 22, 22, 21, 21, 20, 19, 19, 18, 18, // E2-D#3
    17, 16, 16, 15, 15, 14, 14, 13, 12, 12, 11, 11, // E3-D#4
    10,  9,  9,  8,  8,  7,  7,  6,  5,  5,  4,  4, // E4-D#5
     3,  2,  2,  1,  1,  0                          // E5-A5
};
bool sharps [] = {
  // E, F, F#,G, G#,A, A#,B, C, C#,D, D#,
     0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1,
     0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1,
     0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1,
     0, 0, 1, 0, 1, 0
};
bool ledgerlines [] = {
  // E, F, F#,G, G#,A, A#,B, C, C#,D, D#,
     1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, // C4 = middle C
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 1
};

void fillNote (uint8_t note, int x, int y, int colour) {
  // Draw a two slightly offset circles centred on appropriate line or space
  tft.fillCircle(x, y, NOTE_RAD, colour);
  tft.fillCircle(x+2, y, NOTE_RAD, colour);
  if (ledgerlines[note-LOWEST_NOTE]) {
    tft.drawFastHLine(x-NOTE_RAD*2, y, 2+NOTE_RAD*4, colour);
  }
}

void fillSharp (int noteY, int colour) {
  tft.drawBitmap (SHARP_XO,noteY+SHARP_YO,sharp,SHARP_W,SHARP_H,colour);
}

void printNote (uint8_t note) {
  if ((note < LOWEST_NOTE) || (note > HIGHEST_NOTE)) {
    return;
  }
  // Y coordinates start from 0 at the top, so need to 
  // take the starting Y coordinate for the notes and
  // add the distances from the lookup table to get to
  // the right note.
  int noteY = NOTE_YS + notecoords[note-LOWEST_NOTE]*NOTE_YO;
  if (noteY != 0) {
    if (sharps[note-LOWEST_NOTE]) {
      fillNote(note, NOTE_SXO, noteY, ST77XX_BLACK);
      fillSharp(noteY, ST77XX_BLACK);
    } else {
      fillNote(note, NOTE_XO, noteY, ST77XX_BLACK);
    }
  }
}

void clearNote (uint8_t note) {
  if ((note < LOWEST_NOTE) || (note > HIGHEST_NOTE)) {
    return;
  }
  // See comments in printNote about Y coordinates of notes
  int noteY = NOTE_YS + notecoords[note-LOWEST_NOTE]*NOTE_YO;
  if (noteY != 0) {
    if (sharps[note-LOWEST_NOTE]) {
      fillNote(note, NOTE_SXO, noteY, ST77XX_WHITE);
      fillSharp(noteY, ST77XX_WHITE);
    } else {
      fillNote(note, NOTE_XO, noteY, ST77XX_WHITE);
    }
  }
}
