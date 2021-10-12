/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  JC Pro Macro Step Sequencer
//  https://diyelectromusic.wordpress.com/2021/10/10/jc-pro-macro-v2-midi-controller/
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
   Example code for the JC Macro Pro: 
*/
#include <MIDI.h>
#include <RotaryEncoder.h>
#include <Adafruit_NeoPixel.h>

// Uncomment the following to use SoftwareSerial MIDI on the IO pins instead of USB MIDI
//#define SWSERIAL_MIDI 1

#ifdef SWSERIAL_MIDI
#include <SoftwareSerial.h>
#else
#include <USB-MIDI.h>
#endif

//#define TEST 1  // Uncomment to disable MIDI and print to default Serial
//#define TEST_HW 1 // This is a test configuration for prototype hardware!

#define NUM_STEPS  6
#define NUM_TRACKS 2
#define NUM_LEDS   NUM_STEPS
#ifdef TEST_HW
#define EXTRA_LEDS 3 // In addition to the sequence there are three control LEDS
#define SEQLED     1
#define EDITLED    2
#define STEPLED    3 // LEDs for steps start sequentially from this one
#define MODE_PIN   9
#else
#define EXTRA_LEDS 2 // In addition to the sequence there are two control LEDS
#define SEQLED     0
#define EDITLED    1
#define STEPLED    2 // LEDs for steps start sequentially from this one
#define MODE_PIN   8
#endif

// Definitions for the NeoPixels
#define LED_PIN   5
// See the Adafruit_NeoPixel library examples and tutorials for details
Adafruit_NeoPixel strip(NUM_LEDS+EXTRA_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Define various colours to be used as "red,green,blue" triplets
#define LED_STEP_OFF  strip.Color(100,0,0)
#define LED_PLAY_OFF  strip.Color(13,13,13)
#define LED_SEQ_START strip.Color(0,30,0)
#define LED_SEQ_STOP  strip.Color(30,0,0)
//#define LED_STEP_ON   strip.Color(0,50,50)  // Uncomment for all tracks to be the same colour
#ifndef LED_STEP_ON
// Specify colours for each track individually
const byte ledStepOn[NUM_TRACKS][3]={
  {0,50,50},   // Track 0 RGB values
  {50,0,50},   // Track 1 RGB values
};
#endif
#define LED_MIDI_CTRL strip.Color(100,100,100)

// This is required to set up the MIDI library.
#ifdef SWSERIAL_MIDI
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno, but we can't
// use that with the JC Pro Macro 2 as pins 0 and 1 are used for the encoder.
// So this will use the SoftwareSerial library instead.
// This is taken from "AltPinSerial" example in the Arduino MIDI Library.
#define MIDI_RX_PIN 6
#define MIDI_TX_PIN 7
using Transport = MIDI_NAMESPACE::SerialMIDI<SoftwareSerial>;
SoftwareSerial swSerial = SoftwareSerial(MIDI_RX_PIN, MIDI_TX_PIN);
Transport serialMIDI(swSerial);
MIDI_NAMESPACE::MidiInterface<Transport> MIDI((Transport&)serialMIDI);
#else
// USB MIDI Initialisation
USBMIDI_CREATE_INSTANCE(0, MIDI);
#endif

#define MIDI_CHANNEL 1  // Comment out if want to specify different channels per track
#ifndef MIDI_CHANNEL
const byte midiCh[NUM_TRACKS] = {1, 1};
#endif

// Sequencer play modes:
//   0 = Normal mode
//   1 = Reverse mode
//   2 = Double note mode
//   3 = Random mode
//
#define NUM_MODES 4
int seqMode;
bool seqRunning;
bool midictrl;
int curstep;
int multistep;
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
#define RE_A  0  // "CLK"
#define RE_B  1  // "DT"
#define RE_SW 4  // "SW"
RotaryEncoder enc(RE_A, RE_B, RotaryEncoder::LatchMode::FOUR3);
int re_pos = 0;
int last_sw;
int sw_cnt;

// We need an unused analog input as a random seed.
// A7/D6 is one of the pins on one of the headers.
#define RND_POT A7

// The buttons are used as follows:
//   SW2 - Start/Stop the sequencer
//   SW3 - Enter edit mode and edit one of the tracks
//   SW4-9 - Select the note to edit when in edit mode
//   Mode - Change the sequencer play modes
//   Enc Sw - Not used
//
#define NUM_SW 10
#define JSW2  0
#define JSW3  1
#define JSW4  2
#define JSW5  3
#define JSW6  4
#define JSW7  5
#define JSW8  6
#define JSW9  7
#define JMODE 8
#define JRE   9
const int swpins[NUM_SW] = {15, A0, A1, A2, A3, 14, 16, 10, MODE_PIN, 4};
const int swstep[NUM_SW] = {-1, -1, 0, 1, 2, 3, 4, 5, -1, -1};
int     sw[NUM_SW];
int  swcnt[NUM_SW];
int lastsw[NUM_SW];

// There are different modes for the encoder:
//   In "play" mode it changes the tempo.
//   In "edit" mode it selects the note to play.
//
//   The encoder switch will switch in/out of MIDI CC/PC mode
//   where the encoder will send a MIDI PROGRAM or CONTROL message
//   depending on the configuration.
//
// After a certain time in edit mode with no interaction,
// it will automatically switch back to play mode.
//
// Following are used to keep track of what is currently being edited.
int encstep;  // -1 = nothing to edit yet
int enctrack; // -1 = nothing to edit yet
unsigned long enctimeout;
#define ENC_TIMEOUT 5000  // Timeout in mS before mode switches back to play mode

// MIDI Control/Program Change configuration
byte midiprogram;

bool inEditMode () {
  if ((encstep >= 0) && (enctrack >= 0)) {
    return true;
  } else {
    return false;  
  }
}

void swEncoder() {
  // Toggle the MIDI control message function
  midictrl = !midictrl;
}

void decEncoder() {
  if (midictrl) {
    // Update MIDI CC messages
    midiPCCC(-1);
  } else if (inEditMode()) {
    // In edit mode
    if (steps[enctrack][encstep] == 0) {
      // Stay on zero (disabled)
    } else {
      steps[enctrack][encstep]--;
      if (steps[enctrack][encstep] < MIN_NOTE) {
        steps[enctrack][encstep] = 0;  // Disabled
      }
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
  if (midictrl) {
    // Update MIDI CC messages
    midiPCCC(+1);
  } else if (inEditMode()) {
    // In edit mode
    if (steps[enctrack][encstep] < MIN_NOTE) {
      // Jump from disabled to the first note
      steps[enctrack][encstep] = MIN_NOTE;
    } else {
      steps[enctrack][encstep]++;
      if (steps[enctrack][encstep] > MAX_NOTE) {
        steps[enctrack][encstep] = MAX_NOTE;
      }
    }
  } else {
    // In play mode
    tempo+=TEMPO_STEP;
    if (tempo > TEMPO_MAX) {
      tempo = TEMPO_MAX;
    }
  }
}

void initEncoder() {
  // Initialise the encoder switch.
  // Everything else is done in the constructor for the enc object.
  pinMode(RE_SW, INPUT_PULLUP);
  enctrack = -1;
  encstep = -1;
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

    // If the rotary encoder moves, reset the watchdog
    resetEditWd();
  }
}

void serviceEditWd() {
  // Check to see if there is a timeout so an auto mode switch
  unsigned long enctimer = millis();
  if (enctimer > enctimeout) {
    // We have timed out so reset back to play mode
    encstep = -1;
    enctrack = -1;
  }
}

void resetEditWd() {
  // Reset the timeout for the encoder mode changes
  enctimeout = millis() + ENC_TIMEOUT;
}

void initDisplay() {
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(20); // Set BRIGHTNESS fairly low to keep current usage low
}

uint32_t note2rgb (byte note) {
  if (note != 0) {
    // Use the Hue value (0 to 65535) scaled by the MIDI note
    return strip.ColorHSV(map(note, MIN_NOTE, MAX_NOTE, 0, 65535));
  } else {
    return 0;
  }
}

uint32_t track2rgb (int track) {
#ifdef LED_STEP_ON
  return LED_STEP_ON;
#else
  // returns the colour value corresponding to the required track
  return strip.Color(ledStepOn[track][0], ledStepOn[track][1], ledStepOn[track][2]);
#endif
}

// Convert a track and step position into an LED position
// This allows for overlapping use of LEDS, e.g. to superimpose
// several tracks on a single strip of LEDs, or to have
// more steps then LEDs, etc.
int step2LedIdx(int st, int tr) {
  return STEPLED + ((st+(NUM_STEPS*tr))%NUM_LEDS);
}

void scanDisplay() {
  // What the display shows will depend on the encoder
  // mode and state of the switches.
  //
  strip.clear();

  // SW2 is the Sequence Start/Stop switch
  if (seqRunning) {
    strip.setPixelColor(SEQLED, LED_SEQ_START);
  } else {
    strip.setPixelColor(SEQLED, LED_SEQ_STOP);
  }

  // SW3 is the Track Edit selection/MIDI contrl
  if (midictrl) {
    strip.setPixelColor(EDITLED, LED_MIDI_CTRL);
  } else if (enctrack >= 0) {
    strip.setPixelColor(EDITLED, track2rgb(enctrack));
  } else {
    // Off, we're in play mode
  }

  // SW4-9 are the notes being played or edited and
  // which lights up depends on the track and step selected.
  if (inEditMode()) {
    if (steps[enctrack][encstep] != 0) {
      strip.setPixelColor(step2LedIdx(encstep,enctrack), note2rgb(steps[enctrack][encstep]));
    } else {
      //  This step is "off"
      strip.setPixelColor(step2LedIdx(encstep,enctrack), LED_STEP_OFF);
    }
  } else {
    // Play mode - play all tracks
    for (int t=0; t<NUM_TRACKS; t++ ){
      if (steps[t][curstep] != 0) {
        // Choose the colour based on the note to be played
        strip.setPixelColor(step2LedIdx(curstep,t), note2rgb(steps[t][curstep]));
      } else {
        // Check existing colour isn't already set.  If we are using
        // overlapping tracks/LEDs then we don't want to override
        // a set pixel with an OFF pixel.
        // If we do have overlapping tracks, then the last track
        // with an ON pixel will be shown.
        if (strip.getPixelColor(step2LedIdx(curstep,t)) == 0) {
          strip.setPixelColor(step2LedIdx(curstep,t), LED_PLAY_OFF);
        }
      }
    }
  }
 
  strip.show();
}

void initIO() {
  for (int i=0; i<NUM_SW; i++) {
    pinMode(swpins[i], INPUT_PULLUP);
    sw[i] = LOW;  // As this is software driven, I can make it active HIGH
    swcnt[i] = 0;
    lastsw[i] = HIGH;
  }
}

void scanIO() {
  for (int s=0; s<NUM_SW; s++) {
    int tmpsw = digitalRead(swpins[s]);
    if (tmpsw == HIGH) {
      // No switch pressed yet
      swcnt[s] = 0;
      lastsw[s] = HIGH;
      sw[s] = LOW;  // Reset
    } else {
      swcnt[s]++;
    }
    if ((lastsw[s]==HIGH) && (swcnt[s] >= 100)) {
      // Switch is triggered!
      switchAction(s);
      sw[s] = HIGH;  // Triggered
      swcnt[s] = 0;
      lastsw[s] = LOW;

      // If a key has been pressed, reset the watchdog
      resetEditWd();
    }
  }
}

void switchAction (int sw) {
  switch (sw){
    case JSW2:
      // Sequencer start/stop switch
      seqRunning = !seqRunning;
      break;
    case JSW3:
      // Track edit switch
      enctrack++;
      if (enctrack >= NUM_TRACKS) {
        // Wrap around between editable tracks.
        // It will automatically time out of edit mode.
        enctrack = 0;
      }
      break;
    case JMODE:
      // Sequencer play mode switch
      seqMode++;
      if (seqMode >= NUM_MODES) {
        seqMode = 0;
      }
      break;
    case JRE:
      // Rotary encoder switch
      swEncoder();
      break;
    default:
      // Handle SW3 to SW9
      // If a track has been selected then we are almost
      // in edit mode, so we are now choosing our note to edit.
      if (enctrack >= 0) {
        encstep = swstep[sw];
      }
      break;
  }
}

void setup() {
  curstep = 0;
  tempo = TEMPO_START;

  randomSeed (analogRead(RND_POT));

  for (int t=0; t<NUM_TRACKS; t++) {
    lastnote[t] = NO_NOTE;
    for (int i=0; i<NUM_STEPS; i++) {
      steps[t][i] = 0;  // All start silent
    }
  }

  initEncoder();
  initDisplay();
  initIO();

#ifdef TEST
  Serial.begin(9600);
  Serial.println("Running in non-MIDI test mode");
#else
  MIDI.begin(MIDI_CHANNEL_OFF);
#endif
}

void loop() {
  //
  // First handle any IO scanning, which happens every time through the loop
  //
  scanEncoder();
  scanIO();
  scanDisplay();
  serviceEditWd();

  //
  // Now see if it is time to handle the playing of the sequencer
  //

  unsigned long curtime = millis();
  if (curtime < steptime) {
    // Not yet time to play the next step in the sequence
    return;
  }

  // If we are already playing notes then we need to turn them off first
  for (int t=0; t<NUM_TRACKS; t++) {
#ifdef MIDI_CHANNEL
    byte ch = MIDI_CHANNEL;
#else
    byte ch = midiCh[t];
#endif
    if (lastnote[t] != NO_NOTE) {
#ifdef TEST
        Serial.print("MIDI Note Off: ");
        Serial.println(lastnote[t]);
#else
        MIDI.sendNoteOff(lastnote[t], 0, ch);
#endif
        lastnote[t] = NO_NOTE;
    }
  }

  if (!seqRunning) {
    // Sequence is paused
    return;
  }

  // The tempo is in beats per minute (bpm).
  // So beats per second = tempo/60
  // So the time in seconds of each beat = 1/beats per second
  // So time in mS = 1000 / (tempo/60)
  // or time in mS = 60000 / tempo
  steptime = curtime + 60000UL/tempo;

  // Prepare the next step in the sequence
  if (seqMode == 0) {
    // Normal play mode
    curstep++;
  } else if (seqMode == 1) {
    // Reverse play mode
    if (curstep > 0) {
      curstep--;      
    } else {
      curstep = NUM_STEPS-1;
    }
  } else if (seqMode == 2) {
    // Double step mode
    if (multistep == 0) {
      multistep++;
    } else {
      multistep = 0;
      curstep++;
    }
  } else if (seqMode == 3) {
    // Random step mode
    curstep = random(NUM_STEPS);
  } else {
    // Undefined mode
    seqMode = 0;
    return;
  }

  // Sanity check curstep to catch wrap around
  if (curstep >= NUM_STEPS) {
    curstep = 0;
  }

  for (int t=0; t<NUM_TRACKS; t++) {
#ifdef MIDI_CHANNEL
    byte ch = MIDI_CHANNEL;
#else
    byte ch = midiCh[t];
#endif
    if (steps[t][curstep] != 0) {
#ifdef TEST
      Serial.print("MIDI Note On: ");
      Serial.println(lastnote[t]);
#else
      MIDI.sendNoteOn(steps[t][curstep], 127, ch);
#endif
      lastnote[t] = steps[t][curstep];
    }
  }
}

void midiPCCC (int dir) {
  if (dir > 0) {
    // Increment the program number/conrol change
    midiprogram++;
    if (midiprogram > 127) midiprogram = 0;
  } else if (dir < 0) {
    // Decrement the program number/ontrol change
    if (midiprogram > 0) {
      midiprogram--;
    } else {
      midiprogram = 127;
    }
  } else {
    // do nothing if direction hasn't changed
    return;
  }

  // Then send the MIDI Program Change message
#ifdef MIDI_CHANNEL
  byte ch = MIDI_CHANNEL;
#else
  byte ch = midiCh[t];
#endif
  MIDI.sendProgramChange (midiprogram, ch);  
}
