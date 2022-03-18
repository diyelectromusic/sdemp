/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Rotary Phone Decoder Test
//  https://diyelectromusic.wordpress.com/2022/03/10/vintage-rotary-phone-midi-controller-part-2/
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

// Uncomment the following to get a trace of the various parameters suitable
// for using with the Arduino Serial.plotter.
//#define TIMING

// Needs an external PULL-UP resistor (see blog post).
#define PULSE_PIN 2

int lastPulse;
int pulseCount;
unsigned long timing;
int readDigit;
bool phoneOffHook;

#define MIN_PULSE_TIME  60
#define MAX_PULSE_TIME 140
#define DIGIT_GAP_TIME 200
#define ON_HOOK_TIME   500

void setup() {
  Serial.begin(9600);
  pinMode (PULSE_PIN, INPUT);
  timing = millis();
  readDigit = -1;
  phoneOffHook = false;
  pulseCount = 0;
}

void loop() {
  unsigned long newtime = millis();
  int pulse = digitalRead (PULSE_PIN);
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
      Serial.println("Phone: Off-hook");
      pulseCount = 0;
      readDigit = -1;
    } else if (newtime > timing + MIN_PULSE_TIME) {
      // Count it as a valid pulse
      pulseCount++;
      Serial.print("Pulse: ");
      Serial.println(pulseCount);
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
        if (readDigit == 10) readDigit = 0; // "0" is 10 pulses
        Serial.print("Digit: ");
        Serial.println(readDigit);
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
        Serial.println("Phone: On-hook");
        readDigit = -1;
        pulseCount = 0;
      }
    }
  }

  lastPulse = pulse;
}
