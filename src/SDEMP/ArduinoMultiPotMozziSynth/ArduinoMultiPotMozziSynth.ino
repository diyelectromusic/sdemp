/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Mozzi Multi Pot MIDI FM Synthesis
//  https://diyelectromusic.wordpress.com/2020/08/21/arduino-multi-pot-mozzi-fm-synthesis/
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
    Arduino MIDI Library  - https://github.com/FortySevenEffects/arduino_midi_library
    Mozzi Library         - https://sensorium.github.io/Mozzi/
    Arduino Potentiometer - https://www.arduino.cc/en/Tutorial/Potentiometer

  Much of this code is based on the Mozzi example Knob_LightLevel_x2_FMsynth (C) Tim Barrass

*/
#include <MIDI.h>
#include <MozziGuts.h>
#include <Oscil.h> // oscillator
#include <tables/cos2048_int8.h> // for the modulation oscillators
#include <tables/sin2048_int8.h> // sine table for oscillator
#include <tables/saw2048_int8.h> // saw table for oscillator
#include <tables/triangle2048_int8.h> // triangle table for oscillator
#include <tables/square_no_alias_2048_int8.h> // square table for oscillator
#include <mozzi_midi.h>
#include <Smooth.h>
#include <AutoMap.h> // maps unpredictable inputs to a range
#include <ADSR.h>

// Set up the analog inputs - comment out if you aren't using this one
#define WAVT_PIN 0  // Wavetable
#define INTS_PIN 1  // FM intensity
#define RATE_PIN 2  // Modulation Rate
#define MODR_PIN 3  // Modulation Ratio
#define AD_A_PIN 4  // ADSR Attack
#define AD_D_PIN 5  // ADSR Delay

MIDI_CREATE_DEFAULT_INSTANCE();

AutoMap kMapIntensity(0,1023,10,700);
AutoMap kMapModSpeed(0,1023,10,10000);

Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aCarrier;  // Wavetable will be set later
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aModulator(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kIntensityMod(COS2048_DATA);

int wavetable;
int mod_ratio;
int carrier_freq;
long fm_intensity;
int adsr_a, adsr_d;

// smoothing for intensity to remove clicks on transitions
float smoothness = 0.95f;
Smooth <long> aSmoothIntensity(smoothness);

int potcount;
int potWAVT, potMODR, potINTS, potRATE, potAD_A, potAD_D;

// envelope generator
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;

#define LED 13 // shows if MIDI is being recieved

void HandleNoteOn(byte channel, byte note, byte velocity) {
  carrier_freq = mtof(note);
  setFreqs();
  envelope.noteOn();
  digitalWrite(LED, HIGH);
}

void HandleNoteOff(byte channel, byte note, byte velocity) {
  envelope.noteOff();
  digitalWrite(LED, LOW);
}

void setup(){
  pinMode(LED, OUTPUT);

  // Connect the HandleNoteOn function to the library, so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(HandleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(HandleNoteOff);  // Put only the name of the function
  MIDI.begin(MIDI_CHANNEL_OMNI);

  adsr_a = 50;
  adsr_d = 200;
  setEnvelope();

  wavetable = 0;
  setWavetable();

  // Set default parameters for any potentially unused/unread pots
  potcount = 0;
  potWAVT = 0;
  potMODR = 5;
  potINTS = 100;
  potRATE = 500;
  potAD_A = 50;
  potAD_D = 200;

  startMozzi();
}

void setEnvelope() {
  envelope.setADLevels(255, 64);
  envelope.setTimes(adsr_a, adsr_d, 10000, 200); // 10000 is so the note will sustain 10 seconds unless a noteOff comes
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
  MIDI.read();

  // Read the potentiometers - do one on each updateControl scan.
  // Note: each potXXXX value is remembered between scans.
  potcount ++;
  if (potcount >= 6) potcount = 0;
  switch (potcount) {
  case 0:
#ifdef WAVT_PIN
    potWAVT = mozziAnalogRead(WAVT_PIN) >> 8; // value is 0-3
#endif
    break;
  case 1:
#ifdef MODR_PIN
    potMODR = mozziAnalogRead(MODR_PIN) >> 7; // value is 0-7
#endif
    break;
  case 2:
#ifdef INTS_PIN
    potINTS = mozziAnalogRead(INTS_PIN); // value is 0-1023
#endif
    break;
  case 3:
#ifdef RATE_PIN
    potRATE = mozziAnalogRead(RATE_PIN); // value is 0-1023
#endif
    break;
  case 4:
#ifdef AD_A_PIN
    potAD_A = mozziAnalogRead(AD_A_PIN) >> 3; // value is 0-255
#endif
    break;
  case 5:
#ifdef AD_D_PIN
    potAD_D = mozziAnalogRead(AD_D_PIN) >> 3; // value is 0-255
#endif
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

  // See if the envelope changed...
  if ((potAD_A != adsr_a) || (potAD_D != adsr_d)) {
    // Change the envelope
    
    adsr_a = potAD_A;
    adsr_d = potAD_D;
    setEnvelope();
  }

  // Everything else we update every cycle anyway
  mod_ratio = potMODR;

  // Perform the regular "control" updates
  envelope.update();
  setFreqs();

  int INTS_calibrated = kMapIntensity(potINTS);

 // calculate the fm_intensity
  fm_intensity = ((long)INTS_calibrated * (kIntensityMod.next()+128))>>8; // shift back to range after 8 bit multiply

  // use a float here for low frequencies
  float mod_speed = (float)kMapModSpeed(potRATE)/1000;
  kIntensityMod.setFreq(mod_speed);
}


int updateAudio(){
  long modulation = aSmoothIntensity.next(fm_intensity) * aModulator.next();
  return (int)((envelope.next() * aCarrier.phMod(modulation)) >> 8);
}


void loop(){
  audioHook();
}
