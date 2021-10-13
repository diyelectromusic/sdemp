/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  JC Pro Macro MIDI Controller
//  https://diyelectromusic.wordpress.com/2021/10/13/jc-pro-macro-v2-midi-controller-part-2/
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
#include "ccmidi.h"

// Uncomment the following to use SoftwareSerial MIDI on the IO pins instead of USB MIDI
//#define SWSERIAL_MIDI 1

#ifdef SWSERIAL_MIDI
#include <SoftwareSerial.h>
#else
#include <USB-MIDI.h>
#endif

//#define TEST 1  // Uncomment to disable MIDI and print to default Serial
//#define TEST_HW 1 // This is a test configuration for prototype hardware!

#ifdef TEST_HW
#define EXTRA_LEDS 1 // The test board has an additional LED first
#define FIRST_LED  1 // The switch LEDs start sequentially from this one
#define MODE_PIN   9
#else
#define EXTRA_LEDS 0
#define FIRST_LED  0
#define MODE_PIN   8
#endif

// Definitions for the NeoPixels
#define NUM_LEDS  8
#define LED_PIN   5
// See the Adafruit_NeoPixel library examples and tutorials for details
Adafruit_NeoPixel strip(NUM_LEDS+EXTRA_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Define various colours to be used as "red,green,blue" triplets
#define LED_COLOUR_EXAMPLE strip.Color(100,100,100)

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
//   SW2-9  - Enable a MIDI CC/PC configuration
//   Mode   - Toggle between "live" and "encoder SW to activate" modes
//   Enc Sw - In non-live mode used to send the control value
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
#define JNOSW 255
const int swpins[NUM_SW] = {15, A0, A1, A2, A3, 14, 16, 10, MODE_PIN, 4};
int     sw[NUM_SW];
int  swcnt[NUM_SW];
int lastsw[NUM_SW];

// Assumes NUM_CCS <= NUM_LEDS < NUM_SW
#define NUM_CCS 8
byte  swcc[NUM_CCS];
int swccto[NUM_CCS];

#define SWCCTOMAX 1000 // Counter value to "timeout" LED displays for MIDI CC switches

int livemode;
int swenable;

void swEncoder() {
  if (swenable == JNOSW) {
    // No active switch
    return;
  }
  if (isMidiCCSwitch(swenable) || isMidiCCOneshot(swenable)) {
    return;
  }
  midiSendPCCC (getMidiCC(swenable), swcc[swenable]);
}

void decEncoder() {
  if (swenable == JNOSW) {
    // No active switch
    return;
  }
  // Only update the control value if this is a continuous controller
  if (isMidiCCSwitch(swenable) || isMidiCCOneshot(swenable)) {
    return;
  }
  if (swcc[swenable] > getMidiCCMin(swenable)) {
    swcc[swenable]--;
  } else {
    swcc[swenable] = getMidiCCMax(swenable);
  }
  if (livemode) {
    midiSendPCCC (getMidiCC(swenable), swcc[swenable]);
  }
}

void incEncoder() {
  if (swenable == JNOSW) {
    // No active switch
    return;
  }
  // Only update the control value if this is a continuous controller
  if (isMidiCCSwitch(swenable) || isMidiCCOneshot(swenable)) {
    return;
  }
  swcc[swenable]++;
  if (swcc[swenable] > getMidiCCMax(swenable)) {
    swcc[swenable] = getMidiCCMin(swenable);
  }
  if (livemode) {
    midiSendPCCC (getMidiCC(swenable), swcc[swenable]);
  }
}

void initEncoder() {
  // Initialise the encoder switch.
  // Everything else is done in the constructor for the enc object.
  pinMode(RE_SW, INPUT_PULLUP);
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
  }
}

uint32_t cc2rgb (byte ccval) {
  // Need to turn the 0 to 127 (i.e. 7 bits) into a value
  // in the 0 to 65535 range (i.e. 16 bits)
  return strip.ColorHSV(ccval<<9);
}

void initDisplay() {
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(20); // Set BRIGHTNESS fairly low to keep current usage low
}

void setLedDisplay (int sw) {
  strip.setPixelColor(FIRST_LED+sw, cc2rgb(swcc[sw]));
}

void scanDisplay() {
  strip.clear();

  // Scan the active LED
  if (swenable != JNOSW) {
    setLedDisplay(swenable);
  }
  strip.show();
}

void initIO() {
  swenable = JNOSW;
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
    }
  }

  timeoutCCSwitchWd();
}

void timeoutCCSwitchWd () {
  // If any of the MIDI CC Switches were pressed, then time them out
  for (int i=0; i<NUM_CCS; i++) {
    if (isMidiCCSwitch(i) || isMidiCCOneshot(i)) {
      if (swccto[i] > 0) {
        swccto[i]--;
      } else {
        if (swenable == i) {
          swenable = JNOSW;
        }
      }
    }
  }
}

void resetCCSwitchWd (int sw) {
  swccto[sw] = SWCCTOMAX;
}

void switchAction (int sw) {
  switch (sw){
    case JMODE:
      // Handle smaller MODE switch
      // Toggle "live mode" - i.e. if updates are sent immediately or need the encoder to send
      livemode = !livemode;
      break;
    case JRE:
      // Rotary encoder switch
      swEncoder();
      break;
    default:
      // Handle SW2 to SW9
      if (isMidiCCSwitch(sw)) {
        // This is an "ON/OFF" controller
        if (swcc[sw] == MIDICC_ON) {
          swcc[sw] = MIDICC_OFF;
        } else {
          swcc[sw] = MIDICC_ON;
        }
        swenable = sw;
        resetCCSwitchWd(sw);

        // Output the MIDI CC value
        midiSendPCCC (getMidiCC(sw), swcc[sw]);
      } else if (isMidiCCOneshot(sw)) {
        // This is a "single value", "one-shot" cotroller
        swcc[sw] = getMidiCCMax(sw);
        swenable = sw;
        resetCCSwitchWd(sw);

        // Output the MIDI CC value
        midiSendPCCC (getMidiCC(sw), swcc[sw]);
      } else {
        // This is a continuous controller
        if (swenable == sw) {
          // If re-pressing active switch, turn it off
          swenable = JNOSW;
        } else {
          // Choose the next switch
          swenable = sw;
        }
      }
      break;
  }
}

void setup() {
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

  //
  // Now see if it is necessary to act on the IO information
  //

}

byte getMidiCC (int idx) {
  return midiCC[idx][MCC];
}
byte getMidiCCMin (int idx) {
  return midiCC[idx][MMIN];
}
byte getMidiCCMax (int idx) {
  return midiCC[idx][MMAX];
}
bool isMidiCCSwitch (int idx) {
  if (midiCC[idx][MMAX] == CCSW) {
    return true;
  } else {
    return false;
  }
}

bool isMidiCCOneshot (int idx) {
  if (midiCC[idx][MMIN] == CCSW) {
    return true;
  } else {
    return false;
  }
}

void midiSendPCCC (byte pccc, byte value) {
#ifdef MIDI_CHANNEL
  byte ch = MIDI_CHANNEL;
#else
  byte ch = midiCh[t];
#endif
  if (pccc == PROGRAM) {
    // Program Change
    MIDI.sendProgramChange (value, ch);  
  } else {
    // Control Change
    MIDI.sendControlChange (pccc, value, ch);  
  }
}
