/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Xiao MIDI Program and Control Button Messenger
//  https://diyelectromusic.wordpress.com/2023/04/11/xiao-samd21-arduino-and-midi-part-6/
//
      MIT License
      
      Copyright (c) 2023 diyelectromusic (Kevin)
      
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
    Button               - https://www.arduino.cc/en/Tutorial/Button
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Adafruit SSD1306 Library - https://learn.adafruit.com/monochrome-oled-breakouts/arduino-library-and-examples
    Adafruit GFX Library     - https://learn.adafruit.com/adafruit-gfx-graphics-library
    Xiao Getting Started - https://wiki.seeedstudio.com/Seeeduino-XIAO/
*/
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <MIDI.h>
#include <USB-MIDI.h>

// This is required to set up the MIDI library on the default serial port.
MIDI_CREATE_DEFAULT_INSTANCE();

// This initialised USB MIDI.
USBMIDI_CREATE_INSTANCE(0, UMIDI);

// The following define the MIDI parameters to use.
#define MIDI_CHANNEL 1  // MIDI Channel number (1 to 16)
// This it the MIDI Control Change message to change
#define MIDI_CONTROLLER   7  // CC 7 = Channel Volume

int midiPC;
int midiBank;
int midiCC;

// On the XIAO using an Expander Board, some pins are being reused
// that already have external pull-ups for other purposes.
// D2, D9, D10 all have external pull-ups as part of the SPI interface
// to the SD card slot.
// D8 is also part of the SD card interface, but doesn't.
// D1 is connected to the on-board button, so that can be used too.
// D0 is free via the first Grove connector.
#define NUM_BTNS 6
int btnPins[NUM_BTNS] = {0,1,2,8,9,10};
int btnPullups[NUM_BTNS] = {INPUT_PULLUP, INPUT_PULLUP, INPUT, INPUT_PULLUP, INPUT, INPUT};

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
  display.setTextSize(2);
  display.setCursor(0,0);
  display.clearDisplay();
}

void printNum (int num) {
  if (num < 10) {
    display.print("  ");
  } else if (num < 100) {
    display.print(" ");
  }
  display.print(num);
}

void midiDisplay () {
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("PC = ");
  printNum(midiPC+1);
  display.print("\nBK = ");
  printNum(midiBank);
  display.print("\nCC = ");
  printNum(midiCC);
  display.display();
}

int btnLast[NUM_BTNS];
void btnInit() {
  for (int i=0; i<NUM_BTNS; i++) {
    pinMode (btnPins[i], btnPullups[i]);
    btnLast[i] = HIGH;
  }
}

int btnTriggered (unsigned btn) {
  int retval = 0;
  if (btn < NUM_BTNS) {
    int newval = digitalRead(btnPins[btn]);

    // Looking for a HIGH to LOW transition
    // as all buttons will be configured for PULLUP
    // either via internal or external pull-up resistors.
    if ((newval == LOW) && (btnLast[btn] == HIGH)) {
      // Button triggered
      retval = 1;
    }
    if ((newval == LOW) && (btnLast[btn] == LOW)) {
      // Button is held down
      retval = 2;
    }

    btnLast[btn] = newval;
  }

  return retval;
}

void setup() {
  MIDI.begin(MIDI_CHANNEL_OFF);
  UMIDI.begin(MIDI_CHANNEL_OFF);

  btnInit();

  displayInit();
  printInit();

  midiDisplay();
}

bool btns[NUM_BTNS];
void loop() {
  bool updatedisplay = false;
  for (int i=0; i<NUM_BTNS; i++) {
    int btntrig = btnTriggered(i);
    if (btntrig) {
      updatedisplay = true;
      bool btnheld = (btntrig == 2);
      switch(i) {
        case 0:
          midiPCUp(btnheld);
          break;
        case 1:
          midiPCDown(btnheld);
          break;
        case 2:
          midiBankUp(btnheld);
          break;
        case 3:
          midiBankDown(btnheld);
          break;
        case 4:
          midiCCUp(btnheld);
          break;
        case 5:
          midiCCDown(btnheld);
          break;
        default:
          // Unrecognised button - ignore
          break;
      }
      // Include a short delay for debouncing purposes
      delay(200);
    }
  }
  if (updatedisplay) {
    midiDisplay();
  }
}

// MIDI handling functions

void midiPCUp (bool btnheld) {
  if (btnheld) {
    midiPC += 10;
  } else {
    midiPC++;
  }
  if (midiPC > 127) midiPC = 127;
  MIDI.sendProgramChange(midiPC, MIDI_CHANNEL);
  UMIDI.sendProgramChange(midiPC, MIDI_CHANNEL);
}

void midiPCDown (bool btnheld) {
  if (btnheld) {
    midiPC -= 10;
  } else {
    midiPC--;
  }
  if (midiPC < 0) midiPC = 0;
  MIDI.sendProgramChange(midiPC, MIDI_CHANNEL);
  UMIDI.sendProgramChange(midiPC, MIDI_CHANNEL);
}

#define MIDI_CC_BANK_MSB 0
#define MIDI_CC_BANK_LSB 32
void midiBankUp (bool btnheld) {
  if (btnheld) {
    midiBank += 10;
  } else {
    midiBank++;
  }
  // Support 14-bit MSB/LSB bank select...
  if (midiBank > 16383) midiBank = 6383;
  MIDI.sendControlChange(MIDI_CC_BANK_MSB, (midiBank>>7), MIDI_CHANNEL);
  MIDI.sendControlChange(MIDI_CC_BANK_LSB, (midiBank & 0x7F), MIDI_CHANNEL);
  UMIDI.sendControlChange(MIDI_CC_BANK_MSB, (midiBank>>7), MIDI_CHANNEL);
  UMIDI.sendControlChange(MIDI_CC_BANK_LSB, (midiBank & 0x7F), MIDI_CHANNEL);
}

void midiBankDown (bool btnheld) {
  if (btnheld) {
    midiBank -= 10;
  } else {
    midiBank--;
  }
  // Support 14-bit MSB/LSB bank select...
  if (midiBank < 0) midiBank = 0;
  MIDI.sendControlChange(MIDI_CC_BANK_MSB, (midiBank>>7), MIDI_CHANNEL);
  MIDI.sendControlChange(MIDI_CC_BANK_LSB, (midiBank & 0x7F), MIDI_CHANNEL);
  UMIDI.sendControlChange(MIDI_CC_BANK_MSB, (midiBank>>7), MIDI_CHANNEL);
  UMIDI.sendControlChange(MIDI_CC_BANK_LSB, (midiBank & 0x7F), MIDI_CHANNEL);
}

void midiCCUp (bool btnheld) {
  if (btnheld) {
    midiCC += 10;
  } else {
    midiCC++;
  }
  if (midiCC > 127) midiCC = 127;
  MIDI.sendControlChange(MIDI_CONTROLLER, midiCC, MIDI_CHANNEL);
  UMIDI.sendControlChange(MIDI_CONTROLLER, midiCC, MIDI_CHANNEL);
}

void midiCCDown (bool btnheld) {
  if (btnheld) {
    midiCC -= 10;
  } else {
    midiCC--;
  }
  if (midiCC < 0) midiCC = 0;
  MIDI.sendControlChange(MIDI_CONTROLLER, midiCC, MIDI_CHANNEL);
  UMIDI.sendControlChange(MIDI_CONTROLLER, midiCC, MIDI_CHANNEL);
}
