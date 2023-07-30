/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Simple MIDI Serial Monitor
//  https://diyelectromusic.wordpress.com/2022/04/06/simple-midi-serial-monitor/
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
    Arduino Software Serial Library - https://www.arduino.cc/en/Reference/softwareSerial
    USB Host 2.0 Library - https://github.com/felis/USB_Host_Shield_2.0
    Arduino USB Transport - https://github.com/lathoub/Arduino-USBMIDI
    Arduino UHS2MIDI Transport - https://github.com/YuuichiAkagawa/Arduino-UHS2MIDI
*/
#include <MIDI.h>

// Uncomment which of these should be used for the MIDI INPUT.
//
// Different boards will support different interfaces, for example:
//    Uno/Nano: SW_SERIAL, SW_SERIAL2, USB_HOST (with additional shield).
//    Leonardo: USB_DEVICE, HW_SERIAL2, SW_SERIAL, SW_SERIAL2, USB_HOST (with additional shield).
//    Mega: HW_SERIAL2, HW_SERIAL3, HW_SERIAL4, SW_SERIAL, SW_SERIAL2, USB_HOST (with additional shield).
//
// Note: In all cases Serial - the first hardware serial port
//       is used for the data output to the serial port.
//
#define MIDI_HW_SERIAL2  1
//#define MIDI_HW_SERIAL3  2
//#define MIDI_HW_SERIAL4  3
//#define MIDI_SW_SERIAL   4
//#define MIDI_SW_SERIAL2  5
//#define MIDI_USB_HOST    6
//#define MIDI_USB_DEVICE  7

// ---- Definitions for MIDI INPUT devices ----
//
#ifdef MIDI_HW_SERIAL2
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_HS2);
#endif

#ifdef MIDI_HW_SERIAL3
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI_HS3);
#endif

#ifdef MIDI_HW_SERIAL4
MIDI_CREATE_INSTANCE(HardwareSerial, Serial3, MIDI_HS4);
#endif

#ifdef MIDI_SW_SERIAL
#include <SoftwareSerial.h>
// From https://www.arduino.cc/en/Reference/softwareSerial
// This default configurationis not supported on ATmega32U4
// or ATmega2560 based boards, see MIDI_SW_SERIAL2.
#define SS_RX  2
#define SS_TX  3
using Transport = MIDI_NAMESPACE::SerialMIDI<SoftwareSerial>;
SoftwareSerial sSerial = SoftwareSerial(SS_RX, SS_TX);
Transport serialMIDI(sSerial);
MIDI_NAMESPACE::MidiInterface<Transport> MIDI_SS((Transport&)serialMIDI);
#endif

#ifdef MIDI_SW_SERIAL2
#include <SoftwareSerial.h>
// From https://www.arduino.cc/en/Reference/softwareSerial
// there are limitations on which pins can be used with a
// Mega 2560 or Leonardo/Micro (ATmega32U4), so be sure to
// check.  There are no pin limitations for the Uno.
// These are fine for the ATmega32U4 based boards.
#define SS2_RX  10
#define SS2_TX  11
using Transport = MIDI_NAMESPACE::SerialMIDI<SoftwareSerial>;
SoftwareSerial sSerial2 = SoftwareSerial(SS2_RX, SS2_TX);
Transport serialMIDI2(sSerial2);
MIDI_NAMESPACE::MidiInterface<Transport> MIDI_SS2((Transport&)serialMIDI2);
#endif

#ifdef MIDI_USB_HOST
#include <UHS2-MIDI.h>
USB Usb;
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, MIDI_UH);
#endif

#ifdef MIDI_USB_DEVICE
#include <USB-MIDI.h>
USBMIDI_CREATE_INSTANCE(0, MIDI_UD);
#endif


#define MIDI_LED LED_BUILTIN

void ledOn() {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, HIGH);
#endif
}

void ledOff() {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, LOW);
#endif
}

void ledInit() {
#ifdef MIDI_LED
  pinMode (MIDI_LED, OUTPUT);
#endif
  ledOff();
}

void setup() {
  // Using the standard serial link for the "monitor"
  Serial.begin(9600);
  Serial.print("MIDI Monitor Starting on ");
  ledInit();

  // Initialise all INPUT devices to listen on all channels.
  // Disable the THRU for all devices.
#ifdef MIDI_HW_SERIAL2
  Serial.print("HW-SERIAL2 ");
  MIDI_HS2.begin(MIDI_CHANNEL_OMNI);
  MIDI_HS2.turnThruOff();
  MIDI_HS2.setHandleSystemExclusive(printMidiSysEx);
#endif
#ifdef MIDI_HW_SERIAL3
  Serial.print("HW-SERIAL3 ");
  MIDI_HS3.begin(MIDI_CHANNEL_OMNI);
  MIDI_HS3.turnThruOff();
  MIDI_HS3.setHandleSystemExclusive(printMidiSysEx);
#endif
#ifdef MIDI_HW_SERIAL4
  Serial.print("HW-SERIAL4 ");
  MIDI_HS4.begin(MIDI_CHANNEL_OMNI);
  MIDI_HS4.turnThruOff();
  MIDI_HS4.setHandleSystemExclusive(printMidiSysEx);
#endif
#ifdef MIDI_SW_SERIAL
  Serial.print("SW-SERIAL ");
  MIDI_SS.begin(MIDI_CHANNEL_OMNI);
  MIDI_SS.turnThruOff();
  MIDI_SS.setHandleSystemExclusive(printMidiSysEx);
#endif
#ifdef MIDI_SW_SERIAL2
  Serial.print("SW-SERIAL2 ");
  MIDI_SS2.begin(MIDI_CHANNEL_OMNI);
  MIDI_SS2.turnThruOff();
  MIDI_SS2.setHandleSystemExclusive(printMidiSysEx);
#endif
#ifdef MIDI_USB_HOST
  Serial.print("USB-Host ");
  MIDI_UH.begin(MIDI_CHANNEL_OMNI);
  MIDI_UH.turnThruOff();
  MIDI_UH.setHandleSystemExclusive(printMidiSysEx);
  Usb.Init();  
#endif
#ifdef MIDI_USB_DEVICE
  Serial.print("USB-Device ");
  MIDI_UD.begin(MIDI_CHANNEL_OMNI);
  MIDI_UD.turnThruOff();
  MIDI_UD.setHandleSystemExclusive(printMidiSysEx);
#endif
  Serial.print("\n");
}

void loop() {
#ifdef MIDI_USB_HOST
  Usb.Task();
#endif

#ifdef MIDI_HW_SERIAL2
  if (MIDI_HS2.read()) {
    ledOn();
    midi::MidiType mt  = MIDI_HS2.getType();
    if (mt != midi::SystemExclusive) {
      printMidiMsg(mt, MIDI_HS2.getData1(), MIDI_HS2.getData2(), MIDI_HS2.getChannel());
    }
  }
#endif
#ifdef MIDI_HW_SERIAL3
  if (MIDI_HS3.read()) {
    ledOn();
    midi::MidiType mt  = MIDI_HS3.getType();
    if (mt != midi::SystemExclusive) {
      printMidiMsg(mt, MIDI_HS3.getData1(), MIDI_HS3.getData2(), MIDI_HS3.getChannel());
    }
  }
#endif
#ifdef MIDI_HW_SERIAL4
  if (MIDI_HS4.read()) {
    ledOn();
    midi::MidiType mt  = MIDI_HS4.getType();
    if (mt != midi::SystemExclusive) {
      printMidiMsg(mt, MIDI_HS4.getData1(), MIDI_HS4.getData2(), MIDI_HS4.getChannel());
    }
  }
#endif
#ifdef MIDI_SW_SERIAL
  if (MIDI_SS.read()) {
    ledOn();
    midi::MidiType mt  = MIDI_SS.getType();
    if (mt != midi::SystemExclusive) {
      printMidiMsg(mt, MIDI_SS.getData1(), MIDI_SS.getData2(), MIDI_SS.getChannel());
    }
  }
#endif
#ifdef MIDI_SW_SERIAL2
  if (MIDI_SS2.read()) {
    ledOn();
    midi::MidiType mt  = MIDI_SS2.getType();
    if (mt != midi::SystemExclusive) {
      printMidiMsg(mt, MIDI_SS2.getData1(), MIDI_SS2.getData2(), MIDI_SS2.getChannel());
    }
  }
#endif
#ifdef MIDI_USB_HOST
  if (MIDI_UH.read()) {
    ledOn();
    midi::MidiType mt  = MIDI_UH.getType();
    if (mt != midi::SystemExclusive) {
      printMidiMsg(mt, MIDI_UH.getData1(), MIDI_UH.getData2(), MIDI_UH.getChannel());
    }
  }
#endif
#ifdef MIDI_USB_DEVICE
  if (MIDI_UD.read()) {
    ledOn();
    midi::MidiType mt  = MIDI_UD.getType();
    if (mt != midi::SystemExclusive) {
      printMidiMsg(mt, MIDI_UD.getData1(), MIDI_UD.getData2(), MIDI_UD.getChannel());
    }
  }
#endif
  ledOff();
}

void printMidiMsg (uint8_t cmd, uint8_t d1, uint8_t d2, uint8_t ch) {
  switch (cmd) {
    case midi::ActiveSensing:
      // Ignore
      return;
      break;
  }
  if (ch<10) Serial.print(" ");
  Serial.print(ch);
  Serial.print(" 0x");
  Serial.print(cmd, HEX);
  Serial.print(" 0x");
  Serial.print(d1, HEX);
  Serial.print(" 0x");
  Serial.print(d2, HEX);
  Serial.print("\t");
  switch(cmd) {
    case midi::NoteOff:
      Serial.print("NoteOff\t");
      Serial.print(d1);
      Serial.print("\t");
      Serial.print(d2);
      break;
    case midi::NoteOn:
      Serial.print("NoteOn\t");
      Serial.print(d1);
      Serial.print("\t");
      Serial.print(d2);
      break;
    case midi::AfterTouchPoly:
      Serial.print("AfterTouchPoly\t");
      Serial.print(d1);
      Serial.print("\t");
      Serial.print(d2);
      break;
    case midi::ControlChange:
      Serial.print("ControlChange\t");
      Serial.print(d1);
      Serial.print("\t");
      Serial.print(d2);
      break;
    case midi::ProgramChange:
      Serial.print("ProgramChange\t");
      Serial.print(d1);
      break;
    case midi::AfterTouchChannel:
      Serial.print("AfterTouchChannel\t");
      Serial.print(d1);
      break;
    case midi::PitchBend:
      Serial.print("PitchBend\t");
      Serial.print(d1);
      Serial.print("\t");
      Serial.print(d2);
      break;
  }
  Serial.print("\n");
}

void printMidiSysEx (byte *inArray, unsigned inSize) {
  Serial.print("SysEx Array size = ");
  Serial.println(inSize);
  int idx=0;
  for (int i=0; i<inSize; i++) {
   if (inArray[i] < 16) {
     Serial.print("0");
   }
   Serial.print(inArray[i], HEX);
   idx++;
   if (idx >= 16) {
     Serial.print("\n");
     idx = 0;
   }
  }
  if (idx != 0) {
    Serial.print("\n");
  }
}
