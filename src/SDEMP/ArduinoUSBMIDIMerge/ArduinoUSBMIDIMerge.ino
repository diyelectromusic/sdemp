/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino USB MIDI Merge
//  https://diyelectromusic.wordpress.com/2021/06/11/arduino-usb-midi-merge/
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
  Using principles from the following tutorials:
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    USB Host 2.0 Library - https://github.com/felis/USB_Host_Shield_2.0
    Arduino UHS2MIDI Transport - https://github.com/YuuichiAkagawa/Arduino-UHS2MIDI

  This code is based on the following examples:
    Arduino MIDI Library: DualMerger
    UHS2MIDI Basic Example
*/
#include <MIDI.h>
#include <UHS2-MIDI.h>

USB Usb;
UHS2MIDI_CREATE_DEFAULT_INSTANCE(&Usb);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, midiB);

void setup()
{
  // Disable the automatic THRU for the USB port.
  // Don't want this sending out by default over USB too.
  //
  // Leave it on for the serial channel, so that anything
  // received over serial is automatically sent out over serial too.
  //
  midiB.begin(MIDI_CHANNEL_OMNI);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  Usb.Init();  
}

void loop()
{
  Usb.Task();

  // Allow the automatic processing of the serial port THRU.
  midiB.read();

  // If there is data from USB, echo it to the serial port too.
  if (MIDI.read())
  {
      // Thru on A has already pushed the input message to out A.
      // Forward the message to out B as well.
      midiB.send(MIDI.getType(),
                 MIDI.getData1(),
                 MIDI.getData2(),
                 MIDI.getChannel());
  }
}
