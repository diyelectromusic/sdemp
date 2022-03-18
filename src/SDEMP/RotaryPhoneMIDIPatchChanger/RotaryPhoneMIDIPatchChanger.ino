/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Rotary Phone MIDI Patch Changer
//  https://diyelectromusic.wordpress.com/2022/03/11/vintage-rotary-phone-midi-controller-part-4/
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

#define MIDI_CHANNEL 1 // Channel 1 to 16
MIDI_CREATE_DEFAULT_INSTANCE();

// Needs an external PULL-UP resistor (see blog post).
#define PULSE_PIN  2
#define PULSE_SCAN 20 // in mS

#define NO_DIGIT (-1)
int lastDigit;
bool lastOffHook;
int patchNumber;

void setup() {
  MIDI.begin(MIDI_CHANNEL);
  rotarySetup();
  lastDigit = NO_DIGIT;
  lastOffHook = false;
  patchNumber = 0;
}

void loop() {
  // Allow software MIDI THRU functionality
  MIDI.read();

  int digit = rotaryNewDigit();
  if (digit != lastDigit) {
    // Something has changed...
    if (digit != NO_DIGIT) {
      // Multiply existing patch number x10
      // and add on the new digit...
      patchNumber = patchNumber * 10 + digit;
    }
  }
  bool offHook = rotaryOffHook();
  if (lastOffHook && !offHook) {
    // Phone back on the hook
    if ((patchNumber > 0) && (patchNumber <= 128)) {
      MIDI.sendProgramChange(patchNumber - 1, MIDI_CHANNEL);
      patchNumber = 0;
    }
  }
  if (!lastOffHook && offHook) {
    // Phone off the hook
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
