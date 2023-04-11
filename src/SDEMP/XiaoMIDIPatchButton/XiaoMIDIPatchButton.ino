/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Xiao MIDI Patch Button
//  https://diyelectromusic.wordpress.com/2023/04/11/xiao-samd21-arduino-and-midi-part-6/
//
      MIT License
      
      Copyright (c) 2023 diyelectromusic (Kevin)
      
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
    Button               - https://www.arduino.cc/en/Tutorial/Button
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Xiao Getting Started - https://wiki.seeedstudio.com/Seeeduino-XIAO/
*/
#include <MIDI.h>
#include <USB-MIDI.h>

// This is required to set up the MIDI library on the default serial port.
MIDI_CREATE_DEFAULT_INSTANCE();

// This initialised USB MIDI.
USBMIDI_CREATE_INSTANCE(0, UMIDI);

// Define the patch number to use. Patch numbers go from 1 to 128.
// For example, 59 is Tuba in the General MIDI Instrument Set.
#define MIDI_PATCH   59
#define MIDI_CHANNEL 1

#define BTN_PIN 1 // User button on the XIAO expansion board
int lastbtn;

#define MIDI_LED LED_BUILTIN

void setup() {
  pinMode(MIDI_LED, OUTPUT);
  digitalWrite(MIDI_LED, HIGH); // XIAO LED is reverse logic
  MIDI.begin(MIDI_CHANNEL_OFF);
  UMIDI.begin(MIDI_CHANNEL_OFF);

  // Initialise the button pin to input (active LOW)
  pinMode (BTN_PIN, INPUT_PULLUP);
  lastbtn = HIGH;
}

void loop() {
  int btn = digitalRead (BTN_PIN);

  // Send patch change on HIGH to LOW transition only
  if ((btn == LOW) && (lastbtn == HIGH))
  {
    digitalWrite(MIDI_LED, LOW);
    // Send our MIDI message to change the patch number.
    // NB: Must convert the 1-128 number to a 0-127 number for sending.

    // Send over MIDI serial
    MIDI.sendProgramChange (MIDI_PATCH-1, MIDI_CHANNEL);

    // Send over MIDI USB
    UMIDI.sendProgramChange (MIDI_PATCH-1, MIDI_CHANNEL);

    // Include a short delay so the switch isn't continuously read and triggered
    delay (300);
    digitalWrite(MIDI_LED, HIGH);
  }
  lastbtn = btn;
}
