/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MP3 Piano
//  https://diyelectromusic.wordpress.com/2020/07/03/arduino-mp3-piano/
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
    Button        - http://www.arduino.cc/en/Tutorial/Button
    VS1053 Shield - https://www.elecrow.com/wiki/index.php?title=VS1053_MP3_Shield

*/
#include <SPI.h>
#include <SdFat.h>
#include <vs1053_SdFat.h>

// The MP3 shield uses quite a number of pins for itself.
// In fact it uses almost all the digital pins apart from
// a couple, so for the switches in this sketch, I'm just
// using the analog pins in "digital" mode.

// Set up the input and output pins to be used for the keys
int keys[] = {
  A0, A1, A2, A3, A4
};

// This is the magic initialisation code for the MP3 shield
// (Taken from the demo program with the library)
//
SdFat  sd;
vs1053 MP3player;

void setup() {
  Serial.begin(9600);
  int numkeys = sizeof(keys)/sizeof(keys[0]);     // Programming trick to get the number of keys

  // Initialise the input buttons as piano "keys"
  for (int k = 0; k < numkeys; k++) {
    pinMode (keys[k], INPUT);
  }

  // Now initialise the MP3 player and card reading code
  // (this is all taken from the demo code)
  //
  if (!sd.begin(SD_SEL, SPI_FULL_SPEED)) sd.initErrorHalt();
  if (!sd.chdir("/")) sd.errorHalt("sd.chdir");

  // This is really lazy - there are a number of things that could
  // fail, so this really ought to check for an error code being returned...
  MP3player.begin();

  // Set to full volume (0 for each L and R channel means "full")
  MP3player.setVolume(0, 0);

  // Check we can play each track
  for (int k=0; k<numkeys; k++) {
    Serial.print  ("Testing track ");
    Serial.println(k+1);
    playMp3 (k+1);
  }

  Serial.print("Ready\n");
}

void loop() {
  // Check each button and if pressed play the MP3 file for that note.
  //
  // If no buttons are pressed, turn off all notes.
  // Note - the last button to be checked will be
  // the note that ends up being played.
  //
  int numkeys = sizeof(keys)/sizeof(keys[0]);
  int playing = -1;
  for (int k = 0; k < numkeys; k++) {
    int buttonState = digitalRead (keys[k]);
    if (buttonState == HIGH) {
      playing = k;
    }
  }
  
  if (playing != -1) {
    // This assumes each track is named something like
    //    track001.mp3 to track009.mp3
    //
    // Note: playing will go from 0 to numkeys-1
    //       so need to add 1 here.
    //
    // Any more than 9 keys will be ignored.
    //
    Serial.println(playing+1);
    playMp3 (playing+1);
  }
}

void playMp3 (int track) {
  MP3player.playTrack(track);
  while (MP3player.isPlaying()) {
    // Need to wait for the file to finish playing
    delay (300);
  }
}
