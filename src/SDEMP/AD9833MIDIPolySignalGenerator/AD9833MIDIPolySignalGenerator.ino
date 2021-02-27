/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  AD9833 MIDI Poly Signal Generator
//  https://diyelectromusic.wordpress.com/2021/02/27/ad9833-midi-poly-signal-generator/
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
*/
#include <SPI.h>
#include <MD_AD9833.h>
#include <MIDI.h>
#include "pitches.h"

// Uncomment this to disable MIDI and enable fixed tones for testing
// with polyphony and frequency information dumped to the serial console.
//#define TEST 1
unsigned long testMillis;

// Pins for SPI link with the AD9833
#define DATA  11  // SPI Data pin number (hardware SPI = 11)
#define CLK   13  // SPI Clock pin number (hardware SPI = 13)
// SPI Load pin numbers (FSYNC in AD9833 usage)
#define NUM_ADS 6
int fsync[NUM_ADS] = {5, 6, 7, 8, 9, 10};
MD_AD9833* ad[NUM_ADS];

#define POT1 A0
#define POT2 A1
#define BTN1 3   // Slider switch
#define BTN2 2   // Slider switch
#define BTN3 4   // Button

int freqscale;
int freqdiff;
int lastbtn1,lastbtn2,lastbtn3;

// NB: playing[] stores the current playing frequency, not MIDI note number.
// It also supports a maximum of NUM_NOTES depending on the polyphony mode.
//
// If multiple signal generators are attached to each playing note, then
// the number of notes supported here will be less.
//
#define NUM_NOTES 6
int playing[NUM_NOTES];
int playidx;

// Define the polyphony mode.
//    polymode=0 -> 1 note polyphony (monophonic), but using all 6 signal generators.
//    polymode=1 -> 2 note polyphony, 3 sig gens each.
//    polymode=2 -> 3 note polyphony, 2 sig gens each.
//    polymode=3 -> 6 note polyphony, 1 sig gen each.
//
// In the code, it talks about the concepts of notes, oscillators and signal generators.
//
// We need one oscillator to play a note, so they are tied one to one with the
// polyphony mode, but each oscillator will have one or more signal generators
// associated with it depending on the mode.
//
// For example, in polymode 1 above, for 2-note polyphony, this is configured
// as two oscillators, each of three signal generators each.
//
// For polymode 0, there is one oscillator, but it uses all six signal generators.
//
#define NUM_MODES 4
int numNotes[NUM_MODES]={1,2,3,6};
int polymode;
#define DEFAULT_POLY_MODE 0 // Use this to set the default mode (e.g. if no BTN3)

// Comment this out to ignore new notes in polyphonic modes
#define POLY_OVERWRITE 1

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

#ifdef POLY_OVERWRITE
  // Overwrites the oldest note playing
  playidx++;
  if (playidx >= numNotes[polymode]) playidx=0;
  playing[playidx] = notes[pitch-MIDI_NOTE_START];
  oscSetFrequency(playidx, playing[playidx]);
#else
  // Ignore new notes until there is space
  for (int i=0; i<numNotes[polymode]; i++) {
    if (playing[i] == 0) {
      playing[i] = notes[pitch-MIDI_NOTE_START];
      oscSetFrequency(i, playing[i]);
      return;
    }
  }
#endif
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  if ((pitch < MIDI_NOTE_START) || (pitch >= MIDI_NOTE_START+numnotes)) {
    // The note is out of range for us so do nothing
    return;
  }

  for (int i=0; i<numNotes[polymode]; i++) {
    if (playing[i] == notes[pitch-MIDI_NOTE_START]) {
      oscClearFrequency(i);
      playing[i]= 0;
    }
  }
}

#ifdef TEST
void dumpAdParams() {
  Serial.print(polymode);
  Serial.print("\t");
  for (int i=0; i<NUM_ADS; i++) {
    Serial.print(playing[i]);
    Serial.print("\t");
    Serial.print(ad[i]->getFrequency(MD_AD9833::CHAN_0));
    Serial.print("(");
    Serial.print(ad[i]->getMode());
    Serial.print(")\t");
  }
  Serial.print("\n");
}
#endif  

#ifdef TEST
void playTestMidiNotes() {
  // Play six "note ons" to test the polyphony
  handleNoteOn(1, 79, 127);
  handleNoteOn(1, 76, 127);
  handleNoteOn(1, 72, 127);
  handleNoteOn(1, 67, 127);
  handleNoteOn(1, 64, 127);
  handleNoteOn(1, 60, 127);
}
#endif  

// This function replays any existing enabled notes.
// It is useful when the sound parameters change.
//
void soundAllNotes() {
  for (int i=0; i<numNotes[polymode]; i++) {
    if (playing[i] != 0) {
      // Replay frequencies for all playing notes
      oscSetFrequency(i, playing[i]);
    }
  }  
}

// This clears all playing note information.
// It is useful when the polyphony mode changes.
//
void resetAllNotes() {
  for (int i=0; i<NUM_ADS; i++) {
    // Turn off all the signal generators
    ad[i]->setFrequency(MD_AD9833::CHAN_0, 0);
    ad[i]->setMode(MD_AD9833::MODE_OFF);

    // And mark all potential playing notes as "free"
    playing[i] = 0;
  }    
}

// Helper function to set the frequency for one actual
// sound generator, which additional debug information
// designed to be used from within the adSetFrequency function.
//
void adSetFrequency(int osc, float freq) {
  ad[osc]->setFrequency(MD_AD9833::CHAN_0, freq);
#ifdef TEST
  Serial.print(freq);
  Serial.print("\t");
#endif  
}

// This is the main frequency function for each oscillator.
// It is fully aware of polyphony modes and will handle the
// application of freqscale and freqdiff to multiple signal
// generatos in the oscillator as required in the appropriate modes.
//
// When freqscale and freqdiff are zero it would be nice to
// have all signal generators within an oscillator sounding
// at the same frequency.
//
// However, due to (what I am assuming) are imperfections in
// either the timings, clocks, or accuracy of the frequency
// or phase settings across multiple signal generators, I was
// finding that there would be a very slow phase-in-and-out
// of apparently "same" frequencies meaning that eventually
// the audio would almost dissappear completely when signals
// are cancelling out!  For this reason if there is no modulation
// of the frequency, I will only sound one signal generator even
// in modes where an osciallator could be sounding them all.
//
// This makes for a quieter non-modulated output, but it is
// a reliable one.
//
void oscSetFrequency(int osc, float freq) {
#ifdef TEST
  Serial.print(polymode);
  Serial.print(" [");
  Serial.print(osc);
  Serial.print("]: (");
  Serial.print(freqscale);
  Serial.print(":");
  Serial.print(freqdiff);
  Serial.print(") freq=");
  Serial.print(freq);
  Serial.print("\t");
  for (int i=0; i<NUM_ADS; i++) {
    Serial.print(playing[i]);
    Serial.print("\t");
  }
#endif
  if (polymode==3) {
    // Full six-note polyphony, one oscillator each
    if (osc < NUM_ADS) {
      adSetFrequency(osc, freq);
    }
  } else if (polymode==2) {
    // Three note polyphony, two oscillators each
    if (osc < NUM_ADS/2) {
      adSetFrequency(osc*2, freq);
      if ((freqscale==0) && (freqdiff==0)) {
        // Clear other signal generators
        adSetFrequency(osc*2+1, 0);
      } else if ((freqscale==0) && (freqdiff!=0)) {
        adSetFrequency(osc*2+1, freq+(float)freqdiff);
      } else { // freescale != 0; freediff = "don't care"
        adSetFrequency(osc*2+1, (freq+(float)freqdiff)*(float)freqscale);
      }
    }
  } else if (polymode==1) {
    // Two note polyphony, three oscillators each
    if (osc < NUM_ADS/3) {
      adSetFrequency(osc*3, freq);
      if ((freqscale==0) && (freqdiff==0)) {
        adSetFrequency(osc*3+1, 0);
        adSetFrequency(osc*3+2, 0);
      } else if ((freqscale==0) && (freqdiff!=0)) {
        adSetFrequency(osc*3+1, freq+(float)freqdiff);
        adSetFrequency(osc*3+2, freq+(float)freqdiff*2);
      } else {
        adSetFrequency(osc*3+1, (freq+(float)freqdiff)*(float)freqscale);
        adSetFrequency(osc*3+2, (freq+(float)freqdiff*2)*(float)freqscale*2);
      }
    }
  } else {
    // Monophonic, full six oscillators!
    adSetFrequency(0, freq);
    for (int i=1; i<NUM_ADS; i++) {
      if ((freqscale==0) && (freqdiff==0)) {
        adSetFrequency(i, 0);
      } else if ((freqscale==0) && (freqdiff!=0)) {
        adSetFrequency(i, freq+(float)freqdiff*i);
      } else {
        adSetFrequency(i, (freq+(float)freqdiff*i)*(float)freqscale*i);
      }
    }
  }
#ifdef TEST
  Serial.print("\n");
#endif
}

// The main function to stop oscillators.
// Fully aware of the different polyphony modes.
//
void oscClearFrequency(int osc) {
  if (polymode==3) {
    // Full six-note polyphony
    if (osc < NUM_ADS) {
      adSetFrequency(osc, 0);
    }
  } else if (polymode==2) {
    if (osc < NUM_ADS/2) {
      adSetFrequency(osc*2, 0);
      adSetFrequency(osc*2+1, 0);
    }
  } else if (polymode==1) {
    // Two note polyphony, three oscillators each
    if (osc < NUM_ADS/3) {
      adSetFrequency(osc*3, 0);
      adSetFrequency(osc*3+1, 0);
      adSetFrequency(osc*3+2, 0);
    }
  } else {
    // Monophonic, full six oscillators!
    for (int i=0; i<NUM_ADS; i++) {
      adSetFrequency(i, 0);
    }
  }
}

// Sets the oscillators to SINE or TRIANGLE modes
// as required.  Fully aware of the different polyphony
// modes and will set multiple oscillators if required.
//
// This is mean to work on a single "bank" of signal
// generators at a time, so in a multi-generator polyphonic
// mode, it will set the mode for "all the first signal
// generators for each oscillator".
//
void oscSetMode(int bank, MD_AD9833::mode_t mode) {
  if (polymode==3) {
    // When running as 6-note polyphony, one switch controls them all
    for (int i=0; i<NUM_ADS; i++) {
      ad[i]->setMode(mode);
    }
  } else if (polymode==2) {
    if (bank<NUM_ADS/3) {
      ad[bank]->setMode(mode);
      ad[bank+2]->setMode(mode);
      ad[bank+4]->setMode(mode);
    }
  } else if (polymode==1) {
    if (bank<NUM_ADS/2) {
      ad[bank]->setMode(mode);
      ad[bank+3]->setMode(mode);
    }
  } else {
    // When monophonic, again one switch controls them all
    for (int i=0; i<NUM_ADS; i++) {
      ad[i]->setMode(mode);
    }
  }
}

void setup () {
  // initialise each instance of the AD9833
  for (int i=0; i<NUM_ADS; i++) {
    ad[i] = new MD_AD9833(fsync[i]);
    ad[i]->begin();
    ad[i]->setMode(MD_AD9833::MODE_OFF);
    ad[i]->setFrequency(MD_AD9833::CHAN_0, 0);
  }

  // Initialise pots and buttons
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);

  // If switch is HIGH then start LOW to force a "first time" setup
  lastbtn1 = digitalRead(BTN1)?LOW:HIGH;
  lastbtn2 = digitalRead(BTN2)?LOW:HIGH;
  lastbtn2 = HIGH;
  freqscale = 0;
  freqdiff = 0;
  polymode = DEFAULT_POLY_MODE;

#ifdef TEST
  Serial.begin(9600);
#else
  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to the specified MIDI channel
  MIDI.begin(MIDI_CHANNEL);
#endif
  numnotes = sizeof(notes)/sizeof(notes[0]);

#ifdef TEST
  // Turn on the signal and leave it on!
  playTestMidiNotes();
#endif
}

// This is the "loop within the loop" which handles things
// that need more frequent processing, but also supports
// the idea of scanning whilst delaying the main loop.
void subLoopDelay (unsigned long msdelay) {
  unsigned long d = millis()+msdelay;
  // Using do{}while means this will run at least once
  do {
     // Check for MIDI messages
#ifndef TEST
     MIDI.read();
#endif
  } while (millis() < d);
}

void loop () {
  subLoopDelay(0);

  bool resetNotes = false;
  
#ifdef BTN3
  int btn3 = digitalRead(BTN3);
  if ((btn3 == LOW) && (lastbtn3 == HIGH)) {
    // Button was pressed
    polymode++;
    if (polymode >= NUM_MODES) polymode=0;

    // Clear all playing oscillators/notes and
    // force mode change to update oscillator modes.
    resetAllNotes();
    resetNotes = true;

    // Pause processing for a while for crude "debouncing"
    subLoopDelay(300);
  }
  lastbtn3 = btn3;
#endif

  int btn1 = digitalRead(BTN1);
  if ((btn1 != lastbtn1) || resetNotes) {
    // Switch was changed
    if (btn1 == HIGH) oscSetMode(0, MD_AD9833::MODE_SINE);
    if (btn1 == LOW)  oscSetMode(0, MD_AD9833::MODE_TRIANGLE);
  }
  lastbtn1 = btn1;

  int btn2 = digitalRead(BTN2);
  if ((btn2 != lastbtn2) || resetNotes) {
    // Switch was changed
    if (btn2 == HIGH) {
      if (polymode==2) {
        // In three-note polyphonic mode, second button sets second oscillator
        oscSetMode(1, MD_AD9833::MODE_SINE);
      } else if (polymode==1) {
        // In two-note polyphonic mode, second button sets oscillators 2 and 3
        oscSetMode(1, MD_AD9833::MODE_SINE);
        oscSetMode(2, MD_AD9833::MODE_SINE);
      } else {
        // In 6-note polyphony or monophonic mode, ignore second switch
      }
    }
    if (btn2 == LOW) {
      if (polymode==2) {
         oscSetMode(1, MD_AD9833::MODE_TRIANGLE);
      } else if (polymode==1) {
         oscSetMode(1, MD_AD9833::MODE_TRIANGLE);
         oscSetMode(2, MD_AD9833::MODE_TRIANGLE);
      } // else nothing to do
    }
  }
  lastbtn2 = btn2;

#ifdef TEST
    if (resetNotes) {
       playTestMidiNotes();
    }
#endif
  subLoopDelay(0);

  // Check the potentiometers for the frequency modifiers.
 
  // Pot will return 0 to 1023, so scale it into selecting
  // a frequency multiplying factor of 0 to 31.
  // i.e. we are reducing the 10-bit analog port to a 6-bit port
  int pot1 = analogRead(POT1) >> 4;

  // Scale difference to 0 to 255
  int pot2 = analogRead(POT2) >> 2;

  if ((pot1!=freqscale) || (pot2!=freqdiff)) {
    freqscale = pot1;
    freqdiff = pot2;
    soundAllNotes();
  }

  // Short delay to stop the pots triggering too quickly
  // when on the boundaries between values!
  subLoopDelay(200);
  
#ifdef TEST
  if (testMillis < millis()) {
     dumpAdParams();
     testMillis = 2000+millis();
  }
#endif
}
