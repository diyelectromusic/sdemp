/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  AD9833 MIDI Signal Generator
//  https://diyelectromusic.wordpress.com/2020/11/23/ad9833-midi-signal-generator-part-2/
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
#define DATA   11  // SPI Data pin number (hardware SPI = 11)
#define CLK    13  // SPI Clock pin number (hardware SPI = 13)
#define FSYNC1 10  // SPI Load pin number (FSYNC in AD9833 usage)
#define FSYNC2  9  // SPI Load pin for second AD9833

// Uncomment this if we are using two AD9833.  They will both
// share the same SPI pins but will be individually selected
// by having different FSYNC pins.
//
// If there is only one then comment this out.
//
#define DUAL_AD9833 1

// Uncomment this to disable the square wave handling
#define NOSQUARE 1

MD_AD9833 AD1(FSYNC1); // Hardware SPI, selected by FSYNC1
#ifdef DUAL_AD9833
MD_AD9833 AD2(FSYNC2); // Hardware SPI, selected by FSYNC2
#endif
//MD_AD9833  AD1(DATA, CLK, FSYNC); // Software SPI with any pins

// Define the pins for the two buttons to select the wave.
// If you want both waveforms to always be the same, then
// comment out the definition for BTN2.
//
#define BTN1   2   // Input Button (active low)
#define BTN2   3   // Input Button (active low)

#define POT    A0

int mode1;
int mode2;
int lastbtn1;
int lastbtn2;
int freqscale;

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

  AD1.setFrequency(MD_AD9833::CHAN_0, notes[pitch-MIDI_NOTE_START]);
#ifdef DUAL_AD9833
  AD2.setFrequency(MD_AD9833::CHAN_0, freqscale*notes[pitch-MIDI_NOTE_START]);
#endif
  noteon = pitch;
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  // If we get a noteoff for the current playing note,
  // then turn it off.  Otherwise we've probably already
  // replaced any previously playing note with a new one!
  if (pitch == noteon) {
    AD1.setFrequency(MD_AD9833::CHAN_0, 0);
#ifdef DUAL_AD9833
    AD2.setFrequency(MD_AD9833::CHAN_0, 0);
#endif
    noteon = 0;
  }
}

void setup () {
  pinMode (BTN1, INPUT_PULLUP);
  AD1.begin();
  AD1.setFrequency(MD_AD9833::CHAN_0, 0);
#ifdef DUAL_AD9833
#ifdef BTN2
  pinMode (BTN2, INPUT_PULLUP);
#endif
  AD2.begin();
  AD2.setFrequency(MD_AD9833::CHAN_0, 0);
#endif
  mode1 = 1;
  mode2 = 1;
  lastbtn1=HIGH;
  lastbtn2=HIGH;
  freqscale=0;

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

  // Check the potentiometer for the frequency multiplier
  // Pot will return 0 to 1023, so scale it into selecting
  // a frequency multiplying factor of 0 to 32.
  // i.e. we are reducing the 10-bit analog port to a 6-bit port
  freqscale = analogRead (POT) >> 4;

  // Check the button for a mode change
  int btn1 = digitalRead(BTN1);
  if ((btn1 == LOW) && (lastbtn1 == HIGH)) {
     // We have a HIGH to LOW transition
     mode1++;
     switch (mode1) {
       case 1: AD1.setMode(MD_AD9833::MODE_SINE);     break;
       case 2: AD1.setMode(MD_AD9833::MODE_TRIANGLE); break;
#ifndef NOSQUARE
       case 3: AD1.setMode(MD_AD9833::MODE_SQUARE1);  break;
       case 4: AD1.setMode(MD_AD9833::MODE_SQUARE2);  break;
#endif
       default: AD1.setMode(MD_AD9833::MODE_OFF);
         mode1 = 0;
         break;
     }
#ifdef DUAL_AD9833
#ifndef BTN2
     // If BTN2 is not being used, but we have dual AD9833 then
     // ensure that the two modes are the same.
     switch (mode1) {
       case 1: AD2.setMode(MD_AD9833::MODE_SINE);     break;
       case 2: AD2.setMode(MD_AD9833::MODE_TRIANGLE); break;
#ifndef NOSQUARE
       case 3: AD2.setMode(MD_AD9833::MODE_SQUARE1);  break;
       case 4: AD2.setMode(MD_AD9833::MODE_SQUARE2);  break;
#endif
       default: AD2.setMode(MD_AD9833::MODE_OFF);     break;
     }
#endif
#endif

     // Include a short delay so it doesn't trigger straight away
     // (this is a crude way of "debouncing" the switch!)
     delay (500);
  }
  lastbtn1 = btn1;

  // Repeat all the above for the second button if we have it
#ifdef DUAL_AD9833
#ifdef BTN2
  int btn2 = digitalRead(BTN2);
  if ((btn2 == LOW) && (lastbtn2 == HIGH)) {
     mode2++;
     switch (mode2) {
       case 1: AD2.setMode(MD_AD9833::MODE_SINE);     break;
       case 2: AD2.setMode(MD_AD9833::MODE_TRIANGLE); break;
#ifndef NOSQUARE
       case 3: AD2.setMode(MD_AD9833::MODE_SQUARE1);  break;
       case 4: AD2.setMode(MD_AD9833::MODE_SQUARE2);  break;
#endif
       default: AD2.setMode(MD_AD9833::MODE_OFF);
         mode2 = 0;
         break;
     }
     delay (500);
  }
  lastbtn2 = btn2;
#endif
#endif
}
