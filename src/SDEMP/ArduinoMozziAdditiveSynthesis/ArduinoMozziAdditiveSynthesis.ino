/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Mozzi Additive Synthesis
//  https://diyelectromusic.wordpress.com/2020/08/22/arduino-mozzi-additive-synthesis/
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
#include <tables/sin2048_int8.h> // sine table for oscillators
#include <mozzi_midi.h>
#include <ADSR.h>

//#define DEBUG 1

MIDI_CREATE_DEFAULT_INSTANCE();

Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin0(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin3(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin4(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin5(SIN2048_DATA);
#define HARMONICS 5

// Set up the multipliers
#define NUMWAVES 6
int waves[NUMWAVES][HARMONICS] = {
   {1, 2, 2, 3, 3},
   {2, 3, 4, 5, 6},
   {2, 3, 5, 7, 9},
   {3, 5, 7, 9, 11},
   {2, 4, 6, 8, 10},
   {2, 4, 8, 16, 32}
};

int harmonics;
int carrier_freq;
long nextwave;
long lastwave;

int potcount;
int gain[HARMONICS+1];  // [0] is the fixed gain for the carrier
int gainscale;

// envelope generator
ADSR <CONTROL_RATE, AUDIO_RATE> envelope;

#define LED 13 // shows if MIDI is being recieved

void HandleNoteOn(byte channel, byte note, byte velocity) {
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

#ifdef DEBUG
  Serial.begin(9600);
#else
  // Connect the HandleNoteOn function to the library, so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(HandleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(HandleNoteOff);  // Put only the name of the function
  MIDI.begin(MIDI_CHANNEL_OMNI);
#endif

  envelope.setADLevels(255, 128);
  envelope.setTimes(50, 200, 10000, 200); // 10000 is so the note will sustain 10 seconds unless a noteOff comes

  potcount = 0;
  gain[0] = 255;
  startMozzi();

#ifdef DEBUG
  carrier_freq = 440;
  setFreqs();
#endif
}

void setFreqs(){
  aSin0.setFreq(carrier_freq);
  aSin1.setFreq(carrier_freq*waves[harmonics][0]);
  aSin2.setFreq(carrier_freq*waves[harmonics][1]);
  aSin3.setFreq(carrier_freq*waves[harmonics][2]);
  aSin4.setFreq(carrier_freq*waves[harmonics][3]);
  aSin5.setFreq(carrier_freq*waves[harmonics][4]);
}

void updateControl(){
  MIDI.read();

  // Read the potentiometers - do one on each updateControl scan.
  potcount ++;
  if (potcount >= 6) potcount = 0;

  if (potcount == 0) {
    int newharms = mozziAnalogRead(potcount) >> 7;  // Range 0 to 7
    if (newharms >= NUMWAVES) newharms = NUMWAVES-1;
    if (newharms != harmonics) {
      harmonics = newharms;
      setFreqs();
    }
  } else {
     gain[potcount] = mozziAnalogRead(potcount) >> 2;  // Range 0 to 255
  }

  // Pre-estimate the eventual scaling factor required based on the
  // combined gain by counting the number of divide by 2s required to
  // get the total back under 256.  This is used in updateAudio for scaling.
  gainscale = 0;
  long gainval = 255*(long)(gain[0]+gain[1]+gain[2]+gain[3]+gain[4]+gain[5]);
  while (gainval > 255) {
    gainscale++;
    gainval >>= 1;
  }

#ifdef DEBUG
  for (int i=0; i<6; i++) {
    Serial.print (gain[i]);
    Serial.print ("\t");
  }
  Serial.print (gainscale);
  Serial.print ("\t");
  Serial.print (lastwave);
  Serial.print ("\t");
  Serial.println (nextwave);
#endif

  // Perform the regular "control" updates
  envelope.update();
}


int updateAudio(){
  lastwave = ((long)gain[0]*aSin0.next() +
              gain[1]*aSin1.next() +
              gain[2]*aSin2.next() +
              gain[3]*aSin3.next() +
              gain[4]*aSin4.next() +
              gain[5]*aSin5.next()
                 );
  nextwave  = lastwave >> gainscale;

  return (int)(((long)envelope.next() * nextwave) >> 8);
}


void loop(){
  audioHook();
}
