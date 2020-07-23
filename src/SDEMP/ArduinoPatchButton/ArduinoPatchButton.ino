/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Patch Button
//  https://diyelectromusic.wordpress.com/2020/06/05/midi-patch-button/
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
    Button               - https://www.arduino.cc/en/Tutorial/Button
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
*/
#include <MIDI.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

int btnPin = 16;      // Use 3-13 for the Arduino Uno or Nano; For Wemos D1 Mini, 16 = D0
int midiPatch   = 51; // Define the patch number - 51 is Tuba in the General MIDI standard.
int midiChannel = 1;  // Define which MIDI channel to transmit on (1 to 16).

void setup() {
  // put your setup code here, to run once:
  MIDI.begin(MIDI_CHANNEL_OFF);

  // This updates speed of the serial channel so that it can be used
  // with a PC rather than a MIDI port.
  //
  // This can be used with software such as the Hairless MIDI Bridge
  // and loopMidi during testing.
  //
  // It is commented out for "real" use.
  //
  //Serial.begin (115200);

  // Initialise the button pin to input (active HIGH)
  pinMode (btnPin, INPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead (btnPin) == HIGH) {
    // Send our MIDI message to change the patch number
    MIDI.sendProgramChange (midiPatch, midiChannel);

    // Include a short delay so the switch isn't continuously read and triggered
    delay (500);
  }
}
