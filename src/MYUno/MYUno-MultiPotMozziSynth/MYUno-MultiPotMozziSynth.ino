/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Make Your Uno Synth - Note Melody
//  https://diyelectromusic.wordpress.com/2023/01/05/arduino-make-your-uno-synth/
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
  Using principles from the following Arduino tutorials:
    Mozzi Library         - https://sensorium.github.io/Mozzi/
    Arduino Potentiometer - https://www.arduino.cc/en/Tutorial/Potentiometer

  Much of this code is based on the Mozzi example Knob_LightLevel_x2_FMsynth (C) Tim Barrass

*/
#include <MozziGuts.h>
#include <Oscil.h> // oscillator
#include <tables/cos2048_int8.h> // for the modulation oscillators
#include <tables/sin2048_int8.h> // sine table for oscillator
#include <tables/saw2048_int8.h> // saw table for oscillator
#include <tables/triangle2048_int8.h> // triangle table for oscillator
#include <tables/square_no_alias_2048_int8.h> // square table for oscillator
#include <Smooth.h>
#include <ADSR.h>

#define POT_ZERO 15 // Anything below this value is treated as "zero"

// Set up the analog inputs - comment out if you aren't using this one
#define WAVT_PIN 0  // Wavetable
#define INTS_PIN 1  // FM intensity
#define RATE_PIN 2  // Modulation Rate
#define MODR_PIN 3  // Modulation Ratio
#define FREQ_PIN 4  // Optional Frequency Control

#define MIN_FREQ    220  // Range min to min+1023

#define CONTROL_RATE 256 // Hz, powers of 2 are most reliable

// The original example used AutoMap to calibrate the range of values
// to be expected from the sensors. However AutoMap is really for use
// when you don't know the range of values that a sensor might produce,
// for example with a light dependant resistor.
//
// When you know the full range, e.g. when using a potentiometer, then
// AutoMap is largely obsolete.  And in my case, when there are some options
// to use a fixed value rather than arange then AutoMap is actually
// determinental to the final output.
//
// Experimentally I was finding AutoMap produced a very different output
// for a fixed value for the Intensity of 500 compared to a potentiometer
// reading a value of 500.  I still don't really know why, but I've taken
// out the AutoMap as a consequence and simplified the processing in the
// updateControl function too.
//
//AutoMap kMapIntensity(0,1023,10,700);
//AutoMap kMapModSpeed(0,1023,10,10000);

Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aCarrier;  // Wavetable will be set later
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aModulator(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kIntensityMod(COS2048_DATA);

int wavetable;
int mod_ratio;
int mod_rate;
int carrier_freq;
long fm_intensity;
int adsr_a, adsr_d;
int testcount;
int playing;
int lastfreq;

// smoothing for intensity to remove clicks on transitions
float smoothness = 0.95f;
Smooth <long> aSmoothIntensity(smoothness);

int potcount;
int potWAVT, potMODR, potINTS, potRATE, potFREQ;


void setup(){
  mod_rate = -1;  // Force an update on first control scan...
  wavetable = 0;
  setWavetable();

  potcount = 0;
  lastfreq = 0;

  startMozzi(CONTROL_RATE);
}

void setFreqs(){
  //calculate the modulation frequency to stay in ratio
  int mod_freq = carrier_freq * mod_ratio;

  // set the FM oscillator frequencies
  aCarrier.setFreq(carrier_freq);
  aModulator.setFreq(mod_freq);
}

void setWavetable() {
  switch (wavetable) {
  case 1:
    aCarrier.setTable(TRIANGLE2048_DATA);
    break;
  case 2:
    aCarrier.setTable(SAW2048_DATA);
    break;
  case 3:
    aCarrier.setTable(SQUARE_NO_ALIAS_2048_DATA);
    break;
  default: // case 0
    aCarrier.setTable(SIN2048_DATA);
  }
}

void updateControl(){
  // Read the potentiometers - do one on each updateControl scan.
  // Note: each potXXXX value is remembered between scans.
  potcount ++;
  switch (potcount) {
  case 1:
    potWAVT = mozziAnalogRead(WAVT_PIN) >> 8; // value is 0-3
    break;
  case 2:
    potINTS = mozziAnalogRead(INTS_PIN); // value is 0-1023
    if (potINTS<POT_ZERO) potINTS = 0;
    break;
  case 3:
    potRATE = mozziAnalogRead(RATE_PIN); // value is 0-1023
    if (potRATE<POT_ZERO) potRATE = 0;
    break;
  case 4:
    potMODR = mozziAnalogRead(MODR_PIN) >> 7; // value is 0-7
    break;
  case 5:
    potFREQ = mozziAnalogRead(FREQ_PIN); // value is 0-1023
    break;
  default:
    potcount = 0;
  }

  // See if the wavetable changed...
  if (potWAVT != wavetable) {
    // Change the wavetable
    wavetable = potWAVT;
    setWavetable();
  }

  // See if the frequency changed...
  if (lastfreq != potFREQ) {
    carrier_freq = MIN_FREQ + potFREQ;
    setFreqs();
  }
  lastfreq = potFREQ;

  // Everything else we update every cycle anyway
  mod_ratio = potMODR;

  // Perform the regular "control" updates
  setFreqs();

  // calculate the fm_intensity
  if (potRATE==0) {
     fm_intensity = (long)potINTS;
  } else {
     fm_intensity = ((long)potINTS * (kIntensityMod.next()+128))>>8; // shift back to range after 8 bit multiply
  }

  // use a float here for low frequencies
  float mod_speed = 0.0;
  if (potRATE != mod_rate) {
    mod_rate = potRATE;
    mod_speed = (float)potRATE/100;
    kIntensityMod.setFreq(mod_speed);
  }
}


int updateAudio(){
  long modulation = aSmoothIntensity.next(fm_intensity) * aModulator.next();
  return aCarrier.phMod(modulation);
}


void loop(){
  audioHook();
}
