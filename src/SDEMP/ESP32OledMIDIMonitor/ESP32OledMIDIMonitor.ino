/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  ESP32C3 OLED Mini MIDI Monitor
//  https://diyelectromusic.com/2025/02/14/esp32c3-oled-mini-midi-montor/
//
      MIT License
      
      Copyright (c) 2025 diyelectromusic (Kevin)
      
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
#include <MIDI.h>
#include <U8g2lib.h>
#include <Wire.h>

//#define TEST

// there is no 72x40 constructor in u8g2 hence the 72x40 screen is
// mapped in the middle of the 132x64 pixel buffer of the SSD1306 controller
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE, 6, 5);
int width = 72;
int height = 40;
int xOffset = 30; // = (132-w)/2
int yOffset = 12; // = (64-h)/2

// Default coords go L->R, U->D
// So swap Y to make 0 at the bottom
#define XC(x) ((x)+xOffset)
#define YC(y) (height-1-(y)+yOffset)

#define MIDI_LED 8

#define MIDI_CHANNEL MIDI_CHANNEL_OMNI // or 1..16
MIDI_CREATE_INSTANCE(HardwareSerial, Serial0, MIDI);

// ----------------------------------
// MIDI Handling
// ----------------------------------
void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  gfxSetMidiNote(pitch);
  ledOn ();
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  gfxClearMidiNote(pitch);
  ledOff ();
}

void midiScan () {
  MIDI.read();
}

void midiInit () {
  MIDI.begin(MIDI_CHANNEL);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.turnThruOn();
}

// ----------------------------------
// LED Handling
// ----------------------------------
void ledOn () {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, LOW);
#endif
}

void ledOff () {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, HIGH);
#endif
}

void ledInit() {
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
  ledOff();
#endif
}

// ----------------------------------
// Graphics Handling
// ----------------------------------
bool gfxUpdate;
void gfxInit() {
  delay(1000);
  u8g2.begin();
  u8g2.setContrast(255); // set contrast to maximum 
  u8g2.setBusClock(400000); //400kHz I2C
  u8g2.clearBuffer();
  //u8g2.drawFrame(XC(0), YC(height-1), width, height);
  //delay(1000);
  gfxUpdate = true;
}

// 72x40 display to highlight 128 MIDI notes,
// but in rows of 12 - i.e. in octaves.
//
// So can have up to 6x4 pixels:
//    6x12 = 72
//    4x10 = 40
// Gives 120 indicators on the display.
//
// Leave a pixel between each means 5x3 blocks.
//
// So MIDI note to (x,y) coords:
//   x = 6 * (MIDI % 12)
//   y = 4 * (MIDI / 12)
//
// NB: Some adjustments are required to properly
//     centre the blocks on the displays, and
//     will be different if using different rotations.
void gfxSetMidiNote (int n) {
  int x = 6 * (n % 12);
  int y = 4 * (n / 12);
  u8g2.setDrawColor(1);  // On
  u8g2.drawBox(XC(x-3), YC(y+2), 5, 3);
  gfxUpdate = true;
}

void gfxClearMidiNote (int n) {
  int x = 6 * (n % 12);
  int y = 4 * (n / 12);
  u8g2.setDrawColor(0);  // Off
  u8g2.drawBox(XC(x-3), YC(y+2), 5, 3);
  gfxUpdate = true;
}

void gfxScan() {
  if (gfxUpdate) {
    u8g2.sendBuffer(); // transfer internal memory to the display
    gfxUpdate = false;
  }
}

byte testcnt = 0;
void testScan () {
  if (testcnt <128) {
    handleNoteOn(1, testcnt, 64);
  } else {
    handleNoteOff(1, testcnt-128, 0);
  }
  testcnt++;
}

// ----------------------------------
// Arduino setup/loop
// ----------------------------------
void setup(void)
{
  gfxInit();
  midiInit();
  ledInit();
}

unsigned loopcnt;
void loop(void)
{
  midiScan();
  if (loopcnt % 32 == 0) {
    gfxScan();
#ifdef TEST
    testScan();
#endif
  }
  loopcnt++;
}
