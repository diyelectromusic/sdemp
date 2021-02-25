/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  AD9833 Poly Signal Generator - Part 2
//  https://diyelectromusic.wordpress.com/2021/02/25/ad9833-poly-signal-generator-part-2/
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

#define POT1 A0
#define POT2 A1
#define BTN1 2
#define BTN2 3

int mode1,mode2;
int freq1,freq2;
int lastbtn1,lastbtn2;
int freqbase;

#define FREQBASE1 0
#define FREQBASE2 1050

void adSetFrequencyAll(MD_AD9833::channel_t ch, float freq, float freqdiff) {
  for (int i=0; i<NUM_ADS; i++) {
    ad[i]->setFrequency(ch, freq+freqdiff*i);
  }
}

void adSetModeAll(MD_AD9833::mode_t mode) {
  for (int i=0; i<NUM_ADS; i++) {
    ad[i]->setMode(mode);
  }
}

void setup () {
  // initialise each instance of the AD9833
  for (int i=0; i<NUM_ADS; i++) {
    ad[i] = new MD_AD9833(fsync[i]);
    ad[i]->begin();
    ad[i]->setMode(MD_AD9833::MODE_OFF);
  }

  // Initialise pots and buttons
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);

  // If switch is HIGH then start LOW to force a "first time" setup
  lastbtn1 = digitalRead(BTN1)?LOW:HIGH;
  lastbtn2 = digitalRead(BTN1)?LOW:HIGH;

  // Same with analog - force a reading
  freq1 = -1;
  freq2 = -1;
}

void loop () {
  int btn1 = digitalRead(BTN1);
  if ((btn1 != lastbtn1)) {
    // Switch was changed
    if (btn1 == HIGH) adSetModeAll(MD_AD9833::MODE_SINE);
    if (btn1 == LOW)  adSetModeAll(MD_AD9833::MODE_TRIANGLE);
  }
  lastbtn1 = btn1;

  int btn2 = digitalRead(BTN2);
  if ((btn2 != lastbtn2)) {
    // Switch was changed
    if (btn2 == HIGH) freqbase = FREQBASE1;
    if (btn2 == LOW)  freqbase = FREQBASE2;
  }
  lastbtn2 = btn2;

  // Check the potentiometers for the frequency.
  //
  // Pots will return 0 to 1023, so add 30 to give
  // a frequency range of 30 to 1053 Hz.
  //
  int potval1 = 30 + analogRead(POT1);
  int potval2 = analogRead(POT2);
  if ((potval1 != freq1) || (potval2 != freq2)) {
    // Change the frequency of the default channel
    freq1 = potval1;
    freq2 = potval2;
    adSetFrequencyAll(MD_AD9833::CHAN_0, freqbase+freq1, freq2);
  }
}
