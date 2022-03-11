/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Rotary Phone MIDI Random Sequencer
//  https://diyelectromusic.wordpress.com/2022/03/11/vintage-rotary-phone-midi-controller-part-5/
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
#include <MIDI.h>

// Uncomment the following to get a trace of the various parameters suitable
// for using with the Arduino Serial.plotter.
//#define TIMING
//#define DEBUG

#define MIDI_CHANNEL 1 // Channel 1 to 16
MIDI_CREATE_DEFAULT_INSTANCE();

// Needs an external PULL-UP resistor (see blog post).
#define PULSE_PIN   2
#define RANDOM_SEED A0

#define NUM_NOTES 37
int notes[NUM_NOTES] = {
  24, 24, 26, 28, 31, 31, 33,
  36, 36, 38, 40, 43, 43, 45,
  48, 48, 50, 52, 55, 55, 57,
  60, 60, 62, 64, 67, 67, 69,
  72, 72, 74, 76, 79, 79, 81,
  84, 84
};

// These are the "action" functions
void doOnHookAction () {
#ifdef DEBUG
  Serial.println("On Hook");
#else
  // Turn off all playing notes...
  for (int i=0; i<NUM_NOTES; i++) {
    MIDI.sendNoteOff(notes[i], 0, MIDI_CHANNEL);
  }
#endif
}

void doOffHookAction () {
#ifdef DEBUG
  Serial.println("Off Hook");
#endif
}

void doPulseAction (int pulseCount) {
#ifdef DEBUG
  Serial.print("Pulse ");
  Serial.println(pulseCount);
#else
  // Pick a random note from our list and start playing it
  MIDI.sendNoteOn (notes[random(NUM_NOTES)], 127, MIDI_CHANNEL);
#endif
}

void doDigitAction (int digit) {
#ifdef DEBUG
  Serial.print("Digit ");
  Serial.println(digit);
#endif
}

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL_OFF);
#endif

  // seed the random number generator
  randomSeed(analogRead(RANDOM_SEED));

  rotarySetup();
}

void loop() {
  rotaryLoop();
}

// Rotary Phone Handling Functions.
// These call the following "callbacks" at the appropriate times:
//   doOnHookAction()
//   doOffHookAction()
//   doPulseAction(pulseCount)
//   doDigitAction(digit)
//
int  r_lastPulse;
int  r_pulseCount;
int  r_readDigit;
bool r_phoneOffHook;
unsigned long r_timing;

#define MIN_PULSE_TIME  60
#define MAX_PULSE_TIME 140
#define DIGIT_GAP_TIME 200
#define ON_HOOK_TIME   500

void rotarySetup() {
  pinMode (PULSE_PIN, INPUT);
  r_timing = millis();
  r_phoneOffHook = false;
}

void rotaryLoop() {
  int pulse = digitalRead (PULSE_PIN);
#ifdef TIMING
  Serial.print(pulse);
  Serial.print(",");
  Serial.print(r_phoneOffHook);
  Serial.print(",");
  Serial.print(r_pulseCount);
  Serial.print(",");
  Serial.print(r_readDigit);
  Serial.print("\n");
#endif  
  unsigned long newtime = millis();
  if ((r_lastPulse == HIGH) && (pulse == LOW)) {
    // We have a falling edge.
    // So is it a pulse, or going off-hook, or even a "bounce"?
    //
    // We can use the time since the last rising edge
    // to tell us...
    // If time < MIN PULSE time it was probably a bounce
    // If time > MAX PULSE time it was probably going off-hook
    if (newtime > r_timing + MAX_PULSE_TIME) {
      r_phoneOffHook = true;
      r_pulseCount = 0;
      r_readDigit = 0;
      doOffHookAction();
    } else if (newtime > r_timing + MIN_PULSE_TIME) {
      // Count it as a valid pulse
      r_pulseCount++;
      doPulseAction(r_pulseCount);
    } else {
      // Probably a short bounce, so ignore
    }

    // reset the timer
    r_timing = newtime;
  }

  if ((r_lastPulse == LOW) && (pulse == LOW)) {
    // If we've stayed low, then we are either between pulses
    // or between digits, so need to work out which...
    if (newtime > r_timing + DIGIT_GAP_TIME) {
      // Store the pulseCount so far and reset for the next digit
      if (r_pulseCount > 0) {
        r_readDigit = r_pulseCount;
        doDigitAction(r_readDigit);
      } else {
        // Nothing to report yet
      }
      r_pulseCount = 0;
    } else {
      // Just keep waiting, might just be between pulses
    }
  }

  if ((r_lastPulse == LOW) && (pulse == HIGH)) {
    // We have a rising edge, so reset the timer
    r_timing = newtime;
  }

  if ((r_lastPulse == HIGH) && (pulse == HIGH)) {
    // If we've stayed HIGH for longer than a pulse
    // then assume the phone is back on the hook.
    if (newtime > r_timing + ON_HOOK_TIME) {
      if (r_phoneOffHook) {
        r_phoneOffHook = false;
        r_readDigit = 0;
        r_pulseCount = 0;
        doOnHookAction();
      }
    }
  }

  r_lastPulse = pulse;
}
