/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Delay
//  https://diyelectromusic.wordpress.com/2020/08/11/arduino-midi-delay/
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
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library

*/
#include <MIDI.h>
#include "TimerOne.h"

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// The MIDI channel to listen on
// Set it to MIDI_CHANNEL_OMNI to listen on all channels
#define MIDI_CHANNEL 1

// Number of echoes to play for each note
#define ECHOS 5

// And the delay in milliseconds between them
#define ECHO_DELAY 300

// We can reduce the velocity on each echo if we wish
#define ECHO_DECR  30

// You can change the pitch of an echo with this value
// For example, set to 1 to rise a semitone on each echo
// or to -1 to fall a semitone.
#define ECHO_SHIFT 0

// This project runs code on the command of one of the Arduino Timers.
// This determines the length of the "tick" of the timer.
//
// Using a TICK of 1000 means our code runs every
// 1000 microseconds (1 milli second), i.e. at a frequency of 1kHz.
//
#define TICK 1000

// Need to keep track of which notes are in various stages of delay
#define NUMNOTES 16
struct delayNotes_s {
  unsigned long milliOn;
  unsigned long milliOff;
  byte note;
  byte velocity;  // Only storing NoteOn velocity here so no "after touch"
  byte channel;
  byte count;
};
delayNotes_s delayNotes[NUMNOTES];
int processNote;
#define NO_MILLI_OFF 0xFFFFFFFF

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

  midiNoteOn (pitch, velocity, channel);
  addNoteDelay (pitch, velocity, channel);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  midiNoteOff (pitch, velocity, channel);
  addNoteOffDelay (pitch);
}

void setup() {
  // Use an Arduino Timer1 to generate a regular "tick"
  // to run our code every TICK microseconds.
  Timer1.initialize (TICK);
  Timer1.attachInterrupt (handleMIDIDelay);
  
  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to the specified MIDI channel
  MIDI.begin(MIDI_CHANNEL);
  MIDI.turnThruOff();  // Disable automatic MIDI Thru function
}

void loop() {
  // All the loop has to do is call the MIDI read function
  MIDI.read();
}

// Functions to play the note over MIDI
void midiNoteOn (byte note, byte velocity, byte channel) {
  MIDI.sendNoteOn (note, velocity, channel); 
}

void midiNoteOff (byte note, byte velocity, byte channel) {
  MIDI.sendNoteOff (note, velocity, channel); 
}

void handleMIDIDelay () {
  // This code runs every "TICK"
  processNote++;
  if (processNote >= NUMNOTES) {
    processNote = 0;
  }

  // Ordinarily I'd be using a loop here:
  //    for (i=0; i<NUMNOTES; i++)
  // But in order to keep processing within this routine
  // as short as possible, we do just one note on each "TICK".
  int i = processNote;
  
  // For each note, check its timer value and if expired, play an echo
  if (delayNotes[i].count > 0) {
    // There are still "counts" left on this note, so process it
    unsigned long milli = millis();

    // Check Note On
    if (delayNotes[i].milliOn < milli) {
      // time has passed for the note, so "play it"

      // are we pitch shifting?
      byte note = delayNotes[i].note + ECHO_SHIFT*(ECHOS-delayNotes[i].count+1);
      midiNoteOn (note, delayNotes[i].velocity, delayNotes[i].channel);

      // Set the time for the next echo
      delayNotes[i].milliOn += ECHO_DELAY;

      // Reduce the velocity (but don't go below zero)
      if (delayNotes[i].velocity > ECHO_DECR) {
        delayNotes[i].velocity -= ECHO_DECR;
      }
      // Note all "counting" will be done when processing the "note off"
    }

    // Check Note Off
    if (delayNotes[i].milliOff < milli) {
      // time has passed for the note off, so send note off
      // Note: There is no stored "note off" velocity information, so use 0

      // are we pitch shifting?
      byte note = delayNotes[i].note + ECHO_SHIFT*(ECHOS-delayNotes[i].count+1);
      midiNoteOff (note, 0, delayNotes[i].channel);

      // And set the time for the next "off"
      delayNotes[i].milliOff += ECHO_DELAY;

      // Now reduce the number of echoes left
      delayNotes[i].count--;
      if (delayNotes[i].count == 0) {
        // If the count is up, remove this note
        removeNoteDelayIdx (i);
      }
    }
  }
}

void addNoteDelay (byte note, byte velocity, byte channel) {
  // Find a free slot to add our note
  // If there are no free slots, we ignore it
  for (int i; i<NUMNOTES; i++) {
    if (delayNotes[i].note == 0) {
      // store our note in the first free slot
      addNoteDelayIdx (i, note, velocity, channel);

      // Nothing else to do once stored
      return;
    }
  }
}

void addNoteOffDelay (byte note) {
  // Find our previously stored note
  for (int i; i<NUMNOTES; i++) {
    if (delayNotes[i].note == note) {
      // store our note if we don't already have a good milliOff value
      if (delayNotes[i].milliOff == NO_MILLI_OFF) {
        addNoteOffDelayIdx (i);
        return;
      }
    }
  }
}

void removeNoteDelay (byte note) {
  // Find our note and remove it
  for (int i; i<NUMNOTES; i++) {
    if (delayNotes[i].note == note) {
      // remove our note
      removeNoteDelayIdx (i);
    }
  }
}

void addNoteDelayIdx (int index, byte note, byte velocity, byte channel) {
  // It is important that we don't get a TICK happening part way through
  // adding our note otherwise it will get very confused!
  //
  // using noInterrupts()/interrupts() temporarily pauses the TICK.
  //
  // This is only a problem here as the add function is called from the
  // main code (via the MIDI callback).  This isn't needed for any of the
  // other functions as they are called from within the TICK itself anyway.
  //
  noInterrupts();
  delayNotes[index].note = note;
  delayNotes[index].velocity = velocity;
  delayNotes[index].channel = channel;
  delayNotes[index].count = ECHOS;
  delayNotes[index].milliOn = millis() + ECHO_DELAY;
  // Set a large value here so that it is a long way "in the future".
  // This will be set accurately when we receive a proper "note off" event.
  delayNotes[index].milliOff = NO_MILLI_OFF;
  interrupts();
}

void addNoteOffDelayIdx (int index) {
  delayNotes[index].milliOff = millis() + ECHO_DELAY;
}

void removeNoteDelayIdx (int index) {
  delayNotes[index].note  = 0;
  delayNotes[index].velocity = 0;
  delayNotes[index].channel = 0;
  delayNotes[index].count = 0;
  delayNotes[index].milliOn = 0;  
  delayNotes[index].milliOff = 0;  
}
