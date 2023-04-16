/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Step Sequencer
//  https://diyelectromusic.wordpress.com/2021/01/02/arduino-midi-step-sequencer/
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
    Analog Input  - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
*/
#include <MIDI.h>

#define NUM_POTS 8  // Starting at A0
int pots[NUM_POTS] = {A0, A1, A2, A3, A4, A5, A6, A7};
#define MIN_POT_READING 4 // Value for the lowest note

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

int midiChannel = 1;  // Define which MIDI channel to transmit on (1 to 16).

int numNotes;
int playingNote;
int lastNote;

// Set the MIDI codes for the notes to be played
int notes[] = {
9,10,11,12,  13,14,15,16, 17,18,19,20,  // A- to G#0
21,22,23,24, 25,26,27,28, 29,30,31,32,  // A0 to G#1
33,34,35,36, 37,38,39,40, 41,42,43,44,  // A1 to G#2
45,46,47,48, 49,50,51,52, 53,54,55,56,  // A2 to G#3
57,58,59,60, 61,62,63,64, 65,66,67,68,  // A3 to G#4
69,70,71,72, 73,74,75,76, 77,78,79,80,  // A4 to G#5
81,82,83,84, 85,86,87,88, 89,90,91,92,  // A5 to G#6
93,94,95,96, 97,98,99,100, 101,102,103,104, // A6 to G#7
105,106,107,108, 109,110,111,112, 113,114,115,116, // A7 to G#8
};

void setup() {
  // This is a programming trick to work out the number of notes we've listed
  numNotes = sizeof(notes)/sizeof(notes[0]);

  playingNote = 0;

  MIDI.begin(MIDI_CHANNEL_OFF);
}

void loop() {
  // Loop through each note in the sequence in turn, playing
  // the note defined by the potentiomer for that point in the sequence.

  if (playingNote >= NUM_POTS) {
    playingNote = 0;
  }

  // Take the reading for this pot
  int potReading = analogRead (pots[playingNote]);

  // if the reading is zero (or almost zero), turn off any playing note
  if (potReading < MIN_POT_READING) {
    if (lastNote != -1) {
      MIDI.sendNoteOff(notes[lastNote], 0, midiChannel);
      lastNote = -1;
      
    }
  } else {
    // This is a special function that will take one range of values,
    // in this case the analogue input reading between 0 and 1023, and
    // produce an equivalent point in another range of numbers, in this
    // case choose one of the notes in our list.
    int playing = map(potReading, MIN_POT_READING, 1023, 0, numNotes-1);

    // If we are already playing a note then we need to turn it off first
    if (lastNote != -1) {
        MIDI.sendNoteOff(notes[lastNote], 0, midiChannel);
    }
    MIDI.sendNoteOn(notes[playing], 127, midiChannel);
    lastNote = playing;
  }

  // Move on to the next note after a short delay.
  // 250 = 1/4 of a second so this is same as 240 beats per minute.
  delay(250);
  playingNote++;
}
