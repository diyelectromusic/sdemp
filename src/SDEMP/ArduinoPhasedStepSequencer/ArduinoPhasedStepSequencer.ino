/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Phased Step Sequencer
//  https://diyelectromusic.wordpress.com/2021/07/24/arduino-phased-step-sequencer/
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
    Potentiometer        - https://www.arduino.cc/en/Tutorial/Potentiometer
    Arduino Digital Pins - https://www.arduino.cc/en/Tutorial/Foundations/DigitalPins
    Arduino Timer One    - https://www.arduino.cc/reference/en/libraries/timerone/
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
*/
#include <TimerOne.h>
#include <MIDI.h>

//#define TEST 1
//#define DBGMIDI 1
//#define DBGALG  1
//#define DBGTICK 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1  // Define which MIDI channel to transmit on (1 to 16).

#define TICK    5000  // 1 tick every 5 milliseconds
#define MINTICK  100  // Minimum number of ticks for the step

#define NUMTRACKS 8
uint16_t trackcnt[NUMTRACKS];
uint16_t trackmax[NUMTRACKS];
int tracksteps[NUMTRACKS];
int tracklast[NUMTRACKS];
#define SKIPTRACK 0xFF

#define NUMSTEPS 8
int stepnotes[NUMSTEPS];

// There are several threads of notes to play all from this series
#define NUMNOTES 11
byte notes[NUMNOTES] = {
  // Pentatonic
  60, // C4
  62, // D4
  64, // E4
  67, // G4
  69, // A4
  72, // C5
  74, // D5
  76, // E5
  79, // G5
  81, // A5
  84, // C6
};

// Each thread of notes can include a "modifier" for different effects.
// This is added to the note number, so a modifier of +12 plays up an octave
// and a modifier of -12 plays down an octave.
//
// Note: there is no checking that the modifier + notes in the above table
//       will still be in range... so don't include modifiers that take notes[]
//       below zero or above 127.
//
int modifier[NUMTRACKS] = {
  -24, -12, -5, 0, 0, +7, +12, +24,
};

// Need to set up the single analog pin to use for the MUX
#define MUXPIN A0
// And the digital lines to be used to control it.
// Four lines are required for 16 pots on the MUX.
// You can drop this down to three if only using 8 pots.
#define MUXLINES 4
int muxpins[MUXLINES]={A5,A4,A3,A2};
// Uncomment this if you want to control the MUX EN pin
// (optional, only required if you have several MUXs or
// are using the IO pins for other things too).
// This is active LOW.
//#define MUXEN    3

// We need one pot for each track and one pot for each step
#define NUMPOTS (NUMTRACKS+NUMSTEPS)

int muxAnalogRead (int mux) {
  if (mux >= NUMPOTS) {
    return 0;
  }

#ifdef MUXEN
  digitalWrite(MUXEN, LOW);  // Enable MUX
#endif

  // Translate mux pot number into the digital signal
  // lines required to choose the pot.
  for (int i=0; i<MUXLINES; i++) {
    if (mux & (1<<i)) {
      digitalWrite(muxpins[i], HIGH);
    } else {
      digitalWrite(muxpins[i], LOW);
    }
  }

  // And then read the analog value from the MUX signal pin
  int algval = analogRead(MUXPIN);

#ifdef MUXEN
  digitalWrite(MUXEN, HIGH);  // Disable MUX
#endif
  
#ifdef TEST
#ifdef DBGALG
  Serial.print(mux);
  Serial.print(" --> algval=");
  Serial.println(algval);
#endif
#endif

  return algval;
}

void muxInit () {
  for (int i=0; i<MUXLINES; i++) {
    pinMode(muxpins[i], OUTPUT);
    digitalWrite(muxpins[i], LOW);
  }

#ifdef MUXEN
  pinMode(MUXEN, OUTPUT);
  digitalWrite(MUXEN, HIGH); // start disabled
#endif
}

void doTick () {
  // On each tick, we need to do the following:
  //   Update each step's counter.
  //   Compare it with the timeout value.
  //   If timed out, toggle the step.
  for (int i=0; i<NUMTRACKS; i++) {
    if (trackmax[i] == SKIPTRACK) {
      // Stop any existing playing note
      if (tracklast[i] != SKIPTRACK) {
        stopStep(i, tracklast[i]);
        tracklast[i] = SKIPTRACK;

        // Clear counters/steps ready for next time
        trackcnt[i] = 0;
        tracksteps[i] = 0;
      }
    } else {
      trackcnt[i]++;
      if (trackcnt[i] >= trackmax[i]) {
        trackcnt[i] = 0;

        // Update the step counter for this track and play the next note
        tracksteps[i]++;
        if (tracksteps[i] >= NUMSTEPS) {
          tracksteps[i] = 0;
        }
        tracklast[i] = tracksteps[i];
        playStep(i, tracklast[i], tracksteps[i]);
      }
    }
  }
#ifdef TEST
#ifdef DBGTICK
  Serial.print("Max: ");
  for (int i=0; i<NUMTRACKS; i++) {
    Serial.print(trackmax[i]);
    Serial.print("\t");
  }
  Serial.print("   Count: ");
  for (int i=0; i<NUMTRACKS; i++) {
    Serial.print(trackcnt[i]);
    Serial.print("\t");
  }
  Serial.print("\n");
#endif
#endif
}

void stopStep (int track, int laststep) {
  if (laststep == SKIPTRACK) {
    return;
  }

  byte lastnote = stepnotes[laststep] + modifier[track];
#ifndef TEST
  MIDI.sendNoteOff(lastnote, 0, MIDI_CHANNEL);
#endif
}

void playStep (int track, int laststep, int nextstep) {
  // There is an optional modifier for each track to
  // allow for different effects, so need to add that
  // here before playing the note...
  byte nextnote = stepnotes[nextstep] + modifier[track];
  stopStep (track, laststep);

#ifdef TEST
#ifdef DBGMIDI
  Serial.print(track);
  Serial.print(": OFF: ");
  Serial.print(laststep);
  Serial.print("  ON: ");
  Serial.print(nextstep);
  Serial.print(" -> ");
  Serial.print(nextnote);
  Serial.print("\n");
#endif
#else
  MIDI.sendNoteOn(nextnote, 127, MIDI_CHANNEL);
#endif
}

void setup() {
#ifdef TEST
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL_OFF);
#endif

  for (int i=0; i<NUMTRACKS; i++) {
    tracksteps[i] = 0;
    trackmax[i] = SKIPTRACK;
    trackcnt[i] = 0;
  }

  muxInit();

  delay(1000);
  Timer1.initialize(TICK);
  Timer1.attachInterrupt(doTick);
}

void loop() {
#ifdef TEST
  delay(1000);
#endif
  // First read the pots to set timings for the tracks
  for (int i=0; i<NUMTRACKS; i++) {
    // Scale timings to fit range from desired minimum "tick"
    int algval = muxAnalogRead(i);
    if (algval>1020) {
      // Disable this track
      trackmax[i] = SKIPTRACK;
    } else {
      trackmax[i] = MINTICK + algval;
    }
  }

  // Then read the pots defining the notes to play on the steps
  for (int i=0; i<NUMSTEPS; i++) {
    // Scale to required note range
    // Note: first "step" pot is after last "track" pot
    stepnotes[i] = notes[map(muxAnalogRead(i+NUMTRACKS), 0,1023, 0, NUMNOTES-1)];
  }
}
