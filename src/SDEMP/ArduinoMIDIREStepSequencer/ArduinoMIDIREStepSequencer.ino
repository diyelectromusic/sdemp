/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Rotary Encoder Step Sequencer
//  https://diyelectromusic.wordpress.com/2021/09/30/arduino-midi-rotary-encoder-step-sequencer/
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
*/
#include <MIDI.h>
#include <RotaryEncoder.h>
#include <HT16K33.h>

#define NUM_STEPS 32

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1  // Define which MIDI channel to transmit on (1 to 16).

int curstep;
byte lastnote;
int tempo;
#define NO_NOTE 0xFF

// Tempo in beats per minute
#define TEMPO_STEP    2  // change per "click" of the encoder
#define TEMPO_MIN    30  // 1 every two seconds
#define TEMPO_MAX   480  // 8 a second
#define TEMPO_START 360  // 6 a second

#define MIN_NOTE 36  // C2
#define MAX_NOTE 84  // C6

byte steps[NUM_STEPS];
unsigned long steptime;

// I2C 7 Segment Display
//   C/CLK = A5/SCL
//   D/DAT = A4/SDA
HT16K33 seg(0x70);  // Pass in I2C address of the display

// Rotary Encoder
#define RE_A  A0  // "CLK"
#define RE_B  A1 // "DT"
#define RE_SW A2 // "SW"
RotaryEncoder enc(RE_A, RE_B, RotaryEncoder::LatchMode::TWO03);
int re_pos = 0;
int last_sw;
int sw_cnt;

// There are three modes for the encoder, which swap when the switch is pressed:
//   Mode = 0: Play mode: the display matchs the sequence being played
//   Mode = 1: Select Step; Enc updates display; switch selects this step and switches to note mode.
//   Mode = 2: Select Note; Enc updates notes live; switch switches back to step mode.
//
// When in modes 1 or 2 it switches back to mode 0 after a timeout
//
byte encmode;
int encstep;
unsigned long enctimeout;
#define ENC_TIMEOUT 5000  // Timeout in mS before mode switches back to play mode

void swEncoder() {
  if (encmode==2) {
    // In note mode
    encmode = 1; // Switch to step mode
    if (encstep < NUM_STEPS-1) {
      encstep++;   // And automatically increment the step
    }
  } else if (encmode==1) {
    // In step mode
    encmode = 2; // Switch to note mode
  } else {
    // In Play mode
    encmode = 1; // Switch to step mode
    encstep = 0; // Start from the first step
  }
  // It will auto-switch back to play mode on timeout
}

void decEncoder() {
  if (encmode==2) {
    // In note mode
    if (steps[encstep] == 0) {
      // Stay on zero (disabled)
    } else {
      steps[encstep]--;
      if (steps[encstep] < MIN_NOTE) {
        steps[encstep] = 0;  // Disabled
      }
    }
  } else if (encmode==1){
    // In step mode
    encstep--;
    if (encstep < 0 ) {
      encstep = NUM_STEPS-1;
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
  if (encmode==2) {
    // In note mode
    if (steps[encstep] < MIN_NOTE) {
      // Jump from disabled to the first note
      steps[encstep] = MIN_NOTE;
    } else {
      steps[encstep]++;
      if (steps[encstep] > MAX_NOTE) {
        steps[encstep] = MAX_NOTE;
      }
    }
  } else if (encmode==1){
    // In step mode
    encstep++;
    if (encstep >= NUM_STEPS ) {
      encstep = 0;
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
  int new_pos = enc.getPosition();
  if (new_pos != re_pos) {
    int re_dir = (int)enc.getDirection();
    if (re_dir < 0) {
      decEncoder();
    } else if (re_dir > 0) {
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
  if ((last_sw==HIGH) && (sw_cnt >= 1000)) {
    // Switch is triggered!
    swEncoder();
    sw_cnt = 0;
    last_sw = LOW;
    resetEncoder();
  }

  timeEncoder();
}

void initDisplay() {
  // Initialise the I2C display
  seg.begin();
  Wire.setClock(100000);
  seg.displayOn();
  seg.brightness(2);
  seg.displayClear();
}

void scanDisplay() {
  // What the display shows will depend on the encoder mode!
  int dispnum;
  if (encmode==2) {
    // Note display
    dispnum = steps[encstep];
  } else if (encmode==1) {
    // Step display (use 1 to NUM_STEPS not 0 indexed)
    dispnum = encstep + 1;
  } else {
    // Play mode (use 1 to NUM_STEPS)
    dispnum = curstep + 1;
  }
  // Update the display
  seg.setDigits(1); // Display minimum of a single digit
  seg.displayInt(dispnum);
}

void setup() {
  curstep = 0;
  lastnote = NO_NOTE;
  tempo = TEMPO_START;

  for (int i=0; i<NUM_STEPS; i++) {
    steps[i] = 0;  // All start silent
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

  // If we are already playing a note then we need to turn it off first
  if (lastnote != NO_NOTE) {
      MIDI.sendNoteOff(lastnote, 0, MIDI_CHANNEL);
      lastnote = NO_NOTE;
  }

  if (steps[curstep] != 0) {
    MIDI.sendNoteOn(steps[curstep], 127, MIDI_CHANNEL);
    lastnote = steps[curstep];
  }
}
