/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Slider Mux Phased Rhythms
//  
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
#include <MIDI.h>
#include <TimerOne.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Comment this out if you don't want NoteOff messages - e.g. for drum tracks
//#define SEND_NOTE_OFF

// Define the rhythm using MIDI note numbers for drum tracks
// Examples:
//   35 = Acoustic Bass Drum
//   38 = Acoustic Snare
//   42 = Closed Hi-Hat
// etc
#define NUM_NOTES 2
#define NUM_STEPS 8
int steps[NUM_STEPS][NUM_NOTES] = {
/* Step 0 */ { 35, 42 },  
/* Step 1 */ {  0,  0 },  
/* Step 2 */ {  0, 42 },  
/* Step 3 */ {  0,  0 },  
/* Step 4 */ { 38, 42 },  
/* Step 5 */ {  0,  0 },  
/* Step 6 */ {  0, 42 },  
/* Step 7 */ {  0,  0 },  
};

#define MIN_TEMPO 5000 // 1 tick every 5 milliseconds
#define MINTICK   20   // Minimum number of ticks for the track

#define NUM_TRACKS 4
int midichan[NUM_TRACKS]={10, 10, 10, 10};
uint16_t trackcnt[NUM_TRACKS];
uint16_t trackmax[NUM_TRACKS];
int track[NUM_TRACKS];

// Uncomment this to use for the "slider board" with an Arduino Nano
//#define SLIDER_BOARD_NANO 1

// Uncomment to use a multiplexer
//#define ALGMUX 1
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

#define NUMPOTS 4
// In MUX mode this is the pot number for the MUX.
// In non-MUX mode these are Arduino analog pins.
#ifdef ALGMUX
int pots[NUMPOTS] = {0,1,2,3};
#else
// (actually, you could get away with using 0,1,2,3... here too)
int pots[NUMPOTS] = {A0,A1,A2,A3};
#endif

// Optional: comment out if you want to stick with the default tempo
#define TEMPO_POT A7
int lasttempo;

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

  // And then read the analog value from the MUX signal pin.
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
  //   Update each track's counter.
  //   Compare it with the timeout value.
  //   If timed out, play the next step in the rhythm for that track.
  for (int i=0; i<NUM_TRACKS; i++) {
    trackcnt[i]++;
    if (trackcnt[i] >= trackmax[i]) {
      // This track has a step to play
      trackcnt[i] = 0;
#ifdef SEND_NOTE_OFF
      // Stop any already playing notes on the track
      for (int n=0; n<NUM_NOTES; n++) {
        if (steps[track[i]][n] != 0) {
          MIDI.sendNoteOff (steps[track[i]][n], 0, midichan[i]);
        }
      }
#endif
      // Now move on to the next step for the track
      track[i]++;
      if (track[i] >= NUM_STEPS) {
        track[i] = 0;
      }
      // Play all notes for this step on this track
      for (int n=0; n<NUM_NOTES; n++) {
        if (steps[track[i]][n] != 0) {
          MIDI.sendNoteOn (steps[track[i]][n], 127, midichan[i]);
        }
      }
    }
  }
}

void setup() {
  for (int i=0; i<NUM_TRACKS; i++) {
    track[i] = NUM_STEPS-1;
    trackmax[i] = MINTICK;
    trackcnt[i] = 0;
  }

#ifdef ALGMUX
  muxInit();
#endif

#ifdef SLIDER_BOARD_NANO
  // This is a 'hack' to use two of the Arduino's IO
  // pins as VCC and GND for the potentiometers.
  //
  // This is specific to my "slider" board, so you
  // probably don't need to worry about doing this.
  pinMode(6,OUTPUT);
  pinMode(7,OUTPUT);
  digitalWrite(6,LOW);  // GND
  digitalWrite(7,HIGH); // VCC
#endif

  MIDI.begin(MIDI_CHANNEL_OFF);

  delay(1000);
  lasttempo = MIN_TEMPO;
  Timer1.initialize(lasttempo);
  Timer1.attachInterrupt(doTick);
}

void loop() {
  for (int i=0; i<NUMPOTS && i<NUM_TRACKS; i++) {
    // Create a number of ticks value between MIN and MIN+255
    int algval = ALGREAD(pots[i])>>2;
    trackmax[i] = MINTICK + algval;
  }

#ifdef TEMPO_POT
  int newtempo = MIN_TEMPO + 4*analogRead(TEMPO_POT);
  if (newtempo != lasttempo) {
    Timer1.setPeriod(newtempo);
    lasttempo = newtempo;
  }
#endif
}
