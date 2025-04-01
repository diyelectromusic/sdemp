/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Philip Glass Knee Play 1 from Einstein on the Beach
//  https://diyelectromusic.com/2025/04/01/minimalist-lo-fi-minimalism/
//
      MIT License
      
      Copyright (c) 2025 diyelectromusic (Kevin)
      
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
#include <HT16K33.h>
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL_BASS 1
#define MIDI_CHANNEL_SOP  2
#define MIDI_CHANNEL_TEN  3

// Three instrument/voice tracks, one number track

#define STEPS 36
uint8_t bass[STEPS] = {
45,-1,-1,-1,-1,-1,-1,-1,43,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,36,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};
uint8_t ten[STEPS] = {
57,0,57,0,57,0,57,0,55,0,55,0,55,0,55,0,55,0,55,0,48,0,48,0,48,0,48,0,48,0,48,0,48,0,48,0
};
uint8_t sop[STEPS] = {
60,0,60,0,60,0,60,0,62,0,62,0,62,0,62,0,62,0,62,0,64,0,64,0,64,0,64,0,64,0,64,0,64,0,64,0
};
uint8_t num[STEPS] = {
1,-1,2,-1,3,-1,4,-1,1,-1,2,-1,3,-1,4,-1,5,-1,6,-1,1,-1,2,-1,3,-1,4,-1,5,-1,6,-1,7,-1,8,-1
};

uint8_t lastbass, lastten, lastsop;

// I2C 7 Segment Display
//   C/CLK = A5/SCL
//   D/DAT = A4/SDA
HT16K33 seg(0x70);  // Pass in I2C address of the display

void displayInit() {
  // Initialise the I2C display
  seg.begin();
  Wire.setClock(100000);
  seg.displayOn();
  seg.brightness(2);
  seg.displayClear();
}

void displayClear () {
  seg.displayClear();
}

void displayDigit (int dig) {
  seg.displayInt(dig);
}

void doBass (int i) {
  if (bass[i] == 0) {
    // Rest
    if (lastbass != 0) {
      MIDI.sendNoteOff(lastbass, 0, MIDI_CHANNEL_BASS);
    }
    lastbass = 0;
  } else if (bass[i] < 128) {
    // New note
    if (lastbass != 0) {
      MIDI.sendNoteOff(lastbass, 0, MIDI_CHANNEL_BASS);
    }
    MIDI.sendNoteOn(bass[i], 64, MIDI_CHANNEL_BASS);
    lastbass = bass[i];
  } else {
    // do nothing
  }
}

void doVoices (int i) {
  if (ten[i] == 0) {
    if (lastten != 0) {
      MIDI.sendNoteOff(lastten, 0, MIDI_CHANNEL_TEN);
    }
    lastten = 0;
  } else if (ten[i] < 128) {
    if (lastten != 0) {
      MIDI.sendNoteOff(lastten, 0, MIDI_CHANNEL_TEN);
    }
    MIDI.sendNoteOn(ten[i], 64, MIDI_CHANNEL_TEN);
    lastten = ten[i];
  } else {
    // do nothing
  }

  if (sop[i] == 0) {
    if (lastsop != 0) {
      MIDI.sendNoteOff(lastsop, 0, MIDI_CHANNEL_SOP);
    }
    lastsop = 0;
  } else if (sop[i] < 128) {
    if (lastsop != 0) {
      MIDI.sendNoteOff(lastsop, 0, MIDI_CHANNEL_SOP);
    }
    MIDI.sendNoteOn(sop[i], 64, MIDI_CHANNEL_SOP);
    lastsop = sop[i];
  } else {
    // do nothing
  }

  if (num[i] == 0) {
    displayClear();
  } else if (num[i] < 10) {
    displayDigit(num[i]);
  } else {
    // do nothing
  }
}

void doVoiceSkip () {
  if (lastten != 0) {
    MIDI.sendNoteOff(lastten, 0, MIDI_CHANNEL_TEN);
  }
  lastten = 0;

  if (lastsop != 0) {
    MIDI.sendNoteOff(lastsop, 0, MIDI_CHANNEL_SOP);
  }
  lastsop = 0;

  displayClear();
}

void setup() {
  displayInit();
  MIDI.begin();
  randomSeed(analogRead(0));

  delay(5000);

  // Introduction
  for (int phrase=0; phrase<3; phrase++) {
    for (int i=0; i<STEPS; i++) {
      doBass(i);
      delay(300);
    }
  }

  // Two full rounds as is
  for (int phrase=0; phrase<2; phrase++) {
    for (int i=0; i<STEPS; i++) {
      doBass(i);
      doVoices(i);
      delay(300);
    }
  }
}

void loop() {
  int skip = random(4);
  for (int i=0; i<STEPS; i++) {
    doBass(i);
    if ((skip == 1) && (i==0)) {
      // Skip first beat of 4/4 bar
      doVoiceSkip();
    } else if ((skip == 2) && (i==8)) {
      // Skip first beat of 6/4 bar
      doVoiceSkip();
    } else if ((skip == 3) && (i==20)) {
      // Skip first beat of 8/4 bar
      doVoiceSkip();
    } else {
      doVoices(i);
    }
    delay(300);
  }
}
