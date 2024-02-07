/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Slider Mux Phased Relays
//  https://diyelectromusic.wordpress.com/2021/07/23/arduino-mux-slider-phased-relays/
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
*/
#include <TimerOne.h>

#define TICK    2000  // 1 tick every 2 milliseconds
#define MINTICK  100  // Minimum number of ticks for the relay

#define NUMRELAYS 8
int relaypins[NUMRELAYS]={3,4,5,6,7,8,9,10};
uint16_t relaycnt[NUMRELAYS];
uint16_t relaymax[NUMRELAYS];
int relay[NUMRELAYS];

// Uncomment to use a multiplexer
#define ALGMUX 1
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

#define NUMPOTS 8
// In MUX mode this is the pot number for the MUX.
// In non-MUX mode these are Arduino analog pins.
#ifdef ALGMUX
int pots[NUMPOTS] = {0,1,2,3,4,5,6,7};
#else
// (actually, you could get away with using 0,1,2,3... here too)
int pots[NUMPOTS] = {A0,A1,A2,A3,A4,A5,A6,A7};
#endif
int last[NUMPOTS];

#ifdef ALGMUX
#define ALGREAD muxAnalogRead
#else
#define ALGREAD analogRead
#endif

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
  //   Update each relay's counter.
  //   Compare it with the timeout value.
  //   If timed out, toggle the relay.
  for (int i=0; i<NUMRELAYS; i++) {
    relaycnt[i]++;
    if (relaycnt[i] >= relaymax[i]) {
      relaycnt[i] = 0;
      relay[i] = !relay[i];
      digitalWrite(relaypins[i], relay[i]);
    }
  }
}

void setup() {
  Serial.begin(9600);
  delay(200);
  for (int i=0; i<NUMRELAYS; i++) {
    pinMode(relaypins[i], OUTPUT);
    relay[i] = LOW;
    digitalWrite(relaypins[i], LOW);
    relaymax[i] = 0;
    relaycnt[i] = 0;
  }
  delay(200);

#ifdef ALGMUX
  muxInit();
#endif
  // Perform a test read
  scanPots();

  delay(1000);
  Timer1.initialize(TICK);
  Timer1.attachInterrupt(doTick);
}

void scanPots () {
  for (int i=0; i<NUMPOTS && i<NUMRELAYS; i++) {
    int alg = ALGREAD(pots[i]);
    relaymax[i] = MINTICK + alg; // Value between 250 and 250+1023
    Serial.print(alg);
    Serial.print("\t");
  }
  Serial.print("\n");
}

void loop() {
  scanPots();
}
