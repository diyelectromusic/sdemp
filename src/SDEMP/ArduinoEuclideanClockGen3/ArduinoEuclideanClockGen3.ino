/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Euclidian Clock Gate - Part 4
//  https://diyelectromusic.com/2024/10/28/arduino-euclidean-gate-sequencer-part-4/
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

    UI Implementation based on HAGIWO's "Euclidean rhythm sequencer":
        https://note.com/solder_state/n/n433b32ea6dbc
*/
#include "TimerOne.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RotaryEncoder.h>  // https://github.com/mathertel/RotaryEncoder

#define PCB_BUILD  // Use the pin-out variant for my Clock Gen Shield PCB

// Can do GATE or TRIGGER.  Uncomment for TRIGGER
//#define TRIGGER_OUTPUT

//#define TIMER_TOGGLE 2
#define TEMPO_MIN 20
#define TEMPO_MAX 400
#define TEMPO_DEF 120
int seqTempo;

// -------------------------------------------
// Sequencer parameters
// -------------------------------------------
#define GATES 6
#define STEPS 16
#define PATTERNS (STEPS+1)

int  seqPatterns[GATES]; // Which pattern is used for this GATE
int  seqOffsets[GATES] = {0,0,0,0,0,0};
bool seqMute[GATES] = {false,false,false,false,false,false};
int  seqLimits[GATES] = {STEPS,STEPS,STEPS,STEPS,STEPS,STEPS}; // 1-indexed limit
int  seqCounter[GATES] = {0,0,0,0,0,0};

// A few prototypes to help things build...
void updateMenu (int incr, int submenu);
bool patternStepOn (int gate, int step);
void patternRefresh (int gate);

// -------------------------------------------
// Encoder and display
// -------------------------------------------
#define OLED_ADDR 0x3C
#define OLED_W    128
#define OLED_H    64
#define OLED_ON   WHITE
#define OLED_OFF  BLACK
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

#define ENC_A  4
#define ENC_B  5
#define ENC_SW 6
RotaryEncoder enc(ENC_A, ENC_B, RotaryEncoder::LatchMode::FOUR3);

bool uiUpdated;
int lastSwitch;
// uiFocus:
//   0 = main menu - ie selecting between submenus
//   <>0 = specific submenu
int uiFocus;
int uiSelection;  // Submenu active
int uiChannel;    // Active channel/GATE (0-5)

void uiSetup (void) {
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.setTextColor(OLED_ON);

  uiFocus = 0;
  uiSelection = 0;
  uiChannel = 0;

  // Encoder switch
  pinMode(ENC_SW, INPUT_PULLUP);
  lastSwitch = HIGH;

  seqTempo = TEMPO_DEF;

  uiRequest();
}

void uiScan (void) {
  enc.tick();  
}

void uiLoop (void) {
  int encDir = (int)enc.getDirection();
  if (encDir < 0) {
    if (uiFocus == 0) {
      // moving through main menu
      uiSelection--;
      if (uiSelection < 0) uiSelection = 6;
    } else {
      updateMenu (-1, uiSelection);
    }
    uiRequest();
  } else if (encDir > 0) {
    if (uiFocus == 0) {
      // moving through main menu
      uiSelection++;
      if (uiSelection > 6) uiSelection = 0;
    } else {
      updateMenu (1, uiSelection);
    }
    uiRequest();
  } else {
    // do nothing
  }

  int newSwitch = digitalRead(ENC_SW);
  if ((newSwitch == LOW) && (lastSwitch == HIGH)) {
    uiFocus = !uiFocus;
    updateMenu (0, uiSelection);
    uiRequest();
  }
  lastSwitch = newSwitch;
}

void uiRequest (void) {
  // Request a display update
  uiUpdated = true;
}

// Base coordinates for each display for each channel
const byte ch_x[GATES] = {0, 40, 80, 15, 55, 95};
const byte ch_y[GATES] = {0, 0,  0,  32, 32, 32};

// Offsets from the base coordinates for each point on the polygons
const byte xc[STEPS] = {15,21,26,29,30,29,26,21,15,9, 4, 1, 0, 1,4,9};
const byte yc[STEPS] = {0, 1, 4, 9, 15,21,26,29,30,29,26,21,15,9,4,1};

// Temporary buffer for corner coordinates of the polygons
byte line_x[STEPS];
byte line_y[STEPS];

void uiUpdate (void) {
  if (uiUpdated) {
    display.clearDisplay();
    display.setTextSize(1);

    // --- Menu items
    display.setCursor(120,0);
    display.print(uiChannel+1);  // Gate/channel selected
    display.setCursor(120,9);
    display.print("P"); // Pattern
    display.setCursor(120,18);
    display.print("O"); // Offset
    display.setCursor(0,36);
    display.print("L"); // Limit
    display.setCursor(0,45);
    display.print("M"); // Mute
    display.setCursor(0,54);
    display.print("R"); // Reset

    // --- Display cursor
    if (uiSelection < 3) {
      int yo = uiSelection*9;
      if (uiFocus == 0) {
        display.drawTriangle(113,0+yo, 113, 6+yo, 118, 3+yo, OLED_ON);
      } else {
        display.fillTriangle(113,0+yo, 113, 6+yo, 118, 3+yo, OLED_ON);
      }
    } else if (uiSelection < 6) {
      int yo = (uiSelection-3) * 9;
      if (uiFocus == 0) {
        display.drawTriangle(12, 36+yo, 12, 42+yo, 7, 39+yo, OLED_ON);
      } else {
        display.fillTriangle(12, 36+yo, 12, 42+yo, 7, 39+yo, OLED_ON);
      }
    } else {
      // tempo display - deal with that later
    }

    // --- Step markers
    for (int i=0; i<GATES; i++) {
      for (int j=0; j<seqLimits[i]; j++) {
        display.drawPixel(xc[j]+ch_x[i], yc[j]+ch_y[i], OLED_ON);
      }
    }

    // --- Sequence Polygons
    for (int i=0; i<GATES; i++) {
      if (seqPatterns[i] == 0) {
        // Nothing to do if no points to draw
      } else if (seqPatterns[i] == 1) {
        // Single step pattern is a special case - need a centre to edge, radial line
        display.drawLine(ch_x[i]+15, ch_y[i]+15, ch_x[i]+xc[seqOffsets[i]], ch_y[i]+yc[seqOffsets[i]], OLED_ON);
      } else {
        // Build up the lines to draw the polygon in a buffer
        for (int i=0; i<STEPS; i++) {
          line_x[i] = 0;
          line_y[i] = 0;
        }
        int linecnt=0;
        for (int j=0; j<STEPS; j++) {
          if (patternStepOn(i, j)) {
            line_x[linecnt] = ch_x[i] + xc[j];
            line_y[linecnt] = ch_y[i] + yc[j];
            linecnt++;
          }
        }
        // Draw the lines
        for (int i=0; i<linecnt-1; i++) {
          display.drawLine(line_x[i], line_y[i], line_x[i+1], line_y[i+1], OLED_ON);
        }
        // Close the polygon between last and first points
        display.drawLine(line_x[0], line_y[0], line_x[linecnt-1], line_y[linecnt-1], OLED_ON);
      }
    }

    // --- Playing markers
    for (int i=0; i<GATES; i++) {
      if (seqMute[i] == 0) {
        int step = seqCounter[i];
        if (patternStepOn(i, step)) {
          // Current step is being played
          display.fillCircle(ch_x[i]+xc[step], ch_y[i]+yc[step], 3, OLED_ON);
        } else {
          // Current step is not being played
          display.drawCircle(ch_x[i]+xc[step], ch_y[i]+yc[step], 1, OLED_ON);
        }
      }
    }

    // Last thing is the tempo over the top
    if (uiSelection == 6) {
      display.fillRect (41,20,45,23, OLED_OFF);
      display.drawRect (41,20,45,23, OLED_ON);
      if (uiFocus != 0) {
        display.drawRect (40,19,47,25, OLED_ON);
      }
/*    This should work, but for some reason using setTextSize(2)
      and print seems to leave artefacts on the display...
      ---------------------
      display.setCursor (46,24);
      display.setTextSize(2);
      if (seqTempo < 100) display.print(" ");
      display.print(seqTempo);
*/
      // Note: default font has 6x8 characters, but we're using size=2
      if (seqTempo >= 100) {
         display.drawChar (46,24, '0'+seqTempo/100, OLED_ON, OLED_OFF, 2);
      }
      display.drawChar (46+12,24, '0'+(seqTempo/10)%10, OLED_ON, OLED_OFF, 2);
      display.drawChar (46+24,24, '0'+seqTempo%10, OLED_ON, OLED_OFF, 2);
    }

    display.display();
    uiUpdated = false;
  }
}

void updateMenu (int incr, int submenu) {
  switch (submenu) {
    case 0: // channel
      uiChannel += incr;
      if (uiChannel >= GATES-1) uiChannel = GATES-1;
      if (uiChannel < 0) uiChannel = 0;
      break;

    case 1: // pattern
      seqPatterns[uiChannel] += incr;
      if (seqPatterns[uiChannel] > PATTERNS-1) seqPatterns[uiChannel] = PATTERNS-1;
      if (seqPatterns[uiChannel] < 0) seqPatterns[uiChannel] = 0;
      patternRefresh (uiChannel);
      break;

    case 2: // start offset
      seqOffsets[uiChannel] += incr;
      if (seqOffsets[uiChannel] > STEPS-1) seqOffsets[uiChannel] = STEPS-1;
      if (seqOffsets[uiChannel] < 0) seqOffsets[uiChannel] = 0;
      patternRefresh (uiChannel);
      break;

    case 3: // limit/reset
      seqLimits[uiChannel] += incr;
      if (seqLimits[uiChannel] > STEPS) seqLimits[uiChannel] = STEPS;
      if (seqLimits[uiChannel] < 1) seqLimits[uiChannel] = 1;
      break;

    case 4: // mute
      seqMute[uiChannel] = !seqMute[uiChannel];
      uiFocus = 0;
      break;

    case 5: // reset
      for (int i=0; i<GATES; i++) {
        seqCounter[i] = 0;
      }
      uiFocus = 0;
      break;

    case 6:  // tempo
     seqTempo += (incr*5);
     if (seqTempo < TEMPO_MIN) seqTempo = TEMPO_MIN;
     if (seqTempo > TEMPO_MAX) seqTempo = TEMPO_MAX;
     sequencerSet(seqTempo);
     break;

    default: // ignore
      break;
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

// Record of the active pattern for each gate
uint8_t activePattern[GATES][STEPS];

// NB: pattern and step are zero-indexed
bool patternStepOn (int gate, int step) {
  if ((gate >= GATES) || (step >= STEPS)) {
    return false;
  }

  if (activePattern[gate][step]) {
    return true;
  } else {
    return false;
  }  
}

void patternRefresh (int gate) {
  // Update the "working" copy of the pattern from the
  // stored copy, taking into account any start offset
  if (gate < GATES) {
    int pattern = seqPatterns[gate];

    int os = seqOffsets[gate];
    for (int i=0; i<os; i++) {
      activePattern[gate][i] = pgm_read_byte(&(patterns[pattern][STEPS-os+i]));
    }
    for (int i=os; i<STEPS; i++) {
      activePattern[gate][i] = pgm_read_byte(&(patterns[pattern][i-os]));
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
#define TIMER_PERIOD 10000  // Period in uS

void timerSetup (void) {
#ifdef TIMER_TOGGLE
  pinMode(TIMER_TOGGLE, OUTPUT);
#endif

  sequencerSetup();

  // Finally enable the timer...
  Timer1.initialize(TIMER_PERIOD);
  Timer1.attachInterrupt(timerLoop);
}

int timerToggle;
void timerLoop (void) {
#ifdef TIMER_TOGGLE
  timerToggle = !timerToggle;
  digitalWrite(TIMER_TOGGLE, timerToggle);
#endif
  sequencerLoop();
  uiScan();
}

// -------------------------------------------
//    Sequencer
// -------------------------------------------

long tempoTicks;
int tempoTickCnt;

void sequencerSetup (void) {
  sequencerSet(seqTempo);
  tempoTickCnt = tempoTicks;
  
  for (int i=0; i<GATES; i++) {
    seqPatterns[i] = initPatterns[i];
    patternRefresh(i);
  }
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

void sequencerLoop(void) {
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
    gateOutput();
#else
    // When in GATE mode, need ot ensure
    // all gates are off for at least one TICK
    gateClear();
#endif
  }
  else if (tempoTickCnt == 0) {
#ifdef TRIGGER_OUTPUT
    // When in TRIGGER mode clear outputs after just 1 TICK
    gateClear();
#else
    // When in GATE mode output last GATE values
    // and leave them on until next step
    gateOutput();
#endif

    // Process next gate value ready for next update
    gateUpdate();
    uiRequest();

    // Reset timer
    tempoTickCnt = tempoTicks;
  }
}

// -------------------------------------------
//    Gate handling routines
// -------------------------------------------

int gateStep[GATES];

void gateSetup (void) {
  for (int i=0; i<GATES; i++) {
    pinMode (gatePins[i], OUTPUT);
    digitalWrite(gatePins[i], LOW);
    gateStep[i] = 0;
  }
}

void gateClear (void) {
  for (int i=0; i<GATES; i++) {
    digitalWrite(gatePins[i], LOW);
  }
}

void gateOutput (void) {
  for (int i=0; i<GATES; i++) {
    digitalWrite(gatePins[i], gateStep[i]);
  }  
}

void gateUpdate (void) {
  // Calculate the next set of GATE values from the patterns
  for (int i=0; i<GATES; i++) {
    if (patternStepOn(i, seqCounter[i]) && (seqMute[i] == 0)) {
      gateStep[i] = HIGH;
    } else {
      gateStep[i] = LOW;
    }

    seqCounter[i]++;
    if (seqCounter[i] >= seqLimits[i]) {
      seqCounter[i] = 0;
    }
  }
}

// -------------------------------------------
//    Basic Arduino setup/loop
// -------------------------------------------

void setup(void) {
  Serial.begin(9600);
  uiSetup();
  gateSetup();
  timerSetup();
}

void loop(void) {
  uiLoop();
  uiUpdate();
}
