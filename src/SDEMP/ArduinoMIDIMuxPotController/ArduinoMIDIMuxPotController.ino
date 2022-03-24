/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI MUX Pot Controller
//  https://diyelectromusic.wordpress.com/2022/03/24/arduino-usb-midi-mux-pot-controller/
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
    Analog Input  - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    74HC4051 Multiplexer - https://www.gammon.com.au/forum/?id=11976
*/
#include <MIDI.h>

//#define DEBUG

#define NUM_POTS 16  // Up to 8 or up to 16 depending on which multiplexer is in use
#define MUX_POT  A5 // Pin to read the MUX value
#define MUX_S0   2  // Digital IO pins to control the MUX
#define MUX_S1   3
#define MUX_S2   4
#define MUX_S3   5  // HC4067 (16 ports) only, comment out if using a HC4051 (8 ports).

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1  // Define which MIDI channel to transmit on (1 to 16).

int pot;
uint8_t pots[NUM_POTS];

// Map potentiometers onto MIDI CC messages
uint8_t midiCC[NUM_POTS] = {
/* 00 */ 3,
/* 01 */ 74,
/* 02 */ 7,
/* 03 */ 89,
/* 04 */ 9,
/* 05 */ 20,
/* 06 */ 85,
/* 07 */ 91,
#ifdef MUX_S3
/* 08 */ 76,
/* 09 */ 21,
/* 10 */ 86,
/* 11 */ 94,
/* 12 */ 77,
/* 13 */ 22,
/* 14 */ 87,
/* 15 */ 71,
#endif
};

void setup() {
  // Set up the output pins to control the multiplexer
  pinMode (MUX_S0, OUTPUT);
  pinMode (MUX_S1, OUTPUT);
  pinMode (MUX_S2, OUTPUT);
#ifdef MUX_S3
  pinMode (MUX_S3, OUTPUT);
#endif
  pot = 0;

#ifdef DEBUG
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL_OFF);
#endif
}

void loop() {
  // Take the reading for this pot:
  //  Set the MUX control pins to choose the pot.
  //  Then read the pot.
  //
  // The MUX control works as a 3 or 4-bit number.
  // The HC4051 requires three bits to choose one of eight
  // ports whereas the HC4067 requires four bits to choose
  // one of sixteen.
  //
  // Taking the HC4051, the bits ordered as follows:
  //    S2-S1-S0
  //     0  0  0 = IO port 0
  //     0  0  1 = IO port 1
  //     0  1  0 = IO port 2
  //     ...
  //     1  1  1 = IO port 7
  //
  // So we can directly link this to the lowest bits
  // (1,2,4) of "playingNote".
  //
  // For the 16-port variant, we need a fourth bit.
  //
  if (pot & 1) { digitalWrite(MUX_S0, HIGH);
        } else { digitalWrite(MUX_S0, LOW);
        }
  if (pot & 2) { digitalWrite(MUX_S1, HIGH);
        } else { digitalWrite(MUX_S1, LOW);
        }
  if (pot & 4) { digitalWrite(MUX_S2, HIGH);
        } else { digitalWrite(MUX_S2, LOW);
        }
#ifdef MUX_S3
  if (pot & 8) { digitalWrite(MUX_S3, HIGH);
        } else { digitalWrite(MUX_S3, LOW);
        }
#endif
  // Scale the potentiometer to the MIDI 0 to 127 range.
  // i.e. take the 10-bit value (0-1023) and turn it into a 7-bit value.
  uint8_t potReading = (analogRead (MUX_POT) >> 3);

  // If the value has changed since last time, send a new CC message.
  if (pots[pot] != potReading) {
#ifdef DEBUG
    Serial.print(pot);
    Serial.print("\tMIDI CC ");
    Serial.print(midiCC[pot]);
    Serial.print(" -> ");
    Serial.println(potReading);
#else
    MIDI.sendControlChange(midiCC[pot], potReading, MIDI_CHANNEL);
#endif
    pots[pot] = potReading;
  }

  // Move to the next pot
  pot++;
  if (pot >= NUM_POTS) pot = 0;
}
