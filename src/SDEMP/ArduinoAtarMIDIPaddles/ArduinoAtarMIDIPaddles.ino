/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Atari MIDI Paddles
//  https://diyelectromusic.com/2025/06/23/arduino-midi-atari-paddles/
//
      MIT License
      
      Copyright (c) 2025 diyelectromusic (Kevin)
      
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
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Arduino Atari Paddles - https://diyelectromusic.com/2025/06/16/atari-2600-controller-shield-pcb-revisited/
*/
#include <TimerOne.h>
#include <MIDI.h>
#include "avgeAnalog.h"

//#define TEST

MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 1  // MIDI Channel number (1 to 16, or MIDI_CHANNEL_OMNI)

#define NUMPADS 4

int midiCC[NUMPADS] = {
  0x01,  // Modulation wheel
//  0x02,  // Breath controller
//  0x04,  // Foot controller
//  0x05,  // Portamento time
  0x07,  // Channel volume
//  0x08,  // Balance
//  0x0A,  // Pan
  0x0B,  // Expression control
//  0x0C,  // Effect control 1
//  0x0D,  // Effect control 2
  0x10,  // General purpose control 1
//  0x11,  // General purpose control 2
//  0x12,  // General purpose control 3
//  0x13,  // General purpose control 4
//  0x14,  // General purpose control 5
//  0x15,  // General purpose control 6
//  0x16,  // General purpose control 7
//  0x17,  // General purpose control 8
};

////////////////////////////////////////////////////////
//
// Atari paddle control reading.
//
// Everything from this point on
// is hard coded for reading 4 paddles
// on A0, A1, A2, A3 only.
//
int pad_pins[NUMPADS]={A0, A1, A2, A3};
#define RAW_START   10  // Equivalent TICKS count for alg=0
#define RAW_END    350  // Equivalent TICKS count for alg=1023
#define RAW_BREAK  1000 // Max #TICKs
#define RAW_TICK   100  // TICK in uS

// Note: scan all analog values together.
unsigned padState;
unsigned padCount[4];
unsigned atariValue[4];

void atariAnalogSetup() {
  Timer1.initialize(RAW_TICK);
  Timer1.attachInterrupt(atariAnalogScan);
  padState = 0;
}

// Occurs every timer interrupt.
void atariAnalogScan (void) {
  if (padState == 0) {
    // Drop the lines to GND.
    // Set all to OUTPUTS.
    // Set all LOW.
    // Set all back to INPUTS.
    DDRC  = DDRC | 0x0F;     // A0-A3 set to OUTPUT
    PORTC = PORTC & ~(0x0F); // A0-A3 set to LOW (0)
    padState++;
  } else if (padState == 1) {
    DDRC  = DDRC & ~(0x0F);  // A0-A3 set to INPUT
    for (int i=0; i<4; i++) {
      padCount[i] = 0;
    }
    padState++;
  } else if (padState > RAW_BREAK) {
    // Times up, so assume all values are now fixed.
    for (int i=0; i<4; i++) {
      atariValue[i] = 1023 - map(constrain(padCount[i],RAW_START,RAW_END),RAW_START,RAW_END,0,1023);
    }

    // Now start the whole process off again
    padState = 0;
  } else {
    for (int i=0; i<4; i++) {
      if ((PINC & (1<<i)) == 0) {
        // Haven't reached max value yet
        padCount[i]++;
      }
    }
    // Update overall counter
    padState++;
  }
}

int atariAnalogRead (int pin) {
  if ((pin >= A0) && (pin <= A3)) {
    return atariValue[pin-A0];
  } else {
    return -1;
  }
}

// Atari paddle control reading - End
//
////////////////////////////////////////////////////////

class AtariPot : public AvgePot {
public:
  AtariPot (int potpin) : AvgePot (potpin) {}

  unsigned avgeReader(int potpin) override  {
    return atariAnalogRead(potpin);
  }
};
AtariPot *atariPot[NUMPADS];

uint16_t lastCC[NUMPADS];

void setup() {
#ifdef TEST
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL);
#endif
  atariAnalogSetup();
  for (int i=0; i<NUMPADS; i++) {
    lastCC[i] = 0;
    atariPot[i] = new AtariPot(pad_pins[i]);
  }
}

void loop() {
#ifndef TEST
  MIDI.read(); // For MIDI THRU
#endif
  for (int i=0; i<NUMPADS; i++) {
    // Convert from 10 bit to 7 bit
    unsigned val = atariPot[i]->avgeAnalogRead()>>3;
    if (val != lastCC[i]) {
#ifndef TEST
      MIDI.sendControlChange(midiCC[i], val, MIDI_CHANNEL);
#endif
      lastCC[i] = val;
    }
#ifdef TEST
    Serial.print(val);
    Serial.print("\t");
    Serial.print(analogRead(A0+i));
    Serial.print("\t");
#endif
  }
#ifdef TEST
  Serial.print("\n");
#endif
}
