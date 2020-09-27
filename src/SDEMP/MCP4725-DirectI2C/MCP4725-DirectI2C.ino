/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MCP4725 Direct I2C
//  https://diyelectromusic.wordpress.com/
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
     Peter Fleury I2C Interface - http://www.peterfleury.epizy.com/avr-software.html
     Adafruit MCP4725 Library   - https://learn.adafruit.com/mcp4725-12-bit-dac-tutorial/overview

*/
extern "C" {
#include "i2cmaster.h"
}

/* A note on I2C Bus Speeds.

   Standard mode I2C is 100kbps, but there is also a fast mode of 400kbps.
   To change the mode, there if a definition at the top of twimaster.c as follows:

#define SCL_CLOCK  100000L

   This can be changed 400000L to select the higher speed mode.

*/

int mcp4725addr;  // Will be in the range 0x60 to 0x67

const PROGMEM uint16_t DACLookup_FullSine_8Bit[256] =
{
  2048, 2098, 2148, 2198, 2248, 2298, 2348, 2398,
  2447, 2496, 2545, 2594, 2642, 2690, 2737, 2784,
  2831, 2877, 2923, 2968, 3013, 3057, 3100, 3143,
  3185, 3226, 3267, 3307, 3346, 3385, 3423, 3459,
  3495, 3530, 3565, 3598, 3630, 3662, 3692, 3722,
  3750, 3777, 3804, 3829, 3853, 3876, 3898, 3919,
  3939, 3958, 3975, 3992, 4007, 4021, 4034, 4045,
  4056, 4065, 4073, 4080, 4085, 4089, 4093, 4094,
  4095, 4094, 4093, 4089, 4085, 4080, 4073, 4065,
  4056, 4045, 4034, 4021, 4007, 3992, 3975, 3958,
  3939, 3919, 3898, 3876, 3853, 3829, 3804, 3777,
  3750, 3722, 3692, 3662, 3630, 3598, 3565, 3530,
  3495, 3459, 3423, 3385, 3346, 3307, 3267, 3226,
  3185, 3143, 3100, 3057, 3013, 2968, 2923, 2877,
  2831, 2784, 2737, 2690, 2642, 2594, 2545, 2496,
  2447, 2398, 2348, 2298, 2248, 2198, 2148, 2098,
  2048, 1997, 1947, 1897, 1847, 1797, 1747, 1697,
  1648, 1599, 1550, 1501, 1453, 1405, 1358, 1311,
  1264, 1218, 1172, 1127, 1082, 1038,  995,  952,
   910,  869,  828,  788,  749,  710,  672,  636,
   600,  565,  530,  497,  465,  433,  403,  373,
   345,  318,  291,  266,  242,  219,  197,  176,
   156,  137,  120,  103,   88,   74,   61,   50,
    39,   30,   22,   15,   10,    6,    2,    1,
     0,    1,    2,    6,   10,   15,   22,   30,
    39,   50,   61,   74,   88,  103,  120,  137,
   156,  176,  197,  219,  242,  266,  291,  318,
   345,  373,  403,  433,  465,  497,  530,  565,
   600,  636,  672,  710,  749,  788,  828,  869,
   910,  952,  995, 1038, 1082, 1127, 1172, 1218,
  1264, 1311, 1358, 1405, 1453, 1501, 1550, 1599,
  1648, 1697, 1747, 1797, 1847, 1897, 1947, 1997
};

void setup() {
  Serial.begin(9600);
  
  // Set A2 and A3 as Outputs to make them our GND and Vcc,
  // which will power the MCP4725
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  digitalWrite(A2, LOW);//Set A2 as GND
  digitalWrite(A3, HIGH);//Set A3 as Vcc

  // activate internal pullups for I2C
  digitalWrite(SDA, 1);
  digitalWrite(SCL, 1);

  i2c_init();
  delay (1000);   // short delay to let things stabilize

  // Work out the address of the MCP4725
  mcp4725addr = 0;
  for (int i=0; i<8; i++) {
    int addr = 0x60+i;
    int err = i2c_start(addr<<1 + I2C_WRITE);
    i2c_stop();
    if (err == 0) {
      Serial.print ("Device found at address: 0x");
      Serial.println (addr, HEX);
      mcp4725addr = addr;
    } else {
      Serial.print ("No device found at address: 0x");
      Serial.print (addr, HEX);
      Serial.print (" Error: ");
      Serial.println (err, HEX);
    }
  }
}

void dacWrite (uint16_t dacvalue) {
  if (i2c_start(mcp4725addr<<1 + I2C_WRITE)) {
    // Can't find device, so...
    i2c_stop();
    return;
  }

  // There are several modes of writing DAC values (see the MCP4725 datasheet).
  // In summary:
  //     Fast Write Mode requires two bytes:
  //          0x0n + Upper 4 bits of data - d11-d10-d9-d8
  //                 Lower 8 bits of data - d7-d6-d5-d4-d3-d2-d1-d0
  //
  //     "Normal" DAC write requires three bytes:
  //          0x40 - Write DAC register (can use 0x50 if wanting to write to the EEPROM too)
  //          Upper 8 bits - d11-d10-d9-d9-d7-d6-d5-d4
  //          Lower 4 bits - d3-d2-d1-d0-0-0-0-0
  //
  uint8_t val1 = (dacvalue & 0xf00) >> 8; // Will leave top 4 bits zero = "fast write" command
  uint8_t val2 = (dacvalue & 0xff);
  i2c_write(val1);
  i2c_write(val2);  
  i2c_stop();
}

void loop() {
  for (int i=0; i<256; i++) {
    dacWrite(pgm_read_word(&(DACLookup_FullSine_8Bit[i])));
  }
}
