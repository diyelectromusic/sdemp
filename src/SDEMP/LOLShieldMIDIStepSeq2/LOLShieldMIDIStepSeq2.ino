/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  LOL Shield MIDI Step Sequencer - Part 2
//  https://diyelectromusic.wordpress.com/2021/01/22/lolshield-midi-step-sequencer-part-2/
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
    LOL Shield Examples  - https://github.com/jprodgers/LoLshield

*/
#include <MIDI.h>
#include <Charliplexing.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port
// which is pin 1 for transmitting on the Wemos D1 Mini.
MIDI_CREATE_DEFAULT_INSTANCE();

// Size of the LolShield display
#define LOL_MAX_H 14
#define LOL_MAX_V 9

#define MIDI_TX_CHANNEL 2
#define MIDI_RX_CHANNEL 1

// This sets the "tempo" of the sequencer by defining
// how long to wait between each step (in mS).
// So 500 = 2 steps a second, or 120 bpm.
//    250 = 4 steps a second, or 240 bpm.
//    125 = 8 steps a second, or 480 bpm.
#define DELAYS 8
unsigned long delays[DELAYS]={500,300,250,200,180,160,130,100};
int tempo=4;
#define DELAY_MIDI_CC 0x01 // Modulation Wheel

// Define the note to trigger each animation
#define MIDI_NOTES LOL_MAX_V
#define SCALES 9
const byte PROGMEM midiNotes[SCALES][MIDI_NOTES] = {
  {
  // C3 C4 G4 C5 E5 G5 A#5 C6 D6 - Closest notes to a natural harmonic series on C
     48,60,67,72,76,79,82,84,86
  },
  {
  // C4 D4 E4 F#4 G#4 A#4 C5,D5 E5 - Whole tone scale
     60,62,64,66, 68, 70, 72,74,76
  },
  {
  // C3 F#3 C4 D#4 F#4 A4 C5 D#5 F#5 - Diminished chord
     48,54, 60,63, 66, 69,72,75, 78
  },
  {
  // C3 G3 B3 C4 E4 F4 G4 B4 C5 - Pentatonic scale
     48,55,59,60,64,65,57,71,72
  },
  {
  // C3 G3 C4 D#4 F4 F#4 G4 A#4 C5 - Blues scale
     48,55,60,63, 65,66, 67,70, 72
  },
  {
  // C3 G3 D4 E4 A4 A#4 D5 F#5 A5
     48,55,62,64,69,70, 74,78, 81
  },
  {
  // C2 C3 G3 D4 D#4 A#4 D5 F5 A5
     36,48,55,62,63, 70, 74,77,81
  },
  {
  // C3 G3 E4 A4 B4 E5 F#5 B5 E6
     48,55,64,69,71,76,78, 83,88
  },
  {
  // C3 G3 A#3 D4 F4 G4 A#4 D5 F5
     48,55,58, 62,65,67,70, 74,77
  }
};
byte scaleNotes[MIDI_NOTES];
int midiScale;
int newmidiScale;

int seqstep;
int golCurr=0;
int golNext=1;
bool firststep;

// Definitions for the Game of Life
// (Taken from the Life example for the LOLShield)
#define RESEEDRATE 5000       //Sets the rate the world is re-seeded
#define SIZEX 14              //Sets the X axis size
#define SIZEY 9               //Sets the Y axis size
byte world[SIZEX][SIZEY][2];  //Creates a double buffer world
long density = 30;            //Sets density % during seeding
int geck = 0;                 //Counter for re-seeding

void setup() {
  MIDI.begin(MIDI_RX_CHANNEL);

  // Disable MIDI THRU functionality to prevent
  // sending/receiving clashes between the two different interfaces.
  MIDI.turnThruOff();

  // This means we can write to the "display" as we go,
  // but it won't actually becomem active (i.e. visible)
  // until we call the LedSign::Flip() function
  //
  LedSign::Init();

  // Initialise the new world and then make it active
  seedWorld();
  swapWorlds();
  firststep = true;
}

// Note: Don't want to stop listening for MIDI messages whilst
//       waiting for things to happen music wise.
//
// This function implements a delay() but keeps the MIDI
// reading running while we're waiting.
//
void delayWithMidiRead(unsigned long mS) {
  unsigned long end_mS = mS + millis();
  unsigned long time_mS  = millis();
  while (time_mS < end_mS) {
    checkMidiIn();
    time_mS = millis();
  }
}

void checkMidiIn() {
  if (MIDI.read()) {
    switch (MIDI.getType()) {
      case midi::ProgramChange: {
        byte pcMsg = MIDI.getData1();
        if (pcMsg < SCALES) {
          newmidiScale = pcMsg;
        }
      } break;
      case midi::ControlChange: {
        byte ccMSg = MIDI.getData1();
        if(ccMSg == DELAY_MIDI_CC) {
          // Scale the 0-127 value to 0-7
          tempo = MIDI.getData2() >> 4;
        }
      } break;
      default:
        break;
    }
  }
}

void loop() {
  // Check for MIDI messages for us first
  checkMidiIn();
  
  // Each time through the loop we play a step of the GOL grid
  if (!firststep) {
    // If the MIDI scale changes, need to be able to turn off all previous notes
    int lastmidiScale = midiScale;
    midiScale = newmidiScale;
    for (int i=0; i<LOL_MAX_V; i++) {
      // Send all the MIDI note off messages for the old scale
      MIDI.sendNoteOff(pgm_read_byte_near(&midiNotes[lastmidiScale][i]), 0, MIDI_TX_CHANNEL);

      // If the cell is "on" in the GOL, then play the corresponding MIDI note
      if (world[seqstep][i][golCurr]) {
        MIDI.sendNoteOn(pgm_read_byte_near(&midiNotes[midiScale][i]), 127, MIDI_TX_CHANNEL);
      }
    }
  }

  // Meanwhile we are also calculating the next step of the GOL
  GOLloop (seqstep);

  seqstep++;
  if (seqstep >= LOL_MAX_H) {
    seqstep = 0;
    firststep = false;
  }

  // Wait for the next step of the sequence
  if (!firststep) {
    delayWithMidiRead(delays[tempo]);
  }
}

void swapWorlds() {
  golCurr = golCurr?0:1;
  golNext = golNext?0:1;
}

////////////////////////////////////////////////////////////////
//
// These functions are pretty much lifted straight out
// of the Life example for the LOLShield.
//
// Note: we do the calculation using world[][][golCurr]
// not world[][][0], and store the result in world[][][golNext]
// not world[][][1], which eliminates the copy step each time.
//
// This also only processes one column at a time...
//
////////////////////////////////////////////////////////////////

void GOLloop(int col) {
  // Birth and death cycle 
  int x = col;
  for (int y = 0; y < SIZEY; y++) { 
    // Default is for cell to stay the same
    world[x][y][golNext] = world[x][y][golCurr];
    int count = neighbours(x, y); 
    geck++;
    if (count == 3 && world[x][y][golCurr] == 0) {
      // A new cell is born
      world[x][y][golNext] = 1;
      LedSign::Set(x,y,1);
    } 
    else if ((count < 2 || count > 3) && world[x][y][golCurr] == 1) {
      // Cell dies
      world[x][y][golNext] = 0;
      LedSign::Set(x,y,0);
    }
  }

  // We've completed a full grid
  if (col >= (SIZEX-1)) {
    // Update the display

    //Counts and then checks for re-seeding
    //Otherwise the display will die out at some point
    geck++;
    if (geck > RESEEDRATE){
      seedWorld();
      geck = 0;
    }
  
    // Finally swap golCurr/golNext
    swapWorlds();
  }
}

//Re-seeds based off of RESEEDRATE
void seedWorld(){
  randomSeed(analogRead(5));
  for (int i = 0; i < SIZEX; i++) {
    for (int j = 0; j < SIZEY; j++) {
      if (random(100) < density) {
        world[i][j][golNext] = 1;
      }
    } 
  }
}

//Runs the rule checks, including screen wrap
int neighbours(int x, int y) {
  return world[(x + 1) % SIZEX][y][golCurr] + 
    world[x][(y + 1) % SIZEY][golCurr] + 
    world[(x + SIZEX - 1) % SIZEX][y][golCurr] + 
    world[x][(y + SIZEY - 1) % SIZEY][golCurr] + 
    world[(x + 1) % SIZEX][(y + 1) % SIZEY][golCurr] + 
    world[(x + SIZEX - 1) % SIZEX][(y + 1) % SIZEY][golCurr] + 
    world[(x + SIZEX - 1) % SIZEX][(y + SIZEY - 1) % SIZEY][golCurr] + 
    world[(x + 1) % SIZEX][(y + SIZEY - 1) % SIZEY][golCurr]; 
}
