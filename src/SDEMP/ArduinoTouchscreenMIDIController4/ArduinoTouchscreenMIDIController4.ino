/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Touchscreen X-Y MIDI Controller - Part 2
//  https://diyelectromusic.wordpress.com/2022/06/25/arduino-touchscreen-x-y-midi-controller-part-2/
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
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SPI.h>
#include <MIDI.h>

MCUFRIEND_kbv tft;

//#define TEST 1     // Uncomment for general testing
//#define MIDITEST 1 // Uncomment for MIDI messages printed to Serial
//#define TSTEST 1 // Touchscreen testing
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
int xc_touched;
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

// Touchscreen Definitions
// Obtained from the MCUFriend_kbv/TouchScreen_Calibr_native.ino example
const int XP=6,XM=A2,YP=A1,YM=7; //320x480 ID=0x1581
const int TS_LEFT=933,TS_RT=288,TS_TOP=968,TS_BOT=188;

/* Other details from TouchScreen_Calibr_native.ino output
PORTRAIT  CALIBRATION     320 x 480
x = map(p.x, LEFT=933, RT=288, 0, 320)
y = map(p.y, TOP=968, BOT=188, 0, 480)

LANDSCAPE CALIBRATION     480 x 320
x = map(p.y, LEFT=968, RT=188, 0, 480)
y = map(p.x, TOP=288, BOT=933, 0, 320)
*/
// Measure resistance across XP and XM with a multimeter
// And use as the last value here. Mine is 380 ohms
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 380);

// If the resolution or rotation changes, then the gfx
// routines will need re-writing to cope.
unsigned w, h;
#define ROTATION 1  // Want display in landscape mode, USB port on right
#define MINPRESSURE 5
#define MAXPRESSURE 2000

// Taken from MCUFRIEND_kbv/button_simple.ino example
//
int pixel_x, pixel_y;     //Touch_getXY() updates global vars
bool Touch_getXY(void)
{
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);      //restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   //because TFT control pins
    digitalWrite(XM, HIGH);
#ifdef TSTEST
    Serial.print("Touch: ");
    Serial.print(p.x);
    Serial.print("\t");
    Serial.print(p.y);
    Serial.print("\t");
    Serial.print(p.z);
#endif
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    if (pressed) {
     /* // Portrait mode
        pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width());
        pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
      */
        // Landscape mode
        pixel_x = map(p.y, TS_TOP, TS_BOT, 0, tft.width());
        pixel_y = map(p.x, TS_RT, TS_LEFT, 0, tft.height());
#ifdef TSTEST
        Serial.print("\tPressed");
#endif
    }
#ifdef TSTEST
    Serial.print("\n");
#endif
    return pressed;
}

// Taken from MCUFRIEND_kbv/button_simple.ino example
//
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

void gfxSetup() {
  uint16_t tftid = tft.readID();
  tft.begin(tftid);

  tft.setRotation(ROTATION);
  w = tft.width();
  h = tft.height();
  tft.fillScreen(BLACK);

  gfxInit();

#ifdef TEST
  Serial.print("Touchscreen ");
  Serial.print(tftid);
  Serial.print("\t");
  Serial.print(w);
  Serial.print("x");
  Serial.println(h);
#endif
}

void gfxLoop() {
  if (Touch_getXY()) {
    ypotval = map(pixel_y, 0, h, 0, 1023);
    xpotval = map(pixel_x, 0, w, 0, 1023);
    x2pb(pixel_x);

#ifdef NUM_NOTES
    int note = x2note(pixel_x);
    if ((note != NONOTE) && (playing == NONOTE)) {
      // Start a new note playing
      midiNoteOn (notes[note]);
      gfxNoteOn (note);
      playing = note;
    }
#endif
  } else {
    // No touch
#ifdef NUM_NOTES
    // If we're playing a note, and we've already "debounced" the non-touch
    if ((playing != NONOTE) && (xc_touched == 0)) {
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
    tft.drawFastVLine(wi-1, 0, h, GREEN);
    tft.drawFastVLine(wi, 0, h, GREEN);
    tft.drawFastVLine(wi+1, 0, h, GREEN);
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


void x2pb (int xc) {
  // Convert a 0 to "width" touch screen value into a pitch bend value
  // Pitch bend messages use values between 0 and 16384, but the
  // Arduino MIDI library wants data as an int between -8192 and +8192.
  if (xc_touched == 0) {
    // This is the first touch, so remember the value
    xc_base = xc;
  }
  // Want to scale the pitch bend value so that staying the same
  // uses the value of 8192, and that it is centred on xc_start
  int adj_xc = (w/2) - xc_base + xc;
  pbval = map(constrain(adj_xc, 0, w), 0, w, MIDI_PITCHBEND_MIN, MIDI_PITCHBEND_MAX);
#ifdef PBTEST
  Serial.print("x2pb: w=");
  Serial.print(w);
  Serial.print("\txc_base=");
  Serial.print(xc_base);
  Serial.print("\txc_touched=");
  Serial.print(xc_touched);
  Serial.print("\txc=");
  Serial.print(xc);
  Serial.print("\tadj_xc=");
  Serial.print(adj_xc);
  Serial.print("\tpbval=");
  Serial.print(pbval);
  Serial.print("\n");
#endif
  // Reset the "hasn't been touched" counter
  xc_touched = 5;
}

void reset_x2pb () {
  // We allow a few "non touched" items in case of touchscreen
  // "bouncing" without reseting things...
  if (xc_touched > 0) {
    xc_touched--;
  }
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
#ifdef MIDITEST
  Serial.print("NoteOn: ");
  Serial.println(note);
#endif
#else
  MIDI.sendNoteOn (note, 64, MIDI_CHANNEL);
#endif
}

void midiNoteOff (int note) {
#ifdef TEST
#ifdef MIDITEST
  Serial.print("NoteOff: ");
  Serial.println(note);
#endif
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
#ifdef MIDITEST
    Serial.print("Pitch Bend: ");
    Serial.print(pbval);
    Serial.print(" -> (");
    Serial.print((pbval+MIDI_PITCHBEND_MIN) & 0x7F, HEX); // least 7-bits
    Serial.print(",");
    Serial.print(((pbval+MIDI_PITCHBEND_MIN) & 0x3F80) >> 7, HEX); // next 7-bits
    Serial.println(")");
#endif
#else
    MIDI.sendPitchBend (pbval, MIDI_CHANNEL);
#endif
    pbvallast = pbval;
  }

#endif
}
