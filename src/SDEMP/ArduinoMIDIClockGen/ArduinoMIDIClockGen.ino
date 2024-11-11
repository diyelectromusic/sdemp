/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Clock to Trigger
//  https://diyelectromusic.com/2024/11/11/arduino-midi-to-clock-trigger/
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
    Rotary Encoder Library - https://github.com/mathertel/RotaryEncoder
    Adafrit GFX Library    - https://github.com/adafruit/Adafruit-GFX-Library
*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RotaryEncoder.h>  // https://github.com/mathertel/RotaryEncoder
#include <MIDI.h>

#define NUM_DIV  8
#define INIT_DIV 6
uint8_t divisions[NUM_DIV] = {1,2,3,4,6,8,12,24};

#define GATES 6
uint8_t gateDiv[GATES];
bool clockRunning;
#define GATE_TIME 10  // GATE ON time (mS)

// -------------------------------------------
// MIDI
// -------------------------------------------
#define MIDI_CHANNEL MIDI_CHANNEL_OMNI
MIDI_CREATE_DEFAULT_INSTANCE();

void midiHandleClock () {
  if (clockRunning) {
    updateGates();
  }
}
void midiHandleStart () {
  clockRunning = true;
  resetGates();
}
void midiHandleStop () {
  clockRunning = false;
}
void midiHandleContinue () {
  clockRunning = true;
}

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
uint8_t lastSwitch;
uint8_t uiFocus;

void uiSetup (void) {
 if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // error starting up - can go no further
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
    }
  }

  display.setTextColor(WHITE);

  uiFocus = 0;

  // Encoder switch
  pinMode(ENC_SW, INPUT_PULLUP);
  lastSwitch = HIGH;

  uiUpdated = true;
}

void uiLoop (void) {
  enc.tick();
  int encDir = (int)enc.getDirection();
  if (encDir > 0) {
    if (gateDiv[uiFocus] > 0)  {
      gateDiv[uiFocus]--;
    }
    uiUpdated = true;
  } else if (encDir < 0) {
    if (gateDiv[uiFocus] < NUM_DIV-1)  {
      gateDiv[uiFocus]++;
    }
    uiUpdated = true;
  } else {
    // do nothing
  }

  int newSwitch = digitalRead(ENC_SW);
  if ((newSwitch == LOW) && (lastSwitch == HIGH)) {
    // Switch pressed
    uiFocus++;
    if (uiFocus >= GATES) {
      uiFocus = 0;
    }
    uiUpdated = true;
  }
  lastSwitch = newSwitch;
}

uint8_t cursor_x[GATES] = {10,50,90,10,50,90};
uint8_t cursor_y[GATES] = {30,30,30,60,60,60};
#define CURSOR_L 22

void uiDisplay (void) {
  if (uiUpdated) {
    // Output tempo
    display.clearDisplay();

    // Output patterns
    for (int i=0; i<GATES ; i++) {
      int gatediv = 24 / divisions[gateDiv[i]];
      // display.print with setTextSize(2) leaves artefacts on the display...
      if (gatediv > 10) {
         display.drawChar (cursor_x[i], cursor_y[i]-20, '0'+gatediv/10, WHITE, BLACK, 2);
      }
      display.drawChar (12+ cursor_x[i], cursor_y[i]-20, '0'+gatediv%10, WHITE, BLACK, 2);
    }

    // Output cursor
    display.writeFastHLine(cursor_x[uiFocus], cursor_y[uiFocus], CURSOR_L, WHITE);

    display.display();
    uiUpdated = false;
  }
}

// -------------------------------------------
//    Sequencer/Gate handling routines
// -------------------------------------------
uint8_t gatePins[GATES] = {13,12,11,10,3,2};   // Set digital pins for GATE outputs
uint8_t gateCount[GATES];    // Counter for each gate
unsigned long gateTimer[GATES]; // Timer for each gate (mS)

void setupGates (void) {
  for (int i=0; i<GATES; i++) {
    pinMode (gatePins[i], OUTPUT);
    digitalWrite(gatePins[i], LOW);
    gateDiv[i] = INIT_DIV;
    gateCount[i] = 0;
    gateTimer[i] = 0;
  }
}

void scanGates (void) {
  // Output gates at 10mS intervals
  for (int i=0; i<GATES; i++) {
    if (gateTimer[i] == 0) {
      // Gate not active - skip
    } else if (gateTimer[i] < millis()) {
      digitalWrite(gatePins[i], LOW);
      gateTimer[i] = 0;
    } else {
      // Nothing to do
    }
  }
}

void resetGates (void) {
  for (int i=0; i<GATES; i++) {
    gateCount[i] = divisions[gateDiv[i]] - 1;
    gateTimer[i] = 0;
  }
}

void updateGates (void) {
  // Decrement each counter and set gate when zero
  for (int i=0; i<GATES; i++) {
    if (gateCount[i] == 0) {
      digitalWrite(gatePins[i], HIGH);

      // Start timer
      gateTimer[i] = millis() + GATE_TIME;

      // Reset counter
      gateCount[i] = divisions[gateDiv[i]] - 1;
    } else {
      gateCount[i]--;
    }
  }
}

// -------------------------------------------
//    Basic Arduino setup/loop
// -------------------------------------------

void setup(void) {
  uiSetup();
  setupGates();

  MIDI.begin(MIDI_CHANNEL);

  // Callbacks for MIDI real time messages
  MIDI.setHandleClock(midiHandleClock);
  MIDI.setHandleStart(midiHandleStart);
  MIDI.setHandleStop(midiHandleStop);
  MIDI.setHandleContinue(midiHandleContinue);
}

bool toggle;
void loop(void) {
  MIDI.read();
  scanGates();

  // Alternate IO and display to even out load
  // when running (a little).
  toggle = !toggle;
  if (toggle) {
    uiLoop();
  } else {
    uiDisplay();
  }
}
