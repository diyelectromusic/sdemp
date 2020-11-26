/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  JQ6500 MIDI MP3 Player
//  https://diyelectromusic.wordpress.com/2020/11/26/jq6500-midi-mp3-player-module/
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
    JQ6500 Serial Library - https://github.com/sleemanj/JQ6500_Serial
    Arduino MIDI Library  - https://github.com/FortySevenEffects/arduino_midi_library
*/
#include <JQ6500_Serial.h>
#include <MIDI.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set the MIDI Channel to listen on and the
// specific note on the channel to trigger on.
//
// As nothing else can happen while the sound is being
// played it is particularly important to set the trigger
// criteria pretty specifically so that there aren't lots
// of MIDI events queuing up to keep playing the sound!
//
#define MIDI_CHANNEL  1

#define MIDI_NOTES 8
byte midiNotes [MIDI_NOTES] {
  60,62,64,65,67,69,71,72  // Middle C upwards - one octave of white notes
};

#define MIDI_LED  LED_BUILTIN

// Create the mp3 module object, 
//   Arduino Pin 8 is connected to TX of the JQ6500
//   Arduino Pin 9 is connected to one end of a  1k resistor, 
//     the other end of the 1k resistor is connected to RX of the JQ6500
//   If your Arduino is 3v3 powered, you can omit the 1k series resistor
JQ6500_Serial mp3(8,9);

// These are the functions to be called on recieving certain MIDI events.
// Full documentation of how to do this is provided here:
// https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-Callbacks

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  if (channel != MIDI_CHANNEL) {
    return;
  }
  
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  for (int i=0; i<MIDI_NOTES; i++) {
    if (pitch == midiNotes[i]) {
      digitalWrite(MIDI_LED, HIGH);

      // Trigger the sound for file "i"
      // The "IndexNumber" starts at 1 and can be found
      // by trial and error using rhe "full demo" application
      // command "f" - which plays files by their index number.
      mp3.playFileByIndexNumber(i+1);

      digitalWrite(MIDI_LED, LOW);
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  // There is no special handling here.
}

void setup() {
  pinMode (MIDI_LED, OUTPUT);

  // Initialise the MP3 player
  mp3.begin(9600);
  mp3.reset();
  mp3.setVolume(20);
  mp3.setLoopMode(MP3_LOOP_NONE);

  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to the specified MIDI channel
  MIDI.begin(MIDI_CHANNEL);
}

void loop() {
  MIDI.read();
}
