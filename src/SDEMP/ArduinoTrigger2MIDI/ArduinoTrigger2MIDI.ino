/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Drum Trigger to MIDI
//  https://diyelectromusic.com/2024/10/06/arduino-drum-trigger-to-midi/
//
      MIT License
      
      Copyright (c) 2024 diyelectromusic (Kevin)
      
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
    Arduino Interrupts     - https://dronebotworkshop.com/interrupts/
    Arduino Direct PORT IO - https://docs.arduino.cc/hacking/software/PortManipulation
*/
#include <MIDI.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define TRIGGER_LED LED_BUILTIN
//#define TIMING_PIN  12
//#define ACTIVE_LOW // Uncomment this for inverse trigger pulses - i.e. triggering when LOW.

#define MIDI_CHANNEL 10  // 10 is the percussion channel
#define NUM_TRIGGERS 6   // Cannot be changed without re-writing IO routines

int triggerPins[NUM_TRIGGERS] = {2,3,4,5,6,7};

// General MIDI Drum Instrument Notes.
// Choose 6 by uncommenting the lines.
uint8_t midiTrigger[NUM_TRIGGERS] = {
  35,  // Acoustic Bass Drum
//  37,  // Side Stick
  38,  // Acoustic Snare Drum
//  39,  // Hand Clap
//  47,  // Low-Mid Tom
  42,  // Closed Hi-Hat
//  46,  // Open Hi-Hat
  51,  // Ride Cymbal 1
//  56,  // Cowbell
  62,  // Mute Hi Conga
//  63,  // Open Hi Conga
//  66,  // Low Timbale
  68,  // Low Agogo
//  69,  // Cabasa
//  75,  // Claves
//  77,  // Low Wood Block
//  80,  // Mute Triangle
};

int lastTriggerState[NUM_TRIGGERS];

void triggerSetup() {
  for (int i=0; i<NUM_TRIGGERS; i++) {
    lastTriggerState[i] = LOW;
#ifdef ACTIVE_LOW
    // Take advantage of the built-in PULL UP resistors
    pinMode(triggerPins[i], INPUT_PULLUP);
#else
    // This will require external PULL DOWN resistors
    pinMode(triggerPins[i], INPUT);
#endif
  }
}

void triggerOn (uint8_t drumNote) {
#ifdef TRIGGER_LED
  digitalWrite(TRIGGER_LED, HIGH);
#endif

  // Output MIDI NoteOn for this trigger
  MIDI.sendNoteOn(drumNote, 127, MIDI_CHANNEL);
}

void triggerOff (uint8_t drumNote) {
  // Output MIDI NoteOff for this trigger
  MIDI.sendNoteOff(drumNote, 0, MIDI_CHANNEL);

#ifdef TRIGGER_LED
  digitalWrite(TRIGGER_LED, LOW);
#endif
}

void triggerLoop () {
  for (int i=0; i<NUM_TRIGGERS; i++) {
    int triggerState = digitalRead(triggerPins[i]);
    if (triggerState && !lastTriggerState[i]) {
      // LOW->HIGH
#ifdef ACTIVE_LOW
      triggerOff(midiTrigger[i]);
#else
      triggerOn(midiTrigger[i]);
#endif
    }
    else if (!triggerState && lastTriggerState[i]) {
      // HIGH->LOW
#ifdef ACTIVE_LOW
      triggerOn(midiTrigger[i]);
#else
      triggerOff(midiTrigger[i]);
#endif
    }
    lastTriggerState[i] = triggerState;
  }
}

void setup() {
  MIDI.begin(MIDI_CHANNEL_OFF);
#ifdef TRIGGER_LED
  pinMode(TRIGGER_LED, OUTPUT);
  digitalWrite(TRIGGER_LED, LOW);
#endif
#ifdef TIMING_PIN
  pinMode(TIMING_PIN, OUTPUT);
  digitalWrite(TIMING_PIN, LOW);
#endif
  triggerSetup();
}

int timingToggle;
void loop () {
#ifdef TIMING_PIN
  timingToggle = !timingToggle;
  digitalWrite(TIMING_PIN, timingToggle);
#endif
  triggerLoop();
}
