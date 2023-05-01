/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Xiao IO Expander MIDI Control Messenger
//  https://diyelectromusic.wordpress.com/2023/04/12/xiao-samd21-arduino-and-midi-part-7/
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
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Adafruit SSD1306 Library - https://learn.adafruit.com/monochrome-oled-breakouts/arduino-library-and-examples
    Adafruit GFX Library     - https://learn.adafruit.com/adafruit-gfx-graphics-library
    Xiao Getting Started - https://wiki.seeedstudio.com/Seeeduino-XIAO/
    Hobbytronics I2C ALG - https://www.hobbytronics.co.uk/electronic-components/misc-ic/adc-i2c-slave
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

#define NUM_CC 8
uint8_t cc[NUM_CC]={
  1,   // Modulation
  2,   // Breath
  7,   // Channel volume
  8,   // Balance
 10,   // Pan
 11,   // Expression
 12,   // Effect Control 1
 13,   // Effect Control 2
};

uint8_t midival[NUM_CC];

// ------------------------------------------------------------------------
//
// Initialise the Hobbytronics I2C ADC I2C Expander
// https://www.hobbytronics.co.uk/electronic-components/misc-ic/adc-i2c-slave
//
// All code based on the example code provided in the datasheet.

// I2C Address: Select 40 to 47 by setting three address lines
#define HT_I2C_ADDR 40
unsigned htADC[10];

// ALG I2C config:
//   Bit 0 = 8 or 10 bit: 1=8-bit
//   Bit 1:2 = 00 (raw reading); 01 (filtered); 10, 11 = reserved: 01 selected
#define HT_I2C_CONFIG B00000011

void htAlgI2CSetup () {
  Wire.begin(); // join i2c bus (address optional for master) 
  Wire.beginTransmission(HT_I2C_ADDR); // transmit to device 
  Wire.write(HT_I2C_CONFIG); // send config options 
  Wire.endTransmission(); // stop transmitting 
}

void htAlgI2CScan()  {
  // Get 8-bit ADC data 
  // We only want single byte values (0 to 255) 
  unsigned char rcvd=0; // keep track of how many characters received 
  Wire.requestFrom(HT_I2C_ADDR, 10); // request 10 bytes from peripheral device 
  rcvd=0; 
  while(Wire.available()) 
  {
    if(rcvd < 10) {
      htADC[rcvd] = Wire.read(); // receive a byte as character 
    }
    rcvd++; 
  } 
} 

int htAnalogRead (int adc) {
  if (adc < 10) {
    return htADC[adc];
  }
  return 0;
}
//   End of HT ALG I2C Expander Code
// ------------------------------------------------------------------------


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

void displayScan () {
  display.display();
}

void displayCCInit () {
  for (int c=0; c<8; c++) {
    displayCC (c, 127);
  }
  display.display();
  delay(1000);
  for (int c=0; c<8; c++) {
    displayCC (c, 0);
  }
  display.display();
}

void displayCC (int c, uint8_t ccval) {
  int r_x = (c * oled_w) / 8;
  int r_w = (oled_w / 8) - 1;
  int r_y = (ccval * oled_h) / 128;

  display.fillRect (r_x, oled_h-r_y, r_w, r_y, WHITE);
  display.fillRect (r_x, 0, r_w, oled_h-r_y, BLACK);
}

void setup() {
  MIDI.begin(MIDI_CHANNEL_OFF);
  UMIDI.begin(MIDI_CHANNEL_OFF);

  htAlgI2CSetup();

  displayInit();
  displayCCInit();
}

void loop() {
  displayScan();

  htAlgI2CScan();

  for (int c=0; c<NUM_CC; c++)
  {
    // Convert a 8-bit reading into a 7-bit reading for use with MIDI
    int aval = htAnalogRead(c)>>1;
    if (midival[c] != aval) {
      midival[c] = aval;
  
      // Pot reading has changed
      displayCC(c, midival[c]);
      MIDI.sendControlChange(cc[c], midival[c], MIDI_CHANNEL);
      UMIDI.sendControlChange(cc[c], midival[c], MIDI_CHANNEL);
    }
  }
}
