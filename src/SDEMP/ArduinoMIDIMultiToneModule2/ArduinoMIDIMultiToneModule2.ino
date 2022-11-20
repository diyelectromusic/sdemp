/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
// Arduino MIDI Multi Tone Module - A New, New Hope
// https://diyelectromusic.wordpress.com/2022/11/20/arduino-tones-a-new-new-hope/
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
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Brett Hagman's Tone Library - https://github.com/bhagman/Tone

  The number of tones possible depends on the number of timers on the board
  being used.  There are three on the ATmega168/328 (Uno/Nano) but six if
  you are using an ATmega1280/2560 based board (Mega).

  If you use the full number of Timers/tones then it will mess with the delay()
  and millis() functions!

*/
#include <MIDI.h>
#include <Tone.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set up the output pins to be used for the speaker
#define POLYPHONY 3
int speakers[POLYPHONY] = {4,5,6};

Tone player[POLYPHONY];
byte playing[POLYPHONY];
int  playidx;

// Set the mode for the polyphony:
//   Comment out: Play first available notes, then no more.
//   Uncomment:   Replace notes in the sequence they were played.
//
//#define POLYOVERWRITE 1

// Allow MIDI pass-through for more polyphony
//   Unncomment this for notes greater than the three
//   supported to be passed on thru MIDI.
//#define MIDI_PASS_THRU 1

// Allow multi-channel MIDI MONOPHONIC operation.
//   Uncomment this to listen on consecutive MIDI channels
//   up to the limits of POLYPHONY.
#define MIDI_MULTI_MONO 1

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

#ifdef MIDI_MULTI_MONO
  // Filter on MIDI channel
  if ((channel >= MIDI_CHANNEL) && (channel <= MIDI_CHANNEL+POLYPHONY-1)) {
    // Play the tone that corresponds to the MIDI channel.
    // Note: This will simply override any existing playing tones.
    int idx = channel-MIDI_CHANNEL;
    player[idx].play(notes[pitch-MIDI_NOTE_START]);
    playing[idx] = pitch;

    return;
  }
#else
#ifdef POLYOVERWRITE
  // Find next free to play.  Overwrite older notes playing.
  playidx++;
  if (playidx >= POLYPHONY) {
    playidx = 0;
  }
  player[playidx].play(notes[pitch-MIDI_NOTE_START]);
  playing[playidx] = pitch;

  return;
#else
  // Find a free tone slot to play. Ignore if no free slots.
  for (int i=0; i<POLYPHONY; i++) {
    if (playing[i] == 0) {
      // We have a free slot so claim it and play the note
      playing[i] = pitch;
      player[i].play(notes[pitch-MIDI_NOTE_START]);

      // Then stop looking for free slots...
      // (or it will fill them all up with the same note!)
      return;
    }
  }
#endif
#endif

#ifdef MIDI_PASS_THRU
  // If we haven't already returned, we aren't playing the note ourselves,
  // so pass it on via MIDI.
  MIDI.sendNoteOn (pitch, velocity, channel);
#endif
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
#ifdef MIDI_MULTI_MONO
  // Filter on MIDI channel
  if ((channel >= MIDI_CHANNEL) && (channel <= MIDI_CHANNEL+POLYPHONY-1)) {
    // Play the tone that corresponds to the MIDI channel.
    // Note: This will simply override any existing playing tones.
    int idx = channel-MIDI_CHANNEL;
    if (playing[idx] == pitch) {
      // This is our note, turn it off
      player[idx].stop();
      playing[idx] = 0;
      
      return;
    }
  }
#else
  // Find which tone is playing this note and turn it off.
  // If we don't recognise the note, then just ignore it.
  for (int i=0; i<POLYPHONY; i++) {
    if (playing[i] == pitch) {
      // This is our note, turn it off
      player[i].stop();
      playing[i] = 0;

      return;
    }
  }
#endif

#ifdef MIDI_PASS_THRU
  // If we haven't already returned, we aren't handling the note ourselves,
  // so pass it on via MIDI.
  MIDI.sendNoteOff (pitch, velocity, channel);
#endif
}

void setup() {
  // Initialise the tones
  for (int i=0; i<POLYPHONY; i++) {
    player[i].begin(speakers[i]);
  }
  
  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

#ifdef MIDI_MULTI_MONO
  // This listens to all MIDI channels and filter out later
  MIDI.begin(MIDI_CHANNEL_OMNI);
#else
  // This listens to the specified MIDI channel
  MIDI.begin(MIDI_CHANNEL);
#endif

#ifdef MIDI_PASS_THRU
 // Disable automatic MIDI THRU functionality
 MIDI.turnThruOff();
#endif
  numnotes = sizeof(notes)/sizeof(notes[0]);
}

void loop() {
  // All the loop has to do is call the MIDI read function
  MIDI.read();
}
