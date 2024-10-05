/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Euclidian Clock Gate
//  https://diyelectromusic.com/2024/10/04/arduino-euclidean-gate-sequencer/
//
      MIT License
      
      Copyright (c) 2024 diyelectromusic (Kevin)
      
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
#include "TimerOne.h"

//#define TIMER_TOGGLE 2

// Can do GATE or TRIGGER.  Uncomment for TRIGGER
//#define TRIGGER_OUTPUT

// -------------------------------------------
//    Sequencer IO and Patterns
// -------------------------------------------
#define GATES    6
int gatePins[GATES] = {8,9,10,11,12,13};   // Set digital pins for GATE outputs
int initPatterns[GATES] = {3,5,7,11,13,4}; // Initial patterns to use for each GATE
#define TEMPO_POT  A0   // Comment out if not using a pot
#define INIT_TEMPO 25   // Initial tempo if not using a pot

#define STEPS    16
#define PATTERNS (STEPS+1)
#define DEFAULT_PATTERN 4
const uint8_t patterns[PATTERNS][STEPS] PROGMEM = {
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},  // 0
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},  // 1
  {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},  // 2
  {1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0},  // 3
  {1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0},  // 4
  {1,0,0,0,1,0,0,1,0,0,1,0,0,1,0,0},  // 5
  {1,0,0,1,0,0,1,0,1,0,0,1,0,0,1,0},  // 6
  {1,0,0,1,0,1,0,1,0,0,1,0,1,0,1,0},  // 7
  {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0},  // 8
  {1,0,1,0,1,0,1,0,1,1,0,1,0,1,0,1},  // 9
  {1,0,1,0,1,1,0,1,1,0,1,0,1,1,0,1},  // 10
  {1,0,1,1,0,1,1,0,1,1,0,1,1,0,1,1},  // 11
  {1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1},  // 12
  {1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1},  // 13
  {1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1},  // 14
  {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1},  // 15
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}   // 16
};

// NB: pattern and step are zero-indexed
bool patternStepOn (unsigned pattern, unsigned step) {
  if ((pattern >= PATTERNS) || (step >= STEPS)) {
    return false;
  }

  int stepstate = pgm_read_byte(&(patterns[pattern][step]));
  if (stepstate) {
    return true;
  } else {
    return false;
  }  
}

// -------------------------------------------
//    Timer routines to drive the sequencer
// -------------------------------------------

// The bpm tempo will be variable but controlled from a pot.
// But the basic steps will be set at 4 steps per beat.
// Total number of steps = 16
// Speed of each step = bpm * 4
//
// For a tempo range of say 20-240 bpm that means each
// step is running at around 80-960 steps-per-min
// or around 3 to 32 steps a second.
//
// So shortest step time interval is 1/32 second ~= 30mS
// Longest step time inteval is 1/2 to 1S = 500-1000mS
//
// The system will be running with a fixed "TICK"
// of 10mS so the tempo control will essentially
// define the number of TICKS that each step will take.
//
// If scale a pot reading down to 7 bits (0..127)
// then can use a range of 3 to 130 TICKS per step
// i.e. 30mS through to around 4S per step.
//
#define TIMER_PERIOD 10000   // Period in uS
#define TEMPO_MIN        3   // 30mS minimum
#define TEMPO_SCALE      3   // >>TEMPO_SCALE for ALG readings

int tempoTicks;
int tempoTickCnt;
void sequencerSetup (void) {
#ifdef TIMER_TOGGLE
  pinMode(TIMER_TOGGLE, OUTPUT);
#endif

  tempoTicks = INIT_TEMPO;
  tempoTickCnt = tempoTicks;

  // Finally enable the timer...
  Timer1.initialize(TIMER_PERIOD);
  Timer1.attachInterrupt(sequencerLoop);
}

void sequencerSet (int ticks) {
  tempoTicks = ticks;
}

int timerToggle;
void sequencerLoop (void) {
#ifdef TIMER_TOGGLE
  timerToggle = !timerToggle;
  digitalWrite(TIMER_TOGGLE, timerToggle);
#endif
  // Only process if the TEMPO says it is time for a step
  //
  // Then update each gate as follows:
  //   Decrement its step counter and wrap as necessary
  //   Set gate as per chosen pattern
  // This ensures the gate is on for a single step
  //
  // NB: As this is a GATE signal, for consecutive steps
  //     they would just merge into each other, so instead
  //     always ensure there is at least one TICK where
  //     all GATEs are OFF before the next step is played.
  tempoTickCnt--;
  if (tempoTickCnt == 1) {
#ifdef TRIGGER_OUTPUT
    // When in TRIGGER mode output for just one TICK
    outputGates();
#else
    // When in GATE mode, need ot ensure
    // all gates are off for at least one TICK
    clearGates();
#endif
  }
  else if (tempoTickCnt == 0) {
#ifdef TRIGGER_OUTPUT
    // When in TRIGGER mode clear outputs after just 1 TICK
    clearGates();
#else
    // When in GATE mode output last GATE values
    // and leave them on until next step
    outputGates();
#endif

    // Process next gate value ready for next update
    updateGates();

    // Reset timer
    tempoTickCnt = tempoTicks;
  }
  else {
    // Nothing to do at the moment
  }
}

// -------------------------------------------
//    Sequencer/Gate handling routines
// -------------------------------------------

int stepCnt;             // Current step count
int gateStep[GATES];     // Current step output value for this GATE
int gatePatterns[GATES]; // Which pattern is used for this GATE

void setupGates () {
  for (int i=0; i<GATES; i++) {
    pinMode (gatePins[i], OUTPUT);
    digitalWrite(gatePins[i], LOW);
    gatePatterns[i] = initPatterns[i];
    gateStep[i] = 0;
  }
  stepCnt = 0;
}

void clearGates () {
  for (int i=0; i<GATES; i++) {
    digitalWrite(gatePins[i], LOW);
  }
}

void outputGates () {
  for (int i=0; i<GATES; i++) {
    digitalWrite(gatePins[i], gateStep[i]);
  }  
}

void updateGates () {
  // Calculate the next set of GATE values from the patterns
  for (int i=0; i<GATES; i++) {
    if (patternStepOn(gatePatterns[i], stepCnt)) {
      gateStep[i] = HIGH;
    } else {
      gateStep[i] = LOW;
    }
  }

  stepCnt++;
  if (stepCnt >= STEPS) {
    stepCnt = 0;
  }
}

// -------------------------------------------
//    Basic Arduino setup/loop
// -------------------------------------------

void setup() {
  setupGates();
  sequencerSetup();
}

void loop() {
#ifdef TEMPO_POT
  int newtempo = analogRead(TEMPO_POT) >> TEMPO_SCALE;
  sequencerSet (TEMPO_MIN + newtempo);
#endif
}
