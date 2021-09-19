/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  PC USB MIDI to Arduino Serial MIDI - 2
//  https://diyelectromusic.wordpress.com/2021/09/19/usb-midi-to-midi-re-revisited/
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

// This is required in order to expose the MIDI library's idea
// of a MIDI message to the message handling routines.
USING_NAMESPACE_MIDI;
typedef Message<MIDI_NAMESPACE::DefaultSettings::SysExMaxSize> MidiMessage;

// This is "magic" to get the USB MIDI side up and running
USBMIDI_CREATE_INSTANCE(0, MIDI_USB);

// And this is the "magic" for the serial side
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_S);

// Comment this out if you don't want a MIDI LED indication
#define MIDI_LED LED_BUILTIN

void ledInit () {
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
#endif
}
void ledOn () {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, HIGH);
#endif
}

void ledOff() {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, LOW);
#endif
}

void setup() {
  ledInit();

  MIDI_USB.setHandleMessage(onUsbMessage);
  MIDI_S.setHandleMessage(onSerialMessage);

  MIDI_USB.begin(MIDI_CHANNEL_OMNI);
  MIDI_S.begin(MIDI_CHANNEL_OMNI);

  // Software THRU is enabled by default for the serial
  // transport, so turn it off here.
  MIDI_S.turnThruOff();
}

void loop()
{
  MIDI_USB.read();
  MIDI_S.read();
}

void onUsbMessage(const MidiMessage& message) {
  ledOn();
  
  // We should be able to use the following but it isn't
  // working for me, so I'm having to break out the messages
  // individually.
  //
//  MIDI_S.send(message);
  switch (message.type) {
    case NoteOn:
      MIDI_S.sendNoteOn(message.data1, message.data2, message.channel);
      break;
    case NoteOff:
      MIDI_S.sendNoteOff(message.data1, message.data2, message.channel);
      break;
    case ProgramChange:
      MIDI_S.sendProgramChange(message.data1, message.channel);
      break;
    case ControlChange:
      MIDI_S.sendControlChange(message.data1, message.data2, message.channel);
      break;
    case PitchBend:
      MIDI_S.sendPitchBend(message.data1, message.channel);
      break;
    case AfterTouchPoly:
      MIDI_S.sendAfterTouch(message.data1, message.data2, message.channel);
      break;
    case AfterTouchChannel:
      MIDI_S.sendAfterTouch(message.data1, message.channel);
      break;
  }

  ledOff();
}

void onSerialMessage(const MidiMessage& message) {
  ledOn();

  // We should be able to use the following but it isn't
  // working for me, so I'm having to break out the messages
  // individually.
  //
//  MIDI_USB.send(message);
  switch (message.type) {
    case NoteOn:
      MIDI_USB.sendNoteOn(message.data1, message.data2, message.channel);
      break;
    case NoteOff:
      MIDI_USB.sendNoteOff(message.data1, message.data2, message.channel);
      break;
    case ProgramChange:
      MIDI_USB.sendProgramChange(message.data1, message.channel);
      break;
    case ControlChange:
      MIDI_USB.sendControlChange(message.data1, message.data2, message.channel);
      break;
    case PitchBend:
      MIDI_USB.sendPitchBend(message.data1, message.channel);
      break;
    case AfterTouchPoly:
      MIDI_USB.sendAfterTouch(message.data1, message.data2, message.channel);
      break;
    case AfterTouchChannel:
      MIDI_USB.sendAfterTouch(message.data1, message.channel);
      break;
  }

  ledOff();
}
