/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Touch Piano
//  https://diyelectromusic.wordpress.com/2020/06/07/arduino-touch-piano/
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
    toneMelody       - http://www.arduino.cc/en/Tutorial/Tone
    CapacitiveSensor - https://playground.arduino.cc/Main/CapacitiveSensor/

*/

#include <CapacitiveSensor.h>

// Pitches taken from definitions in toneMelody
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523

// Set up the output pin to be used for the speaker
#define SPEAKER  12

// Set the notes to be played by each key
int notes[] = {
  NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4, NOTE_C5
};

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
  Serial.begin(9600);
  numcaps = sizeof (caps)/sizeof(caps[0]);
  Serial.print("\n\nArduino Touch Piano\n");
}

void loop() {
  // Check each sensor and if touched play the note.
  // If no sensors are touched, turn off all notes.
  int playing = -1;
  for (int k = 0; k < numcaps; k++) {
    Serial.print("\t");
    long sensorvalue =  caps[k]->capacitiveSensor(20);
    Serial.print(sensorvalue);
    if (sensorvalue > SENSOR_THRESHOLD) {
      playing = k;
    }
  }
  Serial.print("\n");

  if (playing != -1) {
    tone (SPEAKER, notes[playing]);
  } else {
    // If we didn't find a button pressed, make sure everything is off
    noTone(SPEAKER);
  }
}
