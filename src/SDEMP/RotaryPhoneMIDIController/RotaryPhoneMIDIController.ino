/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Rotary Phone MIDI Controller
//  https://diyelectromusic.wordpress.com/2022/03/29/vintage-rotary-phone-midi-controller-part-7/
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
  Note: This relies on using a 120k external PULL-UP resistor on the IO pin
        connected to the RED wire.  WHITE connects to GND.

        Refer to the blog post for details.
*/
#include <TimerOne.h>
#include <MIDI.h>

// Uncomment the following to get a trace of the various parameters suitable
// for using with the Arduino Serial.plotter.
//#define TIMING

#define MIDI_CHANNEL 1 // Channel 1 to 16
MIDI_CREATE_DEFAULT_INSTANCE();

// Needs an external PULL-UP resistor (see blog post).
#define PULSE_PIN  2
#define PULSE_SCAN 20 // in mS
#define MODE_PIN   3
#define LED_PIN    LED_BUILTIN

#define NO_DIGIT (-1)
int lastDigit;
bool lastOffHook;
int patchNumber;

#define NUM_NOTES 10

// Modes:
//   0 = MIDI General Purpose Controller
//   1 = basic major scale
//   2 = Simple pentatonic scale
//   3 = Multi-step pentatonic scale
//   4 = Program Change
#define M_CTRLCH 0
#define M_MAJ    1
#define M_PENT   2
#define M_MPENT  3
#define M_PROGCH 4
#define NUM_MODES 5
int mode;

#define M0_CC_MSG 16 // General Purpose Controller 1

// Basic major scale
int m1notes[NUM_NOTES] = {60, 62, 64, 65, 67, 69, 71, 72, 74, 76};

// Here is a simple "one note per digit" example
int m2notes[NUM_NOTES] = {36, 48, 55, 60, 62, 64, 67, 69, 72, 79};

// Here is a more complex example, with 3 notes that can be
// played in sequence for every digit.
#define NUM_STEPS  3
int m3notes[NUM_NOTES][NUM_STEPS] = {
/* "0" */  {36, 43, 48,},
/* "1" */  {48, 52, 55,},
/* "2" */  {55, 57, 60,},
/* "3" */  {60, 72, 64,},
/* "4" */  {62, 64, 69,},
/* "5" */  {64, 67, 76,},
/* "6" */  {67, 72, 79,},
/* "7" */  {69, 45, 81,},
/* "8" */  {72, 48, 60,},
/* "9" */  {79, 67, 55,},
};

int playing[NUM_NOTES];
int modesw;

void initMode() {
  pinMode(MODE_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  mode = M_CTRLCH;
  modesw = HIGH;
  indicateMode();
}

void updateMode() {
  int newmode = digitalRead(MODE_PIN);
  if ((newmode == HIGH) && (modesw == LOW)) {
    //Switch was just released - it is LOW when pressed
    mode++;
    if (mode >= NUM_MODES) mode = 0;
    digitalWrite(LED_PIN, HIGH);
    indicateMode();
    delay(300);  // Crude "debouncing"
    digitalWrite(LED_PIN, LOW);
  }
  modesw = newmode;
}

void indicateMode() {
  // Send a MIDI note message to indicate the mode
  for (int i=0; i<=mode; i++) {
    MIDI.sendNoteOn(72+i, 64, MIDI_CHANNEL);
    delay(200);
    MIDI.sendNoteOff(72+i, 0, MIDI_CHANNEL);
  }
}

void setup() {
#ifdef TIMING
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL_OFF);
#endif
  rotarySetup();
  lastDigit = NO_DIGIT;
  lastOffHook = false;
  patchNumber = 0;
  initMode();
}

void loop() {
  updateMode();

#ifdef TIMING
  rotaryLoop();
#endif
  int digit = rotaryNewDigit();
  if (digit != lastDigit) {
    // Something has changed...
    if (digit != NO_DIGIT) {
      if (mode == M_PROGCH) {
        // Multiply existing patch number x10
        // and add on the new digit...
        patchNumber = patchNumber * 10 + digit;
      } else if (mode == M_CTRLCH) {
        MIDI.sendControlChange(M0_CC_MSG, digit, MIDI_CHANNEL);
        digit = NO_DIGIT;
      } else {
        // One of the scales
        if (mode == M_MAJ) {
          MIDI.sendNoteOn (m1notes[digit], 127, MIDI_CHANNEL);
        } else if (mode == M_PENT) {
          MIDI.sendNoteOn (m2notes[digit], 127, MIDI_CHANNEL);
        } else if (mode == M_MPENT) {
          MIDI.sendNoteOn (m3notes[digit][playing[digit]], 127, MIDI_CHANNEL);
          playing[digit]++;
          if (playing[digit] >= NUM_STEPS) {
            playing[digit] = 0;
          }
        } else {
          // Unrecognised mode
        }
      }
    }
  }
  bool offHook = rotaryOffHook();
  if (lastOffHook && !offHook) {
    // Phone back on the hook
//    Serial.print("\nOn Hook\n");
    if (mode == M_PROGCH) {
      if ((patchNumber > 0) && (patchNumber <= 128)) {
        MIDI.sendProgramChange(patchNumber - 1, MIDI_CHANNEL);
        patchNumber = 0;
      }
    } else if (mode == M_CTRLCH) {
      // Nothing to do
    } else {
      // One of the scales - send all NoteOff messages
      // in case we got stuck with random notes on in a mode when changing!
      for (int i=0; i<NUM_NOTES; i++) {
        MIDI.sendNoteOff (m1notes[i], 0, MIDI_CHANNEL);
        MIDI.sendNoteOff (m2notes[i], 0, MIDI_CHANNEL);
        for (int s=0; s<NUM_STEPS; s++) {
          MIDI.sendNoteOff (m3notes[i][s], 0, MIDI_CHANNEL);
        }
      }
    }
  }
  if (!lastOffHook && offHook) {
    // Phone off the hook
//    Serial.print("Off Hook\n");
    patchNumber = 0;
  }

  lastOffHook = offHook;
  lastDigit = digit;
}

//
// Decoding the rotary phone dial.
//
// rotaryDial() is called periodically from a timer and
// the results can be read with the following functions:
//   rotaryDigit()    - returns the last digit reported
//   rotaryNewDigit() - only returns the last digit if it hasn't already been read
//
// rotarySetup() is called from setup().
//
int pulse;
int lastPulse;
int pulseCount;
int readDigit;
bool newDigit;
bool phoneOffHook;
unsigned long timing;

#define MIN_PULSE_TIME  60
#define MAX_PULSE_TIME 140
#define DIGIT_GAP_TIME 200
#define ON_HOOK_TIME   500

void rotarySetup () {
  pinMode (PULSE_PIN, INPUT);
  timing = millis();
  readDigit = 0;
  newDigit = false;
  phoneOffHook = false;
  pulseCount = 0;

  Timer1.initialize(PULSE_SCAN);
  Timer1.attachInterrupt(rotaryDial);  
}

// Returns:
//  NO_DIGIT = no digit read
//       0-9 = last digit read
//
int rotaryDigit () {
  if (readDigit == 0) {
    // No digits read
    return NO_DIGIT;
  } else if (readDigit == 10) {
    // "0" read
    return 0;
  } else {
    return readDigit;
  }
}

int rotaryNewDigit () {
  if (newDigit) {
    newDigit = false;
    return rotaryDigit();
  } else {
    return NO_DIGIT;
  }
}

bool rotaryOffHook () {
  return phoneOffHook;
}

void rotaryLoop () {
#ifdef TIMING
  Serial.print(pulse);
  Serial.print(",");
  Serial.print(phoneOffHook);
  Serial.print(",");
  Serial.print(pulseCount);
  Serial.print(",");
  Serial.print(readDigit);
  Serial.print("\n");
#endif  
}

void rotaryDial () {
  unsigned long newtime = millis();
  pulse = digitalRead (PULSE_PIN);
  if ((lastPulse == HIGH) && (pulse == LOW)) {
    // We have a falling edge.
    // So is it a pulse, or going off-hook, or even a "bounce"?
    //
    // We can use the time since the last rising edge
    // to tell us...
    // If time < MIN PULSE time it was probably a bounce
    // If time > MAX PULSE time it was probably going off-hook
    if (newtime > timing + MAX_PULSE_TIME) {
      phoneOffHook = true;
      pulseCount = 0;
      readDigit = 0;
    } else if (newtime > timing + MIN_PULSE_TIME) {
      // Count it as a valid pulse
      pulseCount++;
    } else {
      // Probably a short bounce, so ignore
    }

    // reset the timer
    timing = newtime;
  }

  if ((lastPulse == LOW) && (pulse == LOW)) {
    // If we've stayed low, then we are either between pulses
    // or between digits, so need to work out which...
    if (newtime > timing + DIGIT_GAP_TIME) {
      // Store the pulseCount so far and reset for the next digit
      if (pulseCount > 0) {
        readDigit = pulseCount;
        newDigit = true;
      } else {
        // Nothing to report yet
      }
      pulseCount = 0;
    } else {
      // Just keep waiting, might just be between pulses
    }
  }

  if ((lastPulse == LOW) && (pulse == HIGH)) {
    // We have a rising edge, so reset the timer
    timing = newtime;
  }

  if ((lastPulse == HIGH) && (pulse == HIGH)) {
    // If we've stayed HIGH for longer than a pulse
    // then assume the phone is back on the hook.
    if (newtime > timing + ON_HOOK_TIME) {
      if (phoneOffHook) {
        phoneOffHook = false;
        readDigit = 0;
        pulseCount = 0;
      }
    }
  }

  lastPulse = pulse;
}
