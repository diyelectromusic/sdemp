/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Trinket USB-MIDI Multi Pot Mozzi FM Synthesis
//  https://diyelectromusic.wordpress.com/2021/03/22/trinket-usb-midi-multi-pot-mozzi-synthesis/
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
#define ALGREAD analogRead
#else
#define ALGREAD mozziAnalogRead
#endif

// Set up the analog inputs - comment out if you aren't using this one
#define WAVT_PIN 3  // Wavetable
#define INTS_PIN 4  // FM intensity
#define RATE_PIN 2  // Modulation Rate
#define MODR_PIN 1  // Modulation Ratio

// ADSR default parameters in mS
#define ADSR_A      100
#define ADSR_D      200
#define ADSR_S  1000000  // Large so the note will sustain unless a noteOff comes
#define ADSR_R      200
#define ADSR_ALVL   255  // Level 0 to 255
#define ADSR_DLVL   200  // Level 0 to 255

#define CONTROL_RATE 256 // Hz, powers of 2 are most reliable

Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aCarrier;  // Wavetable will be set later
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aModulator(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kIntensityMod(COS2048_DATA);

int wavetable;
int mod_ratio;
int mod_rate;
int carrier_freq;
long fm_intensity;

// smoothing for intensity to remove clicks on transitions
float smoothness = 0.95f;
Smooth <long> aSmoothIntensity(smoothness);

int potcount;
int potWAVT, potMODR, potINTS, potRATE;

// envelope generator
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;

#define LED LED_BUILTIN // shows if MIDI is being recieved

void HandleNoteOn(byte channel, byte note, byte velocity) {
   if (velocity == 0) {
      HandleNoteOff(channel, note, velocity);
      return;
  }
  envelope.noteOff(); // Stop any already playing note
  carrier_freq = mtof(note);
  setFreqs();
  envelope.noteOn();
  digitalWrite(LED, HIGH);
}

void HandleNoteOff(byte channel, byte note, byte velocity) {
  if (carrier_freq == mtof(note)) {
    // If we are still playing the same note, turn it off
    envelope.noteOff();
  }

  digitalWrite(LED, LOW);
}

void setup(){
  pinMode(LED, OUTPUT);

  // Connect the HandleNoteOn function to the library, so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(HandleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(HandleNoteOff);  // Put only the name of the function
  MIDI.begin(MIDI_CHANNEL);

  mod_rate = 0;
  wavetable = 0;
  setWavetable();

  envelope.setADLevels(ADSR_ALVL, ADSR_DLVL);
  envelope.setTimes(ADSR_A, ADSR_D, ADSR_S, ADSR_R);

  // Set default parameters for any potentially unused/unread pots
  potcount = 0;
  potWAVT = 2;
  potMODR = 5;
  potINTS = 500;
  potRATE = 150;

#ifdef USBHnotD
  // USB Host MIDI Initialisation
  if (UsbH.Init() == -1) {
    while (1); // Everything stops if it fails...
  }
#endif

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
#ifdef USBHnotD
  // USB Host MIDI Handling
  UsbH.Task();
#endif
  MIDI.read();

  // Read the potentiometers - do one on each updateControl scan.
  // Note: each potXXXX value is remembered between scans.
  potcount ++;
  if (potcount >= 4) potcount = 0;
  switch (potcount) {
  case 0:
#ifdef WAVT_PIN
    potWAVT = ALGREAD(WAVT_PIN) >> 8; // value is 0-3
#endif
    break;
  case 1:
#ifdef INTS_PIN
    potINTS = ALGREAD(INTS_PIN); // value is 0-1023
#endif
    break;
  case 2:
#ifdef RATE_PIN
    potRATE = ALGREAD(RATE_PIN); // value is 0-1023
#endif
    break;
  case 3:
#ifdef MODR_PIN
    potMODR = ALGREAD(MODR_PIN) >> 7; // value is 0-7
#endif
    break;
 default:
    potcount = 0;
  }

  MIDI.read();

  // See if the wavetable changed...
  if (potWAVT != wavetable) {
    // Change the wavetable
    wavetable = potWAVT;
    setWavetable();
  }

  // Everything else we update every cycle anyway
  mod_ratio = potMODR;

  // Perform the regular "control" updates
  envelope.update();
  setFreqs();

  MIDI.read();

  // calculate the fm_intensity
  if (potRATE==0) {
     fm_intensity = (long)potINTS;
  } else {
     fm_intensity = ((long)potINTS * (kIntensityMod.next()+128))>>8; // shift back to range after 8 bit multiply
  }

  // use a float here for low frequencies
  if (potRATE != mod_rate) {
    mod_rate = potRATE;
    float mod_speed = (float)potRATE/100;
    kIntensityMod.setFreq(mod_speed);
  }
}


int updateAudio(){
  long modulation = aSmoothIntensity.next(fm_intensity) * aModulator.next();
  return MonoOutput::from8Bit((envelope.next() * aCarrier.phMod(modulation)) >> 8);
}


void loop(){
  audioHook();
}
