/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Euclidian Clock Gate - Part 3
//  https://diyelectromusic.com/2024/10/27/arduino-euclidean-gate-sequencer-part-3/
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
/*
  Using principles from the following Arduino tutorials:
    Timer One Library      - https://docs.arduino.cc/libraries/timerone/
    Rotary Encoder Library - https://github.com/mathertel/RotaryEncoder
    Adafrit GFX Library    - https://github.com/adafruit/Adafruit-GFX-Library
*/
#include "TimerOne.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RotaryEncoder.h>  // https://github.com/mathertel/RotaryEncoder

//#define PCB_BUILD  // Use the pin-out variant for my Clock Gen Shield PCB

// Can do GATE or TRIGGER.  Uncomment for TRIGGER
//#define TRIGGER_OUTPUT

//#define TIMER_TOGGLE 2
#define TEMPO_MIN 20
#define TEMPO_MAX 400
int tempo;

#define GATES 6
int gatePatterns[GATES]; // Which pattern is used for this GATE
void updatePattern (int dir, int gate);

// -------------------------------------------
// Encoder and display
// -------------------------------------------
#define OLED_ADDR 0x3C
#define OLED_W    128
#define OLED_H    64
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

#define ENC_A  4
#define ENC_B  5
#define ENC_SW 6
RotaryEncoder enc(ENC_A, ENC_B, RotaryEncoder::LatchMode::FOUR3);

bool uiUpdated;
int lastSwitch;
int uiFocus;

void uiSetup (void) {
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.setTextColor(WHITE);

  uiFocus = GATES;

  // Encoder switch
  pinMode(ENC_SW, INPUT_PULLUP);
  lastSwitch = HIGH;

  tempo = (TEMPO_MAX-TEMPO_MIN) / 2;

  uiUpdated = true;
}

void uiLoop (void) {
  enc.tick();
  int encDir = (int)enc.getDirection();
  if (encDir < 0) {
    if (uiFocus < GATES) {
      updatePattern (-1, uiFocus);
    } else {
      tempo-=5;
      if (tempo < TEMPO_MIN) tempo = TEMPO_MIN;
      sequencerSet(tempo);
    }
    uiUpdated = true;
  } else if (encDir > 0) {
    if (uiFocus < GATES) {
      updatePattern (1, uiFocus);
    } else {
      tempo+=5;
      if (tempo > TEMPO_MAX) tempo = TEMPO_MAX;
      sequencerSet(tempo);
    }
    uiUpdated = true;
  } else {
    // do nothing
  }

  int newSwitch = digitalRead(ENC_SW);
  if ((newSwitch == LOW) && (lastSwitch == HIGH)) {
    // Switch pressed
    uiFocus++;
    if (uiFocus > GATES) {
      uiFocus = 0;
    }
    uiUpdated = true;
  }
  lastSwitch = newSwitch;
}

int cursor_x[GATES+1] = {10,30,50,70,90,110,20};
int cursor_y[GATES+1] = {60,60,60,60,60,60,35};
int cursor_l[GATES+1] = {12,12,12,12,12,12,54};

void uiUpdate (void) {
  if (uiUpdated) {
    // Output tempo
    display.clearDisplay();
    display.setTextSize(3);
    display.setCursor(20,10);
    if (tempo<100) display.print(" ");
    if (tempo<10) display.print(" ");
    display.print(tempo);

    // Output patterns
    display.setTextSize(1);
    for (int i=0; i<GATES ; i++) {
      display.setCursor(10+20*i,50);
      if (gatePatterns[i] < 10) display.print(" ");
      display.print(gatePatterns[i]);
    }

    // Output cursor
    display.writeFastHLine(cursor_x[uiFocus], cursor_y[uiFocus], cursor_l[uiFocus], WHITE);

    display.display();
    uiUpdated = false;
  }
}

// -------------------------------------------
//    Sequencer IO and Patterns
// -------------------------------------------
#ifdef PCB_BUILD
int gatePins[GATES] = {13,12,11,10,3,2};   // Set digital pins for GATE outputs
#else
int gatePins[GATES] = {13,12,11,10,9,8};   // Set digital pins for GATE outputs
#endif
int initPatterns[GATES] = {4,3,5,7,11,13}; // Initial patterns to use for each GATE

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

void updatePattern (int dir, int gate) {
  if (gate < GATES) {
    if (dir > 0) {
      if (gatePatterns[gate] < (PATTERNS-1)) {
        gatePatterns[gate]++;
      } else {
        gatePatterns[gate] = 0;
      }
    } else {
      if (gatePatterns[gate] > 0) {
        gatePatterns[gate]--;
      } else {
        gatePatterns[gate] = PATTERNS-1;
      }
    }
  }
}

// -------------------------------------------
//    Timer routines to drive the sequencer
// -------------------------------------------

// The bpm tempo will be variable but controlled from an encoder.
// But the basic steps will be set at 4 steps per beat.
// Total number of steps = 16
// Speed of each step = bpm * 4
//
// For a tempo range of say 20-240 bpm that means each
// step is running at around 80-960 steps-per-min
// or around 1.3 to 16 steps a second.
//
// So shortest step time interval is 1/16 second ~= 63mS
// Longest step time inteval is 1/2 to 1S = 500-1000mS
//
// The system will be running with a fixed "TICK"
// of 10mS so the tempo control will essentially
// define the number of TICKS that each step will take.
//
#define TIMER_PERIOD 10000   // Period in uS

int timerCounter;
long tempoTicks;
int tempoTickCnt;
void timerSetup (void) {
#ifdef TIMER_TOGGLE
  pinMode(TIMER_TOGGLE, OUTPUT);
#endif

  sequencerSet(tempo);
  tempoTickCnt = tempoTicks;

  // Finally enable the timer...
  Timer1.initialize(TIMER_PERIOD);
  Timer1.attachInterrupt(sequencerLoop);
  timerCounter = 0;
}

void sequencerSet (int newtempo) {
  // Convert bpm into ticks per step
  // NB: 4 steps in each beat...
  // So:
  //   Beats per second = bpm / 60
  //   Steps per second = 4 * bpm / 60
  //   Period of steps (uS) = (1000000 * 60) / (4 * bpm)
  //   Number of ticks = (1000000 * 60) / (TIMER_PERIOD_uS * bpm * 4)
  tempoTicks = (60L * 1000000L / (long)TIMER_PERIOD) / ((long)newtempo * 4L);
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

void setupGates (void) {
  for (int i=0; i<GATES; i++) {
    pinMode (gatePins[i], OUTPUT);
    digitalWrite(gatePins[i], LOW);
    gatePatterns[i] = initPatterns[i];
    gateStep[i] = 0;
  }
  stepCnt = 0;
}

void clearGates (void) {
  for (int i=0; i<GATES; i++) {
    digitalWrite(gatePins[i], LOW);
  }
}

void outputGates (void) {
  for (int i=0; i<GATES; i++) {
    digitalWrite(gatePins[i], gateStep[i]);
  }  
}

void updateGates (void) {
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

void setup(void) {
  uiSetup();
  setupGates();
  timerSetup();
}

void loop(void) {
  uiLoop();
  uiUpdate();
}
