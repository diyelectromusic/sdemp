/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Button Bank Select
//  https://diyelectromusic.wordpress.com/2022/05/05/arduino-midi-button-bank-select/
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
  Using principles from the following Arduino tutorials:
    Arduino MIDI Library    - https://github.com/FortySevenEffects/arduino_midi_library
    Arduino Button          - https://www.arduino.cc/en/Tutorial/BuiltInExamples/Button
    Arduino USB Transport   - https://github.com/lathoub/Arduino-USBMIDI
*/
#include <MIDI.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno/Nano and
// the USB serial port if using a Leonardo/Pro Micro.
MIDI_CREATE_DEFAULT_INSTANCE();

// Uncomment this to include MIDI USB device support.
// This is only available on certain boards (e.g. ATmega32U4)
#define MIDI_USB_DEV
#include <USB-MIDI.h>
#ifdef MIDI_USB_DEV
USBMIDI_CREATE_INSTANCE(0, MIDI_UD);
#endif

#define MIDI_CHANNEL 1    // Define which MIDI channel to transmit on (1 to 16).

#define NUM_BTNS 6
int btns[NUM_BTNS] = {2,3,4,5,6,7};
int lastbtns[NUM_BTNS];

int ccs[NUM_BTNS][2] = {
  // MSB, LSB
  {    0,   0},
  {    0,   1},
  {    0,   2},
  {    0,   3},
  {    0,   4},
  {    0,   5},
};

// Bank Select related MIDI Control Change message values
#define BANKSELMSB 0
#define BANKSELLSB 32

void setup() {
  for (int i=0; i<NUM_BTNS; i++) {
    pinMode(btns[i], INPUT_PULLUP);
    lastbtns[i] = HIGH;
  }

  MIDI.begin(MIDI_CHANNEL);
#ifdef MIDI_USB_DEV
  MIDI_UD.begin(MIDI_CHANNEL);
#endif
}

void loop() {
  for (int i=0; i<NUM_BTNS; i++) {
    int btnval = digitalRead(btns[i]);

    if ((btnval == LOW) && (lastbtns[i] == HIGH)) {
      // Button pressed
      midiBankSelect(i);
      delay(300);  // Simple switch "debouncing"
    }
    lastbtns[i] = btnval;
  }
}

void midiBankSelect (int btn) {
  MIDI.sendControlChange (BANKSELMSB, ccs[btn][0], MIDI_CHANNEL);
  MIDI.sendControlChange (BANKSELLSB, ccs[btn][1], MIDI_CHANNEL);
  MIDI.sendProgramChange (0, MIDI_CHANNEL);
#ifdef MIDI_USB_DEV
  MIDI_UD.sendControlChange (BANKSELMSB, ccs[btn][0], MIDI_CHANNEL);
  MIDI_UD.sendControlChange (BANKSELLSB, ccs[btn][1], MIDI_CHANNEL);
  MIDI_UD.sendProgramChange (0, MIDI_CHANNEL);
#endif
}
