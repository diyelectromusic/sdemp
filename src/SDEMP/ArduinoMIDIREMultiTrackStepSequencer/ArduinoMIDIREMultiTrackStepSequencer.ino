/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Rotary Encoder Multi-Track Step Sequencer (NeoPixels)
//  https://diyelectromusic.wordpress.com/2021/10/03/arduino-midi-rotary-encoder-multi-track-step-sequencer/
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
    Analog Input  - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Adafruit NeoPixels - https://learn.adafruit.com/adafruit-neopixel-uberguide
*/
#include <MIDI.h>
#include <RotaryEncoder.h>
#include <Adafruit_NeoPixel.h>

#define NUM_STEPS    6
#define NUM_TRACKS   2

// Definitions for the NeoPixels
#define LED_PIN   6
#define LED_COUNT (NUM_STEPS*NUM_TRACKS)
// See the Adafruit_NeoPixel library examples and tutorials for details
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Define various colours to be used as "red,green,blue" triplets
#define LED_STEP_OFF  strip.Color(0,0,0)
#define LED_PLAY_OFF  strip.Color(15,15,15)
//#define LED_STEP_ON   strip.Color(0,50,50)  // Uncomment for all tracks to be the same colour
#ifndef LED_STEP_ON
// Specify colours for each track individually
const byte ledStepOn[NUM_TRACKS][3]={
  {0,50,50},   // Track 0 RGB values
  {50,50,0}    // Track 1 RGB values
};
#endif

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1  // Comment out if want to specify different channels per track
#ifndef MIDI_CHANNEL
const byte midiCh[NUM_TRACKS] = {1, 1};
#endif

int curstep;
byte lastnote[NUM_TRACKS];
int tempo;
#define NO_NOTE 0xFF

// Tempo in beats per minute
#define TEMPO_STEP    2  // change per "click" of the encoder
#define TEMPO_MIN    30  // 1 every two seconds
#define TEMPO_MAX   480  // 8 a second
#define TEMPO_START 360  // 6 a second

#define MIN_NOTE 36  // C2
#define MAX_NOTE 84  // C6

byte steps[NUM_TRACKS][NUM_STEPS];
unsigned long steptime;

// Rotary Encoder
#define RE_A  A4  // "CLK"
#define RE_B  A3 // "DT"
#define RE_SW A2 // "SW"
RotaryEncoder enc(RE_A, RE_B, RotaryEncoder::LatchMode::TWO03);
int re_pos = 0;
int last_sw;
int sw_cnt;

// Optional "IO as power" (comment out if not required)
#define RE_VCC A1
#define RE_GND A0

// There are three modes for the encoder, which swap when the switch is pressed:
//   Mode = 0: Play mode: the display matchs the sequence being played
//   Mode = 1: Select Track; Enc updates display; switch selects this track and switches to step mode.
//   Mode = 2: Select Step; Enc updates display; switch selects this step and switches to note mode.
//   Mode = 3: Select Note; Enc updates notes live; switch switches back to step mode.
//
// When in modes 1, 2 or 3 it switches back to mode 0 after a timeout
//
byte encmode;
int encstep;
int enctrack;
unsigned long enctimeout;
#define ENC_TIMEOUT 5000  // Timeout in mS before mode switches back to play mode

void swEncoder() {
  if (encmode==3) {
    // In note mode
    encmode = 2; // Switch back to step mode
    if (encstep < NUM_STEPS-1) {
      encstep++;   // And automatically increment the step
    }
  } else if (encmode==2) {
    // In step mode
    encmode = 3; // Switch to note mode
  } else if (encmode==1) {
    // In track mode
    encmode = 2; // Switch to step mode
  } else {
    // In Play mode
    encmode = 1;  // Switch to track mode
    enctrack = 0; // Start from the first track
    encstep = 0;  // Start from the first step
  }
  // It will auto-switch back to play mode on timeout
}

void decEncoder() {
  if (encmode==3) {
    // In note mode
    if (steps[enctrack][encstep] == 0) {
      // Stay on zero (disabled)
    } else {
      steps[enctrack][encstep]--;
      if (steps[enctrack][encstep] < MIN_NOTE) {
        steps[enctrack][encstep] = 0;  // Disabled
      }
    }
  } else if (encmode==2){
    // In step mode
    encstep--;
    if (encstep < 0 ) {
      encstep = NUM_STEPS-1;
    }
  } else if (encmode==1){
    // In track mode
    enctrack--;
    if (enctrack < 0 ) {
      enctrack = NUM_TRACKS-1;
    }
  } else {
    // In play mode
    tempo-=TEMPO_STEP;
    if (tempo < TEMPO_MIN) {
      tempo = TEMPO_MIN;
    }
  }
}

void incEncoder() {
  if (encmode==3) {
    // In note mode
    if (steps[enctrack][encstep] < MIN_NOTE) {
      // Jump from disabled to the first note
      steps[enctrack][encstep] = MIN_NOTE;
    } else {
      steps[enctrack][encstep]++;
      if (steps[enctrack][encstep] > MAX_NOTE) {
        steps[enctrack][encstep] = MAX_NOTE;
      }
    }
  } else if (encmode==2){
    // In step mode
    encstep++;
    if (encstep >= NUM_STEPS ) {
      encstep = 0;
    }
  } else if (encmode==1){
    // In track mode
    enctrack++;
    if (enctrack >= NUM_TRACKS ) {
      enctrack = 0;
    }
  } else {
    // In play mode
    tempo+=TEMPO_STEP;
    if (tempo > TEMPO_MAX) {
      tempo = TEMPO_MAX;
    }
  }
}

void timeEncoder() {
  // Check to see if there is a timeout so an auto mode switch
  if (encmode==0) {
    // Already in play mode so nothing to do
    return;
  }

  unsigned long enctimer = millis();
  if (enctimer > enctimeout) {
    // We have timed out so reset back to play mode
    encmode = 0;
    encstep = 0;
  }
}

void resetEncoder() {
  // Reset the timeout for the encoder mode changes
  enctimeout = millis() + ENC_TIMEOUT;
}

void initEncoder() {
#ifdef RE_VCC
  pinMode(RE_VCC, OUTPUT);
  pinMode(RE_GND, OUTPUT);
  digitalWrite(RE_GND, LOW);
  digitalWrite(RE_VCC, HIGH);
#endif

  // Initialise the encoder switch.
  // Everything else is done in the constructor for the enc object.
  pinMode(RE_SW, INPUT_PULLUP);
  encmode = 0;
  encstep = 0;
}

void scanEncoder() {
  // Read the rotary encoder
  enc.tick();
  
  // Check the encoder direction
  // I'm reversing it here to get the orientation I want
  int new_pos = enc.getPosition();
  if (new_pos != re_pos) {
    int re_dir = (int)enc.getDirection();
    if (re_dir > 0) {
      decEncoder();
    } else if (re_dir < 0) {
      incEncoder();
    } else {
      // if re_dir == 0; do nothing
    }
    re_pos = new_pos;
    resetEncoder();
  }

  // Check the encoder switch with some software debouncing
  int re_sw = digitalRead(RE_SW);
  if (re_sw == HIGH) {
    // No switch pressed yet
    sw_cnt = 0;
    last_sw = HIGH;
  } else {
    sw_cnt++;
  }
  if ((last_sw==HIGH) && (sw_cnt >= 100)) {
    // Switch is triggered!
    swEncoder();
    sw_cnt = 0;
    last_sw = LOW;
    resetEncoder();
  }

  timeEncoder();
}

void initDisplay() {
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(20); // Set BRIGHTNESS fairly low to keep current usage low
}

uint32_t note2led (byte note) {
  if (note != 0) {
    // Use the Hue value (0 to 65535) scaled by the MIDI note
    return strip.ColorHSV(map(note, MIN_NOTE, MAX_NOTE, 0, 65535));
  } else {
    return 0;
  }
}

uint32_t track2led (int track) {
#ifdef LED_STEP_ON
  return LED_STEP_ON;
#else
  // returns the colour value corresponding to the required track
  return strip.Color(ledStepOn[track][0], ledStepOn[track][1], ledStepOn[track][2]);
#endif
}

// Convert a track and step position into an LED position
#define Step2LedIdx(st,tr) (st+(NUM_STEPS*tr))
void scanDisplay() {
  // What the display shows will depend on the encoder mode!
  strip.clear();
  if (encmode==3) {
    // Note display - show colour based on the current note and current track
    if (steps[enctrack][encstep] != 0) {
      strip.setPixelColor(Step2LedIdx(encstep,enctrack), note2led(steps[enctrack][encstep]));
    } else {
      //  This step is "off"
      strip.setPixelColor(Step2LedIdx(encstep,enctrack), LED_STEP_OFF);
    }
  } else if (encmode==2) {
    // Step display - just highlight the current step for the current track
    strip.setPixelColor(Step2LedIdx(encstep,enctrack), track2led(enctrack));
  } else if (encmode==1) {
    // Track display - highlight the current track start and end LEDs
    strip.setPixelColor(Step2LedIdx(0,enctrack), track2led(enctrack));
    strip.setPixelColor(Step2LedIdx(NUM_STEPS-1,enctrack), track2led(enctrack));
  } else {
    // Play mode - play all tracks
    for (int t=0; t<NUM_TRACKS; t++ ){
      if (steps[t][curstep] != 0) {
        // Choose the colour based on the note to be played
        strip.setPixelColor(Step2LedIdx(curstep,t), note2led(steps[t][curstep]));
      } else {
        strip.setPixelColor(Step2LedIdx(curstep,t), LED_PLAY_OFF);
      }
    }
  }
 
  strip.show();
}

void setup() {
  curstep = 0;
  tempo = TEMPO_START;

  for (int t=0; t<NUM_TRACKS; t++) {
    lastnote[t] = NO_NOTE;
    for (int i=0; i<NUM_STEPS; i++) {
      steps[t][i] = 0;  // All start silent
    }
  }

  initEncoder();
  initDisplay();

  MIDI.begin(MIDI_CHANNEL_OFF);
}

void loop() {
  //
  // First handle any IO scanning, which happens every time through the loop
  //
  scanEncoder();
  scanDisplay();

  //
  // Now see if it is time to handle the playing of the sequencer
  //

  unsigned long curtime = millis();
  if (curtime < steptime) {
    // Not yet time to play the next step in the sequence
    return;
  }

  // The tempo is in beats per minute (bpm).
  // So beats per second = tempo/60
  // So the time in seconds of each beat = 1/beats per second
  // So time in mS = 1000 / (tempo/60)
  // or time in mS = 60000 / tempo
  steptime = curtime + 60000UL/tempo;

  // Prepare the next step in the sequence
  curstep++;
  if (curstep >= NUM_STEPS) {
    curstep = 0;
  }

  for (int t=0; t<NUM_TRACKS; t++) {
    // If we are already playing a note then we need to turn it off first
#ifdef MIDI_CHANNEL
    byte ch = MIDI_CHANNEL;
#else
    byte ch = midiCh[t];
#endif
    if (lastnote[t] != NO_NOTE) {
        MIDI.sendNoteOff(lastnote[t], 0, ch);
        lastnote[t] = NO_NOTE;
    }
  
    if (steps[t][curstep] != 0) {
      MIDI.sendNoteOn(steps[t][curstep], 127, ch);
      lastnote[t] = steps[t][curstep];
    }
  }
}
