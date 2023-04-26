/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Multi-pot Controller
//  https://diyelectromusic.wordpress.com/2023/04/26/arduino-midi-multi-pot-controller/
//
      MIT License
      
      Copyright (c) 2023 diyelectromusic (Kevin)
      
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
    Potentiometer        - https://www.arduino.cc/en/Tutorial/Potentiometer
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
*/
#include <MIDI.h>

// MIDI Channel for transmitting (1-16)
#define MIDI_CHANNEL 1

// Uncomment this to include USB MIDI Device Functionality too
// on boards that support it (e.g. ATmega32U4 or SAMD21 based).
//#define USB_MIDI_DEVICE 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#ifdef USB_MIDI_DEVICE
#include <USB-MIDI.h>
USBMIDI_CREATE_INSTANCE(0, UMIDI);
#endif

#define NUMPOTS 8
int pots[NUMPOTS] = {A0,A1,A2,A3,A4,A5,A6,A7};
//int pots[NUMPOTS] = {A1,A2,A3,A4,A5,A8,A9,A10};
int cc[NUMPOTS] = {
//  0x01,  // Modulation wheel
//  0x07,  // Channel volume
//  0x08,  // Balance
//  0x0A,  // Pan
//  0x0B,  // Expression control
//  0x0C,  // Effect control 1
//  0x0D,  // Effect control 2
  0x10,  // General purpose control 1
  0x11,  // General purpose control 2
  0x12,  // General purpose control 3
  0x13,  // General purpose control 4
  0x14,  // General purpose control 5
  0x15,  // General purpose control 6
  0x16,  // General purpose control 7
  0x17,  // General purpose control 8
};
int midival[NUMPOTS];
int algval[NUMPOTS][3];

void setup() {
  MIDI.begin(MIDI_CHANNEL);
#ifdef USB_MIDI_DEVICE
  UMIDI.begin(MIDI_CHANNEL);
#endif

  // Preset the "last" values to something invalid to
  // force an update on the first time through the loop.
  for (int i=0; i<NUMPOTS; i++) {
    midival[i] = -1;
  }
}

int pot;
void loop() {
  // As this is a controller, we process any MIDI messages
  // we receive and allow the MIDI library to enact it's THRU
  // functionality to pass them on.
  //
  // NB: Only do this for MIDI serial.
  //     In future could add USB<->Serial routing too.
  MIDI.read();

  pot++;
  if (pot >= NUMPOTS) pot = 0;

  // Map values from range 0..1023 to 0..127
  int aval = analogRead(pots[pot])>>3;

  // Only check once we have a stable value
  if ((aval == algval[pot][0]) && (algval[pot][0]==algval[pot][1]) && (algval[pot][1]==algval[pot][2])) {
    if (aval != midival[pot]) {
      // The value for this pot has changed so send a new MIDI message
      MIDI.sendControlChange (cc[pot], aval, MIDI_CHANNEL);
#ifdef USB_MIDI_DEVICE
      UMIDI.sendControlChange (cc[pot], aval, MIDI_CHANNEL);
#endif
    }
    midival[pot] = aval;
  }
  algval[pot][2] = algval[pot][1];
  algval[pot][1] = algval[pot][0];
  algval[pot][0] = aval;
}
