/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Touchscreen X-Y MIDI Controller
//  https://diyelectromusic.wordpress.com/2022/06/23/arduino-touchscreen-x-y-midi-controller/
//
      MIT License
      
      Copyright (c) 2022 diyelectromusic (Kevin)
      
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

//#define TEST 1     // Uncomment for general testing
//#define MIDITEST 1 // Uncomment for MIDI messages printed to Serial
//#define MTEST 1    // Uncomment this too to check the maths!
//#define PBTEST 1   // Uncomment for pitch bend checking

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1  // Define which MIDI channel to transmit on (1 to 16).

// Treat the X and Y directions as a MIDI control signal, using any of the available MIDI CC messages.
// Optional: Also treat the X direction as a MIDI note number to be played on start of touch.
// Optional: Treat X direction as pitch bend
//#define Y_MIDI_CC  1  // Modulation Wheel
//#define Y_MIDI_CC 11  // Expression control
//#define Y_MIDI_CC 12  // Effect control 1
//#define Y_MIDI_CC 13  // Effect control 2
//#define Y_MIDI_CC 16  // General purpose control 1
#define Y_MIDI_CC 74  // Filter cut-off
//#define X_MIDI_CC 1
#define MIDI_PITCH_BEND // Uncomment for pitch bend

uint8_t xcclast;
uint8_t ycclast;
uint8_t playing;
int ypotval;
int ypotlast;
int xpotval;
int xpotlast;
int xc_base;
int pbval;
int pbvallast;

// Comment out if using just as an X-Y controller with no note values to play
#define NUM_NOTES 8
uint8_t notes [NUM_NOTES] = {60, 62, 64, 65, 67, 69, 71, 72};
//#define NUM_NOTES 13
//uint8_t notes [NUM_NOTES] = {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72};

#ifndef NUM_NOTES
#define NUM_LINES 8
#else
#define NUM_LINES NUM_NOTES
#endif

#define NONOTE (255)

// XPT2046 Touchscreen Definitions
//
#define T_CS  8
// MOSI=11, MISO=12, SCK=13
#define T_IRQ  -1  //  Set to 255 to disable interrupts and rely on software polling
XPT2046_Touchscreen ts(T_CS, T_IRQ);
#define T_MAX 3850UL  // Should be 4095, but by experiment, working to this value
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

  gfxInit();

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
    ypotval = xy2val(p.y);
    xpotval = xy2val(p.x);
    x2pb(p.x);

#ifdef NUM_NOTES
    int note = x2note(xt2xdsp(p.x));
    if ((note != NONOTE) && (playing == NONOTE)) {
      // Start a new note playing
      midiNoteOn (notes[note]);
      gfxNoteOn (note);
      playing = note;
    }
#endif

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
  } else {
    // No touch
#ifdef NUM_NOTES
    if (playing != NONOTE) {
      // NoteOff
      midiNoteOff (notes[playing]);
      gfxNoteOff (playing);
      playing = NONOTE;
    }
#endif
    reset_x2pb();
  }
}

void gfxInit () {
  // Draw the lines between notes
  int note_width = w / NUM_LINES;
  for (int wi=note_width; wi<w; wi+=note_width) {
    gfx->drawFastVLine(wi-1, 0, h, GREEN);
    gfx->drawFastVLine(wi, 0, h, GREEN);
    gfx->drawFastVLine(wi+1, 0, h, GREEN);
  }
}

void gfxNoteOn (int note) {
}

void gfxNoteOff (int note) {
}

int x2note (int xc) {
  // Find the note number corresponding to a specific X coord
  //    XC MIN < note xc < XC MIN + NOTE width
  //
  // If outside the range, returns NONOTE.
  //
  int note_width = w / NUM_LINES;

  int xbtn = NONOTE;
  for (int i=0; i<NUM_LINES && xc>0; i++) {
    if (xc < note_width) {
      // We're in the space for this pot
      xbtn = i;
      break;
    }
    xc = xc - note_width;
  }

  return xbtn;
}

int xy2val (int xyc) {
  // Translated X/Y coord into a 0 to 1023 range similar to an analogRead().
  // NOTE: This is NOT the display X/Y coord (0 to 319), but the TOUCH
  //       X/Y coord, which goes from 0 to T_MAX.  This means we don't lose
  //       resolution, but does mean some translating is required...
  //
  // Actually, I cheat here and tweak the "touch" range manually to 
  // match what seems to come back for better control/accuracy.
  //
  int t_min = T_MIN;
  int t_max = T_MAX;
//  int t_min = (int)((T_MAX*(long)POT_Y_MIN)/(long)h);
//  int t_max = (int)((T_MAX*((long)POT_Y_MIN+(long)POT_H))/(long)h);
  if (xyc < t_min) xyc = t_min;
  if (xyc > t_max) xyc = t_max;

  long t_h = t_max-t_min;
  int retval = (int)(((long)xyc - (long)t_min)*1023L/t_h);
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

void x2pb (int xc) {
  // Convert a 0 to 4095 touch screen value into a pitch bend value
  // Pitch bend messages use values between 0 and 16384, but the
  // Arduino MIDI library wants data as an int between -8192 and +8192.
  if (xc_base == -1) {
    // This is the first touch, so remember the value
    xc_base = xc;
  }
  // Want to scale the pitch bend value so that staying the same
  // uses the value of 8192, and that it is centred on xc_start
  int adj_xc = 2048 - xc_base + xc;
  pbval = map(constrain(adj_xc, 0, 4095), 0, 4095, MIDI_PITCHBEND_MIN, MIDI_PITCHBEND_MAX);
#ifdef PBTEST
  Serial.print("x2pb: xc_base=");
  Serial.print(xc_base);
  Serial.print("\txc=");
  Serial.print(xc);
  Serial.print("\tadj_xc=");
  Serial.print(adj_xc);
  Serial.print("\tpbval=");
  Serial.print(pbval);
  Serial.print("\n");
#endif
}

void reset_x2pb () {
  xc_base = -1;
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

  ypotval = 1023;  // Prefix to "high"
  ypotlast = 1023;
  xpotval = 1023;
  xpotlast = 1023;
  playing = NONOTE;
  reset_x2pb();
}

void midiNoteOn (int note) {
#ifdef TEST
  Serial.print("NoteOn: ");
  Serial.println(note);
#else
  MIDI.sendNoteOn (note, 64, MIDI_CHANNEL);
#endif
}

void midiNoteOff (int note) {
#ifdef TEST
  Serial.print("NoteOff: ");
  Serial.println(note);
#else
  MIDI.sendNoteOff (note, 0, MIDI_CHANNEL);
#endif
}

void loop() {
  // As this is a controller, we process any MIDI messages
  // we receive and allow the MIDI library to enact it's THRU
  // functionality to pass them on.
  MIDI.read();

  gfxLoop();

  // The virtual pots are configured "backwards" i.e. zero at the TOP
  // so reverse then when turning the pot values into CC messages.
  //
  // Also, there remain inaccuracies in the readings here when using
  // the touchscreen, so I re-map the full analog range into a smaller range.
  //
  // Also note the additional use of constrain() to ensure the values
  // are within the range passed to the map() function.
  //
#ifdef Y_MIDI_CC
  int ccval = map(constrain(ypotval, 100, 900), 100, 900, 127, 0);
  if (ccval != ycclast) {
    // The value for the pot has changed so send a new MIDI message
#ifdef TEST
#ifdef MIDITEST
    Serial.print("Y CC 0x");
    if (Y_MIDI_CC<16) Serial.print("0");
    Serial.print(Y_MIDI_CC,HEX);
    Serial.print(" -> ");
    Serial.println(ccval);
#endif
#else
    MIDI.sendControlChange (Y_MIDI_CC, ccval, MIDI_CHANNEL);
#endif
    ycclast = ccval;
  }
#endif
#ifdef X_MIDI_CC
  ccval = map(constrain(xpotval, 100, 900), 100, 900, 127, 0);
  if (ccval != xcclast) {
    // The value for the pot has changed so send a new MIDI message
#ifdef TEST
#ifdef MIDITEST
    Serial.print("X CC 0x");
    if (X_MIDI_CC<16) Serial.print("0");
    Serial.print(X_MIDI_CC,HEX);
    Serial.print(" -> ");
    Serial.println(ccval);
#endif
#else
    MIDI.sendControlChange (X_MIDI_CC, ccval, MIDI_CHANNEL);
#endif
    xcclast = ccval;
  }
#endif
#ifdef MIDI_PITCH_BEND
  // Take the pitch bend value and turn it into a pitch bend message.
  // The actual message is represented as two 7-bit values: PBCMD, LSB, MSB
  // But the MIDI library takes in a value between MIDI_PITCHBEND_MIN and _MAX
  if (pbval != pbvallast) {
#ifdef TEST
    Serial.print("Pitch Bend: ");
    Serial.print(pbval);
    Serial.print(" -> (");
    Serial.print((pbval+MIDI_PITCHBEND_MIN) & 0x7F, HEX); // least 7-bits
    Serial.print(",");
    Serial.print(((pbval+MIDI_PITCHBEND_MIN) & 0x3F80) >> 7, HEX); // next 7-bits
    Serial.println(")");
#else
    MIDI.sendPitchBend (pbval, MIDI_CHANNEL);
#endif
    pbvallast = pbval;
  }

#endif
}
