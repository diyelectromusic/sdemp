/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Xiao MIDI Synth Program and Control Messenger
//  https://diyelectromusic.com/2025/06/29/xiao-esp32-c3-midi-synthesizer-part-6/
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
/*
  Using principles from the following tutorials:
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Adafruit SSD1306 Library - https://learn.adafruit.com/monochrome-oled-breakouts/arduino-library-and-examples
    Adafruit GFX Library     - https://learn.adafruit.com/adafruit-gfx-graphics-library
    Xiao Getting Started - https://wiki.seeedstudio.com/Seeeduino-XIAO/
*/
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <MIDI.h>

// Comment this out for use on a MCU without USB.
#define HAS_USB

// Comment this out for serial to serial routing
// instead of serial to USB.
// USB, if present, is always routed back to serial.
//#define SER_TO_USB

// These define which interface is sent the PC/CC message
//#define MIDI_USB_PCCC
#define MIDI_SER_PCCC

// This is required to set up the MIDI library on the default serial port.
// Different boards may required different serial ports:
//   XIAO SAMD21:   Serial1
//   XIAO ESP32C3:  Serial0
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial0, MIDI);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

#ifdef HAS_USB
#include <USB-MIDI.h>
USBMIDI_CREATE_INSTANCE(0, UMIDI);
#endif

// The following define the MIDI parameters to use.
#define MIDI_CHANNEL 1  // MIDI Channel number (1 to 16)

// Comment out to send a Program Change instead of a Control Change message
//#define MIDI_CONTROLLER   1  // CC 1 = Modulation Wheel

int midival;

#define ALG_IO A0
int algval[5];

// Set the screen direction, relative to use of the DIY shield.
//   0 = landscape (pins at top)
//   1 = portrait (pins on the left)
//   2 = landscape (pins at bottom)
//   3 = portrait (pins on the right)
#define OLED_ROT 0

// Dimensions of the OLED screen
#define OLED_W  128
#define OLED_H   64
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
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display = Adafruit_SSD1306(OLED_W, OLED_H, &Wire, OLED_RST);

// Used to refer to the width and height of the display
uint16_t oled_w;
uint16_t oled_h;

// Default font dimensions
// See: https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#define FONT_X (5+1)  // Characters are 5 wide, with a space
#define FONT_Y 8

uint8_t oled_rows;
uint8_t oled_cols;

void displayInit () {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    for (;;); // No display - stop
  }

  display.setRotation(OLED_ROT);
  oled_w = display.width();
  oled_h = display.height();
  oled_cols = oled_w/FONT_X;
  oled_rows = oled_h/FONT_Y;

  // Shows the Adafruit library logo!
  display.display();
  delay(2000);

  display.clearDisplay();
  display.display();
}

void printInit (void) {
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.setTextSize(3);
  display.setCursor(0,0);
  display.clearDisplay();
}

void printNum (int num) {
  display.clearDisplay();
  display.setCursor(0,0);
  if (num < 10) {
    display.print("  ");
  } else if (num < 100) {
    display.print(" ");
  }
  display.print(num);
  display.display();
}

void setup() {
#ifdef HAS_USB
  // Automatic THRU is off for USB, on for Serial
  UMIDI.begin(MIDI_CHANNEL_OMNI);
  UMIDI.turnThruOff();
#endif
  MIDI.begin(MIDI_CHANNEL_OMNI);
#ifdef SER_TO_USB
  MIDI.turnThruOff();
#else
  MIDI.turnThruOn();
#endif

  analogReadResolution(10);

  displayInit();
  printInit();

#ifdef MIDI_CONTROLLER
  printNum(midival);
#else
  printNum(midival+1);
#endif
}

void midiThru(int out, midi::MidiType cmd, midi::DataByte d1, midi::DataByte d2, midi::Channel ch) {
  // Ignore "Active Sensing" and SysEx messages
  if ((cmd != 0xFE) && (cmd != 0xF0)) {
    if (out == 1) {
      MIDI.send(cmd, d1, d2, ch);
    } else {
#ifdef HAS_USB
      UMIDI.send(cmd, d1, d2, ch);
#endif
    }
  }
}

void loop() {
  // Read analog value and convert from 0..1023 to 0..127
  int aval = analogRead(ALG_IO) >> 3;

  // Look for a stable reading from the last few readings
  if ((aval == algval[0]) && (algval[0] == algval[1]) && (algval[1] == algval[2]) && (algval[2] == algval[3]) && (algval[3] == algval[4])) {
    if (midival != aval) {
      midival = aval;
  
      // Pot reading has changed
#ifdef MIDI_CONTROLLER
      printNum(midival);
#ifdef MIDI_SER_PCCC
      MIDI.sendControlChange(MIDI_CONTROLLER, midival, MIDI_CHANNEL);
#endif
#ifdef MIDI_USB_PCCC
      UMIDI.sendControlChange(MIDI_CONTROLLER, midival, MIDI_CHANNEL);
#endif
#else // MIDI Program Change
      // Program Change values are best shown in the 1-128 range
      printNum(midival+1);
#ifdef MIDI_SER_PCCC
      MIDI.sendProgramChange(midival, MIDI_CHANNEL);
#endif
#ifdef MIDI_USB_PCCC
      UMIDI.sendProgramChange(midival, MIDI_CHANNEL);
#endif
#endif
    }
  }
  algval[0] = algval[1];
  algval[1] = algval[2];
  algval[2] = algval[3];
  algval[3] = algval[4];
  algval[4] = aval;

  // Now handling any MIDI routing
#ifdef HAS_USB
  if (UMIDI.read()) {
    midiThru(1, UMIDI.getType(),
                UMIDI.getData1(),
                UMIDI.getData2(),
                UMIDI.getChannel());
  }
#endif

  if (MIDI.read()) {
#ifdef SER_TO_USB
    midiThru(0, MIDI.getType(),
                MIDI.getData1(),
                MIDI.getData2(),
                MIDI.getChannel());
#else
    // Automatically routes serial to serial
    // when THRU is enabled.
#endif
  }
}
