/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Serial MIDI Program and Control Messenger
//  https://diyelectromusic.wordpress.com/2022/04/13/arduino-serial-midi-program-and-control-messenger/
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
*/
#include <MIDI.h>

// To choose which MIDI transport to use, select one of these
// and comment out the others.
// 1. Built-in hardware serial port e.g. UART 0/Serial on an Uno/Nano
//    but can also mean UART 1/Serial1 for USB boards e.g. Leonardo.
// 2. Explicit second hardware serial port (UART 1) e.g. Serial1 on a Leaonardo/Pro Micro/2560
// 3. Software serial port on the defined IO pins.  Some boards have restrictions
//    on which pins to use, so check: https://www.arduino.cc/en/Reference/softwareSerial
//
MIDI_CREATE_DEFAULT_INSTANCE();
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
//#define MIDI_SOFT_SERIAL

// The following define the MIDI parameters to use.
//
#define MIDI_CHANNEL 1  // MIDI Channel number (1 to 16, or MIDI_CHANNEL_OMNI)
#define VOICE_NUMBER 32 // Patch to use (0 to 127)
#define CONTROLLER   1  // Modulation Wheel
//#define MIDI_RECV    1  // Comment out to be the sender

// By default the serial link will automatically
// forward traffic from its IN to its OUT unless
// you tell it not to.
//
// This is the same for all hardware or software
// serial links.  If we don't want that to happen
// then we can turn THRU off here by uncommenting this line.
//
//#define DISABLE_THRU 1

// Comment out to remove the LED indicator or change if you want your own LED
#define MIDI_LED     LED_BUILTIN

// ---------------------------------------------------------------------------------
//
// This is the magic that makes the "software serial" work.
// Define the two pins to use for RX/TX below, but see the following
// for limitations depending on your board:
// https://www.arduino.cc/en/Reference/softwareSerial
//
#define SS_RX  2
#define SS_TX  3
#ifdef MIDI_SOFT_SERIAL
#include <SoftwareSerial.h>
using Transport = MIDI_NAMESPACE::SerialMIDI<SoftwareSerial>;
SoftwareSerial sSerial = SoftwareSerial(SS_RX, SS_TX);
Transport serialMIDI(sSerial);
MIDI_NAMESPACE::MidiInterface<Transport> MIDI((Transport&)serialMIDI);
#endif
// ---------------------------------------------------------------------------------


void handleProgramChange(byte channel, byte program) { 
  // Code to handle the Program Change message goes here
  //
  // We shouldn't need to check the channel here as we
  // told the MIDI library which channel to listen to already.
  //
  if (program == VOICE_NUMBER) {
    ledFlash();
  }
}

void handleControlChange(byte channel, byte control, byte value) { 
  // Code to handle the Control Change message goes here
  //
  // We shouldn't need to check the channel here as we
  // told the MIDI library which channel to listen to already.
  //
  if (control == CONTROLLER) {
    if (value > 63) {
      ledOn();
    } else {
      ledOff();
    }
  }
}

// --------------------------------
//  Optional code to flash LEDs
//
void ledInit() {
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
  digitalWrite(MIDI_LED, LOW);
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
void ledFlash() {
  ledOn();
  delay(300);
  ledOff();
}
// --------------------------------


void setup() {
  ledInit();

  // Listen on MIDI_CHANNEL.
  // This can be set to MIDI_CHANNEL_OMNI to listen on all channels,
  // then further filtering can be done in the callback routines.
  //
  MIDI.begin(MIDI_CHANNEL);

#ifdef DISABLE_THRU
  MIDI.turnThruOff();
#endif

#ifdef MIDI_RECV
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleProgramChange(handleProgramChange);
#endif
}

int ctrlvalue;
void loop() {
  MIDI.read();

#ifdef MIDI_RECV
  // MIDI Receiver
  //
  // If we are the receiver there is nothing else to do as any
  // messages will be handled in the callback routines above.
  //
#else
  // MIDI Sender
  //
  // Add your logic below as required and then call
  //  MIDI.sendProgramChange (voice, channel)
  //  MIDI.sendControlChange (controller, value, channel)
  //
  MIDI.sendProgramChange(VOICE_NUMBER, MIDI_CHANNEL);
  delay(5000);

  MIDI.sendControlChange(CONTROLLER, ctrlvalue, MIDI_CHANNEL);
  ctrlvalue++;
  if (ctrlvalue > 127) ctrlvalue = 0;
  delay(5000);

  // End of MIDI Sender Code
#endif
}
