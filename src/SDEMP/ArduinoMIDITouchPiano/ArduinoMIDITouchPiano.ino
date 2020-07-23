/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Touch Piano
//  https://diyelectromusic.wordpress.com/2020/06/10/arduino-midi-touch-piano/
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
    CapacitiveSensor - https://playground.arduino.cc/Main/CapacitiveSensor/
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library

*/
#include <CapacitiveSensor.h>
#include <MIDI.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

int midiChannel = 1;  // Define which MIDI channel to transmit on (1 to 16).

// Set the notes to be played by each key
// Set the MIDI codes for the notes to be played by each key
int notes[] = {
  60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
};

int lastKey;

// Set up the pins to be used for the capacitive sensors as described in
// the CapacitiveSensor tutorial above.  Sensors need two pins - a common
// pin (pin 2 in this case) and a sensing pin.
//
// I've allowed for the same number of sensors as notes, which means
// using some of the Analog pins as digital I/O too.
//
// There are sensors on digital pins 3-11, and analog pins 14-17.
//
// This uses C++ objects, but we don't need to worry about that here.
// We just need to know how to read the sensors!
//
#define SENSOR_THRESHOLD 200
CapacitiveSensor   cs_2_3 = CapacitiveSensor(2,3);
CapacitiveSensor   cs_2_4 = CapacitiveSensor(2,4);
CapacitiveSensor   cs_2_5 = CapacitiveSensor(2,5);
CapacitiveSensor   cs_2_6 = CapacitiveSensor(2,6);
CapacitiveSensor   cs_2_7 = CapacitiveSensor(2,7);
CapacitiveSensor   cs_2_8 = CapacitiveSensor(2,8);
CapacitiveSensor   cs_2_9 = CapacitiveSensor(2,9);
CapacitiveSensor   cs_2_10 = CapacitiveSensor(2,10);
CapacitiveSensor   cs_2_11 = CapacitiveSensor(2,11);
CapacitiveSensor   cs_2_14 = CapacitiveSensor(2,14);
CapacitiveSensor   cs_2_15 = CapacitiveSensor(2,15);
CapacitiveSensor   cs_2_16 = CapacitiveSensor(2,16);
CapacitiveSensor   cs_2_17 = CapacitiveSensor(2,17);

// This is a programming trick that makes accessing the different
// sensor programming "objects" a bit easier.
//
CapacitiveSensor *caps[] = {
  &cs_2_3, &cs_2_4, &cs_2_5,
  &cs_2_6, &cs_2_7, &cs_2_8, 
  &cs_2_9, &cs_2_10, &cs_2_11, 
  &cs_2_14, &cs_2_15, &cs_2_16, &cs_2_17,
};
int numcaps;

void setup() {
  // put your setup code here, to run once:
  MIDI.begin(MIDI_CHANNEL_OFF);

  // This updates speed of the serial channel so that it can be used
  // with a PC rather than a MIDI port.
  //
  // This can be used with software such as the Hairless MIDI Bridge
  // and loopMidi during testing.
  //
  // It is commented out for "real" use.
  //
  //Serial.begin (115200);

  numcaps = sizeof (caps)/sizeof(caps[0]);
}

void loop() {
  // Check each sensor and if touched play the note.
  // If no sensors are touched, turn off all notes.
  int playing = -1;
  for (int k = 0; k < numcaps; k++) {
    long sensorvalue =  caps[k]->capacitiveSensor(20);
    if (sensorvalue > SENSOR_THRESHOLD) {
      playing = k;
    }
  }

  if (playing != -1) {
    // Only want go send a MIDI message if something has changed
    if (playing != lastKey){
      if (lastKey != -1) {
        MIDI.sendNoteOff(notes[lastKey], 0, midiChannel);
      }
      MIDI.sendNoteOn(notes[playing], 127, midiChannel);
      lastKey = playing;
    }
  } else {
    // If not buttons pressed then turn off the note if necessary
    if (lastKey != -1) {
      MIDI.sendNoteOff(notes[lastKey], 0, midiChannel);
      lastKey = -1;
    }
  }
}
