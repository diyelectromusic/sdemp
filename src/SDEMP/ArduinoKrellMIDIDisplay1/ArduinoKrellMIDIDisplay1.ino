/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Forbidden Planet Krell MIDI Display
//  https://diyelectromusic.com/2025/01/26/forbidden-planet-krell-display-midi-notes/
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

// ----------------------------------
// MIDI and IO Settings
// ----------------------------------

struct MySettings : public MIDI_NAMESPACE::DefaultSettings {
  static const bool Use1ByteParsing = false; // Allow MIDI.read to handle all received data in one go
  static const long BaudRate = 31250;        // Doesn't build without this...
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings);
// MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL MIDI_CHANNEL_OMNI  // 1 to 16 or MIDI_CHANNEL_OMNI
#define MIDI_NOTE_START 48  // C3
#define MIDI_NOTE_END   MIDI_NOTE_START+39

#define MIDI_LED LED_BUILTIN
#define TIMING_PIN 11

// ----------------------------------
// Simple inactivity timer...
// ----------------------------------
unsigned long inactivity;
void resetActivity () {
  inactivity = millis() + 2000; // 2 second timer
}

bool checkActivity () {
  if (inactivity < millis()) {
    return false;  // Timed out - no activity
  }
  return true; // Still going
}

// ----------------------------------
// Display details
// ----------------------------------
#define LED_PIN   10
#define LED_BLOCK 5
#define LED_RINGS 8
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
  for (int i=0; i<LED_COUNT; i++) {
    ledState[i] = false;
  }
  ledUpdate = false;
  strip.begin();
  strip.show();
  strip.setBrightness(50);
}

void scanStrip () {
  if (ledUpdate) {
    ledUpdate = false;
    strip.clear();
    for(int i=0; i<LED_COUNT; i++) {
      if (ledState[i]) {
        strip.setPixelColor(leds[i], strip.Color(80, 35, 0));
      }
    }
#ifdef TIMING_PIN
    digitalWrite(TIMING_PIN, HIGH);
#endif
    strip.show();
#ifdef TIMING_PIN
    digitalWrite(TIMING_PIN, LOW);
#endif
  }
}

void ledOn (int led) {
  if (led < LED_COUNT) {
    ledState[led] = true;
    ledUpdate = true;
  }
}

void ledOff (int led) {
  if (led < LED_COUNT) {
    ledState[led] = false;
    ledUpdate = true;
  }
}

void allLedsOff (void) {
  for(int i=0; i<LED_COUNT; i++) {
    ledState[i] = false;
  }
  ledUpdate = true;
}

// ----------------------------------
// MIDI Handling
// ----------------------------------
void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  if ((pitch < MIDI_NOTE_START) || (pitch > MIDI_NOTE_END)) {
    // The note is out of range for us so do nothing
    return;
  }
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, HIGH);
#endif

  int led = pitch - MIDI_NOTE_START;
  ledOn (led);
  resetActivity();
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  if ((pitch < MIDI_NOTE_START) || (pitch > MIDI_NOTE_END)) {
    // The note is out of range for us so do nothing
    return;
  }

#ifdef MIDI_LED
  digitalWrite(MIDI_LED, LOW);
#endif

  int led = pitch - MIDI_NOTE_START;
  ledOff (led);
  resetActivity();
}

void initMidi () {
  MIDI.begin(MIDI_CHANNEL);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.turnThruOff();
}

// ----------------------------------
// Arduino setup/loop
// ----------------------------------

void setup() {
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
  digitalWrite(MIDI_LED, LOW);
#endif
#ifdef TIMING_PIN
  pinMode(TIMING_PIN, OUTPUT);
  digitalWrite(TIMING_PIN, LOW);
#endif

  initStrip();
  initMidi();
}

unsigned ledcnt;
void loop() {
  MIDI.read();
  ledcnt++;
  if (ledcnt > 256) {
    scanStrip();
    ledcnt = 0;
  }
  if (!checkActivity()) {
    allLedsOff();
    resetActivity();
  }
}
