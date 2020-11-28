/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  LOL Shield MIDI LED Effect
//  https://diyelectromusic.wordpress.com/2020/11/28/lolshield-midi-lighting-effects/
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
    Arduino PROGMEM      - https://www.arduino.cc/reference/en/language/variables/utilities/progmem/
    Arduino Random       - https://www.arduino.cc/reference/en/language/functions/random-numbers/random/

*/
#include <MIDI.h>
#include <Charliplexing.h>

// Uncomment this to run through eeach animation as a test
//#define TEST 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port
// which is pin 1 for transmitting on the Wemos D1 Mini.
MIDI_CREATE_DEFAULT_INSTANCE();

// Size of the LolShield display
#define LOL_MAX_H 14
#define LOL_MAX_V 9

#define MIDI_CHANNEL 1

// Define the note to trigger each animation
#define MIDI_NOTES 3
byte midiNotes[MIDI_NOTES] = {
  60,62, 64
};

// Uncomment this if you want any note just to trigger a random animation
//#define RANDOMANIMATION 1

int animation;     // Current animation, 1 to ANIMATIONS; 0 = no animation playing
int animationstep; // Current animation step, 0 to ANIFRAMES-1
bool resetanimation;

// Set the time in milliseconds to pass between each animation frame
#define ANIMATIONSCAN 100
unsigned long t_now;
unsigned long t_last;

//
// Orientation of the pattern vs the Shield
//   Barrel Jack = bottom left
//   USB         = top left
//
// Animations created using LED Matrix Studio
// http://maximumoctopus.com/electronics/builder.htm
//
#define ANIMATIONS 3
#define ANIFRAMES  6
#define ANIROWS    LOL_MAX_V
const uint16_t PROGMEM ani[ANIMATIONS][ANIFRAMES][ANIROWS] = {
{// Animation 0
   {0B0000000000000000, 0B0000000000000000, 0B0000010000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000},  //  1
   {0B0000000000000000, 0B0000010000000000, 0B0000101000000000, 0B0000010000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000},  //  2
   {0B0000001000000000, 0B0000100000000000, 0B0001000000000000, 0B0000000100000000, 0B0000010000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000},  //  3
   {0B0001000000000000, 0B0000000000000000, 0B0000000000000000, 0B0010000000000000, 0B0000000010000000, 0B0000100000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000},  //  4
   {0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0100000000000000, 0B0000000001000000, 0B0000000000000000, 0B0000100000000000, 0B0000000000000000},  //  5
   {0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B1000000000000000, 0B0000000000100000, 0B0000000000000000},  //  6
},
{// Animation 1
   {0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000010000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000},  //  1
   {0B0000000000000000, 0B0000000000000000, 0B0000000010000000, 0B0000000101000000, 0B0000000010000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000},  //  2
   {0B0000000000000000, 0B0000000001000000, 0B0000000100000000, 0B0000001000000000, 0B0000000000100000, 0B0000000010000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000},  //  3
   {0B0000000000100000, 0B0000001000000000, 0B0000000000000000, 0B0000000000000000, 0B0000010000000000, 0B0000000000010000, 0B0000000100000000, 0B0000000000000000, 0B0000000000000000},  //  4
   {0B0000010000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000100000000000, 0B0000000000001000, 0B0000000000000000, 0B0000000100000000},  //  5
   {0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0001000000000000, 0B0000000000000100},  //  6
},
{// Animation 2
   {0B0000000000000000, 0B0000000000100000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000},  //  1
   {0B0000000000100000, 0B0000000001010000, 0B0000000000100000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000},  //  2
   {0B0000000001000000, 0B0000000010000000, 0B0000000000001000, 0B0000000000100000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000},  //  3
   {0B0000000100000000, 0B0000000000000000, 0B0000000100000000, 0B0000000000000100, 0B0000000001010000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000},  //  4
   {0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000001000000000, 0B0000000000000000, 0B0000000001001000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000},  //  5
   {0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000000000000000, 0B0000010000000000, 0B0000000000000000, 0B0000000001000100, 0B0000000000000000},  //  6  
},
};

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
#ifdef RANDOMANIMATION
      // Just pick a random animation to play
      animation = random(1, ANIMATIONS+1);
#else
      // Trigger the animation number "i"
      // (recall animation goes from 1 to ANIMATIONS)
      // (and animation = 0 means disabled)
      animation = i+1;
#endif
      resetanimation = true;
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  // There is no special handling here.
}

void setup() {
  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  MIDI.begin(MIDI_CHANNEL);

  // Initialise the LolShield too allow us to 
  // use shades (i.e. varying the brightness)
  // starting with full brightness (127).
  //
//  LedSign::Init(GRAYSCALE);
//  LedSign::SetBrightness(127);
  LedSign::Init(DOUBLE_BUFFER);

  t_last = millis();
  animation = 0;
  animationstep = 0;
  resetanimation = false;
#ifdef TEST
  animation = 1;
#endif
}

void loop() {
  // All the loop has to do is call the MIDI read function
  MIDI.read();

  // If there is an animation playing, then move it on a frame
  // at the rate determined by counting up to ANIMATIONSCAN.
  if (animation) {
    // If the animation has changed or has been retriggered,
    // then clear the display and reset the steps
    if (resetanimation) {
        animationstep=0;
        LedSign::Clear();
        LedSign::Flip();
        resetanimation = false;
    }
    t_now = millis();
    if ((t_now-t_last) >= ANIMATIONSCAN) {
      t_last = t_now;
      if (animationstep >= ANIFRAMES) {
        animationstep=0;
        LedSign::Clear();
        LedSign::Flip();
#ifdef TEST
        animation++;
        // Recall animation = 0 means disabled
        // Animations run from 1 to ANIMATIONS
        if (animation > ANIMATIONS) {
          animation = 1;
        }
#else
        animation = 0;
#endif
      } else {
        updateDisplay();
        animationstep++;
      }
    }
  }
}

void updateDisplay () {
  for (int v=0; v<LOL_MAX_V ; v++) {
    for (int h=0; h<LOL_MAX_H; h++) {
      // Need to reverse the horizontal axis to ensure correct orientation
      // as per description at the start of the file for the animations...
      //
      // Also recall animations goes 1 to max, but animationstep goes 0 to max-1
      uint16_t anival = pgm_read_word_near(&ani[animation-1][animationstep][v]);
      if (anival & (1<<(LOL_MAX_H-1-h))) {
        // Set the LED
        LedSign::Set(h, v, 9);
      }
      else {
        // Clear the LED
        LedSign::Set(h, v, 0);
      }
    }
  }
  LedSign::Flip();
}
