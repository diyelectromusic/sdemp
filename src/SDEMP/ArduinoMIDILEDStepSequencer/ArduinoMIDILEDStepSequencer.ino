/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI LED Step Sequencer
//  https://diyelectromusic.wordpress.com/2021/10/18/arduino-midi-led-step-sequencer-with-mcp3008/
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
    Analog Input         - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    74HC4051 Multiplexer - https://www.gammon.com.au/forum/?id=11976
    Analog Input         - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput
    MCP3XXX Library      - https://github.com/bakercp/MCP3XXX
*/
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>

#define NUM_POTS 8  // The number of potentiometers for the STEPs
#define MIN_POT_READING 10   // Value for "off"
#define MAX_POT_READING 1023 // Assumes a 10-bit ADC - can be redefined later if need be

// Optional additional control pots - comment out if not used
#define TEMPO_POT     A0   // Tempo adjust
#define MODE_POT      A1   // Sequencer mode
#define RND_POT       A7    // Unused analog input to use for random seed

// Uncomment the method to be used to read the sequence potentiometers
//#define ALG_ADC 1  // Built-in analog inputs
//#define ALG_MUX 1  // Multiplexer
#define ALG_SPI 1  // MCP3008 SPI ADC

#ifdef ALG_MUX
#define MUX_POT  A5 // Pin to read the MUX value
#define MUX_S0   2  // Digital IO pins to control the MUX
#define MUX_S1   3
#define MUX_S2   4
#define MUX_S3   5  // HC4067 (16 ports) only, comment out if using a HC4051 (8 ports).
#define ALGSETUP muxSetup
#define ALGREAD  muxAnalogRead

#elif ALG_SPI
#include <MCP3XXX.h>
MCP3008 adc;
#define ALGSETUP spiSetup
#define ALGREAD  spiAnalogRead

#else // ALG_ADC
#define ALGSETUP algSetup
#define ALGREAD  algAnalogRead
#endif

int pot;

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CHANNEL 1  // Define which MIDI channel to transmit on (1 to 16).

// Set the MIDI codes for the notes to be played.
// This could be simplied by setting just the first and last
// MIDI notes, but doing it this way allows more control over
// the individual notes to be played.
// For example, this could be set to play the notes in a
// specific scale or range, or to select the notes associated
// with a drum kit, etc, if required.
#define NUM_NOTES (9*12) // 9 octaves
const byte notes[NUM_NOTES] = {
9,10,11,12,  13,14,15,16, 17,18,19,20,  // A- to G#0
21,22,23,24, 25,26,27,28, 29,30,31,32,  // A0 to G#1
33,34,35,36, 37,38,39,40, 41,42,43,44,  // A1 to G#2
45,46,47,48, 49,50,51,52, 53,54,55,56,  // A2 to G#3
57,58,59,60, 61,62,63,64, 65,66,67,68,  // A3 to G#4
69,70,71,72, 73,74,75,76, 77,78,79,80,  // A4 to G#5
81,82,83,84, 85,86,87,88, 89,90,91,92,  // A5 to G#6
93,94,95,96, 97,98,99,100, 101,102,103,104, // A6 to G#7
105,106,107,108, 109,110,111,112, 113,114,115,116, // A7 to G#8
};
#define NONOTE 255

#define TEMPO_MIN    30  // 1 every two seconds
#define TEMPO_MAX   480  // 8 a second
#define TEMPO_START 240  // 4 a second
int tempo;

#define NUM_STEPS NUM_POTS  // Assuming a one to one mapping of pots to steps
int curstep;
byte lastnote;
byte steps[NUM_STEPS];
unsigned long steptime;

// Sequencer modes:
//   0 - normal step play
//   1 - reverse step play
//   2 - random step play
//
#define NUM_MODES 3
int seqmode;

// Definitions for the NeoPixels
#define NUM_LEDS   NUM_STEPS
#define LED_PIN   5

// See the Adafruit_NeoPixel library examples and tutorials for details
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

//
// Standard (built-in) analog inputs
//
#ifdef ALG_ADC
void algSetup () {
  // Nothing to do for built-in analog inputs
}

int algAnalogRead (int ch) {
  return analogRead(A0+ch);
}
#endif

//
// Multiplexed analog inputs
//
#ifdef ALG_MUX
void muxSetup () {
  // Set up the output pins to control the multiplexer
  pinMode (MUX_S0, OUTPUT);
  pinMode (MUX_S1, OUTPUT);
  pinMode (MUX_S2, OUTPUT);
#ifdef MUX_S3
  pinMode (MUX_S3, OUTPUT);
#endif
}

int muxAnalogRead (int ch) {
  // Take the reading for this channel:
  //  Set the MUX control pins to choose the pot.
  //  Then read the pot.
  //
  // The MUX control works as a 3 or 4-bit number.
  // The HC4051 requires three bits to choose one of eight
  // ports whereas the HC4067 requires four bits to choose
  // one of sixteen.
  //
  // Taking the HC4051, the bits ordered as follows:
  //    S2-S1-S0
  //     0  0  0 = IO port 0
  //     0  0  1 = IO port 1
  //     0  1  0 = IO port 2
  //     ...
  //     1  1  1 = IO port 7
  //
  // So we can directly link this to the lowest bits
  // (1,2,4) of "playingNote".
  //
  // For the 16-port variant, we need a fourth bit.
  //
  if (ch & 1) { digitalWrite(MUX_S0, HIGH);
       } else { digitalWrite(MUX_S0, LOW);
              }
  if (ch & 2) { digitalWrite(MUX_S1, HIGH);
       } else { digitalWrite(MUX_S1, LOW);
              }
  if (ch & 4) { digitalWrite(MUX_S2, HIGH);
       } else { digitalWrite(MUX_S2, LOW);
              }
#ifdef MUX_S3
  if (ch & 8) { digitalWrite(MUX_S3, HIGH);
       } else { digitalWrite(MUX_S3, LOW);
              }
#endif
  return analogRead (MUX_POT);
}
#endif

//
// SPI analog inputs
//
#ifdef ALG_SPI
void spiSetup () {
  adc.begin();
}

int spiAnalogRead (int ch) {
  return adc.analogRead(ch);
}
#endif


void displaySetup() {
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(20); // Set BRIGHTNESS fairly low to keep current usage low
}

void displayLoop() {
  strip.clear();
  if (steps[curstep] != NONOTE) {
    // Choose the colour based on the note to be played
    strip.setPixelColor(curstep, note2rgb(steps[curstep]));
  } else {
    // "Step is active but not playing" colour
    strip.setPixelColor(curstep, strip.Color(13,13,13));
  }
  strip.show();
}

uint32_t note2rgb (byte note) {
  if (note != 0) {
    // Use the Hue value (0 to 65535) scaled by the MIDI note
    return strip.ColorHSV(map(note, notes[0], notes[NUM_NOTES-1], 0, 65535));
  } else {
    return 0;
  }
}

void setup() {
  tempo = TEMPO_START;
  seqmode = 0;

  randomSeed (analogRead(RND_POT));

  ALGSETUP();
  displaySetup();

  for (int i=0; i<NUM_STEPS; i++) {
    steps[i] = NONOTE;
  }

  // There is no receiving MIDI going on.
  //
  // You can set this to MIDI_CHANNEL_OMNI and call MIDI.read() in the loop
  // if you want to include software MIDI THRU functionality...
  MIDI.begin(MIDI_CHANNEL_OFF);
}

void loop() {
  // Scan additional pots if required
#ifdef TEMPO_POT
  tempo = map (analogRead(TEMPO_POT), 0, 1023, TEMPO_MIN, TEMPO_MAX);
#endif
#ifdef MODE_POT
  seqmode = map (analogRead(MODE_POT), 0, 1023, 0, NUM_MODES-1);
#endif

  // Scan one of the sequence pots
  int potReading = ALGREAD(pot);

  // if the reading is zero (or almost zero), turn off any playing note
  if (potReading < MIN_POT_READING) {
    steps[pot] = NONOTE;
  } else {
    int noteIdx = map(potReading, MIN_POT_READING, MAX_POT_READING, 0, NUM_NOTES-1);
    // Update the note for this step/pot
    steps[pot] = notes[noteIdx];
  }

  // Move on to the next pot next time round
  pot++;
  if (pot >= NUM_POTS) pot = 0;

  // Update any (optional) display or visualisation
  displayLoop();

  // Now see if it is tie to play a step in the sequence yet
  unsigned long curtime = millis();
  if (curtime < steptime) {
    // Not yet time to play the next step in the sequence
    return;
  }

  // The tempo is in beats per minute (bpm).
  // So beats per second = tempo/60
  // So the time in seconds of each beat = 1/beats per second
  // So time in mS = 1000 / (tempo/60)
  // or time in mS = 60000 / tempo
  steptime = curtime + 60000UL/tempo;

  // Prepare the next step in the sequence
  if (seqmode == 1) {
    //
    // Reverse mode
    //
    if (curstep > 0) {
      curstep--;
    } else {
      curstep = NUM_STEPS-1;
    }
  } else if (seqmode == 2) {
    //
    // Random mode
    //
    curstep = random(NUM_STEPS);
  } else {
    //
    // Normal mode
    //
    curstep++;
    if (curstep >= NUM_STEPS) {
      curstep = 0;
    }
    seqmode = 0;
  }

  // If we are already playing a note then we need to turn it off first
  if (lastnote != NONOTE) {
      MIDI.sendNoteOff(lastnote, 0, MIDI_CHANNEL);
  }

  if (steps[curstep] != NONOTE) {
    MIDI.sendNoteOn(steps[curstep], 127, MIDI_CHANNEL);
  }

  // Store the sounding note (in case the pots change the note in the future)
  lastnote = steps[curstep];
}
