/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Touchscreen MIDI Controller - Part 2
//  https://diyelectromusic.wordpress.com/2021/08/07/arduino-touchscreen-midi-controller-part-2/
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

//#define TEST 1   // Uncomment for generatl testing
//#define MTEST 1  // Uncomment this too to check the maths!

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1  // Define which MIDI channel to transmit on (1 to 16).

#define NUMPOTS 12
int cc[NUMPOTS] = {
  0x01,  // Modulation wheel
//  0x02,  // Breath controller
//  0x04,  // Foot controller
//  0x05,  // Portamento time
//  0x07,  // Channel volume
  0x08,  // Balance
  0x0A,  // Pan
  0x0B,  // Expression control
  0x0C,  // Effect control 1
  0x0D,  // Effect control 2
  0x10,  // General purpose control 1
  0x11,  // General purpose control 2
  0x12,  // General purpose control 3
  0x13,  // General purpose control 4
  0x14,  // General purpose control 5
  0x15,  // General purpose control 6
//  0x16,  // General purpose control 7
//  0x17,  // General purpose control 8
};
int potval[NUMPOTS];
int potlast[NUMPOTS];
int cclast[NUMPOTS];


// XPT2046 Touchscreen Definitions
//
#define T_CS  8
// MOSI=11, MISO=12, SCK=13
#define T_IRQ  -1  //  Set to 255 to disable interrupts and rely on software polling
XPT2046_Touchscreen ts(T_CS, T_IRQ);
#define T_MAX 3850UL  // Should be 4096, but by experiment, working to this value
#define T_MIN  180UL  // Should be 0, but by experiment, working to this value

// ILI9488 Display Definitions
//
#define ILI_CS  10
#define ILI_DC  7
#define ILI_RST 9
Arduino_DataBus *bus = new Arduino_HWSPI(ILI_DC, ILI_CS);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, ILI_RST, 0 /* rotation */, false /* IPS */);

// If the resolution or rotation changes, then the gfx
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

#ifdef MTEST
  Serial.print("ILI9844 Touchscreen ");
  Serial.print(w);
  Serial.print("x");
  Serial.println(h);
#endif
}

void gfxLoop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int pot = x2pot(xt2xdsp(p.x));
    int pval = y2val(p.y);
    if ((pot != -1) && (pval != -1)) {
      potval[pot] = pval;
      gfxPotValue(pot);
      potlast[pot] = potval[pot];
    }
#ifdef MTEST
    Serial.print("gfxLoop: [");
    Serial.print(p.x);
    Serial.print(",");
    Serial.print(p.y);
    Serial.print("]\tPot=");
    Serial.print(pot);
    Serial.print("\tVal=");
    Serial.println(pval);
#endif
  }
}

// Translate pot number into a set of on-screen slider pots.
//
// Assuming a 320x480 display in landscape mode.
// For 12 sliders the following "design" is being used:
//   Width of pots 30px, so total 360px is pots.
//   Height (length) of pots is 300px.
//   Gap between pots is 10px, so total of 110px between buttons.
//   This leaves top/bottom gap of (320-300)/2 = 10
//   And left/right gap of (480-360-110)/2 = 5
//
#define POT_W     30
#define POT_X_MIN 5
#define POT_X_GAP 10
#define POT_Y_MIN 5   // Must be > POT_BAR/2
#define POT_H     300
#define POT_BAR   6

void gfxPotInit (int pot) {
  int xc = pot2xc(pot);

#ifdef MTEST
  Serial.print("gfxPotInit ");
  Serial.print(pot);
  Serial.print("\t");
  Serial.println(xc);
#endif

  // Background to the pot goes "full height"
  gfx->fillRect(xc, 0, POT_W, h, LIGHTGREY);
}

void gfxPotValue (int pot) {
  int xc = pot2xc(pot);
  int oldyc = pot2yc(potlast[pot]);
  int yc = pot2yc(potval[pot]);
#ifdef MTEST
  Serial.print("gfxPotValue ");
  Serial.print(pot);
  Serial.print("\t");
  Serial.print(xc);  
  Serial.print("\t");
  Serial.println(yc);
#endif

  gfx->fillRect(xc, oldyc-POT_BAR/2, POT_W, POT_BAR, LIGHTGREY);
  gfx->fillRect(xc, yc-POT_BAR/2, POT_W, POT_BAR, GREEN);
}

int pot2xc (int pot) {
  // Need remainder after dividing by the number of pots,
  // which will give us the position within the row.
  return POT_X_MIN+(POT_X_GAP+POT_W)*(pot % NUMPOTS);
}

int pot2yc (int val) {
  // Use standard pot values of 0 to 1023 for scaling.
  return (int)(POT_Y_MIN+((long)POT_H*(long)val)/1024L);
}

int x2pot (int xc) {
  // Find the pot number corresponding to a specific X coord
  //    XC MIN < pot xc < XC MIN + POT width
  //
  // If outside the range, returns -1.
  // If inside the gap, returns -1.
  //
  // Ensure pots are in the range.
  if ((xc < POT_X_MIN) || (xc > POT_X_MIN+(POT_W+POT_X_GAP)*NUMPOTS-POT_X_GAP)) return -1;

  xc = xc - POT_X_MIN;

  int xbtn = -1;
  for (int i=0; i<NUMPOTS && xc>0; i++) {
    if (xc < POT_W) {
      // We're in the space for this pot
      xbtn = i;
      break;
    }
    xc = xc - (POT_W+POT_X_GAP);
  }

  return xbtn;
}

int y2val (int yc) {
  // Translated Y coord into a 0 to 1023 range similar to an analogRead().
  // NOTE: This is NOT the display Y coord (0 to 319), but the TOUCH
  //       Y coord, which goes from 0 to T_MAX.  This means we don't lose
  //       resolution, but does mean some translating is required...
  //
  // If outside the range, returns -1.
  //
  // Ensure pots are in the range.
  //
  // Actually, I cheat here and tweak the "touch" range manually to 
  // match what seems to come back for better control/accuracy.
  //
  int t_min = T_MIN;
  int t_max = T_MAX;
//  int t_min = (int)((T_MAX*(long)POT_Y_MIN)/(long)h);
//  int t_max = (int)((T_MAX*((long)POT_Y_MIN+(long)POT_H))/(long)h);
  if (yc < t_min) yc = t_min;
  if (yc > t_max) yc = t_max;

  long t_h = t_max-t_min;
  int retval = (int)(((long)yc - (long)t_min)*1023L/t_h);
#ifdef MTEST
  Serial.print("y2val: ");
  Serial.print(yc);
  Serial.print("\t");
  Serial.print(t_min);
  Serial.print(" < ");
  Serial.print(yc);
  Serial.print(" < ");
  Serial.print(t_max);
  Serial.print("\t(");
  Serial.print(t_h);
  Serial.print(")\tRet=");
  Serial.println(retval);
#endif
  return retval;
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
  MIDI.begin(MIDI_CHANNEL_OMNI);
#endif

  gfxSetup();

  for (int i=0; i<NUMPOTS; i++) {
    potval[i] = 1023;  // Prefix to "high"
    potlast[i] = 1023;
    gfxPotInit(i);
    gfxPotValue(i);
  }
}

int potcnt;
void loop() {
  // As this is a controller, we process any MIDI messages
  // we receive and allow the MIDI library to enact it's THRU
  // functionality to pass them on.
  MIDI.read();

  gfxLoop();

  potcnt++;
  if (potcnt >= NUMPOTS) potcnt = 0;

  // The virtual pots are configured "backwards" i.e. zero at the TOP
  // so reverse then when turning the pot values into CC messages.
  //
  // Also, there remain inaccuracies in the readings here when using
  // the touchscreen, so I re-map the full analog range into a smaller range.
  //
  // Also note the additional use of constrain() to ensure the values
  // are within the range passed to the map() function.
  //
  int ccval = map(constrain(potval[potcnt], 100, 900), 100, 900, 127, 0);
  if (ccval != cclast[potcnt]) {
    // The value for this pot has changed so send a new MIDI message
#ifdef TEST
    Serial.print(potcnt);
    Serial.print(" CC 0x");
    if (cc[potcnt]<16) Serial.print("0");
    Serial.print(cc[potcnt],HEX);
    Serial.print(" -> ");
    Serial.println(ccval);
#else
    MIDI.sendControlChange (cc[potcnt], ccval, MIDI_CHANNEL);
#endif
    cclast[potcnt] = ccval;
  }
}
