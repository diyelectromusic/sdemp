/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  ESP32 Simple MIDI Monitor
//  https://diyelectromusic.com/2024/12/09/esp32-simple-midi-monitor/
//
      MIT License
      
      Copyright (c) 2024 diyelectromusic (Kevin)
      
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
*/
#include <MIDI.h>

// Comment out following to use first UART on the board (GPIO 3/1).
// Note: This means there will be no Serial output.
// The default is to use second UART on the board (GPIO 16/17)
// for MIDI and the first UART (connected to USB) for serial output.
#define UART2_MIDI

#ifndef UART2_MIDI
// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();
#define SERIAL_PRINT false
#endif

#ifdef UART2_MIDI
// Use UART2 instead on pins 16/17
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);  // UART 2 (GPIO 16/17)
#define SERIAL_PRINT true
#endif

// This will only result in code if SERIAL_PRINT is true
#define S if (SERIAL_PRINT)Serial

void serialMidiSetup () {
  // This listens to all MIDI channels
  MIDI.begin(MIDI_CHANNEL_OMNI);
#ifdef UART2_MIDI
  // Need to re-initialise serial port with correct pins.
  // MIDI will call Serial2.begin, but with no specific pin
  // settings, so the ESP32 core will use some defaults,
  // which aren't the same as what we need...
  Serial2.begin(31250, SERIAL_8N1, 16, 17);
#endif
}

#define MIDI_LED 2

void setup() {
  serialMidiSetup();

#if SERIAL_PRINT==true
  // First serial used for serial printing over USB
  Serial.begin(115200);
#endif

  // Use the built-in LED as a MIDI note indicator
  pinMode (MIDI_LED, OUTPUT);
}

void print1Hex (uint val) {
  S.print("0x");
  if (val < 16) S.print("0");
  S.print(val,HEX);
}

void print2Hex (uint val1, uint val2) {
  S.print("0x");
  if (val1 < 16) S.print("0");
  S.print(val1,HEX);
  S.print("\t");
  S.print("0x");
  if (val2 < 16) S.print("0");
  S.print(val2,HEX);
}

void print1Status (char *pS, midi::MidiType type, midi::DataByte d1) {
  print1Hex((uint)type);
  S.print(" ");
  S.print(pS);
  S.print("\t");
  print1Hex((uint)d1);
}

void print2Status (char *pS, midi::MidiType type, midi::DataByte d1, midi::DataByte d2) {
  print1Hex((uint)type);
  S.print(" ");
  S.print(pS);
  S.print("\t");
  print2Hex((uint)d1, (uint)d2);
}

void loop() {
  if (MIDI.read()) {
    // We have a completed MIDI message
    midi::MidiType type = MIDI.getType();

    if (MIDI.isChannelMessage(type)) {
      midi::Channel  ch = MIDI.getChannel();
      midi::DataByte d1 = MIDI.getData1();
      midi::DataByte d2 = MIDI.getData2();
      S.print(ch);
      S.print("\t");
      switch(type) {
        case midi::NoteOff:
          digitalWrite (MIDI_LED, LOW);
          print2Status("NoteOff", type, d1, d2);
          break;

        case midi::NoteOn:
          digitalWrite (MIDI_LED, HIGH);
          print2Status("NoteOn", type, d1, d2);
          break;

        case midi::AfterTouchPoly:
          print2Status("ATPoly", type, d1, d2);
          break;

        case midi::ControlChange:
          print2Status("CtrlCh", type, d1, d2);
          break;

        case midi::ProgramChange:
          print1Status("ProgCh", type, d1);
          break;

        case midi::AfterTouchChannel:
          print1Status("ATChan", type, d1);
          break;

        case midi::PitchBend:
          print2Status("PBend", type, d1, d2);
          break;

        default:
          // Otherwise it is a message we aren't interested in
          break;
      }
      S.print("\n");
    }
    else
    {
      // One of the system messages
      if (type == midi::ActiveSensing) {
        // Ignore
      }
      else
      {
        print1Hex((uint)type);
        S.print("\n");
      }
    }
  }
}
