/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Mozzi Multi Pot MIDI FM Synthesis
//  https://diyelectromusic.wordpress.com/2020/08/21/arduino-multi-pot-mozzi-fm-synthesis/
//
      MIT License
      
      Copyright (c) 2022 diyelectromusic (Kevin)
      
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
//#include <AutoMap.h> // maps unpredictable inputs to a range
#include <ADSR.h>

// Set the MIDI Channel to listen on
#define MIDI_CHANNEL 1

// Uncomment this to pass additional notes on via MIDI OUT
//#define MIDI_PASS_THRU 1

#define POT_ZERO 15 // Anything below this value is treated as "zero"
//#define POT_REVERSE 1 // Uncomment if you want to reverse the direction of the pots

// Set up the analog inputs - comment out if you aren't using this one
#define WAVT_PIN 0  // Wavetable
#define INTS_PIN 1  // FM intensity
#define RATE_PIN 2  // Modulation Rate
#define MODR_PIN 3  // Modulation Ratio
#define AD_A_PIN 4  // ADSR Attack
#define AD_D_PIN 5  // ADSR Delay
//#define AD_R_PIN 6  // ADSR Release
//#define FREQ_PIN 7  // Optional Frequency Control

//#define FREQ_CONT // No ADSR/Trigger - continuous output

//#define TRIGGER_PIN  2  // Optional button trigger
//#define TRIGGER_NOTE 60 // MIDI Note to play if button pressed

// Default potentiometer values if no pot defined
#define DEF_potWAVT 2
#define DEF_potMODR 5
#define DEF_potINTS 500
#define DEF_potRATE 150

// ADSR default parameters in mS
#define ADSR_A       50
#define ADSR_D      200
#define ADSR_S    60000  // Large so the note will sustain unless a noteOff comes
#define ADSR_R      200
#define ADSR_ALVL   250  // Level 0 to 255
#define ADSR_DLVL    64  // Level 0 to 255

#define MIN_FREQ    220  // Range min to min+1023

//#define TEST_NOTE 50 // Comment out to remove test without MIDI
//#define DEBUG     1  // Comment out to remove debugging info - can only be used with TEST_NOTE
                       // Note: This will probably cause "clipping" of the audio...

#ifndef TEST_NOTE
struct MySettings : public MIDI_NAMESPACE::DefaultSettings {
  static const bool Use1ByteParsing = false; // Allow MIDI.read to handle all received data in one go
  static const long BaudRate = 31250;        // Doesn't build without this...
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings);
#endif

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
int adsr_a, adsr_d, adsr_r;
int testcount;
int playing;
int lastfreq;
int trigbtn;

// smoothing for intensity to remove clicks on transitions
float smoothness = 0.95f;
Smooth <long> aSmoothIntensity(smoothness);

int potcount;
int potWAVT, potMODR, potINTS, potRATE, potAD_A, potAD_D, potAD_R, potFREQ;

// envelope generator
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;

#define LED LED_BUILTIN // shows if MIDI is being recieved

void HandleNoteOn(byte channel, byte note, byte velocity) {
   if (velocity == 0) {
      HandleNoteOff(channel, note, velocity);
      return;
  }
#ifdef MIDI_PASS_THRU
  if (playing != 0) {
    // Already playing a note, so pass this one on
#ifndef TEST_NOTE
    MIDI.sendNoteOn(note, velocity, channel);
#endif
    return;
  }
#endif

  envelope.noteOff(); // Stop any already playing note
  carrier_freq = mtof(note);
  setFreqs();
  envelope.noteOn();
  playing = note;
  digitalWrite(LED, HIGH);
}

void HandleNoteOff(byte channel, byte note, byte velocity) {
  if (playing == note) {
    // If we are still playing the same note, turn it off
    envelope.noteOff();
    playing = 0;
  } else {
#ifdef MIDI_PASS_THRU
    // Note message wasn't for us, so pass it on
#ifndef TEST_NOTE
    MIDI.sendNoteOff(note, velocity, channel);
#endif
#endif
  }

  digitalWrite(LED, LOW);
}

void setup(){
  pinMode(LED, OUTPUT);

#ifdef TEST_NOTE
#ifdef DEBUG
  Serial.begin(9600);
#endif
#else
  // Connect the HandleNoteOn function to the library, so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(HandleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(HandleNoteOff);  // Put only the name of the function
  MIDI.begin(MIDI_CHANNEL);
#ifdef MIDI_PASS_THRU
  // Disable automatic THRU handling
  MIDI.turnThruOff();
#endif
#endif

#ifdef TRIGGER_PIN
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
#endif

  adsr_a = ADSR_A;
  adsr_d = ADSR_D;
  adsr_r = ADSR_R;
  setEnvelope();

  mod_rate = -1;  // Force an update on first control scan...
  wavetable = 0;
  setWavetable();

  // Set default parameters for any potentially unused/unread pots
  potcount = 0;
  playing = 0;
  lastfreq = 0;

  startMozzi(CONTROL_RATE);
}

void setEnvelope() {
  envelope.setADLevels(ADSR_ALVL, ADSR_DLVL);
  envelope.setTimes(adsr_a, adsr_d, ADSR_S, adsr_r);
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

int myAnalogRead (int pot) {
#ifdef POT_REVERSE
  return 1023 - mozziAnalogRead(pot);
#else
  return mozziAnalogRead(pot);
#endif
}

void updateControl(){
#ifndef TEST_NOTE
  MIDI.read();
#endif

#ifdef TRIGGER_PIN
  int trig = digitalRead(TRIGGER_PIN);
  if (trigbtn == HIGH && trig == LOW) {
    // Button has been pressed
    HandleNoteOn (MIDI_CHANNEL, TRIGGER_NOTE, 64);
  }
  else if (trigbtn == LOW && trig == HIGH) {
    // Button has been released
    HandleNoteOff (MIDI_CHANNEL, TRIGGER_NOTE, 0);
  }
  else
  {
    // Ignore button
  }
  trigbtn = trig;
#endif

  // Read the potentiometers - do one on each updateControl scan.
  // Note: each potXXXX value is remembered between scans.
  potcount ++;
  if (potcount >= 7) potcount = 0;
  switch (potcount) {
  case 0:
#ifdef WAVT_PIN
    potWAVT = myAnalogRead(WAVT_PIN) >> 8; // value is 0-3
#else
    potWAVT = DEF_potWAVT;
#endif
    break;
  case 1:
#ifdef INTS_PIN
    potINTS = myAnalogRead(INTS_PIN); // value is 0-1023
    if (potINTS<POT_ZERO) potINTS = 0;
#else
    potINTS = DEF_potINTS;
#endif
    break;
  case 2:
#ifdef RATE_PIN
    potRATE = myAnalogRead(RATE_PIN); // value is 0-1023
    if (potRATE<POT_ZERO) potRATE = 0;
#else
    potRATE = DEF_potRATE;
#endif
    break;
  case 3:
#ifdef MODR_PIN
    potMODR = myAnalogRead(MODR_PIN) >> 7; // value is 0-7
#else
    potMODR = DEF_potMODR;
#endif
    break;
  case 4:
#ifdef AD_A_PIN
    potAD_A = myAnalogRead(AD_A_PIN); // value is 0-1023
#else
    potAD_A = ADSR_A;
#endif
    break;
  case 5:
#ifdef AD_D_PIN
    potAD_D = myAnalogRead(AD_D_PIN); // value is 0-1023
#else
    potAD_D = ADSR_D;
#endif
  case 6:
#ifdef AD_R_PIN
    potAD_R = myAnalogRead(AD_R_PIN); // value is 0-1023
#else
    potAD_R = ADSR_R;
#endif
  case 7:
#ifdef FREQ_PIN
    potFREQ = myAnalogRead(FREQ_PIN); // value is 0-1023
    if (potFREQ<POT_ZERO) potFREQ = 0;
#else
    potFREQ = 0; // Disabled
#endif
    break;
  default:
    potcount = 0;
  }

#ifdef TEST_NOTE
#ifdef DEBUG
  Serial.print(potWAVT); Serial.print("\t");
  Serial.print(potINTS); Serial.print("\t");
  Serial.print(potRATE); Serial.print("\t");
  Serial.print(potMODR); Serial.print("\t");
  Serial.print(potAD_A); Serial.print("\t");
  Serial.print(potAD_D); Serial.print("\t");
  Serial.print(potAD_R); Serial.print("\t");
  Serial.print(potFREQ); Serial.print("\t");
#endif
#endif

  // See if the wavetable changed...
  if (potWAVT != wavetable) {
    // Change the wavetable
    wavetable = potWAVT;
    setWavetable();
  }

  // See if the envelope changed...
  if ((potAD_A != adsr_a) || (potAD_D != adsr_d) || (potAD_R != adsr_r)) {
    // Change the envelope
    
    adsr_a = potAD_A;
    adsr_d = potAD_D;
    adsr_r = potAD_R;
    setEnvelope();
  }

#ifdef FREQ_PIN
  // See if the frequency changed...
  // NB: MIDI Notes will take precedent
  if (lastfreq != potFREQ) {
    if (lastfreq == 0) {
      // This is the first time the sound starts, so play the envelope
      envelope.noteOn();
    }
    if (potFREQ == 0) {
      // The frequency needs turning off
      envelope.noteOff();
    } else {
      if (playing) {
        // Use the sounding note as the starting point
        carrier_freq = mtof(playing) + potFREQ;
      } else {
        carrier_freq = MIN_FREQ + potFREQ;
      }
      setFreqs();
    }
  }
  lastfreq = potFREQ;
#endif

  // Everything else we update every cycle anyway
  mod_ratio = potMODR;

  // Perform the regular "control" updates
  envelope.update();
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

#ifdef TEST_NOTE
#ifdef DEBUG
  Serial.print(fm_intensity); Serial.print("\t");
  Serial.print(mod_speed); Serial.print("\n");
#endif
  testcount++;
  if (testcount == 100) {
    HandleNoteOn (1, 50, 127);
  } else if (testcount > 300) {
    testcount = 0;
    HandleNoteOff (1, 50, 0);
  }
#endif
}


int updateAudio(){
  long modulation = aSmoothIntensity.next(fm_intensity) * aModulator.next();
#ifdef FREQ_CONT
  return (int)(aCarrier.phMod(modulation));
#else
  return (int)((envelope.next() * aCarrier.phMod(modulation)) >> 8);
#endif
}


void loop(){
  audioHook();
}
