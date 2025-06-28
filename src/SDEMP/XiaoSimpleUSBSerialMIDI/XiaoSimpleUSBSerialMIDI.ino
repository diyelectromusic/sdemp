/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Xiao Simple USB/Serial MIDI
//  https://diyelectromusic.com/2025/06/28/xiao-usb-device-to-serial-midi-converter/
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
/*
  Using principles from the following tutorials:
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Xiao Getting Started - https://wiki.seeedstudio.com/Seeeduino-XIAO/
*/
#include <Arduino.h>
#include <MIDI.h>
#include <USB-MIDI.h>

// Comment this out for serial to serial routing
// instead of serial to USB.
// USB is always routed back to serial.
#define SER_TO_USB

#define MIDI_LED LED_BUILTIN
#define LED_COUNT 200
int ledcount;

void ledInit() {
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
#endif
  ledcount = 0;
}

void ledOn() {
#ifdef MIDI_LED
  // Xiao on board LED is reversed
  digitalWrite(MIDI_LED, LOW);
#endif
  ledcount = LED_COUNT;
}

void ledOff() {
#ifdef MIDI_LED
  // Xiao on board LED is reversed
  digitalWrite(MIDI_LED, HIGH);
#endif
}

void ledScan () {
  if (ledcount > 0) {
    ledcount--;
  } else {
    ledOff();
    ledcount = 0;
  }
}

// Initialise USB for MIDI.
USBMIDI_CREATE_INSTANCE(0, UMIDI);

// Initialise default serial port for MIDI.
MIDI_CREATE_DEFAULT_INSTANCE();

void setup()
{
  // Automatic THRU is off for USB, on for Serial
  UMIDI.begin(MIDI_CHANNEL_OMNI);
  UMIDI.turnThruOff();
  MIDI.begin(MIDI_CHANNEL_OMNI);
#ifdef SER_TO_USB
  MIDI.turnThruOff();
#else
  MIDI.turnThruOn();
#endif
}

void midiThru(int out, midi::MidiType cmd, midi::DataByte d1, midi::DataByte d2, midi::Channel ch) {
  // Ignore "Active Sensing" and SysEx messages
  if ((cmd != 0xFE) && (cmd != 0xF0)) {
    ledOn();
    if (out == 1) {
      MIDI.send(cmd, d1, d2, ch);
    } else {
      UMIDI.send(cmd, d1, d2, ch);      
    }
  }
}

void loop()
{
  ledScan();

  if (UMIDI.read()) {
    midiThru(1, UMIDI.getType(),
                UMIDI.getData1(),
                UMIDI.getData2(),
                UMIDI.getChannel());
  }

  if (MIDI.read()) {
#ifdef SER_TO_USB
    midiThru(0, MIDI.getType(),
                MIDI.getData1(),
                MIDI.getData2(),
                MIDI.getChannel());
#else
    // Automatically routes serial to serial
    // when THRU is enabled.
#endif
  }
}
