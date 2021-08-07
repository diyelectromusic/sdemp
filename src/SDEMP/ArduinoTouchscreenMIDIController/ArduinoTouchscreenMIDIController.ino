/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Touchscreen MIDI Controller
//  https://diyelectromusic.wordpress.com/2021/08/07/arduino-touchscreen-midi-controller/
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
    ILI9488 Touchscreen - https://emalliab.wordpress.com/2021/08/06/cheap-ili9488-tft-lcd-displays/
*/
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Arduino_GFX_Library.h>
#include <MIDI.h>

//#define TEST 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1  // Define which MIDI channel to transmit on (1 to 16).

#define NUMBTNS 36    // 6x6 Matrix
#define NUMBTNSROW 6  // 6 rows
#define NUMBTNSCOL 6  // 6 in each row

// Set the MIDI codes for the notes to be played by each key.
// Note that the Y direction goes from TOP to BOTTOM, so
// put the lowest notes at the bottom of this table.
// Lowest notes still go on the left though.
byte notes[NUMBTNS] = {
  84, 86, 88, 91, 93, 96, // Pentatonic C6-C7
  72, 74, 76, 79, 81, 84, // Pentatonic C5-C6
  60, 62, 64, 67, 69, 72, // Pentatonic C4-C5
  48, 50, 52, 55, 57, 60, // Pentatonic C3-C4
  36, 38, 40, 43, 45, 48, // Pentatonic C2-C3
  24, 26, 28, 31, 33, 36, // Pentatonic C1-C2
};

byte lastKey;

// Counter for debouncing, number of scans for button to be
// shown as "on" and number of scans between MIDI note On/Off events.
//
#define BTN_ON 1000
unsigned int btns[NUMBTNS];
unsigned int lastbtns[NUMBTNS];

// XPT2046 Touchscreen Definitions
//
#define T_CS  8
// MOSI=11, MISO=12, SCK=13
#define T_IRQ  -1  //  Set to 255 to disable interrupts and rely on software polling
XPT2046_Touchscreen ts(T_CS, T_IRQ);

// ILI9488 Display Definitions
//
#define ILI_CS  10
#define ILI_DC  7
#define ILI_RST 9
Arduino_DataBus *bus = new Arduino_HWSPI(ILI_DC, ILI_CS);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, ILI_RST, 0 /* rotation */, false /* IPS */);

// If the resolution or rotation changes, then the gfxBtnXX
// routines will need re-writing to cope.
unsigned w, h;
#define ROTATION 1  // Want display in landscape mode, USB port on right

void gfxSetup() {
  // Pre-initialise the "chip select" pins to "deselected" (HIGH)
  pinMode(ILI_CS, OUTPUT);
  pinMode(T_CS, OUTPUT);
  digitalWrite(ILI_CS, HIGH);
  digitalWrite(T_CS, HIGH);
  delay(10);
  
  ts.begin();
  ts.setRotation(ROTATION);

  gfx->begin();

  gfx->setRotation(ROTATION);
  w = gfx->width();
  h = gfx->height();
  gfx->fillScreen(BLACK);

#ifdef TEST
  Serial.print("ILI9844 Touchscreen ");
  Serial.print(w);
  Serial.print("x");
  Serial.println(h);
#endif
}

void gfxLoop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int btn = xy2btn(xt2xdsp(p.x), yt2ydsp(p.y));
    if (btn != -1) {
      gfxBtnOn (btn);
      btns[btn] = BTN_ON;
    }
  }

  // "timeout" any buttons that are on if required
  for (int i=0; i<NUMBTNS; i++) {
    if (btns[i] > 0) {
      btns[i]--;
      if (btns[i] == 0) {
        gfxBtnOff(i);
      }
    }
  }
}

// Translate btn number into a 6x6 matrix of on-screen buttons.
//
// Assuming a 320x480 display in landscape mode.
// For a 6x6 grid of buttons, following "design"
// is being used:
//   Radius of buttons 20px, so total 240px is buttons.
//   Gap between buttons is 10px or 20px, so total of 50px or 100px between buttons.
//   This leaves top/bottom gap of (320-240-50)/2 = 15
//   And left/right gap of (480-240-100)/2 = 60
//
// Then "by eye" added a few adjustments too...
//
#define BTN_RAD   20
#define BTN_X_MIN (60+BTN_RAD)
#define BTN_X_GAP (20+5)
#define BTN_Y_MIN (15+BTN_RAD)
#define BTN_Y_GAP 10
void gfxBtnOff (int btn) {
  int xc = btn2xc(btn);  
  int yc = btn2yc(btn);

#ifdef TEST
  Serial.print("gfxBtnOff ");
  Serial.print(btn);
  Serial.print("\t");
  Serial.print(xc);  
  Serial.print("\t");
  Serial.println(yc);
#endif

  gfx->fillCircle(xc, yc, BTN_RAD, BLACK);
  gfx->drawCircle(xc, yc, BTN_RAD, GREEN);
}

void gfxBtnOn (int btn) {
  int xc = btn2xc(btn);  
  int yc = btn2yc(btn);
#ifdef TEST
  Serial.print("gfxBtnOn ");
  Serial.print(btn);
  Serial.print("\t");
  Serial.print(xc);  
  Serial.print("\t");
  Serial.println(yc);
#endif

  gfx->fillCircle(xc, yc, BTN_RAD, GREEN);
}

int btn2xc (int btn) {
  // Need remainder after dividing by the number of rows,
  // which will give us the position within the row.
  return BTN_X_MIN+(BTN_X_GAP+2*BTN_RAD)*(btn % NUMBTNSROW);
}

int btn2yc (int btn) {
  // Need to divide by number of columns to give the row number
  return BTN_Y_MIN+(BTN_Y_GAP+2*BTN_RAD)*(btn/NUMBTNSCOL);
}

int xy2btn (int xc, int yc) {
  // Find the button number corresponding to a specific (X,Y) coord
  // Use a square bounding box for each button of:
  //    XC-RAD < btn xc < XC+RAD
  //    YC-RAD < btn yc < YX+RAD
  //
  // If outside the range, returns -1.
  // If inside the gap, returns -1.
  //
  // Algorithm:
  //   Take coordinate, subtract minimum (BTN_MIN-BTN_RAD)
  //   Divide by BTN+GAP sizes
  //   That should be position in that dimension
  //
#ifdef TEST
  Serial.print("xy2btn: ");
  Serial.print(xc);
  Serial.print(",");
  Serial.println(yc);  
#endif
  // Ensure buttons are in the range.
  // Note: we are using "2*radius + Gap" as the "size of button" for
  //       calculation purposes here, remembering to subtract the last "gap".
  //
  if ((xc < BTN_X_MIN-BTN_RAD) || (xc > BTN_X_MIN-BTN_RAD+(2*BTN_RAD+BTN_X_GAP)*NUMBTNSCOL-BTN_X_GAP)) return -1;
  if ((yc < BTN_Y_MIN-BTN_RAD) || (yc > BTN_Y_MIN-BTN_RAD+(2*BTN_RAD+BTN_Y_GAP)*NUMBTNSROW-BTN_Y_GAP)) return -1;

  xc = xc - (BTN_X_MIN-BTN_RAD);
  yc = yc - (BTN_Y_MIN-BTN_RAD);

  int xbtn = -1;
  int ybtn = -1;
  for (int i=0; i<NUMBTNSCOL && xc>0; i++) {
    if (xc < 2*BTN_RAD) {
      // We're in the space for this button
      xbtn = i;
      break;
    }
    xc = xc - (2*BTN_RAD+BTN_X_GAP);
  }
  for (int i=0; i<NUMBTNSROW && yc>0; i++) {
    if (yc < 2*BTN_RAD) {
      // We're in the space for this button
      ybtn = i;
      break;
    }
    yc = yc - (2*BTN_RAD+BTN_Y_GAP);
  }

  // This ought to go after the next IF, but it doens't really
  // matter and if here, we can include it in the debug output.
  int btn = ybtn*NUMBTNSCOL + xbtn;

#ifdef TEST
  Serial.print("xy2btn: ");
  Serial.print(xbtn);
  Serial.print(" : ");
  Serial.print(ybtn);
  Serial.print(" --> ");
  Serial.println(btn);
#endif
  if ((xbtn == -1) || (ybtn == -1)) {
    // we're out of range
    return -1;
  }

  return btn;
}

unsigned xt2xdsp(unsigned touch_x) {
  return (unsigned)((((unsigned long)w)*touch_x)/4096UL);
}

unsigned yt2ydsp(unsigned touch_y) {
  return (unsigned)((((unsigned long)h)*touch_y)/4096UL);
}

#ifdef TEST
void showBtnCoords() {
  Serial.println("Btn Coords: ");
  for (int i=0; i<NUMBTNS; i++) {
    Serial.print(i);
    Serial.print("\t");
    Serial.print(btn2xc(i));
    Serial.print("\t");
    Serial.print(btn2yc(i));
    Serial.print("\n");
  }

  Serial.print("\nLimits: ");
  Serial.print(BTN_X_MIN-BTN_RAD);
  Serial.print(" < xc < ");
  Serial.print(BTN_X_MIN-BTN_RAD+(2*BTN_RAD+BTN_X_GAP)*NUMBTNSCOL-BTN_X_GAP);
  Serial.print("\t");
  Serial.print(BTN_Y_MIN-BTN_RAD);
  Serial.print(" < xc < ");
  Serial.print(BTN_Y_MIN-BTN_RAD+(2*BTN_RAD+BTN_Y_GAP)*NUMBTNSROW-BTN_Y_GAP);
  Serial.print("\n");

  Serial.print("\nX Values:");
  int xc = BTN_X_MIN-BTN_RAD;
  for (int i=0; i<NUMBTNSCOL; i++) {
    Serial.print("\t");
    Serial.print(xc + (2*BTN_RAD+BTN_X_GAP)*i);
  }
  Serial.print("\nY Values:");
  int yc = BTN_Y_MIN-BTN_RAD;
  for (int i=0; i<NUMBTNSROW; i++) {
    Serial.print("\t");
    Serial.print(yc + (2*BTN_RAD+BTN_Y_GAP)*i);
  }
  Serial.print("\n\n");
}
#endif

void setup() {
#ifdef TEST
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL_OFF);
#endif

  gfxSetup();

  for (int i=0; i<NUMBTNS; i++) {
    gfxBtnOff(i);
  }

#ifdef TEST
  showBtnCoords();
#endif

  lastKey = -1;
}

void loop() {
  gfxLoop();

  // Check each button to see what has changed
  for (int i=0; i<NUMBTNS; i++) {
    if (btns[i]>0 && lastbtns[i]==0) {
      // Start playing this note
#ifdef TEST
      Serial.print("NoteOn: ");
      Serial.println(notes[i]);
#else
      MIDI.sendNoteOn(notes[i], 127, MIDI_CHANNEL);
#endif
    } else if (lastbtns[i]>0 && btns[i]==0) {
      // Stop playing this note
#ifdef TEST
      Serial.print("NoteOff: ");
      Serial.println(notes[i]);
#else
      MIDI.sendNoteOff(notes[i], 0, MIDI_CHANNEL);
#endif
    } else {
      // Nothing has changed
    }
    lastbtns[i] = btns[i];
  }
}
