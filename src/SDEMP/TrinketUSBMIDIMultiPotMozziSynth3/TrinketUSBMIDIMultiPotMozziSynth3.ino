/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Trinket USB-MIDI Multi Pot Mozzi FM Synthesis - Part 3
//  https://diyelectromusic.wordpress.com/2021/03/28/trinket-usb-midi-multi-pot-mozzi-synthesis-part-3/
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
    USB Host Library SAMD - https://github.com/gdsports/USB_Host_Library_SAMD
    UHS2-MIDI(*)          - https://github.com/YuuichiAkagawa/Arduino-UHS2MIDI

  (*) This was written for the original USB Host Library and needs some changes
      to run with the SAMD version of the library.  See my blog post for details!

  Much of this code is based on the Mozzi example Knob_LightLevel_x2_FMsynth (C) Tim Barrass

*/
#include <MozziGuts.h>
#include <Oscil.h> // oscillator
#include <tables/cos2048_int8.h> // for the modulation oscillators
#include <tables/sin2048_int8.h> // sine table for oscillator
#include <tables/saw2048_int8.h> // saw table for oscillator
#include <tables/triangle2048_int8.h> // triangle table for oscillator
#include <tables/square_no_alias_2048_int8.h> // square table for oscillator
#include <mozzi_midi.h>
#include <Smooth.h>
#include <ADSR.h>
#include "avdweb_AnalogReadFast.h"

//#define TEST_TIMING 1   // 1=AUDIO_RATE; 2=CONTROL_RATE; 3=audioHook; 4=MIDI Loop

// Uncomment the following to act as a USB Host device.
// Comment it out to act as a USB device.
#define USBHnotD 1

#ifdef USBHnotD
// Uses USB Host Library Transport for the MIDI library
#include "UHS2-MIDI.h"
USBHost UsbH;
UHS2MIDI_CREATE_DEFAULT_INSTANCE(&UsbH);
#else
// Uses USB MIDI Transport for the MIDI library
#include <USB-MIDI.h>
USBMIDI_CREATE_DEFAULT_INSTANCE();
#endif

// Set the MIDI Channel to listen on
#define MIDI_CHANNEL 1

#if (ARDUINO_ARCH_SAMD)
#define ALGREAD analogReadFast
#else
#define ALGREAD mozziAnalogRead
#endif

#define POT_ZERO 15 // Anything below this value is treated as "zero"

// Set up the analog inputs - comment out if you aren't using this one
#define WAVT_PIN 3  // Wavetable
#define INTS_PIN 4  // FM intensity
#define RATE_PIN 2  // Modulation Rate
#define MODR_PIN 1  // Modulation Ratio

// Default potentiometer values if no pot defined
#define DEF_potWAVT 2
#define DEF_potMODR 5
#define DEF_potINTS 500
#define DEF_potRATE 150

// ADSR default parameters in mS
#define ADSR_A      150
#define ADSR_D      200
#define ADSR_S  1000000  // Large so the note will sustain unless a noteOff comes
#define ADSR_R      200
#define ADSR_ALVL   250  // Level 0 to 255
#define ADSR_DLVL   200  // Level 0 to 255

#define CONTROL_RATE 256   // Hz, powers of 2 are most reliable
#define MIDI_RATE    16    // Cycles through the loop() function

#define NUM_OSC 4

Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aCarrier[NUM_OSC];
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aModulator[NUM_OSC];
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kIntensityMod(COS2048_DATA);

int wavetable;
int mod_ratio;
int mod_rate;
int carrier_freq[NUM_OSC];
long fm_intensity;
int playing[NUM_OSC];
int playidx;

// Comment this out to ignore new notes in polyphonic modes
//#define POLY_OVERWRITE 1

// smoothing for intensity to remove clicks on transitions
float smoothness = 0.95f;
Smooth <long> aSmoothIntensity(smoothness);

int potcount;
int potWAVT, potMODR, potINTS, potRATE;

// envelope generator
ADSR <CONTROL_RATE, AUDIO_RATE> envelope[NUM_OSC];

#define LED LED_BUILTIN // shows if MIDI is being recieved

#ifdef TEST_TIMING
#define testTiming(t,v) if (TEST_TIMING==t) digitalWrite(LED_BUILTIN, v);
#else
#define testTiming(t,v)
#endif

void noteOn (byte note, int osc) {
  carrier_freq[osc] = mtof(note);
  setFreqs(osc);
  envelope[osc].noteOn();
}

void noteOff (int osc) {
  envelope[osc].noteOff();
}

void HandleNoteOn(byte channel, byte note, byte velocity) {
   if (velocity == 0) {
      HandleNoteOff(channel, note, velocity);
      return;
  }

#ifndef TEST_TIMING
  digitalWrite(LED, HIGH);
#endif

#ifdef POLY_OVERWRITE
  // Overwrites the oldest note playing
  playidx++;
  if (playidx >= NUM_OSC) playidx=0;
  playing[playidx] = note;
  noteOff(playidx);
  noteOn(note, playidx);
#else
  // Ignore new notes until there is space
  for (int i=0; i<NUM_OSC; i++) {
    if (playing[i] == 0) {
      playing[i] = note;
      noteOn(note, i);
      return;
    }
  }
#endif
}

void HandleNoteOff(byte channel, byte note, byte velocity) {
  for (int i=0; i<NUM_OSC; i++) {
    if (playing[i] == note) {
      // If we are still playing the same note, turn it off
      noteOff(i);
      playing[i] = 0;
    }
  }
#ifndef TEST_TIMING
  digitalWrite(LED, LOW);
#endif
}

void setup(){
  pinMode(LED, OUTPUT);

#ifdef USBHnotD
  // USB Host MIDI Initialisation
  if (UsbH.Init() == -1) {
    while (1); // Everything stops if it fails...
  }
#endif

  // SAMD21 has 12-bit ADCs but the default is Arduino compatibility of 10-bit ADCs
  // I stick with the default as I'm not using increased resolution for this.
  analogReadResolution(10);

  // Connect the HandleNoteOn function to the library, so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(HandleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(HandleNoteOff);  // Put only the name of the function
  MIDI.begin(MIDI_CHANNEL);

  mod_rate = -1;  // Force an update on first control scan...
  wavetable = 0;
  setWavetable();

  for (int i=0; i<NUM_OSC; i++) {
    envelope[i].setADLevels(ADSR_ALVL, ADSR_DLVL);
    envelope[i].setTimes(ADSR_A, ADSR_D, ADSR_S, ADSR_R);
    aModulator[i].setTable(COS2048_DATA);
  }

  // Set default parameters for any potentially unused/unread pots
  potcount = 0;

  startMozzi(CONTROL_RATE);
}

void setFreqs(int osc){
  //calculate the modulation frequency to stay in ratio
  int mod_freq = carrier_freq[osc] * mod_ratio;

  // set the FM oscillator frequencies
  aCarrier[osc].setFreq(carrier_freq[osc]);
  aModulator[osc].setFreq(mod_freq);
}

void setWavetable() {
  for (int i=0; i<NUM_OSC; i++) {
    switch (wavetable) {
    case 1:
      aCarrier[i].setTable(TRIANGLE2048_DATA);
      break;
    case 2:
      aCarrier[i].setTable(SAW2048_DATA);
      break;
    case 3:
      aCarrier[i].setTable(SQUARE_NO_ALIAS_2048_DATA);
      break;
    default: // case 0
      aCarrier[i].setTable(SIN2048_DATA);
    }
  }
}

void scanMidi () {
  // We can't use the MIDI library setting to read a whole message in one go,
  // so instead, call it a few times and return when we have a complete message
  for (int i=0; i<6; i++) {
    if (MIDI.read()) {
      // We have a complete message so process it
      return;
    }
  }
}

void scanUsb () {
#ifdef USBHnotD
    // USB Host Handling
    UsbH.Task();
#endif  
}

void updateControl(){
  testTiming(2, HIGH);

  // Read the potentiometers.  This is one of the most time-intensive tasks
  // so only read one at a time.  We can also use this for other tasks that
  // don't need to be performed on every control scan.
  //
  // Note: each potXXXX value is remembered between scans.
  potcount ++;
  switch (potcount) {
  case 1:
#ifdef WAVT_PIN
    potWAVT = ALGREAD(WAVT_PIN) >> 8; // value is 0-3
#else
    potWAVT = DEF_potWAVT;
#endif
    break;
  case 2:
#ifdef INTS_PIN
    potINTS = ALGREAD(INTS_PIN); // value is 0-1023
    if (potINTS<POT_ZERO) potINTS = 0;
#else
    potINTS = DEF_potINTS
#endif
    break;
  case 3:
#ifdef RATE_PIN
    potRATE = ALGREAD(RATE_PIN); // value is 0-1023
    if (potRATE<POT_ZERO) potRATE = 0;
#else
    potRATE = DEF_potRATE;
#endif
    break;
  case 4:
#ifdef MODR_PIN
    potMODR = ALGREAD(MODR_PIN) >> 7; // value is 0-7
#else
    potMODR = DEF_potMODR;
#endif
    break;
  case 5:
    // Not a pot obviously, but update the wave table on scans when
    // we're not reading a pot to help keep the control loop short.
    if (potWAVT != wavetable) {
      // Change the wavetable
      wavetable = potWAVT;
      setWavetable();
    }
    break;
 default:
    potcount = 0;
 }

  // Everything else from this point on has to be updated at the
  // CONTROL_RATE - oscillators, envelopes, etc.

  mod_ratio = potMODR;

  // use a float here for low frequencies
  if (potRATE != mod_rate) {
    mod_rate = potRATE;
    float mod_speed = (float)potRATE/100;
    kIntensityMod.setFreq(mod_speed);
  }

  // calculate the fm_intensity
  if (potRATE==0) {
     fm_intensity = (long)potINTS;
  } else {
     fm_intensity = ((long)potINTS * (kIntensityMod.next()+128))>>8; // shift back to range after 8 bit multiply
  }

  for (int i=0; i<NUM_OSC; i++) {
    envelope[i].update();
    setFreqs(i);
  }
  testTiming(2, LOW);
}


int updateAudio(){
  testTiming(1, HIGH);
  long output=0;
  long fmmod = aSmoothIntensity.next(fm_intensity);
  for (int i=0; i<NUM_OSC; i++) {
    // Each .next() function is an 8-bit value. Multiplying together gives a 16-bit value.
    // Adding a number of 16-bit values together gives a larger value depending on the
    // number of values added - so adding 2 together yeilds a 17-bit value.  Adding 4
    // could get into a 18-bit value.  8 gets 19-bits and so on.
    //
    // Whilst we are adding the oscillators together, add them up in a long (32-bit)
    // intermediate value to keep resolution then bit-shift down to a 16-bit
    // value prior to returning using the From16Bit function to scale it appropriately
    // to the required native bit-accuracy of the current platform.
    //
    long modulation = fmmod * aModulator[i].next();
    output += (envelope[i].next() * aCarrier[i].phMod(modulation));
  }

  // Scale back from >16 bits to 16 bits.  Allow for up to 8 oscillators here,
  // by shifting by up to 3 (allowing up to a 19-bit value).
#if (NUM_OSC==1)
  // leave as is
#elif (NUM_OSC==2)
  output = output >> 2;  // 17 to 16 bits
#elif (NUM_OSC==3)
  output = output >> 3;  // 18 to 16 bits
#elif (NUM_OSC==4)
  output = output >> 3;  // 18 to 16 bits
#else
  output = output >> 4;  // 19 to 16 bits
#endif
  // Finally allow Mozzi to scale from this 16-bit intermediary value to the platforms
  // required sample size.
  testTiming(1, LOW);
  return MonoOutput::from16Bit((uint16_t)output);
}

int midicount;
int timingLED=LOW;
void loop(){
  testTiming(3, timingLED);
  timingLED = !timingLED;

  midicount++;
  if (midicount == (MIDI_RATE)/2) {
    scanUsb();
  } else if (midicount > MIDI_RATE) {
    midicount = 0;
    testTiming(4, HIGH);
    scanMidi();
    testTiming(4, LOW);
  }

  audioHook();
}
