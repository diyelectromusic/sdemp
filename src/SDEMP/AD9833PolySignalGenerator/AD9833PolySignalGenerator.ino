/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  AD9833 Poly Signal Generator
//  https://diyelectromusic.wordpress.com/2021/02/24/ad9833-poly-signal-generator/
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
    Arduino Input Pullup Serial - https://www.arduino.cc/en/Tutorial/BuiltInExamples/InputPullupSerial
    Arduino Analog Read Serial  - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogReadSerial
    Arduino SPI Library   - https://www.arduino.cc/en/Reference/SPI
    MD_AD9833 Library     - https://github.com/MajicDesigns/MD_AD9833
*/
#include <SPI.h>
#include <MD_AD9833.h>

// Pins for SPI link with the AD9833
#define DATA  11  // SPI Data pin number (hardware SPI = 11)
#define CLK   13  // SPI Clock pin number (hardware SPI = 13)
// SPI Load pin numbers (FSYNC in AD9833 usage)
#define NUM_ADS 6
int fsync[NUM_ADS] = {5, 6, 7, 8, 9, 10};
MD_AD9833* ad[NUM_ADS];

int freq;

void setup () {
  // initialise each instance of the AD9833
  for (int i=0; i<NUM_ADS; i++) {
    ad[i] = new MD_AD9833(fsync[i]);
    ad[i]->begin();
    ad[i]->setMode(MD_AD9833::MODE_OFF);
  }

  freq = 220;
}

void loop () {
  for (int i=0; i<NUM_ADS; i++) {
     ad[i]->setMode(MD_AD9833::MODE_SINE);
     ad[i]->setFrequency (MD_AD9833::CHAN_0, freq+8*i);
     delay(1000);
  }

  for (int i=0; i<NUM_ADS; i++) {
    ad[i]->setMode(MD_AD9833::MODE_OFF);
  }

  freq = freq*2;
  if (freq > 1760) freq=220;
}
