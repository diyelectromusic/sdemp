/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  AD9833 Signal Generator
//  https://diyelectromusic.wordpress.com/2020/11/19/ad9833-signal-generator-part-2/
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
#define FSYNC 10  // SPI Load pin number (FSYNC in AD9833 usage)

MD_AD9833 AD(FSYNC); // Hardware SPI
//MD_AD9833  AD(DATA, CLK, FSYNC); // Software SPI with any pins

#define POT   A0  // Potentiometer input
#define BTN   2   // Input Button (active low)

int mode;
int freq;
int lastbtn;

void setup () {
  pinMode (BTN, INPUT_PULLUP);
  pinMode (LED_BUILTIN, OUTPUT);
  AD.begin();
  mode = 0;
  freq = 0;
  lastbtn=HIGH;
}

void loop () {
  // Check the button for a mode change
  int btn = digitalRead(BTN);
  if ((btn == LOW) && (lastbtn == HIGH)) {
     // We have a HIGH to LOW transition
     mode++;
     switch (mode) {
       case 1: AD.setMode(MD_AD9833::MODE_SINE);     break;
       case 2: AD.setMode(MD_AD9833::MODE_TRIANGLE); break;
       case 3: AD.setMode(MD_AD9833::MODE_SQUARE1);  break;
       case 4: AD.setMode(MD_AD9833::MODE_SQUARE2);  break;

       default: AD.setMode(MD_AD9833::MODE_OFF);
         mode = 0;
         break;
     }

     // Include a short delay so it doesn't trigger straight away
     // (this is a crude way of "debouncing" the switch!)
     delay (500);
  }
  lastbtn = btn;

  // Check the potentiometer for the frequency
  // Pot will return 0 to 1023, so add 30 to give
  // a frequency range of 30 to 1053 Hz
  // NOTE: This requires a patch to the MD_AD9833.cpp file
  //       in the library!  See my project webpage for details!
  int potval = 30 + analogRead (POT);
  if (potval != freq) {
    // Change the frequency of the default channel
    freq = potval;
    AD.setFrequency(MD_AD9833::CHAN_0, freq);
  }
}
