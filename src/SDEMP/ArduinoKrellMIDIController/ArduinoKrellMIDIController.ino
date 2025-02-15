/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Forbidden Planet Krell MIDI Controller
//  https://diyelectromusic.com/2025/02/15/forbidden-planet-krell-display-midi-cc-controller/
//
      MIT License
      
      Copyright (c) 2025 diyelectromusic (Kevin)
      
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
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>


struct MySettings : public MIDI_NAMESPACE::DefaultSettings {
  static const bool Use1ByteParsing = false; // Allow MIDI.read to handle all received data in one go
  static const long BaudRate = 31250;        // Doesn't build without this...
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings);
// MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1  // 1 to 16

#define MIDI_LED LED_BUILTIN

//#define POTREV // Reverse sense of pots
#define NUM_POTS 4
int pots[NUM_POTS] = {A0, A1, A2, A3};
int midival[NUM_POTS];

uint8_t cc[NUM_POTS] = {
  0x01,  // Modulation wheel
  0x07,  // Channel volume
  0x08,  // Balance
  0x0A,  // Pan
//  0x0B,  // Expression control
//  0x0C,  // Effect control 1
//  0x0D,  // Effect control 2
//  0x10,  // General purpose control 1
//  0x11,  // General purpose control 2
//  0x12,  // General purpose control 3
//  0x13,  // General purpose control 4
//  0x14,  // General purpose control 5
//  0x15,  // General purpose control 6
//  0x16,  // General purpose control 7
//  0x17,  // General purpose control 8
};

// ----------------------------------
// Display details
// ----------------------------------
#define LED_PIN   10
#define LED_BLOCK 5
#define LED_RINGS 4
#define LED_COUNT (LED_BLOCK*LED_RINGS)
#define STRIP_BLOCK 7
#define STRIP_COUNT (LED_RINGS*STRIP_BLOCK)

int ledpattern[LED_BLOCK] = {1,0,5,4,3};
int leds[LED_COUNT];
bool ledState[LED_COUNT];
bool ledUpdate;

Adafruit_NeoPixel strip(STRIP_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void initStrip () {
  for (int i=0; i<LED_RINGS; i++) {
    for (int j=0; j<LED_BLOCK; j++) {
      leds[LED_BLOCK*i + j] = ledpattern[j] + STRIP_BLOCK*i;
    }
  }
  ledUpdate = false;
  for (int i=0; i<LED_COUNT; i++) {
    ledState[i] = false;
  }
  ledUpdate = false;
  strip.begin();
  strip.show();
  strip.setBrightness(50);
}

unsigned long inactivity;
void scanStrip () {
  if (ledUpdate) {
    ledUpdate = false;
    strip.clear();
    for(int i=0; i<LED_COUNT; i++) {
      if (ledState[i]) {
        strip.setPixelColor(leds[i], strip.Color(80, 35, 0));
      }
    }
    strip.show();
  }
}

void ledOn (int ring, int led) {
  int ledidx = ring*LED_BLOCK + led;
  if (ledidx < LED_COUNT) {
    if (!ledState[ledidx]) {
      // Turn LED on
      ledUpdate = true;
      ledState[ledidx] = true;
    }
  }
}

void ledOff (int ring, int led) {
  int ledidx = ring*LED_BLOCK + led;
  if (ledidx < LED_COUNT) {
    if (ledState[ledidx]) {
      // Turn LED off
      ledState[ledidx] = false;
      ledUpdate = true;
    }
  }
}

void ledSetController(int ring, uint8_t value) {
  if (ring >= LED_RINGS) return;

  // Work out how much of the MIDI value per LED.
  // NB: This will round down.
  int ledspan = 127 / LED_BLOCK;
  int ledval = value;
  for (int i=0; i<LED_BLOCK; i++) {
    if (ledval > 0) {
      ledOn(ring, i);
    } else {
      ledOff(ring, i);
    }
    ledval -= ledspan;
  }
}


// ----------------------------------
// MIDI Handling
// ----------------------------------

void sendMidiCC (uint8_t ctrl, uint8_t val) {
  if (ctrl >= NUM_POTS) return;

  MIDI.sendControlChange(cc[ctrl], val, MIDI_CHANNEL);
}

void scanMidi () {
  MIDI.read();
}

void initMidi () {
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOn();
  // Preset the "last" values to something invalid to
  // force an update on the first time through the loop.
  for (int i=0; i<NUM_POTS; i++) {
    midival[i] = -1;
  }
}

// ----------------------------------
// Analog IO Handling
// ----------------------------------

// Use averaging/smoothing as described here:
// https://diyelectromusic.com/2024/12/14/arduino-midi-master-volume-control/
#define AVGEREADINGS 32
int avgepotvals[NUM_POTS][AVGEREADINGS];
int avgepotidx[NUM_POTS];
long avgepottotal[NUM_POTS];

int avgeAnalogRead (int pot) {
  if (pot >= NUM_POTS) return 0;

  int reading = 10*analogRead(pot);
  avgepottotal[pot] = avgepottotal[pot] - avgepotvals[pot][avgepotidx[pot]];
  avgepotvals[pot][avgepotidx[pot]] = reading;
  avgepottotal[pot] = avgepottotal[pot] + reading;
  avgepotidx[pot]++;
  if (avgepotidx[pot] >= AVGEREADINGS) avgepotidx[pot] = 0;
  return (((avgepottotal[pot] / AVGEREADINGS) + 5) / 10);
}

// ----------------------------------
// Arduino setup/loop
// ----------------------------------

void setup() {
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
  digitalWrite(MIDI_LED, LOW);
#endif

  initStrip();
  initMidi();
}

int ledcnt;
int pot;
void loop() {
  scanMidi();

  // Read 10-bit ALG value and convert to MIDI 7-bit
#ifdef POTREV
  int algval = (1023 - avgeAnalogRead (pot)) >> 3;
#else
  int algval = avgeAnalogRead (pot) >> 3;
#endif
  if (algval != midival[pot]) {
    // Value has changed
    ledSetController(pot, algval);
    sendMidiCC(pot, algval);
    midival[pot] = algval;
  }
  pot++;
  if (pot >= NUM_POTS) pot = 0;

  ledcnt++;
  if (ledcnt > 5) {
    scanStrip();
    ledcnt=0;
  }
}
