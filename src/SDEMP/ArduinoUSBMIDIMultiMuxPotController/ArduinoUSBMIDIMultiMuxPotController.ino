   /*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino USB MIDI Multi MUX Pot Controller
//  https://diyelectromusic.wordpress.com/2022/05/05/arduino-midi-multi-mux-controller/
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

// Uncomment this to include MIDI USB device support.
// This is only available on certain boards (e.g. ATmega32U4)
#define MIDI_USB_DEV

#define NUM_MUX  2  // Can have as many multiplexers as analog inputs
int mux_pins[NUM_MUX] = {A2, A3}; // One analog input pin per multiplexer
#define MUX_S0   2  // Digital IO pins to control the MUX
#define MUX_S1   3
#define MUX_S2   4
#define MUX_S3   5  // HC4067 (16 ports) only, comment out if using a HC4051 (8 ports).
#ifdef MUX_S3
#define NUM_POTS 16 // Up to 16 PER MUX if using a HC4067
#else
#define NUM_POTS 8  // Up to 8 PER MUX if using a HC4051
#endif

#define NUM_CCS  32 // MIDI CCs, one for each pot in each multiplexer max = NUM_MUX * NUM_POTS

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#ifdef MIDI_USB_DEV
#include <USB-MIDI.h>
USBMIDI_CREATE_INSTANCE(0, MIDI_UD);
#endif

#define MIDI_CHANNEL 1  // Define which MIDI channel to transmit on (1 to 16).

int pot;

uint8_t lastccs[NUM_CCS];

// Map potentiometers onto MIDI CC messages
uint8_t midiCC[NUM_CCS] = {
/* 00 */ 3,   // Undefined          (miniSynth: VCO waveform)
/* 01 */ 74,  // Sound Controller 5 (miniSynth: VCF cutoff)
/* 02 */ 7,   // Channel Volume     (miniSynth: Master volume)
/* 03 */ 89,  // Undefined          (miniSynth: Reverb rate of decay)
/* 04 */ 9,   // Undefined          (miniSynth: VCO-LFO waveform)
/* 05 */ 20,  // Undefined          (miniSynth: VCF-LFO waveform)
/* 06 */ 85,  // Undefined          (miniSynth: VCA-LCO waveform)
/* 07 */ 91,  // Effects 1 Depth    (miniSynth: Reverb wet/dry ratio)
/* 08 */ 76,  // Sound Controller 7 (miniSynth: VCO-LFO mod rate)
/* 09 */ 21,  // Undefined          (miniSynth: VCF-LFO mod rate)
/* 10 */ 86,  // Undefined          (miniSynth: VCA-LFO mod rate)
/* 11 */ 94,  // Effects 4 Depth    (miniSynth: VCO2 detune)
/* 12 */ 77,  // Sound Controller 8 (miniSynth: VCO-LFO volume)
/* 13 */ 22,  // Undefined          (miniSynth: VCF-LFO volume)
/* 14 */ 87,  // Undefined          (miniSynth: VCA-LFO volume)
/* 15 */ 71,  // Sound Controller 2 (miniSynth: VCF resonance)
/* 16 */ 16,  // General Purpose Controller 1
/* 17 */ 17,  // General Purpose Controller 2
/* 18 */ 18,  // General Purpose Controller 3
/* 19 */ 19,  // General Purpose Controller 4
/* 20 */ 80,  // General Purpose Controller 5
/* 21 */ 81,  // General Purpose Controller 6
/* 22 */ 82,  // General Purpose Controller 7
/* 23 */ 83,  // General Purpose Controller 8
/* 24 */102,  // Undefined
/* 25 */103,  // Undefined
/* 26 */104,  // Undefined
/* 27 */105,  // Undefined
/* 28 */106,  // Undefined
/* 29 */107,  // Undefined
/* 30 */108,  // Undefined
/* 31 */109,  // Undefined
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
  //  Set the MUX control pins to choose the pot in each mux.
  //  Then read the pot from each mux.
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

  // Now read this pot from each MUX
  for (int p=0; p<NUM_MUX; p++) {
    // Scale the potentiometer to the MIDI 0 to 127 range.
    // i.e. take the 10-bit value (0-1023) and turn it into a 7-bit value.
    uint8_t potReading = (analogRead (mux_pins[p]) >> 3);
    int potidx = pot + p*NUM_POTS;

    if (potidx < NUM_CCS) {
      // If the value has changed since last time, send a new CC message.
      if (lastccs[potidx] != potReading) {
#ifdef DEBUG
        Serial.print(pot);
        Serial.print("\tMIDI CC ");
        Serial.print(midiCC[potidx]);
        Serial.print(" -> ");
        Serial.println(potReading);
#else
        MIDI.sendControlChange(midiCC[potidx], potReading, MIDI_CHANNEL);
#endif
#ifdef MIDI_USB_DEV
        MIDI_UD.sendControlChange(midiCC[potidx], potReading, MIDI_CHANNEL);
#endif
        lastccs[potidx] = potReading;
      }
    }
  }

  // Move to the next pot
  pot++;
  if (pot >= NUM_POTS) pot = 0;
}
