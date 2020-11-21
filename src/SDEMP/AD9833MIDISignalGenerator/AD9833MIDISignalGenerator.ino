/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  AD9833 MIDI Signal Generator
//  https://diyelectromusic.wordpress.com/2020/11/21/ad9833-midi-signal-generator/
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
    Arduino Input Pullup Serial - https://www.arduino.cc/en/Tutorial/BuiltInExamples/InputPullupSerial
    Arduino Analog Read Serial  - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogReadSerial
    Arduino SPI Library   - https://www.arduino.cc/en/Reference/SPI
    MD_AD9833 Library     - https://github.com/MajicDesigns/MD_AD9833
    Arduino MIDI Library  - https://github.com/FortySevenEffects/arduino_midi_library
    toneMelody            - http://www.arduino.cc/en/Tutorial/Tone
*/
#include <SPI.h>
#include <MD_AD9833.h>
#include <MIDI.h>
#include "pitches.h"

// Pins for SPI link with the AD9833
#define DATA  11  // SPI Data pin number (hardware SPI = 11)
#define CLK   13  // SPI Clock pin number (hardware SPI = 13)
#define FSYNC 10  // SPI Load pin number (FSYNC in AD9833 usage)

MD_AD9833 AD(FSYNC); // Hardware SPI
//MD_AD9833  AD(DATA, CLK, FSYNC); // Software SPI with any pins

#define BTN   2   // Input Button (active low)

int mode;
int lastbtn;

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set up the MIDI channel to listen on
#define MIDI_CHANNEL 1

// Set up the MIDI codes to respond to by listing the lowest note
#define MIDI_NOTE_START 23   // B0

// Set the notes to be played by each key
int notes[] = {
  NOTE_B0,
  NOTE_C1, NOTE_CS1, NOTE_D1, NOTE_DS1, NOTE_E1, NOTE_F1, NOTE_FS1, NOTE_G1, NOTE_GS1, NOTE_A1, NOTE_AS1, NOTE_B1,
  NOTE_C2, NOTE_CS2, NOTE_D2, NOTE_DS2, NOTE_E2, NOTE_F2, NOTE_FS2, NOTE_G2, NOTE_GS2, NOTE_A2, NOTE_AS2, NOTE_B2,
  NOTE_C3, NOTE_CS3, NOTE_D3, NOTE_DS3, NOTE_E3, NOTE_F3, NOTE_FS3, NOTE_G3, NOTE_GS3, NOTE_A3, NOTE_AS3, NOTE_B3,
  NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
  NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
  NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
  NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7,
  NOTE_C8, NOTE_CS8, NOTE_D8, NOTE_DS8
};
int numnotes;
byte noteon;

// These are the functions to be called on recieving certain MIDI events.
// Full documentation of how to do this is provided here:
// https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-Callbacks

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  if ((pitch < MIDI_NOTE_START) || (pitch >= MIDI_NOTE_START+numnotes)) {
    // The note is out of range for us so do nothing
    return;
  }

  AD.setFrequency(MD_AD9833::CHAN_0, notes[pitch-MIDI_NOTE_START]);
  noteon = pitch;
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  // If we get a noteoff for the current playing note,
  // then turn it off.  Otherwise we've probably already
  // replaced any previously playing note with a new one!
  if (pitch == noteon) {
    AD.setFrequency(MD_AD9833::CHAN_0, 0);
    noteon = 0;
  }
}

void setup () {
  pinMode (BTN, INPUT_PULLUP);
  AD.begin();
  AD.setFrequency(MD_AD9833::CHAN_0, 0);
  mode = 1;
  lastbtn=HIGH;

  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to the specified MIDI channel
  MIDI.begin(MIDI_CHANNEL);

  numnotes = sizeof(notes)/sizeof(notes[0]);
}

void loop () {
  // All the loop has to do is call the MIDI read function
  MIDI.read();

  // Check the button for a mode change
  int btn = digitalRead(BTN);
  if ((btn == LOW) && (lastbtn == HIGH)) {
     // We have a HIGH to LOW transition
     mode++;
     switch (mode) {
       case 1: AD.setMode(MD_AD9833::MODE_SINE);     break;
       case 2: AD.setMode(MD_AD9833::MODE_TRIANGLE); break;
       case 3: AD.setMode(MD_AD9833::MODE_SQUARE1);  break;
       case 4: AD.setMode(MD_AD9833::MODE_SQUARE2);  break;

       default: AD.setMode(MD_AD9833::MODE_OFF);
         mode = 0;
         break;
     }

     // Include a short delay so it doesn't trigger straight away
     // (this is a crude way of "debouncing" the switch!)
     delay (500);
  }
  lastbtn = btn;
}
