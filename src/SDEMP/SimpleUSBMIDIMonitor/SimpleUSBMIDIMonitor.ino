/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Simple USB MIDI Monitor
//  https://diyelectromusic.com/2025/05/23/arduino-pro-mini-usb-host-proto-pcb-build-guide/
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
  Using principles from the following Arduino tutorials:
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    USB Host 2.0 Library - https://github.com/felis/USB_Host_Shield_2.0
    Arduino USB Transport - https://github.com/lathoub/Arduino-USBMIDI

*/
#include <MIDI.h>
#include <UHS2-MIDI.h>

USB Usb;
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, MIDI);

#define MIDI_LED LED_BUILTIN

void setup() {
  // This listens to all MIDI channels
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Initialise the USB stack
  Usb.Init();

  // Use the built-in LED as a MIDI note indicator
  pinMode (MIDI_LED, OUTPUT);
}

void loop() {
  Usb.Task();

  if (MIDI.read()) {
    switch(MIDI.getType()) {
      case midi::NoteOn:
        digitalWrite (MIDI_LED, HIGH);
        delay (100);
        digitalWrite (MIDI_LED, LOW);
        break;

      default:
        // Otherwise it is a message we aren't interested in
        break;
    }
  }
}
