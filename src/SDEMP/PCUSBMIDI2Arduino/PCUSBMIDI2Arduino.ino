/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  PC USB MIDI to Arduino Serial MIDI
//  https://diyelectromusic.wordpress.com/2020/11/05/pc-usb-to-arduino-serial-midi/
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
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    USB-MIDI Transport   - https://github.com/lathoub/Arduino-USBMIDI
*/
#include <MIDI.h>
#include <USB-MIDI.h>

// This is "magic" to get the USB MIDI side up and running
USBMIDI_CREATE_INSTANCE(0, MIDI_USB);

// And this is the "magic" for the serial side
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_S);

#define MIDI_LED LED_BUILTIN

void setup() {
  pinMode(MIDI_LED, OUTPUT);
  
  MIDI_USB.begin(MIDI_CHANNEL_OMNI);
  MIDI_S.begin(MIDI_CHANNEL_OMNI);
  MIDI_S.turnThruOff();
}

void loop()
{
  if (MIDI_USB.read()) {
    digitalWrite(MIDI_LED, HIGH);
    MIDI_S.send(MIDI_USB.getType(), MIDI_USB.getData1(), MIDI_USB.getData2(), MIDI_USB.getChannel());
    digitalWrite(MIDI_LED, LOW);
  }
}
