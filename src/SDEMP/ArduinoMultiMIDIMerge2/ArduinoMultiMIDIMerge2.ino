/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Multi MIDI Merge - Part 2
//  https://diyelectromusic.wordpress.com/2022/05/29/arduino-multi-midi-merge-part-2/
//
      MIT License
      
      Copyright (c) 2022 diyelectromusic (Kevin)
      
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

//#define DEBUG  // Only use if HW_SERIAL is free...

// Set which device is the MIDI OUT device
#define MIDI_OUT  MIDI_HW_SERIAL

// Optional: Set pins for LED indicators for each of the above
//           Set to NOPIN to disable that port/pin
#define NOPIN (-1)
int ledPins[MIDI_LEDS] = { NOPIN, NOPIN, NOPIN, NOPIN, NOPIN, NOPIN, NOPIN, NOPIN };
//int ledPins[MIDI_LEDS] = { 2, 3, 4, 5, 6, 7, 8, 9 };
#define LED_COUNT 200
int ledCnt[MIDI_LEDS];

#if defined(__AVR_ATmega2560__)
// The following can be used if working memory isn't a problem,
// for example if using an ATmega2560 based board which has 8K.
//
#define LARGER_MEMORY
// Array to count the NoteOn/Off messages
#define NUM_CHS   16
#define NUM_NOTES 128
uint8_t notes[NUM_CHS][NUM_NOTES];
#else
// The best we can do in constrained environments, i.e. those MPUs
// with limited working memory, like the ATmega328 (2K) or ATmega32U4 (2.5K),
// is limit the number of polyphonic notes we can count.
//
// In theory, with an ATmega32U4 based board you might be able
// to squeeze in another one or even two, but that wouldn't leave
// much working space for things like the USB stack if you wanted
// to use it.
//
// The approach is to use a single bit per MIDI channel - hence using
// a uint16_t array of notes - to indicate if a NoteOn has already been
// received.
//
// The simplest case is to have two arrays and therefore cope with
// one or two notes being played.
//
// However it is possible to be slightly smarter and treat these two
// bits as a two-bit binary value in their own right.  This means that
// it is actually possible to allow for the following circumstances:
//    note2   note1
//      0       0    = No notes being played
//      0       1    = One note already being played
//      1       0    = Two notes already being played
//      1       1    = Three notes already being played
//
// The logic to support these four cases is hardcoded in the
// handleNoteOnOff() function to move to the correct state
// when a NoteOn or NoteOff message is received.
//
// Note that NoteOn messages are always passed through.
// NoteOff messages are only passed through if there are
// no notes recorded as being played.  This means that notes
// will keep sounding until the last NoteOff has been received.

//
#define NUM_NOTES 128
uint16_t note1[NUM_NOTES];
uint16_t note2[NUM_NOTES];
#endif

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
#ifdef DEBUG
  Serial.begin(9600);
#endif
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
    if (mt == midi::NoteOn || mt == midi::NoteOff) {
      uint8_t d1 = MIDI_HS.getData1();
      uint8_t d2 = MIDI_HS.getData2();
      uint8_t ch = MIDI_HS.getChannel();
      if (handleNoteOnOff(mt,d1, d2, ch)) {
        MIDI_O.send(mt, d1, d2, ch);
      }
    } else if (mt != midi::SystemExclusive) {
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
    if (mt == midi::NoteOn || mt == midi::NoteOff) {
      uint8_t d1 = MIDI_HS2.getData1();
      uint8_t d2 = MIDI_HS2.getData2();
      uint8_t ch = MIDI_HS2.getChannel();
      if (handleNoteOnOff(mt,d1, d2, ch)) {
        MIDI_O.send(mt, d1, d2, ch);
      }
    } else if (mt != midi::SystemExclusive) {
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
    if (mt == midi::NoteOn || mt == midi::NoteOff) {
      uint8_t d1 = MIDI_HS3.getData1();
      uint8_t d2 = MIDI_HS3.getData2();
      uint8_t ch = MIDI_HS3.getChannel();
      if (handleNoteOnOff(mt,d1, d2, ch)) {
        MIDI_O.send(mt, d1, d2, ch);
      }
    } else if (mt != midi::SystemExclusive) {
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
    if (mt == midi::NoteOn || mt == midi::NoteOff) {
      uint8_t d1 = MIDI_HS4.getData1();
      uint8_t d2 = MIDI_HS4.getData2();
      uint8_t ch = MIDI_HS4.getChannel();
      if (handleNoteOnOff(mt, d1, d2, ch)) {
        MIDI_O.send(mt, d1, d2, ch);
      }
    } else if (mt != midi::SystemExclusive) {
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
    if (mt == midi::NoteOn || mt == midi::NoteOff) {
      uint8_t d1 = MIDI_SS.getData1();
      uint8_t d2 = MIDI_SS.getData2();
      uint8_t ch = MIDI_SS.getChannel();
      if (handleNoteOnOff(mt, d1, d2, ch)) {
        MIDI_O.send(mt, d1, d2, ch);
      }
    } else if (mt != midi::SystemExclusive) {
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
    if (mt == midi::NoteOn || mt == midi::NoteOff) {
      uint8_t d1 = MIDI_SS2.getData1();
      uint8_t d2 = MIDI_SS2.getData2();
      uint8_t ch = MIDI_SS2.getChannel();
      if (handleNoteOnOff(mt, d1, d2, ch)) {
        MIDI_O.send(mt, d1, d2, ch);
      }
    } else if (mt != midi::SystemExclusive) {
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
    if (mt == midi::NoteOn || mt == midi::NoteOff) {
      uint8_t d1 = MIDI_UH.getData1();
      uint8_t d2 = MIDI_UH.getData2();
      uint8_t ch = MIDI_UH.getChannel();
      if (handleNoteOnOff(mt, d1, d2, ch)) {
        MIDI_O.send(mt, d1, d2, ch);
      }
    } else if (mt != midi::SystemExclusive) {
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
    if (mt == midi::NoteOn || mt == midi::NoteOff) {
      uint8_t d1 = MIDI_UD.getData1();
      uint8_t d2 = MIDI_UD.getData2();
      uint8_t ch = MIDI_UD.getChannel();
      if (handleNoteOnOff(mt, d1, d2, ch)) {
        MIDI_O.send(mt, d1, d2, ch);
      }
    } else if (mt != midi::SystemExclusive) {
      MIDI_O.send(mt, MIDI_UD.getData1(), MIDI_UD.getData2(), MIDI_UD.getChannel());
    } else {
      int mSxLen = MIDI_UD.getData1() + 256*MIDI_UD.getData2();
      MIDI_O.sendSysEx(mSxLen, MIDI_UD.getSysExArray(), true);
    }
  }
#endif
}

#ifdef LARGER_MEMORY
bool handleNoteOnOff (midi::MidiType cmd, uint8_t note, uint8_t vel, uint8_t channel) {
  channel = channel - 1; // Convert 1-16 to 0-15
  if (cmd == midi::NoteOn) {
    notes[channel][note]++;
#ifdef DEBUG
    Serial.print("ON:  ");
    Serial.print(channel);
    Serial.print("\t");
    Serial.print(note);
    Serial.print("\t->\t");
    Serial.print(notes[channel][note]);
    Serial.print("\n");
#endif
    return true;
  } else if (cmd == midi::NoteOff) {
    if (notes[channel][note] > 0) {
      notes[channel][note]--;
#ifdef DEBUG
      Serial.print("OFF: ");
      Serial.print(channel);
      Serial.print("\t");
      Serial.print(note);
      Serial.print("\t->\t");
      Serial.print(notes[channel][note]);
      Serial.print("\n");
#endif
      if (notes[channel][note] == 0) {
        return true;
      } else {
        // Don't send a NoteOff until all notes have finished playing...
        return false;
      }
    }
  }
}
#else
bool handleNoteOnOff (midi::MidiType cmd, uint8_t note, uint8_t vel, uint8_t channel) {
  uint16_t chmask = (1<<(channel-1));  // Convert channel 1-16 to 0-15
  uint16_t n1 = (note1[note] & chmask);
  uint16_t n2 = (note2[note] & chmask);

  // By treating these two entries as a two-bit binary digit we can actually
  // "count" up to four notes for each channel...
  //
  // But rather than attempting to use loops/etc to calculate the values
  // it is all hard-coded here based on the values of n1 and n2.
  
  if (cmd == midi::NoteOn) {
    if (!n2 && !n1) {
      // Binary 00
      // No notes have been logged yet, so log the first NoteOn
      note1[note] |= chmask;
    } else if (!n2 && n1) {
      // Binary 01
      // Have already logged a NoteOn for the first note, so log a 2nd
      // and move onto binary 10
      note1[note] &= ~chmask;
      note2[note] |= chmask;
    } else if (n2 && !n1) {
      // Binary 10
      // We have two notes logged, so log our third
      note1[note] |= chmask;
    } else {
      // Binary 11
      // We have three notes logged already, so nothing more to do
    }
    // Regardless of what we already know, we always pass on a new
    // NoteOn message in case something needs to retrigger.
    return true;
  } else if (cmd == midi::NoteOff) {
    if (!n2 && !n1) {
      // Binary 00
      // There are no notes already playing, so pass on the NoteOff anyway
      // in case something further down the line
      return true;
    } else if (!n2 && n1) {
      // Binary 01
      // There is only one note playing, so we can now cancel it and send the NoteOff.
      note1[note] &= ~chmask;
      return true;
    } else if (n2 && !n1) {
      // Binary 10
      // There are two notes playing so remove one, dropping back to binary 01,
      // but don't send the NoteOff yet.
      note1[note] |= chmask;
      note2[note] &= ~chmask;
      return false;
    } else {
      // Binary 11
      // There are three notes playing, so drop back to Binary 10,
      // but don't send the NoteOff yet.
      note1[note] &= ~chmask;
      return false;
    }
  }
}
#endif
