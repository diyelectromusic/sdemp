/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Multi MIDI Merge
//  https://diyelectromusic.wordpress.com/2021/12/05/arduino-multi-midi-merge/
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

// Uncomment which of these should be included as active MIDI IN
// devices to be end points for the MIDI merging.
//
// Different boards will support different interfaces, for example:
//   Uno/Nano: HW_SERIAL, SW_SERIAL, SW_SERIAL2, USB_HOST (with additional shield).
//   Leonardo: USB_DEVICE, HW_SERIAL2, SW_SERIAL, SW_SERIAL2, USB_HOST (with additional shield).
//   Mega:     HW_SERIAL, HW_SERIAL2, HW_SERIAL3, HW_SERIAL4, SW_SERIAL, SW_SERIAL2, USB_HOST (with additional sheild).
//
#define MIDI_HW_SERIAL   1
//#define MIDI_HW_SERIAL2  2
//#define MIDI_HW_SERIAL3  3
//#define MIDI_HW_SERIAL4  4
#define MIDI_SW_SERIAL   5
//#define MIDI_SW_SERIAL2  6
//#define MIDI_USB_HOST    7
//#define MIDI_USB_DEVICE  8
#define MIDI_LEDS        8  // Set to last possible MIDI device defined

// Set which device is the MIDI OUT device
#define MIDI_OUT  MIDI_HW_SERIAL

// Optional: Set pins for LED indicators for each of the above
//           Set to NOPIN to disable that port/pin
#define NOPIN (-1)
int ledPins[MIDI_LEDS] = { NOPIN, NOPIN, NOPIN, NOPIN, NOPIN, NOPIN, NOPIN, NOPIN };
//int ledPins[MIDI_LEDS] = { 2, 3, 4, 5, 6, 7, 8, 9 };
#define LED_COUNT 200
int ledCnt[MIDI_LEDS];


// ---- Definitions for MIDI INPUT devices ----
//
#ifdef MIDI_HW_SERIAL
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI_HS);
#endif

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


// ---- Definitions for MIDI OUTPUT device ----
//
#if (MIDI_OUT==MIDI_HW_SERIAL)
#define MIDI_O MIDI_HS
#elif (MIDI_OUT==MIDI_HW_SERIAL2)
#define MIDI_O MIDI_HS2
#elif (MIDI_OUT==MIDI_HW_SERIAL3)
#define MIDI_O MIDI_HS3
#elif (MIDI_OUT==MIDI_HW_SERIAL4)
#define MIDI_O MIDI_HS4
#elif (MIDI_OUT==MIDI_SW_SERIAL)
#define MIDI_O MIDI_SS
#elif (MIDI_OUT==MIDI_SW_SERIAL2)
#define MIDI_O MIDI_SS2
#elif (MIDI_OUT==MIDI_USB_HOST)
#define MIDI_O MIDI_UH
#elif (MIDI_OUT==MIDI_USB_DEVICE)
#define MIDI_O MIDI_UD
#else // No output!?
#endif

// NB: led goes from 1 to max so has to be changed
//     to go from 0 to max-1 to use as an index.
void ledOn (int led) {
  led = led-1;
  if (led < MIDI_LEDS) {
    if (ledPins[led] != NOPIN) {
      digitalWrite(ledPins[led], HIGH);
      ledCnt[led] = LED_COUNT;
    }
  }
}

void ledOff (int led) {
  led = led-1;
  if (led < MIDI_LEDS) {
    if (ledPins[led] != NOPIN) {
      digitalWrite(ledPins[led], LOW);
    }
  }
}

void ledInit () {
  for (int i=0; i<MIDI_LEDS; i++) {
    if (ledPins[i] != NOPIN) {
      pinMode(ledPins[i], OUTPUT);
      digitalWrite(ledPins[i], LOW);
      ledCnt[i] = 0;
    }
  }
}

void ledScan () {
  for (int i=0; i<MIDI_LEDS; i++) {
    if (ledCnt[i] > 0) {
      ledCnt[i]--;
    } else {
      ledOff(i+1);
      ledCnt[i] = 0;
    }
  }
}

void setup()
{
  ledInit();

  // Disable the THRU for all devices.  In principle we could keep
  // it on for the OUTPUT device to automatically pass data coming in
  // on that device straight out, but in reality it makes for simpler
  // code to disable all automatic THRU and handle it all here.
  //
  // Initialise all INPUT devices to listen on all channels.
  //
#ifdef MIDI_HW_SERIAL
  ledOn(MIDI_HW_SERIAL);
  MIDI_HS.begin(MIDI_CHANNEL_OMNI);
  MIDI_HS.turnThruOff();
#endif
#ifdef MIDI_HW_SERIAL2
  ledOn(MIDI_HW_SERIAL2);
  MIDI_HS2.begin(MIDI_CHANNEL_OMNI);
  MIDI_HS2.turnThruOff();
#endif
#ifdef MIDI_HW_SERIAL3
  ledOn(MIDI_HW_SERIAL3);
  MIDI_HS3.begin(MIDI_CHANNEL_OMNI);
  MIDI_HS3.turnThruOff();
#endif
#ifdef MIDI_HW_SERIAL4
  ledOn(MIDI_HW_SERIAL4);
  MIDI_HS4.begin(MIDI_CHANNEL_OMNI);
  MIDI_HS4.turnThruOff();
#endif
#ifdef MIDI_SW_SERIAL
  ledOn(MIDI_SW_SERIAL);
  MIDI_SS.begin(MIDI_CHANNEL_OMNI);
  MIDI_SS.turnThruOff();
#endif
#ifdef MIDI_SW_SERIAL2
  ledOn(MIDI_SW_SERIAL2);
  MIDI_SS2.begin(MIDI_CHANNEL_OMNI);
  MIDI_SS2.turnThruOff();
#endif
#ifdef MIDI_USB_HOST
  ledOn(MIDI_USB_HOST);
  MIDI_UH.begin(MIDI_CHANNEL_OMNI);
  MIDI_UH.turnThruOff();
  Usb.Init();  
#endif
#ifdef MIDI_USB_DEVICE
  ledOn(MIDI_USB_DEVICE);
  MIDI_UD.begin(MIDI_CHANNEL_OMNI);
  MIDI_UD.turnThruOff();
#endif
}

void loop()
{
#ifdef MIDI_USB_HOST
  Usb.Task();
#endif

  // See if any LEDs need turning off
  ledScan();

#ifdef MIDI_HW_SERIAL
  if (MIDI_HS.read()) {
    ledOn(MIDI_HW_SERIAL);
    midi::MidiType mt  = MIDI_HS.getType();
    if (mt != midi::SystemExclusive) {
      MIDI_O.send(mt, MIDI_HS.getData1(), MIDI_HS.getData2(), MIDI_HS.getChannel());
    } else {
      int mSxLen = MIDI_HS.getData1() + 256*MIDI_HS.getData2();
      MIDI_O.sendSysEx(mSxLen, MIDI_HS.getSysExArray(), true);
    }
  }
#endif
#ifdef MIDI_HW_SERIAL2
  if (MIDI_HS2.read()) {
    ledOn(MIDI_HW_SERIAL2);
    midi::MidiType mt  = MIDI_HS2.getType();
    if (mt != midi::SystemExclusive) {
      MIDI_O.send(mt, MIDI_HS2.getData1(), MIDI_HS2.getData2(), MIDI_HS2.getChannel());
    } else {
      int mSxLen = MIDI_HS2.getData1() + 256*MIDI_HS2.getData2();
      MIDI_O.sendSysEx(mSxLen, MIDI_HS2.getSysExArray(), true);
    }
  }
#endif
#ifdef MIDI_HW_SERIAL3
  if (MIDI_HS3.read()) {
    ledOn(MIDI_HW_SERIAL3);
    midi::MidiType mt  = MIDI_HS3.getType();
    if (mt != midi::SystemExclusive) {
      MIDI_O.send(mt, MIDI_HS3.getData1(), MIDI_HS3.getData2(), MIDI_HS3.getChannel());
    } else {
      int mSxLen = MIDI_HS3.getData1() + 256*MIDI_HS3.getData2();
      MIDI_O.sendSysEx(mSxLen, MIDI_HS3.getSysExArray(), true);
    }
  }
#endif
#ifdef MIDI_HW_SERIAL4
  if (MIDI_HS4.read()) {
    ledOn(MIDI_HW_SERIAL4);
    midi::MidiType mt  = MIDI_HS4.getType();
    if (mt != midi::SystemExclusive) {
      MIDI_O.send(mt, MIDI_HS4.getData1(), MIDI_HS4.getData2(), MIDI_HS4.getChannel());
    } else {
      int mSxLen = MIDI_HS4.getData1() + 256*MIDI_HS4.getData2();
      MIDI_O.sendSysEx(mSxLen, MIDI_HS4.getSysExArray(), true);
    }
  }
#endif
#ifdef MIDI_SW_SERIAL
  if (MIDI_SS.read()) {
    ledOn(MIDI_SW_SERIAL);
    midi::MidiType mt  = MIDI_SS.getType();
    if (mt != midi::SystemExclusive) {
      MIDI_O.send(mt, MIDI_SS.getData1(), MIDI_SS.getData2(), MIDI_SS.getChannel());
    } else {
      int mSxLen = MIDI_SS.getData1() + 256*MIDI_SS.getData2();
      MIDI_O.sendSysEx(mSxLen, MIDI_SS.getSysExArray(), true);
    }
  }
#endif
#ifdef MIDI_SW_SERIAL2
  if (MIDI_SS2.read()) {
    ledOn(MIDI_SW_SERIAL2);
    midi::MidiType mt  = MIDI_SS2.getType();
    if (mt != midi::SystemExclusive) {
      MIDI_O.send(mt, MIDI_SS2.getData1(), MIDI_SS2.getData2(), MIDI_SS2.getChannel());
    } else {
      int mSxLen = MIDI_SS2.getData1() + 256*MIDI_SS2.getData2();
      MIDI_O.sendSysEx(mSxLen, MIDI_SS2.getSysExArray(), true);
    }
  }
#endif
#ifdef MIDI_USB_HOST
  if (MIDI_UH.read()) {
    ledOn(MIDI_USB_HOST);
    midi::MidiType mt  = MIDI_UH.getType();
    if (mt != midi::SystemExclusive) {
      MIDI_O.send(mt, MIDI_UH.getData1(), MIDI_UH.getData2(), MIDI_UH.getChannel());
    } else {
      int mSxLen = MIDI_UH.getData1() + 256*MIDI_UH.getData2();
      MIDI_O.sendSysEx(mSxLen, MIDI_UH.getSysExArray(), true);
    }
  }
#endif
#ifdef MIDI_USB_DEVICE
  if (MIDI_UD.read()) {
    ledOn(MIDI_USB_DEVICE);
    midi::MidiType mt  = MIDI_UD.getType();
    if (mt != midi::SystemExclusive) {
      MIDI_O.send(mt, MIDI_UD.getData1(), MIDI_UD.getData2(), MIDI_UD.getChannel());
    } else {
      int mSxLen = MIDI_UD.getData1() + 256*MIDI_UD.getData2();
      MIDI_O.sendSysEx(mSxLen, MIDI_UD.getSysExArray(), true);
    }
  }
#endif
}
