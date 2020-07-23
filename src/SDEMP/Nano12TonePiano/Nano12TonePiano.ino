/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Nano 12 Tone Piano
//  https://diyelectromusic.wordpress.com/2020/06/01/arduino-nano-12-note-keyboard/
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
    toneMelody - http://www.arduino.cc/en/Tutorial/Tone
    Button     - http://www.arduino.cc/en/Tutorial/Button

*/

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


// Set up the input and output pins to be used for the speaker and keys
// NB: On the Arduino Nano, pin 13 is attached to the onboard LED
//     and pins 0 and 1 are used for the serial port.
//     We use all free digital pins for keys and two of the analogue pins.
#define SPEAKER  12
int keys[] = {
  19, 18, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13
};

// Set the notes to be played by each key
int notes[] = {
  NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4, NOTE_C5
};


void setup() {
  int numkeys = sizeof(keys)/sizeof(keys[0]);     // Programming trick to get the number of keys
  // Initialise the input buttons as piano "keys"
  for (int k = 0; k < numkeys; k++) {
    pinMode (keys[k], INPUT);
  } 
}

void loop() {
  // Check each button and if pressed play the note.
  // If no buttons are pressed, turn off all notes.
  // Note - the last button to be checked will be
  // the note that ends up being played.
  int numkeys = sizeof(keys)/sizeof(keys[0]);
  int playing = -1;
  for (int k = 0; k < numkeys; k++) {
    int buttonState = digitalRead (keys[k]);
    if (buttonState == HIGH) {
      playing = k;
    }
  }

  if (playing != -1) {
    tone (SPEAKER, notes[playing]);
  } else {
    // If we didn't find a button pressed, make sure everything is off
    noTone(SPEAKER);
  }
}
