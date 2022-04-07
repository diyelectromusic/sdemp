/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Simple MIDI Serial Monitor 2
//  https://diyelectromusic.wordpress.com/2022/04/07/simple-midi-serial-monitor-part-2/
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

#define MIDI_CHANNEL  1  // Set to MIDI_CHANNEL_OMNI to listen to all

// Uncomment which of these should be used for the MIDI INPUT.
//
// Different boards will support different interfaces, for example:
//    Uno/Nano: HW_SERIAL, SW_SERIAL, SW_SERIAL2, USB_HOST (with additional shield).
//    Leonardo: HW_SERIAL, USB_DEVICE, HW_SERIAL2, SW_SERIAL, SW_SERIAL2, USB_HOST (with additional shield).
//    Mega: HW_SERIAL, HW_SERIAL2, HW_SERIAL3, HW_SERIAL4, SW_SERIAL, SW_SERIAL2, USB_HOST (with additional shield).
//
#define MIDI_HW_SERIAL  1
//#define MIDI_HW_SERIAL2  2
//#define MIDI_HW_SERIAL3  3
//#define MIDI_HW_SERIAL4  4
//#define MIDI_SW_SERIAL   5
//#define MIDI_SW_SERIAL2  6
//#define MIDI_USB_HOST    7
//#define MIDI_USB_DEVICE  8

// If serial port output is required, decide if this is to be over
// the hardware serial port or a Software Serial link.
//
//#define DBG_HW_SERIAL   1
#define DBG_SW_SERIAL   2

// Uncomment this if you want the LED to flash
#define MIDI_LED LED_BUILTIN

// ---- Definitions for MIDI INPUT devices ----
//
#ifdef MIDI_HW_SERIAL
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI_HS);
#define MIDI_I MIDI_HS
#endif

#ifdef MIDI_HW_SERIAL2
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_HS2);
#define MIDI_I MIDI_HS2
#endif

#ifdef MIDI_HW_SERIAL3
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI_HS3);
#define MIDI_I MIDI_HS3
#endif

#ifdef MIDI_HW_SERIAL4
MIDI_CREATE_INSTANCE(HardwareSerial, Serial3, MIDI_HS4);
#define MIDI_I MIDI_HS4
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
#define MIDI_I MIDI_SS
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
#define MIDI_I MIDI_SS2
#endif

#ifdef MIDI_USB_HOST
#include <UHS2-MIDI.h>
USB Usb;
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, MIDI_UH);
#define MIDI_I MIDI_UH
#endif

#ifdef MIDI_USB_DEVICE
#include <USB-MIDI.h>
USBMIDI_CREATE_INSTANCE(0, MIDI_UD);
#define MIDI_I MIDI_UD
#endif

#ifdef DBG_HW_SERIAL
#define DBG_S Serial
#endif

#ifdef DBG_SW_SERIAL
#include <SoftwareSerial.h>
// From https://www.arduino.cc/en/Reference/softwareSerial
// This default configurationis not supported on ATmega32U4
// or ATmega2560 based boards, see MIDI_SW_SERIAL2.
#define SS_RX  2
#define SS_TX  3
SoftwareSerial sSerial = SoftwareSerial(SS_RX, SS_TX);
#define DBG_S sSerial
#endif

// ---------------------------------------------------------------
//
// Add the callbacks to be called when a specific MIDI
// message is received here.
//
// For details, see MIDI.h in the MIDI Library and for
// the format of the each individual function, see midi_Defs.h.
//
// ---------------------------------------------------------------

void handleNoteOn(byte channel, byte pitch, byte velocity) {
#ifdef DBG_S
  DBG_S.print("NoteOn: ");
  DBG_S.print(channel);
  DBG_S.print(" ");
  DBG_S.print(pitch);
  DBG_S.print(" ");
  DBG_S.print(velocity);
  DBG_S.print("\n");
#endif
  ledOn();
}

void handleNoteOff(byte channel, byte pitch, byte velocity) { 
#ifdef DBG_S
  DBG_S.print("NoteOff: ");
  DBG_S.print(channel);
  DBG_S.print(" ");
  DBG_S.print(pitch);
  DBG_S.print(" ");
  DBG_S.print(velocity);
  DBG_S.print("\n");
#endif
  ledOff();
}

void handleProgramChange(byte channel, byte program) { 
#ifdef DBG_S
  DBG_S.print("Program: ");
  DBG_S.print(channel);
  DBG_S.print(" ");
  DBG_S.print(program);
  DBG_S.print("\n");
#endif
}

void handleControlChange(byte channel, byte control, byte value) { 
#ifdef DBG_S
  DBG_S.print("Control: ");
  DBG_S.print(channel);
  DBG_S.print(" ");
  DBG_S.print(control);
  DBG_S.print(" ");
  DBG_S.print(value);
  DBG_S.print("\n");
#endif
}

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
  // If we aren't using HW serial 1 then we can use it for output!
#ifdef DBG_S
  DBG_S.begin(9600);
#endif
  ledInit();

  // Initialise the INPUT device to listen on the appropriate channel.
  // Disable the THRU for all devices.
  MIDI_I.begin(MIDI_CHANNEL);
  MIDI_I.turnThruOff();

  // These functons will be called on receipt of the appropriate MIDI message.
  // One is needed for each type of message to be processed.
  // For the full list, see MIDI.h.
  //
  MIDI_I.setHandleNoteOff(handleNoteOff);
  MIDI_I.setHandleNoteOn(handleNoteOn);
  MIDI_I.setHandleControlChange(handleControlChange);
  MIDI_I.setHandleProgramChange(handleProgramChange);
}

void loop() {
#ifdef MIDI_USB_HOST
  Usb.Task();
#endif
  MIDI_I.read();
}
