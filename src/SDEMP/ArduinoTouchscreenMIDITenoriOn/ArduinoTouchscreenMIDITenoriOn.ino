/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Touchscreen MIDI Tenori-On
//  https://diyelectromusic.wordpress.com/2021/08/17/arduino-touchscreen-mini-midi-tenori-on/
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

// Set the tempo in beats per minute.
// Note: There is a maximum above which the limiting factor
//       is the drawing of the screen, but it should be good
//       up to around 240bpm or thereabouts
#define TEMPO 240  // beats per minute

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1  // Define which MIDI channel to transmit on (1 to 16).

#define NUMBTNS  160 // 10x16 Matrix
#define NUMNOTES  10 // 10 rows of notes
#define NUMSTEPS  16 // 16 notes in each row - so 16 steps for the music

// Set the MIDI codes for the notes to be played by each key.
// Note that the Y direction goes from TOP to BOTTOM, so
// put the lowest notes at the bottom of this table.
// Lowest notes still go on the left though.
const PROGMEM byte notes[NUMBTNS] = {
  // Pentatonic C3-C4
  60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
  57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57,
  55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
  52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
  50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
  48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
  45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
  43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
  40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
  38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38,
};

unsigned int btns[NUMBTNS];

// XPT2046 Touchscreen Definitions
//
#define T_CS  8
// MOSI=11, MISO=12, SCK=13
#define T_IRQ  -1  //  Set to 255 to disable interrupts and rely on software polling
XPT2046_Touchscreen ts(T_CS, T_IRQ);
#define T_MAX 3800UL  // Should be 4096, but by experiment, working to this value
#define T_MIN  200UL  // Should be 0, but by experiment, working to this value

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

int debounce;
void gfxLoop() {
  if (debounce > 0) {
    debounce--;
    return;
  }
  if (ts.touched()) {
    debounce = 50;
    TS_Point p = ts.getPoint();
#ifdef TEST
    Serial.print("[");
    Serial.print(p.x);
    Serial.print(",");
    Serial.print(p.y);
    Serial.print("]\n");
#endif
    int btn = xy2btn(xt2xdsp(p.x), yt2ydsp(p.y));
    if (btn != -1) {
      if (btns[btn]) {
        gfxBtnOff (btn);
        btns[btn] = 0;
      } else {
        gfxBtnOn (btn);
        btns[btn] = 1;
      }
    }
  }
}

// Translate btn number into a matrix of on-screen buttons.
//
// Assuming a 320x480 display in landscape mode.
// For the grid of buttons, following "design"
// is being used:
//   Radius of buttons 10px, so:
//      Total V space of 10x10x2 = 200px is buttons.
//      Total H space of 16x10x2 = 360px is buttons.
//   Gap between buttons is 8px (H) and 10px (V), so:
//      Total V gaps is (10-1)x10 = 90px between buttons.
//      Total H gaps is (16-1)x8 = 120px between buttons.
//   This leaves top/bottom gap of (320-200-90)/2 = 15
//   And left/right gap of (480-360-120)/2 = 0
//
// Then there are a few "by eye" adjustments too
//
#define BTN_RAD   10
#define BTN_X_MIN (10+BTN_RAD)
#define BTN_X_GAP (8)
#define BTN_Y_MIN (15+BTN_RAD)
#define BTN_Y_GAP (10)
void gfxBtnOff (int btn) {
  int xc = btn2xc(btn);  
  int yc = btn2yc(btn);
  gfx->fillCircle(xc, yc, BTN_RAD, BLACK);
  gfx->drawCircle(xc, yc, BTN_RAD, GREEN);
}

void gfxBtnOn (int btn) {
  int xc = btn2xc(btn);  
  int yc = btn2yc(btn);
  gfx->fillCircle(xc, yc, BTN_RAD, GREEN);
}

void gfxBtnPlay (int btn) {
  int xc = btn2xc(btn);  
  int yc = btn2yc(btn);
  gfx->fillCircle(xc, yc, BTN_RAD, BLUE);
}

void gfxBtnStop (int btn) {
  // Depending on status of the btn, revert to its previous display state
  if (btns[btn]) {
    gfxBtnOn(btn);
  } else {
    gfxBtnOff(btn);
  }  
}

int btn2xc (int btn) {
  // Need remainder after dividing by the number of columns,
  // which will give us the position within the row.
  return BTN_X_MIN+(BTN_X_GAP+2*BTN_RAD)*(btn % NUMSTEPS);
}

int btn2yc (int btn) {
  // Need to divide by number of columns to give the row number
  return BTN_Y_MIN+(BTN_Y_GAP+2*BTN_RAD)*(btn/NUMSTEPS);
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
  Serial.print("xy2btn: xc=");
  Serial.print(xc);
  Serial.print(", yc=");
  Serial.print(yc);
  Serial.print("\t");
#endif
  // Ensure buttons are in the range.
  // Note: we are using "2*radius + Gap" as the "size of button" for
  //       calculation purposes here, remembering to subtract the last "gap".
  //
  if ((xc < BTN_X_MIN-BTN_RAD) || (xc > BTN_X_MIN-BTN_RAD+(2*BTN_RAD+BTN_X_GAP)*NUMSTEPS-BTN_X_GAP)) return -1;
  if ((yc < BTN_Y_MIN-BTN_RAD) || (yc > BTN_Y_MIN-BTN_RAD+(2*BTN_RAD+BTN_Y_GAP)*NUMNOTES-BTN_Y_GAP)) return -1;

  xc = xc - (BTN_X_MIN-BTN_RAD);
  yc = yc - (BTN_Y_MIN-BTN_RAD);

  int xbtn = -1;
  int ybtn = -1;
  for (int i=0; i<NUMSTEPS && xc>0; i++) {
    if (xc < 2*BTN_RAD) {
      // We're in the space for this button
      xbtn = i;
      break;
    }
    xc = xc - (2*BTN_RAD+BTN_X_GAP);
  }
  for (int i=0; i<NUMNOTES && yc>0; i++) {
    if (yc < 2*BTN_RAD) {
      // We're in the space for this button
      ybtn = i;
      break;
    }
    yc = yc - (2*BTN_RAD+BTN_Y_GAP);
  }

  // This ought to go after the next IF, but it doens't really
  // matter and if here, we can include it in the debug output.
  int btn = ybtn*NUMSTEPS + xbtn;

#ifdef TEST
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
  if (touch_x < T_MIN) touch_x = T_MIN;
  return (unsigned)((((unsigned long)w)*((unsigned long)touch_x-T_MIN))/(T_MAX-T_MIN));
}

unsigned yt2ydsp(unsigned touch_y) {
  if (touch_y < T_MIN) touch_y = T_MIN;
  return (unsigned)((((unsigned long)h)*((unsigned long)touch_y-T_MIN))/(T_MAX-T_MIN));
}

void setup() {
#ifdef TEST
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL_OFF);
#endif

  gfxSetup();

  for (int i=0; i<NUMBTNS; i++) {
    btns[i] = 0;
    gfxBtnOff(i);
  }
}

unsigned long nextbeat;
int notestep;
void loop() {
  gfxLoop();

  // Check the tempo
  unsigned long timenow = millis();
  if (timenow < nextbeat) {
    // return and wait for the tempo to "tick over" to the next beat
    return;
  }
  // Turn the tempo (beats per minute) into milliseconds
  nextbeat = timenow + 60000UL/(unsigned long)TEMPO;

  // Process previous note graphics.
  // NB: I do graphics and MIDI seperately so that the
  //     MIDI notes are played more at the same time as each
  //     other and not affected by the time it takes to draw
  //     to the display.
  for (int i=0; i<NUMNOTES; i++) {
    int btnidx = i*NUMSTEPS + notestep;
    gfxBtnStop(btnidx);
  }

  // Process previous note off messages
  for (int i=0; i<NUMNOTES; i++) {
    int btnidx = i*NUMSTEPS + notestep;
    // We could check here:
    //     if (btns[btnidx] != 0)
    //
    // But as the playing notes can change on user
    // input at any time, to be sure, we always send
    // note off messages for any of the previous step's notes.
    //
    byte nn = pgm_read_byte(&notes[btnidx]);
#ifndef TEST
    MIDI.sendNoteOff(nn, 0, MIDI_CHANNEL);
#endif
  }

  gfxLoop();

  // Move on to the next step
  notestep++;
  if (notestep >= NUMSTEPS) notestep = 0;

  // And start playing any new notes
  for (int i=0; i<NUMNOTES; i++) {
    int btnidx = i*NUMSTEPS + notestep;
    if (btns[btnidx] != 0) {
      // Start playing this note
      byte nn = pgm_read_byte(&notes[btnidx]);
#ifndef TEST
      MIDI.sendNoteOn(nn, 127, MIDI_CHANNEL);
#endif
    }
  }

  // Finally update the display with the new notes
  for (int i=0; i<NUMNOTES; i++) {
    int btnidx = i*NUMSTEPS + notestep;
    gfxBtnPlay(btnidx);
  }
}
